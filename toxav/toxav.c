/*
 * Copyright © 2016-2017 The TokTok team.
 * Copyright © 2013-2015 Tox project.
 *
 * This file is part of Tox, the free peer to peer instant messenger.
 *
 * Tox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tox.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "toxav.h"

#include "msi.h"
#include "rtp.h"
#include "video.h"


#include "../toxcore/Messenger.h"
#include "../toxcore/logger.h"
#include "../toxcore/util.h"
#include "../toxcore/mono_time.h"


#include "tox_generic.h"

#include "codecs/toxav_codecs.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


#define VIDEO_ACCEPTABLE_LOSS (0.08f) /* if loss is less than this (8%), then don't do anything */
#define AUDIO_ITERATATIONS_WHILE_VIDEO (20)
#define VIDEO_MIN_SEND_KEYFRAME_INTERVAL 5000

#if defined(AUDIO_DEBUGGING_SKIP_FRAMES)
uint32_t _debug_count_sent_audio_frames = 0;
uint32_t _debug_skip_every_x_audio_frame = 10;
#endif



void callback_bwc(BWController *bwc, uint32_t friend_number, float loss, void *user_data);

int callback_invite(void *toxav_inst, MSICall *call);
int callback_start(void *toxav_inst, MSICall *call);
int callback_end(void *toxav_inst, MSICall *call);
int callback_error(void *toxav_inst, MSICall *call);
int callback_capabilites(void *toxav_inst, MSICall *call);

bool audio_bit_rate_invalid(uint32_t bit_rate);
bool video_bit_rate_invalid(uint32_t bit_rate);
bool invoke_call_state_callback(ToxAV *av, uint32_t friend_number, uint32_t state);
ToxAVCall *call_new(ToxAV *av, uint32_t friend_number, TOXAV_ERR_CALL *error);
ToxAVCall *call_get(ToxAV *av, uint32_t friend_number);
ToxAVCall *call_remove(ToxAVCall *call);
bool call_prepare_transmission(ToxAVCall *call);
void call_kill_transmission(ToxAVCall *call);

ToxAV *toxav_new(Tox *tox, TOXAV_ERR_NEW *error)
{
    TOXAV_ERR_NEW rc = TOXAV_ERR_NEW_OK;
    ToxAV *av = NULL;
    Messenger *m = (Messenger *)tox;

    if (tox == NULL) {
        rc = TOXAV_ERR_NEW_NULL;
        goto END;
    }

    if (m->msi_packet) {
        rc = TOXAV_ERR_NEW_MULTIPLE;
        goto END;
    }

    av = (ToxAV *)calloc(sizeof(ToxAV), 1);

    if (av == NULL) {
        LOGGER_WARNING(m->log, "Allocation failed!");
        rc = TOXAV_ERR_NEW_MALLOC;
        goto END;
    }

    if (create_recursive_mutex(av->mutex) != 0) {
        LOGGER_WARNING(m->log, "Mutex creation failed!");
        rc = TOXAV_ERR_NEW_MALLOC;
        goto END;
    }

    av->m = m;
    av->msi = msi_new(av->m);

    if (av->msi == NULL) {
        pthread_mutex_destroy(av->mutex);
        rc = TOXAV_ERR_NEW_MALLOC;
        goto END;
    }

    av->interval = 200;
    av->msi->av = av;

    msi_register_callback(av->msi, callback_invite, msi_OnInvite);
    msi_register_callback(av->msi, callback_start, msi_OnStart);
    msi_register_callback(av->msi, callback_end, msi_OnEnd);
    msi_register_callback(av->msi, callback_error, msi_OnError);
    msi_register_callback(av->msi, callback_error, msi_OnPeerTimeout);
    msi_register_callback(av->msi, callback_capabilites, msi_OnCapabilities);

END:

    if (error) {
        *error = rc;
    }

    if (rc != TOXAV_ERR_NEW_OK) {
        free(av);
        av = NULL;
    }

    return av;
}

void toxav_kill(ToxAV *av)
{
    if (av == NULL) {
        return;
    }

    pthread_mutex_lock(av->mutex);

    /* To avoid possible deadlocks */
    while (av->msi && msi_kill(av->msi, av->m->log) != 0) {
        pthread_mutex_unlock(av->mutex);
        pthread_mutex_lock(av->mutex);
    }

    /* Msi kill will hang up all calls so just clean these calls */
    if (av->calls) {
        ToxAVCall *it = call_get(av, av->calls_head);

        while (it) {
            call_kill_transmission(it);
            it->msi_call = NULL; /* msi_kill() frees the call's msi_call handle; which causes #278 */
            it = call_remove(it); /* This will eventually free av->calls */
        }
    }

    pthread_mutex_unlock(av->mutex);
    pthread_mutex_destroy(av->mutex);

    free(av);
}

Tox *toxav_get_tox(const ToxAV *av)
{
    return (Tox *) av->m;
}

uint32_t toxav_iteration_interval(const ToxAV *av)
{
    /* If no call is active interval is 200 */
    return av->calls ? av->interval : 200;
}

static void *video_play_bg(void *data)
{
    if (data) {
        ToxAVCall *call = (ToxAVCall *)data;

        if (call) {
            VCSession *vc = (VCSession *)call->video.second;
            uint8_t got_video_frame = vc_iterate(vc, call->av->m, call->skip_video_flag,
                                                 &(call->last_incoming_audio_frame_rtimestamp),
                                                 &(call->last_incoming_audio_frame_ltimestamp),
                                                 &(call->last_incoming_video_frame_rtimestamp),
                                                 &(call->last_incoming_video_frame_ltimestamp),
                                                 call->bwc,
                                                 &(call->call_timestamp_difference_adjustment),
                                                 &(call->call_timestamp_difference_to_sender)
                                                );
        }
    }

    return (void *)NULL;
}


void toxav_iterate(ToxAV *av)
{
    pthread_mutex_lock(av->mutex);

    if (av->calls == NULL) {
        pthread_mutex_unlock(av->mutex);
        return;
    }

    uint64_t start = current_time_monotonic();
    int32_t rc = 500;
    uint32_t audio_iterations = 0;

    ToxAVCall *i = av->calls[av->calls_head];

    for (; i; i = i->next) {

        audio_iterations = 0;

        if (i->active) {
            pthread_mutex_lock(i->mutex);
            pthread_mutex_unlock(av->mutex);


#if !defined(_GNU_SOURCE)
            video_play_bg((void *)(i));
#else
            // ------- multithreaded av_iterate for video -------
            pthread_t video_play_thread;
            LOGGER_TRACE(av->m->log, "video_play -----");

            if (pthread_create(&video_play_thread, NULL, video_play_bg, (void *)(i))) {
                LOGGER_WARNING(av->m->log, "error creating video play thread");
            } else {
                // TODO: set lower prio for video play thread ?
            }

            // ------- multithreaded av_iterate for video -------
#endif



            // ------- av_iterate for audio -------
            uint8_t res_ac = ac_iterate(i->audio.second,
                                        &(i->last_incoming_audio_frame_rtimestamp),
                                        &(i->last_incoming_audio_frame_ltimestamp),
                                        &(i->last_incoming_video_frame_rtimestamp),
                                        &(i->last_incoming_video_frame_ltimestamp),
                                        &(i->call_timestamp_difference_adjustment),
                                        &(i->call_timestamp_difference_to_sender)
                                       );

            if (res_ac == 2) {
                i->skip_video_flag = 1;
            } else {
                i->skip_video_flag = 0;
            }

            // ------- av_iterate for audio -------



            /*
             * compile toxcore with "-D_GNU_SOURCE" to activate "pthread_tryjoin_np" solution!
             */
#if !defined(_GNU_SOURCE)
            // pthread_join(video_play_thread, NULL);
#else

            while (pthread_tryjoin_np(video_play_thread, NULL) != 0) {
                if (audio_iterations < AUDIO_ITERATATIONS_WHILE_VIDEO) {
                    /* video thread still running, let's do some more audio */
                    if (ac_iterate(i->audio.second,
                                   &(i->last_incoming_audio_frame_rtimestamp),
                                   &(i->last_incoming_audio_frame_ltimestamp),
                                   &(i->last_incoming_video_frame_rtimestamp),
                                   &(i->last_incoming_video_frame_ltimestamp),
                                   &(i->call_timestamp_difference_adjustment),
                                   &(i->call_timestamp_difference_to_sender)
                                  ) == 0) {
                        // TODO: Zoff: not sure if this sleep is good, or bad??
                        usleep(40);
                    } else {
                        LOGGER_TRACE(av->m->log, "did some more audio iterate");
                    }
                } else {
                    break;
                }

                audio_iterations++;
            }

            pthread_join(video_play_thread, NULL);
#endif



#define MIN(a,b) (((a)<(b))?(a):(b))

            // LOGGER_WARNING(av->m->log, "XXXXXXXXXXXXXXXXXX=================");
            if (i->msi_call->self_capabilities & msi_CapRAudio &&
                    i->msi_call->peer_capabilities & msi_CapSAudio) {
                // use 4ms less than the actual audio frame duration, to have still some time left
                // LOGGER_WARNING(av->m->log, "lp_frame_duration=%d", (int)i->audio.second->lp_frame_duration);
                rc = MIN((i->audio.second->lp_frame_duration - 4), rc);
            }

            if (i->msi_call->self_capabilities & msi_CapRVideo &&
                    i->msi_call->peer_capabilities & msi_CapSVideo) {
                // LOGGER_WARNING(av->m->log, "lcfd=%d", (int)i->video.second->lcfd);
                rc = MIN(i->video.second->lcfd, (uint32_t) rc);
            }

            // LOGGER_WARNING(av->m->log, "rc=%d", (int)rc);
            // LOGGER_WARNING(av->m->log, "XXXXXXXXXXXXXXXXXX=================");


            uint32_t fid = i->friend_number;

            pthread_mutex_unlock(i->mutex);
            pthread_mutex_lock(av->mutex);

            /* In case this call is popped from container stop iteration */
            if (call_get(av, fid) != i) {
                break;
            }

            if ((i->last_incoming_audio_frame_ltimestamp != 0)
                    &&
                    (i->last_incoming_video_frame_ltimestamp != 0)) {
                if (i->reference_diff_timestamp_set == 0) {
                    i->reference_rtimestamp = i->last_incoming_audio_frame_rtimestamp;
                    i->reference_ltimestamp = i->last_incoming_audio_frame_ltimestamp;
                    // this is the difference between local and remote clocks in "ms"
                    i->reference_diff_timestamp = (int64_t)(i->reference_ltimestamp - i->reference_rtimestamp);
                    i->reference_diff_timestamp_set = 1;
                } else {
                    int64_t latency_ms = (int64_t)(
                                             (i->last_incoming_video_frame_rtimestamp - i->last_incoming_audio_frame_rtimestamp) -
                                             (i->last_incoming_video_frame_ltimestamp - i->last_incoming_audio_frame_ltimestamp)
                                         );


                    LOGGER_DEBUG(av->m->log, "VIDEO:delay-to-audio-in-ms=%lld", (long long)latency_ms);
                    // LOGGER_INFO(av->m->log, "CLOCK:delay-to-rmote-in-ms=%lld", (long long)(i->reference_diff_timestamp));
                    LOGGER_DEBUG(av->m->log, "VIDEO:delay-to-refnc-in-ms=%lld",
                                 (long long) - ((i->last_incoming_video_frame_ltimestamp - i->reference_diff_timestamp) -
                                                i->last_incoming_video_frame_rtimestamp));
                    LOGGER_DEBUG(av->m->log, "AUDIO:delay-to-refnc-in-ms=%lld",
                                 (long long) - ((i->last_incoming_audio_frame_ltimestamp - i->reference_diff_timestamp) -
                                                i->last_incoming_audio_frame_rtimestamp));

                    // LOGGER_INFO(av->m->log, "VIDEO latency in ms=%lld", (long long)(i->last_incoming_video_frame_ltimestamp - i->last_incoming_video_frame_rtimestamp));
                    // LOGGER_INFO(av->m->log, "AUDIO latency in ms=%lld", (long long)(i->last_incoming_audio_frame_ltimestamp - i->last_incoming_audio_frame_rtimestamp));

                    // LOGGER_INFO(av->m->log, "VIDEO:3-latency-in-ms=%lld", (long long)(i->last_incoming_audio_frame_rtimestamp - i->last_incoming_video_frame_rtimestamp));

                    //LOGGER_INFO(av->m->log, "AUDIO (to video):latency in a=%lld b=%lld c=%lld d=%lld",
                    //(long long)i->last_incoming_video_frame_rtimestamp,
                    //(long long)i->last_incoming_audio_frame_rtimestamp,
                    //(long long)i->last_incoming_video_frame_ltimestamp,
                    //(long long)i->last_incoming_audio_frame_ltimestamp
                    //);
                }
            }
        }
    }

    pthread_mutex_unlock(av->mutex);

    av->interval = rc < av->dmssa ? 0 : (rc - av->dmssa);
    av->dmsst += current_time_monotonic() - start;

    if (++av->dmssc == 3) {
        av->dmssa = av->dmsst / 3 + 5 /* NOTE Magic Offset for precission */;
        av->dmssc = 0;
        av->dmsst = 0;
    }
}

bool toxav_call(ToxAV *av, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate,
                TOXAV_ERR_CALL *error)
{
    TOXAV_ERR_CALL rc = TOXAV_ERR_CALL_OK;
    ToxAVCall *call;

    pthread_mutex_lock(av->mutex);

    if ((audio_bit_rate && audio_bit_rate_invalid(audio_bit_rate))
            || (video_bit_rate && video_bit_rate_invalid(video_bit_rate))) {
        rc = TOXAV_ERR_CALL_INVALID_BIT_RATE;
        goto END;
    }

    call = call_new(av, friend_number, &rc);

    if (call == NULL) {
        goto END;
    }

    call->audio_bit_rate = audio_bit_rate;
    call->video_bit_rate = video_bit_rate;

    LOGGER_ERROR(av->m->log, "toxav_call:vb=%d", (int)video_bit_rate);

    call->previous_self_capabilities = msi_CapRAudio | msi_CapRVideo;

    call->previous_self_capabilities |= audio_bit_rate > 0 ? msi_CapSAudio : 0;
    call->previous_self_capabilities |= video_bit_rate > 0 ? msi_CapSVideo : 0;

    if (msi_invite(av->msi, &call->msi_call, friend_number, call->previous_self_capabilities) != 0) {
        call_remove(call);
        rc = TOXAV_ERR_CALL_SYNC;
        goto END;
    }

    call->msi_call->av_call = call;

END:
    pthread_mutex_unlock(av->mutex);

    if (error) {
        *error = rc;
    }

    return rc == TOXAV_ERR_CALL_OK;
}

void toxav_callback_call(ToxAV *av, toxav_call_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->ccb.first = callback;
    av->ccb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}

void toxav_callback_call_comm(ToxAV *av, toxav_call_comm_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->call_comm_cb.first = callback;
    av->call_comm_cb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}

bool toxav_answer(ToxAV *av, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate,
                  TOXAV_ERR_ANSWER *error)
{
    pthread_mutex_lock(av->mutex);

    TOXAV_ERR_ANSWER rc = TOXAV_ERR_ANSWER_OK;
    ToxAVCall *call;

    if (m_friend_exists(av->m, friend_number) == 0) {
        rc = TOXAV_ERR_ANSWER_FRIEND_NOT_FOUND;
        goto END;
    }

    if ((audio_bit_rate && audio_bit_rate_invalid(audio_bit_rate))
            || (video_bit_rate && video_bit_rate_invalid(video_bit_rate))
       ) {
        rc = TOXAV_ERR_ANSWER_INVALID_BIT_RATE;
        goto END;
    }

    call = call_get(av, friend_number);

    if (call == NULL) {
        rc = TOXAV_ERR_ANSWER_FRIEND_NOT_CALLING;
        goto END;
    }

    if (!call_prepare_transmission(call)) {
        rc = TOXAV_ERR_ANSWER_CODEC_INITIALIZATION;
        goto END;
    }

    call->audio_bit_rate = audio_bit_rate;
    call->video_bit_rate = video_bit_rate;

    LOGGER_ERROR(av->m->log, "toxav_answer:vb=%d", (int)video_bit_rate);

    call->previous_self_capabilities = msi_CapRAudio | msi_CapRVideo;

    call->previous_self_capabilities |= audio_bit_rate > 0 ? msi_CapSAudio : 0;
    call->previous_self_capabilities |= video_bit_rate > 0 ? msi_CapSVideo : 0;

    if (msi_answer(call->msi_call, call->previous_self_capabilities) != 0) {
        rc = TOXAV_ERR_ANSWER_SYNC;
    }

END:
    pthread_mutex_unlock(av->mutex);

    if (error) {
        *error = rc;
    }

    return rc == TOXAV_ERR_ANSWER_OK;
}

void toxav_callback_call_state(ToxAV *av, toxav_call_state_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->scb.first = callback;
    av->scb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}

bool toxav_call_control(ToxAV *av, uint32_t friend_number, TOXAV_CALL_CONTROL control, TOXAV_ERR_CALL_CONTROL *error)
{
    pthread_mutex_lock(av->mutex);
    TOXAV_ERR_CALL_CONTROL rc = TOXAV_ERR_CALL_CONTROL_OK;
    ToxAVCall *call;

    if (m_friend_exists(av->m, friend_number) == 0) {
        rc = TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_FOUND;
        goto END;
    }

    call = call_get(av, friend_number);

    if (call == NULL || (!call->active && control != TOXAV_CALL_CONTROL_CANCEL)) {
        rc = TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_IN_CALL;
        goto END;
    }

    switch (control) {
        case TOXAV_CALL_CONTROL_RESUME: {
            /* Only act if paused and had media transfer active before */
            if (call->msi_call->self_capabilities == 0 &&
                    call->previous_self_capabilities) {

                if (msi_change_capabilities(call->msi_call,
                                            call->previous_self_capabilities) == -1) {
                    rc = TOXAV_ERR_CALL_CONTROL_SYNC;
                    goto END;
                }

                rtp_allow_receiving(call->audio.first);
                rtp_allow_receiving(call->video.first);
            } else {
                rc = TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION;
                goto END;
            }
        }
        break;

        case TOXAV_CALL_CONTROL_PAUSE: {
            /* Only act if not already paused */
            if (call->msi_call->self_capabilities) {
                call->previous_self_capabilities = call->msi_call->self_capabilities;

                if (msi_change_capabilities(call->msi_call, 0) == -1) {
                    rc = TOXAV_ERR_CALL_CONTROL_SYNC;
                    goto END;
                }

                rtp_stop_receiving(call->audio.first);
                rtp_stop_receiving(call->video.first);
            } else {
                rc = TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION;
                goto END;
            }
        }
        break;

        case TOXAV_CALL_CONTROL_CANCEL: {
            /* Hang up */
            pthread_mutex_lock(call->mutex);

            if (msi_hangup(call->msi_call) != 0) {
                rc = TOXAV_ERR_CALL_CONTROL_SYNC;
                pthread_mutex_unlock(call->mutex);
                goto END;
            }

            call->msi_call = NULL;
            pthread_mutex_unlock(call->mutex);

            /* No mather the case, terminate the call */
            call_kill_transmission(call);
            call_remove(call);
        }
        break;

        case TOXAV_CALL_CONTROL_MUTE_AUDIO: {
            if (call->msi_call->self_capabilities & msi_CapRAudio) {
                if (msi_change_capabilities(call->msi_call, call->
                                            msi_call->self_capabilities ^ msi_CapRAudio) == -1) {
                    rc = TOXAV_ERR_CALL_CONTROL_SYNC;
                    goto END;
                }

                rtp_stop_receiving(call->audio.first);
            } else {
                rc = TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION;
                goto END;
            }
        }
        break;

        case TOXAV_CALL_CONTROL_UNMUTE_AUDIO: {
            if (call->msi_call->self_capabilities ^ msi_CapRAudio) {
                if (msi_change_capabilities(call->msi_call, call->
                                            msi_call->self_capabilities | msi_CapRAudio) == -1) {
                    rc = TOXAV_ERR_CALL_CONTROL_SYNC;
                    goto END;
                }

                rtp_allow_receiving(call->audio.first);
            } else {
                rc = TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION;
                goto END;
            }
        }
        break;

        case TOXAV_CALL_CONTROL_HIDE_VIDEO: {
            if (call->msi_call->self_capabilities & msi_CapRVideo) {
                if (msi_change_capabilities(call->msi_call, call->
                                            msi_call->self_capabilities ^ msi_CapRVideo) == -1) {
                    rc = TOXAV_ERR_CALL_CONTROL_SYNC;
                    goto END;
                }

                rtp_stop_receiving(call->video.first);
            } else {
                rc = TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION;
                goto END;
            }
        }
        break;

        case TOXAV_CALL_CONTROL_SHOW_VIDEO: {
            if (call->msi_call->self_capabilities ^ msi_CapRVideo) {
                if (msi_change_capabilities(call->msi_call, call->
                                            msi_call->self_capabilities | msi_CapRVideo) == -1) {
                    rc = TOXAV_ERR_CALL_CONTROL_SYNC;
                    goto END;
                }

                rtp_allow_receiving(call->video.first);
            } else {
                rc = TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION;
                goto END;
            }
        }
        break;
    }

END:
    pthread_mutex_unlock(av->mutex);

    if (error) {
        *error = rc;
    }

    return rc == TOXAV_ERR_CALL_CONTROL_OK;
}

bool toxav_option_set(ToxAV *av, uint32_t friend_number, TOXAV_OPTIONS_OPTION option, int32_t value,
                      TOXAV_ERR_OPTION_SET *error)
{
    TOXAV_ERR_OPTION_SET rc = TOXAV_ERR_OPTION_SET_OK;

    ToxAVCall *call;

    LOGGER_DEBUG(av->m->log, "toxav_option_set:1 %d %d", (int)option, (int)value);

    if (m_friend_exists(av->m, friend_number) == 0) {
        rc = TOXAV_ERR_OPTION_SET_OTHER_ERROR;
        goto END;
    }

    pthread_mutex_lock(av->mutex);
    call = call_get(av, friend_number);

    if (call == NULL || !call->active || call->msi_call->state != msi_CallActive) {
        pthread_mutex_unlock(av->mutex);
        rc = TOXAV_ERR_OPTION_SET_OTHER_ERROR;
        goto END;
    }

    pthread_mutex_lock(call->mutex);

    LOGGER_DEBUG(av->m->log, "toxav_option_set:2 %d %d", (int)option, (int)value);

    if (option == TOXAV_ENCODER_CPU_USED) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_encoder_cpu_used == (int32_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder cpu_used already set to: %d", (int)value);
        } else {
            vc->video_encoder_cpu_used_prev = vc->video_encoder_cpu_used;
            vc->video_encoder_cpu_used = (int32_t)value;
            LOGGER_WARNING(av->m->log, "video encoder setting cpu_used to: %d", (int)value);
        }
    } else if (option == TOXAV_ENCODER_CODEC_USED) {
        VCSession *vc = (VCSession *)call->video.second;

        if (((int32_t)value >= TOXAV_ENCODER_CODEC_USED_VP8)
                && ((int32_t)value <= TOXAV_ENCODER_CODEC_USED_H264)) {

            if (vc->video_encoder_coded_used == (int32_t)value) {
                LOGGER_WARNING(av->m->log, "video video_encoder_coded_used already set to: %d", (int)value);
            } else {
                vc->video_encoder_coded_used_prev = vc->video_encoder_coded_used;
                vc->video_encoder_coded_used = (int32_t)value;
                LOGGER_WARNING(av->m->log, "video video_encoder_coded_used to: %d", (int)value);
            }
        }
    } else if (option == TOXAV_ENCODER_VP8_QUALITY) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_encoder_vp8_quality == (int32_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder vp8_quality already set to: %d", (int)value);
        } else {
            vc->video_encoder_vp8_quality_prev = vc->video_encoder_vp8_quality;
            vc->video_encoder_vp8_quality = (int32_t)value;

            if (vc->video_encoder_vp8_quality == TOXAV_ENCODER_VP8_QUALITY_HIGH) {
                vc->video_rc_max_quantizer_prev = vc->video_rc_max_quantizer;
                vc->video_rc_min_quantizer_prev = vc->video_rc_min_quantizer;
                vc->video_rc_max_quantizer = TOXAV_ENCODER_VP8_RC_MAX_QUANTIZER_HIGH;
                vc->video_rc_min_quantizer = TOXAV_ENCODER_VP8_RC_MIN_QUANTIZER_HIGH;
            } else {
                vc->video_rc_max_quantizer_prev = vc->video_rc_max_quantizer;
                vc->video_rc_min_quantizer_prev = vc->video_rc_min_quantizer;
                vc->video_rc_max_quantizer = TOXAV_ENCODER_VP8_RC_MAX_QUANTIZER_NORMAL;
                vc->video_rc_min_quantizer = TOXAV_ENCODER_VP8_RC_MIN_QUANTIZER_NORMAL;
            }

            LOGGER_WARNING(av->m->log, "video encoder setting vp8_quality to: %d", (int)value);
        }
    } else if (option == TOXAV_ENCODER_RC_MAX_QUANTIZER) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_rc_max_quantizer == (int32_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder rc_max_quantizer already set to: %d", (int)value);
        } else {
            vc->video_rc_max_quantizer_prev = vc->video_rc_max_quantizer;
            vc->video_rc_max_quantizer = (int32_t)value;
            LOGGER_WARNING(av->m->log, "video encoder setting rc_max_quantizer to: %d", (int)value);
        }
    } else if (option == TOXAV_ENCODER_VIDEO_MAX_BITRATE) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_max_bitrate == (int32_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder video_max_bitrate already set to: %d", (int)value);
        } else {
            vc->video_max_bitrate = (int32_t)value;

            if (call->video_bit_rate > (uint32_t)vc->video_max_bitrate) {
                call->video_bit_rate = (uint32_t)vc->video_max_bitrate;
            }

            LOGGER_WARNING(av->m->log, "video encoder setting video_max_bitrate to: %d", (int)value);
        }
    } else if (option == TOXAV_ENCODER_VIDEO_BITRATE_AUTOSET) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_bitrate_autoset == (uint8_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder video_bitrate_autoset already set to: %d", (int)value);
        } else {
            vc->video_bitrate_autoset = (uint8_t)value;
            LOGGER_WARNING(av->m->log, "video encoder setting video_bitrate_autoset to: %d", (int)value);
        }
    } else if (option == TOXAV_ENCODER_KF_METHOD) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_keyframe_method == (int32_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder keyframe_method already set to: %d", (int)value);
        } else {
            vc->video_keyframe_method_prev = vc->video_keyframe_method;
            vc->video_keyframe_method = (int32_t)value;
            LOGGER_WARNING(av->m->log, "video encoder setting keyframe_method to: %d", (int)value);
        }
    } else if (option == TOXAV_ENCODER_RC_MIN_QUANTIZER) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_rc_min_quantizer == (int32_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder video_rc_min_quantizer already set to: %d", (int)value);
        } else {
            vc->video_rc_min_quantizer_prev = vc->video_rc_min_quantizer;
            vc->video_rc_min_quantizer = (int32_t)value;
            LOGGER_WARNING(av->m->log, "video encoder setting video_rc_min_quantizer to: %d", (int)value);
        }
    } else if (option == TOXAV_DECODER_ERROR_CONCEALMENT) {
        VCSession *vc = (VCSession *)call->video.second;

        if (vc->video_decoder_error_concealment == (int32_t)value) {
            LOGGER_WARNING(av->m->log, "video encoder video_decoder_error_concealment already set to: %d", (int)value);
        } else {
            vc->video_decoder_error_concealment_prev = vc->video_decoder_error_concealment;
            vc->video_decoder_error_concealment = (int32_t)value;
            LOGGER_WARNING(av->m->log, "video encoder setting video_decoder_error_concealment to: %d", (int)value);
        }
    }

    pthread_mutex_unlock(call->mutex);
    pthread_mutex_unlock(av->mutex);
END:

    if (error) {
        *error = rc;
    }

    return rc == TOXAV_ERR_OPTION_SET_OK;
}

bool toxav_video_set_bit_rate(ToxAV *av, uint32_t friend_number, int32_t video_bit_rate, TOXAV_ERR_BIT_RATE_SET *error)
{
    return true;
}

bool toxav_audio_set_bit_rate(ToxAV *av, uint32_t friend_number, int32_t audio_bit_rate, TOXAV_ERR_BIT_RATE_SET *error)
{
    return true;
}

bool toxav_bit_rate_set(ToxAV *av, uint32_t friend_number, int32_t audio_bit_rate,
                        int32_t video_bit_rate, TOXAV_ERR_BIT_RATE_SET *error)
{
    TOXAV_ERR_BIT_RATE_SET rc = TOXAV_ERR_BIT_RATE_SET_OK;
    ToxAVCall *call;

    if (m_friend_exists(av->m, friend_number) == 0) {
        rc = TOXAV_ERR_BIT_RATE_SET_FRIEND_NOT_FOUND;
        goto END;
    }

    if (audio_bit_rate > 0 && audio_bit_rate_invalid(audio_bit_rate)) {
        rc = TOXAV_ERR_BIT_RATE_SET_INVALID_BIT_RATE;
        goto END;
    }

    if (video_bit_rate > 0 && video_bit_rate_invalid(video_bit_rate)) {
        rc = TOXAV_ERR_BIT_RATE_SET_INVALID_BIT_RATE;
        goto END;
    }

    pthread_mutex_lock(av->mutex);
    call = call_get(av, friend_number);

    if (call == NULL || !call->active || call->msi_call->state != msi_CallActive) {
        pthread_mutex_unlock(av->mutex);
        rc = TOXAV_ERR_BIT_RATE_SET_FRIEND_NOT_IN_CALL;
        goto END;
    }

    if (audio_bit_rate >= 0) {
        LOGGER_DEBUG(av->m->log, "Setting new audio bitrate to: %d", audio_bit_rate);

        if (call->audio_bit_rate == (uint32_t)audio_bit_rate) {
            LOGGER_DEBUG(av->m->log, "Audio bitrate already set to: %d", audio_bit_rate);
        } else if (audio_bit_rate == 0) {
            LOGGER_DEBUG(av->m->log, "Turned off audio sending");

            if (msi_change_capabilities(call->msi_call, call->msi_call->
                                        self_capabilities ^ msi_CapSAudio) != 0) {
                pthread_mutex_unlock(av->mutex);
                rc = TOXAV_ERR_BIT_RATE_SET_SYNC;
                goto END;
            }

            /* Audio sending is turned off; notify peer */
            call->audio_bit_rate = 0;
        } else {
            pthread_mutex_lock(call->mutex);

            if (call->audio_bit_rate == 0) {
                LOGGER_DEBUG(av->m->log, "Turned on audio sending");

                /* The audio has been turned off before this */
                if (msi_change_capabilities(call->msi_call, call->
                                            msi_call->self_capabilities | msi_CapSAudio) != 0) {
                    pthread_mutex_unlock(call->mutex);
                    pthread_mutex_unlock(av->mutex);
                    rc = TOXAV_ERR_BIT_RATE_SET_SYNC;
                    goto END;
                }
            } else {
                LOGGER_DEBUG(av->m->log, "Set new audio bit rate %d", audio_bit_rate);
            }

            call->audio_bit_rate = audio_bit_rate;
            pthread_mutex_unlock(call->mutex);
        }
    }

    if (video_bit_rate >= 0) {
        LOGGER_DEBUG(av->m->log, "Setting new video bitrate to: %d", video_bit_rate);

        if (call->video_bit_rate == (uint32_t)video_bit_rate) {
            LOGGER_DEBUG(av->m->log, "Video bitrate already set to: %d", video_bit_rate);
        } else if (video_bit_rate == 0) {
            LOGGER_DEBUG(av->m->log, "Turned off video sending");

            /* Video sending is turned off; notify peer */
            if (msi_change_capabilities(call->msi_call, call->msi_call->
                                        self_capabilities ^ msi_CapSVideo) != 0) {
                pthread_mutex_unlock(av->mutex);
                rc = TOXAV_ERR_BIT_RATE_SET_SYNC;
                goto END;
            }

            call->video_bit_rate = 0;
        } else {
            pthread_mutex_lock(call->mutex);

            if (call->video_bit_rate == 0) {
                LOGGER_DEBUG(av->m->log, "Turned on video sending");

                /* The video has been turned off before this */
                if (msi_change_capabilities(call->msi_call, call->
                                            msi_call->self_capabilities | msi_CapSVideo) != 0) {
                    pthread_mutex_unlock(call->mutex);
                    pthread_mutex_unlock(av->mutex);
                    rc = TOXAV_ERR_BIT_RATE_SET_SYNC;
                    goto END;
                }
            } else {
                LOGGER_DEBUG(av->m->log, "Set new video bit rate %d", video_bit_rate);
            }

            call->video_bit_rate = video_bit_rate;

            LOGGER_ERROR(av->m->log, "toxav_bit_rate_set:vb=%d", (int)video_bit_rate);

            pthread_mutex_unlock(call->mutex);
        }
    }

    pthread_mutex_unlock(av->mutex);
END:

    if (error) {
        *error = rc;
    }

    return rc == TOXAV_ERR_BIT_RATE_SET_OK;
}

void toxav_callback_bit_rate_status(ToxAV *av, toxav_bit_rate_status_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->bcb.first = callback;
    av->bcb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}


void toxav_callback_audio_bit_rate(ToxAV *av, toxav_audio_bit_rate_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->abcb.first = callback;
    av->abcb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}
void toxav_callback_video_bit_rate(ToxAV *av, toxav_video_bit_rate_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->vbcb.first = callback;
    av->vbcb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}

bool toxav_audio_send_frame(ToxAV *av, uint32_t friend_number, const int16_t *pcm, size_t sample_count,
                            uint8_t channels, uint32_t sampling_rate, TOXAV_ERR_SEND_FRAME *error)
{
    TOXAV_ERR_SEND_FRAME rc = TOXAV_ERR_SEND_FRAME_OK;
    ToxAVCall *call;

    uint64_t audio_frame_record_timestamp = current_time_monotonic();

    if (m_friend_exists(av->m, friend_number) == 0) {
        rc = TOXAV_ERR_SEND_FRAME_FRIEND_NOT_FOUND;
        goto END;
    }

    if (pthread_mutex_trylock(av->mutex) != 0) {
        rc = TOXAV_ERR_SEND_FRAME_SYNC;
        goto END;
    }

    call = call_get(av, friend_number);

    if (call == NULL || !call->active || call->msi_call->state != msi_CallActive) {
        pthread_mutex_unlock(av->mutex);
        rc = TOXAV_ERR_SEND_FRAME_FRIEND_NOT_IN_CALL;
        goto END;
    }

    if (call->audio_bit_rate == 0 ||
            !(call->msi_call->self_capabilities & msi_CapSAudio) ||
            !(call->msi_call->peer_capabilities & msi_CapRAudio)) {
        pthread_mutex_unlock(av->mutex);
        rc = TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED;
        goto END;
    }

    pthread_mutex_lock(call->mutex_audio);
    pthread_mutex_unlock(av->mutex);

    if (pcm == NULL) {
        pthread_mutex_unlock(call->mutex_audio);
        rc = TOXAV_ERR_SEND_FRAME_NULL;
        goto END;
    }

    if (channels > 2) {
        pthread_mutex_unlock(call->mutex_audio);
        rc = TOXAV_ERR_SEND_FRAME_INVALID;
        goto END;
    }

    { /* Encode and send */
        if (ac_reconfigure_encoder(call->audio.second, call->audio_bit_rate * 1000, sampling_rate, channels) != 0) {
            pthread_mutex_unlock(call->mutex_audio);
            LOGGER_WARNING(av->m->log, "Failed reconfigure audio encoder");
            rc = TOXAV_ERR_SEND_FRAME_INVALID;
            goto END;
        }

        VLA(uint8_t, dest, sample_count + sizeof(sampling_rate)); /* This is more than enough always */

        sampling_rate = net_htonl(sampling_rate);
        memcpy(dest, &sampling_rate, sizeof(sampling_rate));
        int vrc = opus_encode(call->audio.second->encoder, pcm, sample_count,
                              dest + sizeof(sampling_rate), SIZEOF_VLA(dest) - sizeof(sampling_rate));

        if (vrc < 0) {
            LOGGER_WARNING(av->m->log, "Failed to encode audio frame %s", opus_strerror(vrc));
            pthread_mutex_unlock(call->mutex_audio);
            rc = TOXAV_ERR_SEND_FRAME_INVALID;
            goto END;
        }

#if defined(AUDIO_DEBUGGING_SIMULATE_SOME_DATA_LOSS)
        // set last part of audio frame to all zeros
        size_t ten_percent_size = ((size_t)vrc / 10);
        size_t start_offset = ((size_t)vrc - ten_percent_size - 1);
        memset((dest + 4 + start_offset), (int)0, ten_percent_size);
        LOGGER_WARNING(av->m->log, "* audio packet set some ZERO data at the end *");
#endif

#if defined(AUDIO_DEBUGGING_SKIP_FRAMES)
        // skip sending some audio frames
        _debug_count_sent_audio_frames++;

        if (_debug_count_sent_audio_frames > _debug_skip_every_x_audio_frame) {
            call->audio.first->sequnum++;
            LOGGER_WARNING(av->m->log, "* audio packet sending SKIPPED * %d", (int)call->audio.first->sequnum);
            _debug_count_sent_audio_frames = 0;
        } else {
#endif
            LOGGER_DEBUG(av->m->log, "audio packet record time: seqnum=%d %lu", (int)call->audio.first->sequnum,
                         audio_frame_record_timestamp);

            uint16_t seq_num_save = call->audio.first->sequnum;

            if (rtp_send_data(call->audio.first, dest,
                              vrc + sizeof(sampling_rate),
                              false,
                              audio_frame_record_timestamp,
                              VIDEO_FRAGMENT_NUM_NO_FRAG,
                              0,
                              call->audio_bit_rate,
                              av->m->log) != 0) {
                LOGGER_WARNING(av->m->log, "Failed to send audio packet");
                rc = TOXAV_ERR_SEND_FRAME_RTP_FAILED;
            }

#if 0
            // HINT: send audio data 2 times --------------------------
            ((RTPSession *)call->audio.first)->sequnum = seq_num_save;

            if (rtp_send_data(call->audio.first, dest,
                              vrc + sizeof(sampling_rate),
                              false,
                              audio_frame_record_timestamp,
                              VIDEO_FRAGMENT_NUM_NO_FRAG,
                              0,
                              call->audio_bit_rate,
                              av->m->log) != 0) {
            }

            // HINT: send audio data 2 times --------------------------
#endif


#if defined(AUDIO_DEBUGGING_SKIP_FRAMES)
        }

#endif

    }


    pthread_mutex_unlock(call->mutex_audio);

END:

    if (error) {
        *error = rc;
    }

    return rc == TOXAV_ERR_SEND_FRAME_OK;
}


/* --- VIDEO EN-CODING happens here --- */
/* --- VIDEO EN-CODING happens here --- */
/* --- VIDEO EN-CODING happens here --- */
bool toxav_video_send_frame(ToxAV *av, uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t *y,
                            const uint8_t *u, const uint8_t *v, TOXAV_ERR_SEND_FRAME *error)
{
    TOXAV_ERR_SEND_FRAME rc = TOXAV_ERR_SEND_FRAME_OK;
    ToxAVCall *call;

    // LOGGER_ERROR(av->m->log, "OMX:H:001");

    uint64_t video_frame_record_timestamp = current_time_monotonic();

    if (m_friend_exists(av->m, friend_number) == 0) {
        rc = TOXAV_ERR_SEND_FRAME_FRIEND_NOT_FOUND;
        goto END;
    }

    if (pthread_mutex_trylock(av->mutex) != 0) {
        rc = TOXAV_ERR_SEND_FRAME_SYNC;
        goto END;
    }

    call = call_get(av, friend_number);

    if (call == NULL || !call->active || call->msi_call->state != msi_CallActive) {
        pthread_mutex_unlock(av->mutex);
        rc = TOXAV_ERR_SEND_FRAME_FRIEND_NOT_IN_CALL;
        goto END;
    }


    if (call->video.second->skip_fps != 0) {
        call->video.second->skip_fps_counter++;

        if (call->video.second->skip_fps_duration_until_ts > current_time_monotonic()) {
            // HINT: ok stop skipping frames now, and reset the values
            call->video.second->skip_fps = 0;
            call->video.second->skip_fps_duration_until_ts = 0;
        } else {

            if (call->video.second->skip_fps_counter == call->video.second->skip_fps) {
                LOGGER_DEBUG(av->m->log, "VIDEO:Skipping frame, because of too much FPS!!");
                call->video.second->skip_fps_counter = 0;
                // skip this video frame, receiver can't handle this many FPS
                // rc = TOXAV_ERR_SEND_FRAME_INVALID; // should we tell the client? not sure about this
                //                                     // client may try to resend the frame, which is not what we want
                pthread_mutex_unlock(av->mutex);
                goto END;
            }
        }
    }

    uint64_t ms_to_last_frame = 1;

    if (call->video.second) {
        ms_to_last_frame = current_time_monotonic() - call->video.second->last_encoded_frame_ts;

        if (call->video.second->last_encoded_frame_ts == 0) {
            ms_to_last_frame = 1;
        }
    }

    if (call->video_bit_rate == 0 ||
            !(call->msi_call->self_capabilities & msi_CapSVideo) ||
            !(call->msi_call->peer_capabilities & msi_CapRVideo)) {
        pthread_mutex_unlock(av->mutex);
        rc = TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED;
        goto END;
    }

    pthread_mutex_lock(call->mutex_video);
    pthread_mutex_unlock(av->mutex);

    if (y == NULL || u == NULL || v == NULL) {
        pthread_mutex_unlock(call->mutex_video);
        rc = TOXAV_ERR_SEND_FRAME_NULL;
        goto END;
    }


    // LOGGER_ERROR(av->m->log, "h264_video_capabilities_received=%d",
    //             (int)call->video.second->h264_video_capabilities_received);

    int16_t force_reinit_encoder = -1;

    // HINT: auto switch encoder, if we got capabilities packet from friend ------
    if ((call->video.second->h264_video_capabilities_received == 1)
            &&
            (call->video.second->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_H264)) {
        // when switching to H264 set default video bitrate

        if (call->video_bit_rate > 0) {
            call->video_bit_rate = VIDEO_BITRATE_INITIAL_VALUE_H264;
        }

        call->video.second->video_encoder_coded_used = TOXAV_ENCODER_CODEC_USED_H264;
        // LOGGER_ERROR(av->m->log, "TOXAV_ENCODER_CODEC_USED_H264");
        force_reinit_encoder = -2;

        if (av->call_comm_cb.first) {

            TOXAV_CALL_COMM_INFO cmi;
            cmi = TOXAV_CALL_COMM_ENCODER_IN_USE_H264;

            if (call->video.second->video_encoder_coded_used_hw_accel == TOXAV_ENCODER_CODEC_HW_ACCEL_OMX_PI) {
                cmi = TOXAV_CALL_COMM_ENCODER_IN_USE_H264_OMX_PI;
            }

            av->call_comm_cb.first(av, friend_number, cmi,
                                   0, av->call_comm_cb.second);
        }

    }

    // HINT: auto switch encoder, if we got capabilities packet from friend ------

    if ((call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP8)
            || (call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9)) {

        if (vc_reconfigure_encoder(av->m->log, call->video.second, call->video_bit_rate * 1000,
                                   width, height, -1) != 0) {
            pthread_mutex_unlock(call->mutex_video);
            rc = TOXAV_ERR_SEND_FRAME_INVALID;
            goto END;
        }
    } else {
        // HINT: H264
        if (vc_reconfigure_encoder(av->m->log, call->video.second, call->video_bit_rate * 1000,
                                   width, height, force_reinit_encoder) != 0) {
            pthread_mutex_unlock(call->mutex_video);
            rc = TOXAV_ERR_SEND_FRAME_INVALID;
            goto END;
        }
    }

    if ((call->video_bit_rate_last_last_changed_cb_ts + 2000) < current_time_monotonic()) {
        if (call->video_bit_rate_last_last_changed != call->video_bit_rate) {
            if (av->call_comm_cb.first) {
                av->call_comm_cb.first(av, friend_number,
                                       TOXAV_CALL_COMM_ENCODER_CURRENT_BITRATE,
                                       (int64_t)call->video_bit_rate,
                                       av->call_comm_cb.second);
            }

            call->video_bit_rate_last_last_changed = call->video_bit_rate;
        }

        call->video_bit_rate_last_last_changed_cb_ts = current_time_monotonic();
    }

    int vpx_encode_flags = 0;
    unsigned long max_encode_time_in_us = MAX_ENCODE_TIME_US;

    int h264_iframe_factor = 1;

    if (call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_H264) {
        h264_iframe_factor = 1;
    }

    if (call->video.second->video_keyframe_method == TOXAV_ENCODER_KF_METHOD_NORMAL) {
        if (call->video.first->ssrc < (uint32_t)(VIDEO_SEND_X_KEYFRAMES_FIRST * h264_iframe_factor)) {

            if (call->video.second->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
                // Key frame flag for first frames
                vpx_encode_flags = VPX_EFLAG_FORCE_KF;
                vpx_encode_flags |= VP8_EFLAG_FORCE_GF;
                // vpx_encode_flags |= VP8_EFLAG_FORCE_ARF;

                max_encode_time_in_us = VPX_DL_REALTIME;
                // uint32_t lowered_bitrate = (300 * 1000);
                // vc_reconfigure_encoder_bitrate_only(call->video.second, lowered_bitrate);
                // HINT: Zoff: this does not seem to work
                // vpx_codec_control(call->video.second->encoder, VP8E_SET_FRAME_FLAGS, vpx_encode_flags);
                // LOGGER_ERROR(av->m->log, "I_FRAME_FLAG:%d only-i-frame mode", call->video.first->ssrc);
            }

#ifdef RASPBERRY_PI_OMX
            LOGGER_ERROR(av->m->log, "H264: force i-frame");
            h264_omx_raspi_force_i_frame(av->m->log, call->video.second);
#endif

            call->video.first->ssrc++;
        } else if (call->video.first->ssrc == (uint32_t)(VIDEO_SEND_X_KEYFRAMES_FIRST * h264_iframe_factor)) {
            if (call->video.second->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
                // normal keyframe placement
                vpx_encode_flags = 0;
                max_encode_time_in_us = MAX_ENCODE_TIME_US;
                LOGGER_INFO(av->m->log, "I_FRAME_FLAG:%d normal mode", call->video.first->ssrc);
            }

            call->video.first->ssrc++;
        }
    }


#ifdef VIDEO_ENCODER_SOFT_DEADLINE_AUTOTUNE
    long encode_time_auto_tune = MAX_ENCODE_TIME_US;

    if (call->video.second->last_encoded_frame_ts > 0) {
        encode_time_auto_tune = (current_time_monotonic() - call->video.second->last_encoded_frame_ts) * 1000;
#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS
        encode_time_auto_tune = encode_time_auto_tune * VIDEO_CODEC_FRAGMENT_NUMS;
#endif

        if (encode_time_auto_tune == 0) {
            // if the real delay was 0ms then still use 1ms
            encode_time_auto_tune = 1;
        }

        if (call->video.second->encoder_soft_deadline[call->video.second->encoder_soft_deadline_index] == 0) {
            call->video.second->encoder_soft_deadline[call->video.second->encoder_soft_deadline_index] = 1;
            LOGGER_DEBUG(av->m->log, "AUTOTUNE: delay=[1]");
        } else {
            call->video.second->encoder_soft_deadline[call->video.second->encoder_soft_deadline_index] = encode_time_auto_tune;
            LOGGER_DEBUG(av->m->log, "AUTOTUNE: delay=%d", (int)encode_time_auto_tune);
        }

        call->video.second->encoder_soft_deadline_index = (call->video.second->encoder_soft_deadline_index + 1) %
                VIDEO_ENCODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES;

        // calc mean value
        encode_time_auto_tune = 0;

        for (int k = 0; k < VIDEO_ENCODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES; k++) {
            encode_time_auto_tune = encode_time_auto_tune + call->video.second->encoder_soft_deadline[k];
        }

        encode_time_auto_tune = encode_time_auto_tune / VIDEO_ENCODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES;

        if (encode_time_auto_tune > (1000000 / VIDEO_ENCODER_MINFPS_AUTOTUNE)) {
            encode_time_auto_tune = (1000000 / VIDEO_ENCODER_MINFPS_AUTOTUNE);
        }

        if (encode_time_auto_tune > (VIDEO_ENCODER_LEEWAY_IN_MS_AUTOTUNE * 1000)) {
            encode_time_auto_tune = encode_time_auto_tune - (VIDEO_ENCODER_LEEWAY_IN_MS_AUTOTUNE * 1000); // give x ms more room
        }

        if (encode_time_auto_tune == 0) {
            // if the real delay was 0ms then still use 1ms
            encode_time_auto_tune = 1;
        }
    }

    max_encode_time_in_us = encode_time_auto_tune;
    LOGGER_DEBUG(av->m->log, "AUTOTUNE:MAX_ENCODE_TIME_US=%ld us = %.1f fps", (long)encode_time_auto_tune,
                 (float)(1000000.0f / encode_time_auto_tune));
#endif


    // we start with I-frames (full frames) and then switch to normal mode later

    call->video.second->last_encoded_frame_ts = current_time_monotonic();

    if (call->video.second->send_keyframe_request_received == 1) {
        vpx_encode_flags = VPX_EFLAG_FORCE_KF;
        vpx_encode_flags |= VP8_EFLAG_FORCE_GF;
        // vpx_encode_flags |= VP8_EFLAG_FORCE_ARF;
        call->video.second->send_keyframe_request_received = 0;
    } else {
        LOGGER_DEBUG(av->m->log, "++++ FORCE KEYFRAME ++++:%d %d %d",
                     (int)call->video.second->last_sent_keyframe_ts,
                     (int)VIDEO_MIN_SEND_KEYFRAME_INTERVAL,
                     (int)current_time_monotonic());

        if ((call->video.second->last_sent_keyframe_ts + VIDEO_MIN_SEND_KEYFRAME_INTERVAL)
                < current_time_monotonic()) {
            // it's been x seconds without a keyframe, send one now
            vpx_encode_flags = VPX_EFLAG_FORCE_KF;
            vpx_encode_flags |= VP8_EFLAG_FORCE_GF;

            LOGGER_DEBUG(av->m->log, "1***** FORCE KEYFRAME ++++");
            LOGGER_DEBUG(av->m->log, "2***** FORCE KEYFRAME ++++");
            LOGGER_DEBUG(av->m->log, "3***** FORCE KEYFRAME ++++");
            LOGGER_DEBUG(av->m->log, "4***** FORCE KEYFRAME ++++");
            // vpx_encode_flags |= VP8_EFLAG_FORCE_ARF;
        } else {
            // vpx_encode_flags |= VP8_EFLAG_FORCE_GF;
            // vpx_encode_flags |= VP8_EFLAG_NO_REF_GF;
            // vpx_encode_flags |= VP8_EFLAG_NO_REF_ARF;
            // vpx_encode_flags |= VP8_EFLAG_NO_REF_LAST;
            // vpx_encode_flags |= VP8_EFLAG_NO_UPD_GF;
            // vpx_encode_flags |= VP8_EFLAG_NO_UPD_ARF;
        }
    }


    // for the H264 encoder -------
    x264_nal_t *nal = NULL;
    int i_frame_size = 0;
    // for the H264 encoder -------


    { /* Encode */


        if ((call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP8)
                || (call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9)) {

            LOGGER_DEBUG(av->m->log, "++++++ encoding VP8 frame ++++++");

            // HINT: vp8
            uint32_t result = encode_frame_vpx(av, friend_number, width, height,
                                               y, u, v, call,
                                               &video_frame_record_timestamp,
                                               vpx_encode_flags,
                                               &nal,
                                               &i_frame_size);

            if (result != 0) {
                pthread_mutex_unlock(call->mutex_video);
                rc = TOXAV_ERR_SEND_FRAME_INVALID;
                goto END;
            }

        } else {

            LOGGER_DEBUG(av->m->log, "**##** encoding H264 frame **##**");

            // HINT: H264
#ifdef RASPBERRY_PI_OMX
            uint32_t result = encode_frame_h264_omx_raspi(av, friend_number, width, height,
                              y, u, v, call,
                              &video_frame_record_timestamp,
                              vpx_encode_flags,
                              &nal,
                              &i_frame_size);
#else
            uint32_t result = encode_frame_h264(av, friend_number, width, height,
                                                y, u, v, call,
                                                &video_frame_record_timestamp,
                                                vpx_encode_flags,
                                                &nal,
                                                &i_frame_size);
#endif

            if (result != 0) {
                pthread_mutex_unlock(call->mutex_video);
                rc = TOXAV_ERR_SEND_FRAME_INVALID;
                goto END;
            }

        }
    }

    ++call->video.second->frame_counter;

    LOGGER_DEBUG(av->m->log, "VPXENC:======================\n");
    LOGGER_DEBUG(av->m->log, "VPXENC:frame num=%ld\n", (long)call->video.second->frame_counter);


    { /* Send frames */

        if ((call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP8)
                || (call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9)) {

            uint32_t result = send_frames_vpx(av, friend_number, width, height,
                                              y, u, v, call,
                                              &video_frame_record_timestamp,
                                              vpx_encode_flags,
                                              &nal,
                                              &i_frame_size,
                                              &rc);

            if (result != 0) {
                pthread_mutex_unlock(call->mutex_video);
                goto END;
            }

        } else {
            // HINT: H264
#ifdef RASPBERRY_PI_OMX
            uint32_t result = send_frames_h264_omx_raspi(av, friend_number, width, height,
                              y, u, v, call,
                              &video_frame_record_timestamp,
                              vpx_encode_flags,
                              &nal,
                              &i_frame_size,
                              &rc);

#else
            uint32_t result = send_frames_h264(av, friend_number, width, height,
                                               y, u, v, call,
                                               &video_frame_record_timestamp,
                                               vpx_encode_flags,
                                               &nal,
                                               &i_frame_size,
                                               &rc);
#endif

            if (result != 0) {
                pthread_mutex_unlock(call->mutex_video);
                goto END;
            }
        }
    }

    pthread_mutex_unlock(call->mutex_video);

END:

    if (error) {
        *error = rc;
    }

    return rc == TOXAV_ERR_SEND_FRAME_OK;
}
/* --- VIDEO EN-CODING happens here --- */
/* --- VIDEO EN-CODING happens here --- */
/* --- VIDEO EN-CODING happens here --- */





void toxav_callback_audio_receive_frame(ToxAV *av, toxav_audio_receive_frame_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->acb.first = callback;
    av->acb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}

void toxav_callback_video_receive_frame(ToxAV *av, toxav_video_receive_frame_cb *callback, void *user_data)
{
    pthread_mutex_lock(av->mutex);
    av->vcb.first = callback;
    av->vcb.second = user_data;
    pthread_mutex_unlock(av->mutex);
}


/*******************************************************************************
 *
 * :: Internal
 *
 ******************************************************************************/
void callback_bwc(BWController *bwc, uint32_t friend_number, float loss, void *user_data)
{
    /* Callback which is called when the internal measure mechanism reported packet loss.
     * We report suggested lowered bitrate to an app. If app is sending both audio and video,
     * we will report lowered bitrate for video only because in that case video probably
     * takes more than 90% bandwidth. Otherwise, we report lowered bitrate on audio.
     * The application may choose to disable video totally if the stream is too bad.
     */

    ToxAVCall *call = (ToxAVCall *)user_data;

    if (!call) {
        return;
    }

    if (!call->av) {
        return;
    }

    if (call->video_bit_rate == 0) {
        // HINT: video is turned off -> just do nothing
        return;
    }

    if (!call->video.second) {
        return;
    }

    if (call->video.second->video_bitrate_autoset == 0) {
        // HINT: client does not want bitrate autoset
        return;
    }

    if (loss > 0) {
        LOGGER_ERROR(call->av->m->log, "Reported loss of %f%% : %f", loss * 100, loss);
    }

    pthread_mutex_lock(call->av->mutex);

    // HINT: on high bitrates we lower the bitrate even on small data loss
    if (call->video_bit_rate > VIDEO_BITRATE_SCALAR3_AUTO_VALUE_H264) {
        if ((loss * 100) > VIDEO_BITRATE_AUTO_INC_THRESHOLD) {
            int64_t tmp = (int64_t)((float)call->video_bit_rate * ((1.0f - loss) * VIDEO_BITRATE_AUTO_DEC_FACTOR));

            if (tmp <= 0) {
                tmp = VIDEO_BITRATE_MIN_AUTO_VALUE_H264;
            }

            call->video_bit_rate = (uint32_t)tmp;

            // HINT: sanity check --------------
            if (call->video_bit_rate < VIDEO_BITRATE_MIN_AUTO_VALUE_H264) {
                call->video_bit_rate = VIDEO_BITRATE_MIN_AUTO_VALUE_H264;
            } else if (call->video_bit_rate > VIDEO_BITRATE_MAX_AUTO_VALUE_H264) {
                call->video_bit_rate = VIDEO_BITRATE_MAX_AUTO_VALUE_H264;
            }

            if (call->video_bit_rate > (uint32_t)call->video.second->video_max_bitrate) {
                call->video_bit_rate = (uint32_t)call->video.second->video_max_bitrate;
            }

            // HINT: sanity check --------------

            LOGGER_ERROR(call->av->m->log, "callback_bwc:DEC:1:H:vb=%d loss=%d", (int)call->video_bit_rate, (int)(loss * 100));

            pthread_mutex_unlock(call->av->mutex);
            return;
        }
    }

    if ((loss * 100) < VIDEO_BITRATE_AUTO_INC_THRESHOLD) {
        if (call->video_bit_rate < VIDEO_BITRATE_MAX_AUTO_VALUE_H264) {

            if (call->video_bit_rate < VIDEO_BITRATE_SCALAR_AUTO_VALUE_H264) {
                call->video_bit_rate = call->video_bit_rate + VIDEO_BITRATE_SCALAR_INC_BY_AUTO_VALUE_H264;
            } else if (call->video_bit_rate > VIDEO_BITRATE_SCALAR2_AUTO_VALUE_H264) {
                call->video_bit_rate = call->video_bit_rate + VIDEO_BITRATE_SCALAR2_INC_BY_AUTO_VALUE_H264;
            } else {
                call->video_bit_rate = (uint32_t)((float)call->video_bit_rate * (float)VIDEO_BITRATE_AUTO_INC_TO);
            }

            // HINT: sanity check --------------
            if (call->video_bit_rate < VIDEO_BITRATE_MIN_AUTO_VALUE_H264) {
                call->video_bit_rate = VIDEO_BITRATE_MIN_AUTO_VALUE_H264;
            } else if (call->video_bit_rate > VIDEO_BITRATE_MAX_AUTO_VALUE_H264) {
                call->video_bit_rate = VIDEO_BITRATE_MAX_AUTO_VALUE_H264;
            }

            if (call->video_bit_rate > (uint32_t)call->video.second->video_max_bitrate) {
                call->video_bit_rate = (uint32_t)call->video.second->video_max_bitrate;
            }

            // HINT: sanity check --------------

            LOGGER_DEBUG(call->av->m->log, "callback_bwc:INC:vb=%d loss=%d", (int)call->video_bit_rate, (int)(loss * 100));
        }
    } else if ((loss * 100) > VIDEO_BITRATE_AUTO_DEC_THRESHOLD) {
        if (call->video_bit_rate > VIDEO_BITRATE_MIN_AUTO_VALUE_H264) {

            int64_t tmp = (int64_t)((float)call->video_bit_rate * ((1.0f - loss) * VIDEO_BITRATE_AUTO_DEC_FACTOR));

            if (tmp <= 0) {
                tmp = VIDEO_BITRATE_MIN_AUTO_VALUE_H264;
            }

            call->video_bit_rate = (uint32_t)tmp;

            // HINT: sanity check --------------
            if (call->video_bit_rate < VIDEO_BITRATE_MIN_AUTO_VALUE_H264) {
                call->video_bit_rate = VIDEO_BITRATE_MIN_AUTO_VALUE_H264;
            } else if (call->video_bit_rate > VIDEO_BITRATE_MAX_AUTO_VALUE_H264) {
                call->video_bit_rate = VIDEO_BITRATE_MAX_AUTO_VALUE_H264;
            }

            if (call->video_bit_rate > (uint32_t)call->video.second->video_max_bitrate) {
                call->video_bit_rate = (uint32_t)call->video.second->video_max_bitrate;
            }

            // HINT: sanity check --------------

            LOGGER_ERROR(call->av->m->log, "callback_bwc:DEC:vb=%d loss=%d", (int)call->video_bit_rate, (int)(loss * 100));
        }
    }

    // HINT: sanity check --------------

    if (call->video.second->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_H264) {
        if (call->video_bit_rate < VIDEO_BITRATE_MIN_AUTO_VALUE_H264) {
            call->video_bit_rate = VIDEO_BITRATE_MIN_AUTO_VALUE_H264;
        } else if (call->video_bit_rate > VIDEO_BITRATE_MAX_AUTO_VALUE_H264) {
            call->video_bit_rate = VIDEO_BITRATE_MAX_AUTO_VALUE_H264;
        }
    } else {
        if (call->video_bit_rate < VIDEO_BITRATE_MIN_AUTO_VALUE_VP8) {
            call->video_bit_rate = VIDEO_BITRATE_MIN_AUTO_VALUE_VP8;
        } else if (call->video_bit_rate > VIDEO_BITRATE_MAX_AUTO_VALUE_VP8) {
            call->video_bit_rate = VIDEO_BITRATE_MAX_AUTO_VALUE_VP8;
        }

        call->video_bit_rate = (uint32_t)((float)call->video_bit_rate * VIDEO_BITRATE_CORRECTION_FACTOR_VP8);

        if (call->video_bit_rate < VIDEO_BITRATE_MIN_AUTO_VALUE_VP8) {
            call->video_bit_rate = VIDEO_BITRATE_MIN_AUTO_VALUE_VP8;
        }

    }

    if (call->video_bit_rate > (uint32_t)call->video.second->video_max_bitrate) {
        call->video_bit_rate = (uint32_t)call->video.second->video_max_bitrate;
    }

    // HINT: sanity check --------------

    pthread_mutex_unlock(call->av->mutex);
}

int callback_invite(void *toxav_inst, MSICall *call)
{
    ToxAV *toxav = (ToxAV *)toxav_inst;
    pthread_mutex_lock(toxav->mutex);

    ToxAVCall *av_call = call_new(toxav, call->friend_number, NULL);

    if (av_call == NULL) {
        LOGGER_WARNING(toxav->m->log, "Failed to initialize call...");
        pthread_mutex_unlock(toxav->mutex);
        return -1;
    }

    call->av_call = av_call;
    av_call->msi_call = call;

    if (toxav->ccb.first) {
        toxav->ccb.first(toxav, call->friend_number, call->peer_capabilities & msi_CapSAudio,
                         call->peer_capabilities & msi_CapSVideo, toxav->ccb.second);
    } else {
        /* No handler to capture the call request, send failure */
        pthread_mutex_unlock(toxav->mutex);
        return -1;
    }

    // HINT: tell friend that we have H264 decoder capabilities (1) -------
    uint32_t pkg_buf_len = 2;
    uint8_t pkg_buf[pkg_buf_len];
    pkg_buf[0] = PACKET_TOXAV_COMM_CHANNEL;
    pkg_buf[1] = PACKET_TOXAV_COMM_CHANNEL_HAVE_H264_VIDEO;

    int result = send_custom_lossless_packet(toxav->m, call->friend_number, pkg_buf, pkg_buf_len);
    LOGGER_ERROR(toxav->m->log, "PACKET_TOXAV_COMM_CHANNEL_HAVE_H264_VIDEO=%d\n", (int)result);
    // HINT: tell friend that we have H264 decoder capabilities -------


    pthread_mutex_unlock(toxav->mutex);
    return 0;
}

int callback_start(void *toxav_inst, MSICall *call)
{
    ToxAV *toxav = (ToxAV *)toxav_inst;
    pthread_mutex_lock(toxav->mutex);

    ToxAVCall *av_call = call_get(toxav, call->friend_number);

    if (av_call == NULL) {
        /* Should this ever happen? */
        pthread_mutex_unlock(toxav->mutex);
        return -1;
    }

    if (!call_prepare_transmission(av_call)) {
        callback_error(toxav_inst, call);
        pthread_mutex_unlock(toxav->mutex);
        return -1;
    }

    if (!invoke_call_state_callback(toxav, call->friend_number, call->peer_capabilities)) {
        callback_error(toxav_inst, call);
        pthread_mutex_unlock(toxav->mutex);
        return -1;
    }

    // HINT: tell friend that we have H264 decoder capabilities (1) -------
    uint32_t pkg_buf_len = 2;
    uint8_t pkg_buf[pkg_buf_len];
    pkg_buf[0] = PACKET_TOXAV_COMM_CHANNEL;
    pkg_buf[1] = PACKET_TOXAV_COMM_CHANNEL_HAVE_H264_VIDEO;

    int result = send_custom_lossless_packet(toxav->m, call->friend_number, pkg_buf, pkg_buf_len);
    LOGGER_ERROR(toxav->m->log, "PACKET_TOXAV_COMM_CHANNEL_HAVE_H264_VIDEO=%d\n", (int)result);
    // HINT: tell friend that we have H264 decoder capabilities -------


    pthread_mutex_unlock(toxav->mutex);
    return 0;
}

int callback_end(void *toxav_inst, MSICall *call)
{
    ToxAV *toxav = (ToxAV *)toxav_inst;
    pthread_mutex_lock(toxav->mutex);

    invoke_call_state_callback(toxav, call->friend_number, TOXAV_FRIEND_CALL_STATE_FINISHED);

    if (call->av_call) {
        call_kill_transmission((ToxAVCall *)call->av_call);
        call_remove((ToxAVCall *)call->av_call);
    }

    pthread_mutex_unlock(toxav->mutex);
    return 0;
}

int callback_error(void *toxav_inst, MSICall *call)
{
    ToxAV *toxav = (ToxAV *)toxav_inst;
    pthread_mutex_lock(toxav->mutex);

    invoke_call_state_callback(toxav, call->friend_number, TOXAV_FRIEND_CALL_STATE_ERROR);

    if (call->av_call) {
        call_kill_transmission((ToxAVCall *)call->av_call);
        call_remove((ToxAVCall *)call->av_call);
    }

    pthread_mutex_unlock(toxav->mutex);
    return 0;
}

int callback_capabilites(void *toxav_inst, MSICall *call)
{
    ToxAV *toxav = (ToxAV *)toxav_inst;
    pthread_mutex_lock(toxav->mutex);

    if (call->peer_capabilities & msi_CapSAudio) {
        rtp_allow_receiving(((ToxAVCall *)call->av_call)->audio.first);
    } else {
        rtp_stop_receiving(((ToxAVCall *)call->av_call)->audio.first);
    }

    if (call->peer_capabilities & msi_CapSVideo) {
        rtp_allow_receiving(((ToxAVCall *)call->av_call)->video.first);
    } else {
        rtp_stop_receiving(((ToxAVCall *)call->av_call)->video.first);
    }

    invoke_call_state_callback(toxav, call->friend_number, call->peer_capabilities);

    pthread_mutex_unlock(toxav->mutex);
    return 0;
}

bool audio_bit_rate_invalid(uint32_t bit_rate)
{
    /* Opus RFC 6716 section-2.1.1 dictates the following:
     * Opus supports all bit rates from 6 kbit/s to 510 kbit/s.
     */
    return bit_rate < 6 || bit_rate > 510;
}

bool video_bit_rate_invalid(uint32_t bit_rate)
{
    (void) bit_rate;
    /* TODO(mannol): If anyone knows the answer to this one please fill it up */
    return false;
}

bool invoke_call_state_callback(ToxAV *av, uint32_t friend_number, uint32_t state)
{
    if (av->scb.first) {
        av->scb.first(av, friend_number, state, av->scb.second);
    } else {
        return false;
    }

    return true;
}

ToxAVCall *call_new(ToxAV *av, uint32_t friend_number, TOXAV_ERR_CALL *error)
{
    /* Assumes mutex locked */
    TOXAV_ERR_CALL rc = TOXAV_ERR_CALL_OK;
    ToxAVCall *call = NULL;

    if (m_friend_exists(av->m, friend_number) == 0) {
        rc = TOXAV_ERR_CALL_FRIEND_NOT_FOUND;
        goto END;
    }

    if (m_get_friend_connectionstatus(av->m, friend_number) < 1) {
        rc = TOXAV_ERR_CALL_FRIEND_NOT_CONNECTED;
        goto END;
    }

    if (call_get(av, friend_number) != NULL) {
        rc = TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL;
        goto END;
    }


    call = (ToxAVCall *)calloc(sizeof(ToxAVCall), 1);

    call->last_incoming_video_frame_rtimestamp = 0;
    call->last_incoming_video_frame_ltimestamp = 0;

    call->last_incoming_audio_frame_rtimestamp = 0;
    call->last_incoming_audio_frame_ltimestamp = 0;

    call->reference_rtimestamp = 0;
    call->reference_ltimestamp = 0;
    call->reference_diff_timestamp = 0;
    call->reference_diff_timestamp_set = 0;

    if (call == NULL) {
        rc = TOXAV_ERR_CALL_MALLOC;
        goto END;
    }

    call->av = av;
    call->friend_number = friend_number;

    if (av->calls == NULL) { /* Creating */
        av->calls = (ToxAVCall **)calloc(sizeof(ToxAVCall *), friend_number + 1);

        if (av->calls == NULL) {
            free(call);
            call = NULL;
            rc = TOXAV_ERR_CALL_MALLOC;
            goto END;
        }

        av->calls_tail = av->calls_head = friend_number;
    } else if (av->calls_tail < friend_number) { /* Appending */
        ToxAVCall **tmp = (ToxAVCall **)realloc(av->calls, sizeof(ToxAVCall *) * (friend_number + 1));

        if (tmp == NULL) {
            free(call);
            call = NULL;
            rc = TOXAV_ERR_CALL_MALLOC;
            goto END;
        }

        av->calls = tmp;

        /* Set fields in between to null */
        uint32_t i = av->calls_tail + 1;

        for (; i < friend_number; i ++) {
            av->calls[i] = NULL;
        }

        call->prev = av->calls[av->calls_tail];
        av->calls[av->calls_tail]->next = call;

        av->calls_tail = friend_number;
    } else if (av->calls_head > friend_number) { /* Inserting at front */
        call->next = av->calls[av->calls_head];
        av->calls[av->calls_head]->prev = call;
        av->calls_head = friend_number;
    }

    av->calls[friend_number] = call;

END:

    if (error) {
        *error = rc;
    }

    return call;
}


ToxAVCall *call_get(ToxAV *av, uint32_t friend_number)
{
    /* Assumes mutex locked */
    if (av->calls == NULL || av->calls_tail < friend_number) {
        return NULL;
    }

    return av->calls[friend_number];
}


ToxAVCall *call_remove(ToxAVCall *call)
{
    if (call == NULL) {
        return NULL;
    }

    uint32_t friend_number = call->friend_number;
    ToxAV *av = call->av;

    ToxAVCall *prev = call->prev;
    ToxAVCall *next = call->next;

    /* Set av call in msi to NULL in order to know if call if ToxAVCall is
     * removed from the msi call.
     */
    if (call->msi_call) {
        call->msi_call->av_call = NULL;
    }

    free(call);

    if (prev) {
        prev->next = next;
    } else if (next) {
        av->calls_head = next->friend_number;
    } else {
        goto CLEAR;
    }

    if (next) {
        next->prev = prev;
    } else if (prev) {
        av->calls_tail = prev->friend_number;
    } else {
        goto CLEAR;
    }

    av->calls[friend_number] = NULL;
    return next;

CLEAR:
    av->calls_head = av->calls_tail = 0;
    free(av->calls);
    av->calls = NULL;

    return NULL;
}


bool call_prepare_transmission(ToxAVCall *call)
{
    /* Assumes mutex locked */

    if (call == NULL) {
        return false;
    }

    ToxAV *av = call->av;

    if (!av->acb.first && !av->vcb.first) {
        /* It makes no sense to have CSession without callbacks */
        return false;
    }

    if (call->active) {
        LOGGER_WARNING(av->m->log, "Call already active!\n");
        return true;
    }

    if (create_recursive_mutex(call->mutex_audio) != 0) {
        return false;
    }

    if (create_recursive_mutex(call->mutex_video) != 0) {
        goto FAILURE_3;
    }

    if (create_recursive_mutex(call->mutex) != 0) {
        goto FAILURE_2;
    }

    /* Prepare bwc */
    call->bwc = bwc_new(av->m, call->friend_number, callback_bwc, call);

    { /* Prepare audio */
        call->audio.second = ac_new(av->m->log, av, call->friend_number, av->acb.first, av->acb.second);

        if (!call->audio.second) {
            LOGGER_ERROR(av->m->log, "Failed to create audio codec session");
            goto FAILURE;
        }

        call->audio.first = rtp_new(rtp_TypeAudio, av->m, call->friend_number, call->bwc,
                                    call->audio.second, ac_queue_message);

        if (!call->audio.first) {
            LOGGER_ERROR(av->m->log, "Failed to create audio rtp session");;
            goto FAILURE;
        }
    }

    { /* Prepare video */
        call->video.second = vc_new(av->m->log, av, call->friend_number, av->vcb.first, av->vcb.second);

        if (!call->video.second) {
            LOGGER_ERROR(av->m->log, "Failed to create video codec session");
            goto FAILURE;
        }

        call->video.first = rtp_new(rtp_TypeVideo, av->m, call->friend_number, call->bwc,
                                    call->video.second, vc_queue_message);

        if (!call->video.first) {
            LOGGER_ERROR(av->m->log, "Failed to create video rtp session");
            goto FAILURE;
        }
    }

    call->active = 1;
    return true;

FAILURE:
    bwc_kill(call->bwc);
    rtp_kill(call->audio.first);
    ac_kill(call->audio.second);
    call->audio.first = NULL;
    call->audio.second = NULL;
    rtp_kill(call->video.first);
    vc_kill(call->video.second);
    call->video.first = NULL;
    call->video.second = NULL;
    pthread_mutex_destroy(call->mutex);
FAILURE_2:
    pthread_mutex_destroy(call->mutex_video);
FAILURE_3:
    pthread_mutex_destroy(call->mutex_audio);
    return false;
}


void call_kill_transmission(ToxAVCall *call)
{
    if (call == NULL || call->active == 0) {
        return;
    }

    call->active = 0;

    pthread_mutex_lock(call->mutex_audio);
    pthread_mutex_unlock(call->mutex_audio);
    pthread_mutex_lock(call->mutex_video);
    pthread_mutex_unlock(call->mutex_video);
    pthread_mutex_lock(call->mutex);
    pthread_mutex_unlock(call->mutex);

    bwc_kill(call->bwc);

    rtp_kill(call->audio.first);
    ac_kill(call->audio.second);
    call->audio.first = NULL;
    call->audio.second = NULL;

    rtp_kill(call->video.first);
    vc_kill(call->video.second);
    call->video.first = NULL;
    call->video.second = NULL;

    pthread_mutex_destroy(call->mutex_audio);
    pthread_mutex_destroy(call->mutex_video);
    pthread_mutex_destroy(call->mutex);
}


