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

#include "video.h"

#include "msi.h"
#include "ring_buffer.h"
#include "ts_buffer.h"
#include "rtp.h"

#include "../toxcore/logger.h"
#include "../toxcore/network.h"
#include "../toxcore/Messenger.h"
#include "../toxcore/mono_time.h"


#include "tox_generic.h"

#include "codecs/toxav_codecs.h"

#include <assert.h>
#include <stdlib.h>

/* activate only for debugging!! */
// #define DEBUG_SHOW_H264_DECODING_TIME 1
/* activate only for debugging!! */

VCSession *vc_new(Logger *log, ToxAV *av, uint32_t friend_number, toxav_video_receive_frame_cb *cb, void *cb_data)
{
    VCSession *vc = (VCSession *)calloc(sizeof(VCSession), 1);

    if (!vc) {
        LOGGER_WARNING(log, "Allocation failed! Application might misbehave!");
        return NULL;
    }

    if (create_recursive_mutex(vc->queue_mutex) != 0) {
        LOGGER_WARNING(log, "Failed to create recursive mutex!");
        free(vc);
        return NULL;
    }

    LOGGER_WARNING(log, "vc_new ...");

    // options ---
    vc->video_encoder_cpu_used = VP8E_SET_CPUUSED_VALUE;
    vc->video_encoder_cpu_used_prev = vc->video_encoder_cpu_used;
    vc->video_encoder_vp8_quality = TOXAV_ENCODER_VP8_QUALITY_NORMAL;
    vc->video_encoder_vp8_quality_prev = vc->video_encoder_vp8_quality;
    vc->video_rc_max_quantizer = TOXAV_ENCODER_VP8_RC_MAX_QUANTIZER_NORMAL;
    vc->video_rc_max_quantizer_prev = vc->video_rc_max_quantizer;
    vc->video_rc_min_quantizer = TOXAV_ENCODER_VP8_RC_MIN_QUANTIZER_NORMAL;
    vc->video_rc_min_quantizer_prev = vc->video_rc_min_quantizer;
    vc->video_encoder_coded_used = TOXAV_ENCODER_CODEC_USED_VP8; // DEFAULT: VP8 !!
    vc->video_encoder_coded_used_prev = vc->video_encoder_coded_used;
#ifdef RASPBERRY_PI_OMX
    vc->video_encoder_coded_used_hw_accel = TOXAV_ENCODER_CODEC_HW_ACCEL_OMX_PI;
#else
    vc->video_encoder_coded_used_hw_accel = TOXAV_ENCODER_CODEC_HW_ACCEL_NONE;
#endif
    vc->video_keyframe_method = TOXAV_ENCODER_KF_METHOD_NORMAL;
    vc->video_keyframe_method_prev = vc->video_keyframe_method;
    vc->video_decoder_error_concealment = VIDEO__VP8_DECODER_ERROR_CONCEALMENT;
    vc->video_decoder_error_concealment_prev = vc->video_decoder_error_concealment;
    vc->video_decoder_codec_used = TOXAV_ENCODER_CODEC_USED_VP8; // DEFAULT: VP8 !!
    vc->send_keyframe_request_received = 0;
    vc->h264_video_capabilities_received = 0; // WARNING: always set to zero (0) !!
    vc->show_own_video = 0; // WARNING: always set to zero (0) !!
    vc->skip_fps = 0;
    vc->skip_fps_counter = 0;
    vc->skip_fps_release_counter = 0;
    vc->video_bitrate_autoset = 1;

    vc->dummy_ntp_local_start = 0;
    vc->dummy_ntp_local_end = 0;
    vc->dummy_ntp_remote_start = 0;
    vc->dummy_ntp_remote_end = 0;
    vc->rountrip_time_ms = 0;
    vc->video_play_delay = 0;
    vc->video_play_delay_real = 0;
    vc->video_frame_buffer_entries = 0;
    vc->last_sent_keyframe_ts = 0;

    vc->last_incoming_frame_ts = 0;
    vc->timestamp_difference_to_sender = 0;
    vc->timestamp_difference_adjustment = -450;
    vc->tsb_range_ms = 60;
    vc->startup_video_timespan = 8000;
    vc->incoming_video_bitrate_last_changed = 0;
    vc->network_round_trip_time_last_cb_ts = 0;
    vc->incoming_video_bitrate_last_cb_ts = 0;
    vc->last_requested_lower_fps_ts = 0;
    vc->encoder_frame_has_record_timestamp = 1;
    vc->video_max_bitrate = VIDEO_BITRATE_MAX_AUTO_VALUE_H264; // HINT: should probably be set to a higher value
    // options ---

    vc->incoming_video_frames_gap_ms_index = 0;
    vc->incoming_video_frames_gap_last_ts = 0;
    vc->incoming_video_frames_gap_ms_mean_value = 0;

    for (int i = 0; i < VIDEO_INCOMING_FRAMES_GAP_MS_ENTRIES; i++) {
        vc->incoming_video_frames_gap_ms[i] = 0;
    }

#ifdef USE_TS_BUFFER_FOR_VIDEO

    if (!(vc->vbuf_raw = tsb_new(VIDEO_RINGBUFFER_BUFFER_ELEMENTS))) {
        LOGGER_WARNING(log, "vc_new:rb_new FAILED");
        vc->vbuf_raw = NULL;
        goto BASE_CLEANUP;
    }

#else

    if (!(vc->vbuf_raw = rb_new(VIDEO_RINGBUFFER_BUFFER_ELEMENTS))) {
        LOGGER_WARNING(log, "vc_new:rb_new FAILED");
        vc->vbuf_raw = NULL;
        goto BASE_CLEANUP;
    }

#endif

    LOGGER_WARNING(log, "vc_new:rb_new OK");

    // HINT: tell client what encoder and decoder are in use now -----------
    if (av->call_comm_cb.first) {

        TOXAV_CALL_COMM_INFO cmi;
        cmi = TOXAV_CALL_COMM_DECODER_IN_USE_VP8;

        if (vc->video_decoder_codec_used == TOXAV_ENCODER_CODEC_USED_H264) {
            // don't the the friend if we have HW accel, since it would reveal HW and platform info
            cmi = TOXAV_CALL_COMM_DECODER_IN_USE_H264;
        }

        av->call_comm_cb.first(av, friend_number, cmi, 0, av->call_comm_cb.second);


        cmi = TOXAV_CALL_COMM_ENCODER_IN_USE_VP8;

        if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_H264) {
            if (vc->video_encoder_coded_used_hw_accel == TOXAV_ENCODER_CODEC_HW_ACCEL_OMX_PI) {
                cmi = TOXAV_CALL_COMM_ENCODER_IN_USE_H264_OMX_PI;
            } else {
                cmi = TOXAV_CALL_COMM_ENCODER_IN_USE_H264;
            }
        }

        av->call_comm_cb.first(av, friend_number, cmi, 0, av->call_comm_cb.second);
    }

    // HINT: tell client what encoder and decoder are in use now -----------

    // HINT: initialize the H264 encoder


#ifdef RASPBERRY_PI_OMX
    LOGGER_WARNING(log, "OMX:002");
    vc = vc_new_h264_omx_raspi(log, av, friend_number, cb, cb_data, vc);
    LOGGER_WARNING(log, "OMX:003");
#else
    vc = vc_new_h264(log, av, friend_number, cb, cb_data, vc);
#endif

    // HINT: initialize VP8 encoder
    return vc_new_vpx(log, av, friend_number, cb, cb_data, vc);

BASE_CLEANUP:
    pthread_mutex_destroy(vc->queue_mutex);

#ifdef USE_TS_BUFFER_FOR_VIDEO
    tsb_kill((TSBuffer *)vc->vbuf_raw);
#else
    rb_kill((RingBuffer *)vc->vbuf_raw);
#endif
    vc->vbuf_raw = NULL;
    free(vc);
    return NULL;
}



void vc_kill(VCSession *vc)
{
    if (!vc) {
        return;
    }

#ifdef RASPBERRY_PI_OMX
    vc_kill_h264_omx_raspi(vc);
#else
    vc_kill_h264(vc);
#endif
    vc_kill_vpx(vc);

    void *p;
    uint64_t dummy;

#ifdef USE_TS_BUFFER_FOR_VIDEO
    tsb_drain((TSBuffer *)vc->vbuf_raw);
    tsb_kill((TSBuffer *)vc->vbuf_raw);
#else

    while (rb_read((RingBuffer *)vc->vbuf_raw, &p, &dummy)) {
        free(p);
    }

    rb_kill((RingBuffer *)vc->vbuf_raw);
#endif

    vc->vbuf_raw = NULL;

    pthread_mutex_destroy(vc->queue_mutex);

    LOGGER_DEBUG(vc->log, "Terminated video handler: %p", vc);
    free(vc);
}


void video_switch_decoder(VCSession *vc, TOXAV_ENCODER_CODEC_USED_VALUE decoder_to_use)
{
    if (vc->video_decoder_codec_used != (int32_t)decoder_to_use) {
        if ((decoder_to_use == TOXAV_ENCODER_CODEC_USED_VP8)
                || (decoder_to_use == TOXAV_ENCODER_CODEC_USED_VP9)
                || (decoder_to_use == TOXAV_ENCODER_CODEC_USED_H264)) {

            vc->video_decoder_codec_used = decoder_to_use;
            LOGGER_ERROR(vc->log, "**switching DECODER to **:%d",
                         (int)vc->video_decoder_codec_used);


            if (vc->av) {
                if (vc->av->call_comm_cb.first) {

                    TOXAV_CALL_COMM_INFO cmi;
                    cmi = TOXAV_CALL_COMM_DECODER_IN_USE_VP8;

                    if (vc->video_decoder_codec_used == TOXAV_ENCODER_CODEC_USED_H264) {
                        cmi = TOXAV_CALL_COMM_DECODER_IN_USE_H264;
                    }

                    vc->av->call_comm_cb.first(vc->av, vc->friend_number,
                                               cmi, 0, vc->av->call_comm_cb.second);

                }
            }


        }
    }
}


/* --- VIDEO DECODING happens here --- */
/* --- VIDEO DECODING happens here --- */
/* --- VIDEO DECODING happens here --- */
uint8_t vc_iterate(VCSession *vc, Messenger *m, uint8_t skip_video_flag, uint64_t *a_r_timestamp,
                   uint64_t *a_l_timestamp,
                   uint64_t *v_r_timestamp, uint64_t *v_l_timestamp, BWController *bwc,
                   int64_t *timestamp_difference_adjustment_,
                   int64_t *timestamp_difference_to_sender_)
{

    if (!vc) {
        return 0;
    }

    uint8_t ret_value = 0;
    struct RTPMessage *p;
    bool have_requested_index_frame = false;

    vpx_codec_err_t rc;

    pthread_mutex_lock(vc->queue_mutex);

    uint64_t frame_flags;
    uint8_t data_type;
    uint8_t h264_encoded_video_frame = 0;

    uint32_t full_data_len;

#ifdef USE_TS_BUFFER_FOR_VIDEO
    uint32_t timestamp_out_ = 0;
    uint32_t timestamp_min = 0;
    uint32_t timestamp_max = 0;

    *timestamp_difference_to_sender_ = vc->timestamp_difference_to_sender;

    tsb_get_range_in_buffer((TSBuffer *)vc->vbuf_raw, &timestamp_min, &timestamp_max);


    int64_t want_remote_video_ts = (current_time_monotonic() + vc->timestamp_difference_to_sender +
                                    vc->timestamp_difference_adjustment);

    uint32_t timestamp_want_get = (uint32_t)want_remote_video_ts;


    // HINT: compensate for older clients ----------------
    if (vc->encoder_frame_has_record_timestamp == 0) {
        LOGGER_DEBUG(vc->log, "old client:002");
        vc->tsb_range_ms = (UINT32_MAX - 1);
        timestamp_want_get = (UINT32_MAX - 1);
        vc->startup_video_timespan = 0;
    }

    // HINT: compensate for older clients ----------------

#if 0

    if ((int)tsb_size((TSBuffer *)vc->vbuf_raw) > 0) {
        LOGGER_ERROR(vc->log, "FC:%d min=%ld max=%ld want=%d diff=%d adj=%d roundtrip=%d",
                     (int)tsb_size((TSBuffer *)vc->vbuf_raw),
                     timestamp_min,
                     timestamp_max,
                     (int)timestamp_want_get,
                     (int)timestamp_want_get - (int)timestamp_max,
                     (int)vc->timestamp_difference_adjustment,
                     (int)vc->rountrip_time_ms);
    }

#endif

    // HINT: correct for very false values ------------
    if ((int)tsb_size((TSBuffer *)vc->vbuf_raw) > (VIDEO_RINGBUFFER_BUFFER_ELEMENTS - 2)) {
        // HINT: buffer with incoming video frames is very full
        if (timestamp_want_get > (timestamp_max + 100)) {
            // we wont get a frame like this
            vc->timestamp_difference_adjustment = vc->timestamp_difference_adjustment -
                                                  (timestamp_want_get - timestamp_max) - 10;

            LOGGER_ERROR(vc->log, "DEFF_CORR:--:%d", (int)((timestamp_want_get - timestamp_max) - 10));

            want_remote_video_ts = (current_time_monotonic() + vc->timestamp_difference_to_sender +
                                    vc->timestamp_difference_adjustment);

            timestamp_want_get = (uint32_t)want_remote_video_ts;
        } else if ((timestamp_want_get + 100) < timestamp_min) {
            vc->timestamp_difference_adjustment = vc->timestamp_difference_adjustment +
                                                  (timestamp_min - timestamp_want_get) + 10;

            LOGGER_ERROR(vc->log, "DEFF_CORR:++++:%d", (int)((timestamp_min - timestamp_want_get) + 10));

            want_remote_video_ts = (current_time_monotonic() + vc->timestamp_difference_to_sender +
                                    vc->timestamp_difference_adjustment);

            timestamp_want_get = (uint32_t)want_remote_video_ts;
        }
    }

    // HINT: correct for very false values ------------

    uint16_t removed_entries;
    uint16_t is_skipping = 0;

    // HINT: give me video frames that happend "now" minus some diff
    if (tsb_read((TSBuffer *)vc->vbuf_raw, vc->log, (void **)&p, &frame_flags,
                 &timestamp_out_,
                 timestamp_want_get,
                 vc->tsb_range_ms + vc->startup_video_timespan,
                 &removed_entries,
                 &is_skipping)) {
#else

    if (rb_read((RingBuffer *)vc->vbuf_raw, (void **)&p, &frame_flags)) {
#endif



#if 1

        if ((is_skipping > 0) && (removed_entries > 0)) {
            if ((vc->last_requested_lower_fps_ts + 10000) < current_time_monotonic()) {


                // HINT: tell sender to turn down video FPS -------------
                uint32_t pkg_buf_len = 3;
                uint8_t pkg_buf[pkg_buf_len];
                pkg_buf[0] = PACKET_TOXAV_COMM_CHANNEL;
                pkg_buf[1] = PACKET_TOXAV_COMM_CHANNEL_LESS_VIDEO_FPS;

                if ((vc->last_requested_lower_fps_ts + 12000) < current_time_monotonic()) {
                    pkg_buf[2] = 2;
                } else {
                    pkg_buf[2] = 3; // skip every 3rd video frame and dont encode and dont sent it
                }

                int result = send_custom_lossless_packet(vc->av->m, vc->friend_number, pkg_buf, pkg_buf_len);
                // HINT: tell sender to turn down video FPS -------------

                vc->last_requested_lower_fps_ts = current_time_monotonic();

                LOGGER_WARNING(vc->log, "request lower FPS from sender: %d ms : skip every %d", (int)is_skipping, (int)pkg_buf[2]);
            }
        }

#endif




#if 1
        LOGGER_DEBUG(vc->log, "rtt:drift:1:%d %d %d", (int)(vc->rountrip_time_ms),
                     (int)(-vc->timestamp_difference_adjustment),
                     (int)AV_ADJUSTMENT_BASE_MS);

        if (vc->rountrip_time_ms > (-vc->timestamp_difference_adjustment - AV_ADJUSTMENT_BASE_MS)) {
            // drift
            LOGGER_DEBUG(vc->log, "rtt:drift:2:%d > %d", (int)(vc->rountrip_time_ms),
                         (int)(-vc->timestamp_difference_adjustment - AV_ADJUSTMENT_BASE_MS));

            LOGGER_DEBUG(vc->log, "rtt:drift:3:%d < %d", (int)(vc->timestamp_difference_adjustment),
                         (int)(-MIN_AV_BUFFERING_MS));

            if (tsb_size((TSBuffer *)vc->vbuf_raw) < 10) {
                vc->timestamp_difference_adjustment = vc->timestamp_difference_adjustment - 1;
                LOGGER_DEBUG(vc->log, "rtt:drift:4:---1:%d", (int)(vc->timestamp_difference_adjustment));
            } else {
                vc->timestamp_difference_adjustment = vc->timestamp_difference_adjustment + 1;
            }
        } else if (vc->rountrip_time_ms < (-vc->timestamp_difference_adjustment - AV_ADJUSTMENT_BASE_MS)) {
            // drift
            LOGGER_DEBUG(vc->log, "rtt:drift:5:%d << %d", (int)(vc->rountrip_time_ms),
                         (int)(-vc->timestamp_difference_adjustment - AV_ADJUSTMENT_BASE_MS));

            if (vc->timestamp_difference_adjustment <= -MIN_AV_BUFFERING_MS) {
                LOGGER_DEBUG(vc->log, "rtt:drift:6:%d < %d", (int)(vc->timestamp_difference_adjustment),
                             (int)(-MIN_AV_BUFFERING_MS));

                vc->timestamp_difference_adjustment = vc->timestamp_difference_adjustment + 1;
                LOGGER_DEBUG(vc->log, "rtt:drift:7:+1:%d", (int)(vc->timestamp_difference_adjustment));
            }
        }

#endif






        LOGGER_DEBUG(vc->log, "XLS01:%d,%d",
                     (int)(timestamp_want_get - current_time_monotonic()),
                     (int)(timestamp_out_ - current_time_monotonic())
                    );

        const struct RTPHeader *header_v3_0 = (void *) & (p->header);

        vc->video_play_delay = ((current_time_monotonic() + vc->timestamp_difference_to_sender) - timestamp_out_);
        vc->video_frame_buffer_entries = (uint32_t)tsb_size((TSBuffer *)vc->vbuf_raw);

        LOGGER_DEBUG(vc->log, "seq:%d FC:%d min=%d max=%d want=%d got=%d diff=%d rm=%d pdelay=%d adj=%d dts=%d rtt=%d",
                     (int)header_v3_0->sequnum,
                     (int)tsb_size((TSBuffer *)vc->vbuf_raw),
                     timestamp_min,
                     timestamp_max,
                     (int)timestamp_want_get,
                     (int)timestamp_out_,
                     ((int)timestamp_want_get - (int)timestamp_out_),
                     (int)removed_entries,
                     (int)vc->video_play_delay,
                     (int)vc->timestamp_difference_adjustment,
                     (int)vc->timestamp_difference_to_sender,
                     (int)vc->rountrip_time_ms);

        uint16_t buf_size = tsb_size((TSBuffer *)vc->vbuf_raw);
        int32_t diff_want_to_got = (int)timestamp_want_get - (int)timestamp_out_;


        LOGGER_DEBUG(vc->log, "values:diff_to_sender=%d adj=%d tsb_range=%d bufsize=%d",
                     (int)vc->timestamp_difference_to_sender, (int)vc->timestamp_difference_adjustment,
                     (int)vc->tsb_range_ms,
                     (int)buf_size);


        if (vc->startup_video_timespan > 0) {
            vc->startup_video_timespan = 0;
        }



        // TODO: make it available to the audio session
        // bad hack -> make better!
        *timestamp_difference_adjustment_ = vc->timestamp_difference_adjustment;


        LOGGER_DEBUG(vc->log, "--VSEQ:%d", (int)header_v3_0->sequnum);

        data_type = (uint8_t)((frame_flags & RTP_KEY_FRAME) != 0);
        h264_encoded_video_frame = (uint8_t)((frame_flags & RTP_ENCODER_IS_H264) != 0);

        bwc_add_recv(bwc, header_v3_0->data_length_full);

        if ((int32_t)header_v3_0->sequnum < (int32_t)vc->last_seen_fragment_seqnum) {
            // drop frame with too old sequence number
            LOGGER_WARNING(vc->log, "skipping incoming video frame (0) with sn=%d lastseen=%d old_frames_count=%d",
                           (int)header_v3_0->sequnum,
                           (int)vc->last_seen_fragment_seqnum,
                           (int)vc->count_old_video_frames_seen);

            vc->count_old_video_frames_seen++;

            if ((int32_t)(header_v3_0->sequnum + 1) != (int32_t)vc->last_seen_fragment_seqnum) {
                // TODO: check why we often get exactly the previous video frame here?!?!
            }

            if (vc->count_old_video_frames_seen > 6) {
                // if we see more than 6 old video frames in a row, then either there was
                // a seqnum rollover or something else. just play those frames then
                vc->last_seen_fragment_seqnum = (int32_t)header_v3_0->sequnum;
                vc->count_old_video_frames_seen = 0;
            }

            free(p);
            pthread_mutex_unlock(vc->queue_mutex);
            return 0;
        }

        if ((int32_t)header_v3_0->sequnum != (int32_t)(vc->last_seen_fragment_seqnum + 1)) {
            int32_t missing_frames_count = (int32_t)header_v3_0->sequnum -
                                           (int32_t)(vc->last_seen_fragment_seqnum + 1);


            const Messenger *mm = (Messenger *)(vc->av->m);
            const Messenger_Options *mo = (Messenger_Options *) & (mm->options);

#define NORMAL_MISSING_FRAME_COUNT_TOLERANCE 0
#define WHEN_SKIPPING_MISSING_FRAME_COUNT_TOLERANCE 2

            int32_t missing_frame_tolerance = NORMAL_MISSING_FRAME_COUNT_TOLERANCE;

            if (is_skipping > 0) {
                // HINT: workaround, if we are skipping frames because client is too slow
                //       we assume the missing frames here are the skipped ones
                missing_frame_tolerance = WHEN_SKIPPING_MISSING_FRAME_COUNT_TOLERANCE;
            }

            if (missing_frames_count > missing_frame_tolerance) {

                // HINT: if whole video frames are missing here, they most likely have been
                //       kicked out of the ringbuffer because the sender is sending at too much FPS
                //       which out client cant handle. so in the future signal sender to send less FPS!

                LOGGER_WARNING(vc->log, "missing? sn=%d lastseen=%d",
                               (int)header_v3_0->sequnum,
                               (int)vc->last_seen_fragment_seqnum);


                LOGGER_DEBUG(vc->log, "missing %d video frames (m1)", (int)missing_frames_count);

                if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
                    rc = vpx_codec_decode(vc->decoder, NULL, 0, NULL, VPX_DL_REALTIME);
                }

                // HINT: give feedback that we lost some bytes (based on the size of this frame)
                bwc_add_lost_v3(bwc, (uint32_t)(header_v3_0->data_length_full * missing_frames_count), true);
                LOGGER_ERROR(vc->log, "BWC:lost:002:missing count=%d", (int)missing_frames_count);
            }
        }


        // TODO: check for seqnum rollover!!
        vc->count_old_video_frames_seen = 0;
        vc->last_seen_fragment_seqnum = header_v3_0->sequnum;

        if (skip_video_flag == 1) {
#if 1

            if ((int)data_type != (int)video_frame_type_KEYFRAME) {
                free(p);
                LOGGER_ERROR(vc->log, "skipping incoming video frame (1)");

                if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
                    rc = vpx_codec_decode(vc->decoder, NULL, 0, NULL, VPX_DL_REALTIME);
                }

                // HINT: give feedback that we lost some bytes (based on the size of this frame)
                bwc_add_lost_v3(bwc, header_v3_0->data_length_full, false);
                LOGGER_ERROR(vc->log, "BWC:lost:003");

                pthread_mutex_unlock(vc->queue_mutex);
                return 0;
            }

#endif
        } else {
            // NOOP
        }

        pthread_mutex_unlock(vc->queue_mutex);

        const struct RTPHeader *header_v3 = (void *) & (p->header);

        if (header_v3->flags & RTP_LARGE_FRAME) {
            full_data_len = header_v3->data_length_full;
            LOGGER_DEBUG(vc->log, "vc_iterate:001:full_data_len=%d", (int)full_data_len);
        } else {
            full_data_len = p->len;
            LOGGER_DEBUG(vc->log, "vc_iterate:002");
        }

        // LOGGER_DEBUG(vc->log, "vc_iterate: rb_read p->len=%d data_type=%d", (int)full_data_len, (int)data_type);
        // LOGGER_DEBUG(vc->log, "vc_iterate: rb_read rb size=%d", (int)rb_size((RingBuffer *)vc->vbuf_raw));



        // HINT: give feedback that we lost some bytes
        if (header_v3->received_length_full < full_data_len) {
            // const Messenger *mm = (Messenger *)(vc->av->m);
            // const Messenger_Options *mo = (Messenger_Options *) & (mm->options);

            bwc_add_lost_v3(bwc, (full_data_len - header_v3->received_length_full), false);
            LOGGER_ERROR(vc->log, "BWC:lost:004:lost bytes=%d", (int)(full_data_len - header_v3->received_length_full));
        }


        if ((int)data_type == (int)video_frame_type_KEYFRAME) {

            int percent_recvd = 100;

            if (full_data_len > 0) {
                percent_recvd = (int)(((float)header_v3->received_length_full / (float)full_data_len) * 100.0f);
            }

#if 0

            if (percent_recvd < 100) {
                LOGGER_DEBUG(vc->log, "RTP_RECV:sn=%ld fn=%ld pct=%d%% *I* len=%ld recv_len=%ld",
                             (long)header_v3->sequnum,
                             (long)header_v3->fragment_num,
                             percent_recvd,
                             (long)full_data_len,
                             (long)header_v3->received_length_full);
            } else {
                LOGGER_DEBUG(vc->log, "RTP_RECV:sn=%ld fn=%ld pct=%d%% *I* len=%ld recv_len=%ld",
                             (long)header_v3->sequnum,
                             (long)header_v3->fragment_num,
                             percent_recvd,
                             (long)full_data_len,
                             (long)header_v3->received_length_full);
            }

#endif

#if 1

            if ((percent_recvd < 100) && (have_requested_index_frame == false)) {
                if ((vc->last_requested_keyframe_ts + VIDEO_MIN_REQUEST_KEYFRAME_INTERVAL_MS_FOR_KF)
                        < current_time_monotonic()) {
                    // if keyframe received has less than 100% of the data, request a new keyframe
                    // from the sender
                    uint32_t pkg_buf_len = 2;
                    uint8_t pkg_buf[pkg_buf_len];
                    pkg_buf[0] = PACKET_TOXAV_COMM_CHANNEL;
                    pkg_buf[1] = PACKET_TOXAV_COMM_CHANNEL_REQUEST_KEYFRAME;

                    if (-1 == send_custom_lossless_packet(m, vc->friend_number, pkg_buf, pkg_buf_len)) {
                        LOGGER_WARNING(vc->log,
                                       "PACKET_TOXAV_COMM_CHANNEL_REQUEST_KEYFRAME:RTP send failed");
                    } else {
                        LOGGER_WARNING(vc->log,
                                       "PACKET_TOXAV_COMM_CHANNEL_REQUEST_KEYFRAME:RTP Sent.");
                        vc->last_requested_keyframe_ts = current_time_monotonic();
                    }
                }
            }

#endif

        } else {
            LOGGER_DEBUG(vc->log, "RTP_RECV:sn=%ld fn=%ld pct=%d%% len=%ld recv_len=%ld",
                         (long)header_v3->sequnum,
                         (long)header_v3->fragment_num,
                         (int)(((float)header_v3->received_length_full / (float)full_data_len) * 100.0f),
                         (long)full_data_len,
                         (long)header_v3->received_length_full);
        }


        // LOGGER_ERROR(vc->log, "h264_encoded_video_frame=%d vc->video_decoder_codec_used=%d",
        //             (int)h264_encoded_video_frame,
        //             (int)vc->video_decoder_codec_used);

        if (DISABLE_H264_DECODER_FEATURE == 0) {

            if ((vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264)
                    && (h264_encoded_video_frame == 1)) {
                LOGGER_ERROR(vc->log, "h264_encoded_video_frame:AA");
                video_switch_decoder(vc, TOXAV_ENCODER_CODEC_USED_H264);

            } else if ((vc->video_decoder_codec_used == TOXAV_ENCODER_CODEC_USED_H264)
                       && (h264_encoded_video_frame == 0)) {
                LOGGER_ERROR(vc->log, "h264_encoded_video_frame:BB");
                // HINT: once we switched to H264 never switch back to VP8 until this call ends
                // video_switch_decoder(vc, TOXAV_ENCODER_CODEC_USED_VP8);
            }
        }

        // HINT: somtimes the singaling of H264 capability does not work
        //       as workaround send it again on the first 30 frames
        if ((vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264)
                && ((long)header_v3->sequnum < 30)) {

            // HINT: tell friend that we have H264 decoder capabilities (3) -------
            uint32_t pkg_buf_len = 2;
            uint8_t pkg_buf[pkg_buf_len];
            pkg_buf[0] = PACKET_TOXAV_COMM_CHANNEL;
            pkg_buf[1] = PACKET_TOXAV_COMM_CHANNEL_HAVE_H264_VIDEO;

            int result = send_custom_lossless_packet(m, vc->friend_number, pkg_buf, pkg_buf_len);
            LOGGER_ERROR(vc->log, "PACKET_TOXAV_COMM_CHANNEL_HAVE_H264_VIDEO=%d\n", (int)result);
            // HINT: tell friend that we have H264 decoder capabilities -------

        }

        if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
            // LOGGER_ERROR(vc->log, "DEC:VP8------------");
            decode_frame_vpx(vc, m, skip_video_flag, a_r_timestamp,
                             a_l_timestamp,
                             v_r_timestamp, v_l_timestamp,
                             header_v3, p,
                             rc, full_data_len,
                             &ret_value);
        } else {
            // LOGGER_ERROR(vc->log, "DEC:H264------------");
#ifdef RASPBERRY_PI_OMX
            decode_frame_h264_omx_raspi(vc, m, skip_video_flag, a_r_timestamp,
                                        a_l_timestamp,
                                        v_r_timestamp, v_l_timestamp,
                                        header_v3, p,
                                        rc, full_data_len,
                                        &ret_value);
#else

#ifdef DEBUG_SHOW_H264_DECODING_TIME
            uint32_t start_time_ms = current_time_monotonic();
#endif
            decode_frame_h264(vc, m, skip_video_flag, a_r_timestamp,
                              a_l_timestamp,
                              v_r_timestamp, v_l_timestamp,
                              header_v3, p,
                              rc, full_data_len,
                              &ret_value);

#ifdef DEBUG_SHOW_H264_DECODING_TIME
            uint32_t end_time_ms = current_time_monotonic();

            if ((int)(end_time_ms - start_time_ms) > 4) {
                LOGGER_WARNING(vc->log, "decode_frame_h264: %d ms", (int)(end_time_ms - start_time_ms));
            }

#endif

#endif
        }

        return ret_value;
    } else {
        // no frame data available
        // LOGGER_WARNING(vc->log, "Error decoding video: rb_read");
        if (removed_entries > 0) {
            LOGGER_WARNING(vc->log, "removed entries=%d", (int)removed_entries);
        }
    }

    pthread_mutex_unlock(vc->queue_mutex);

    return ret_value;
}

/* --- VIDEO DECODING happens here --- */
/* --- VIDEO DECODING happens here --- */
/* --- VIDEO DECODING happens here --- */


int vc_queue_message(void *vcp, struct RTPMessage *msg)
{
    /* This function is called with complete messages
     * they have already been assembled. but not yet decoded
     * (data is still compressed by video codec)
     * this function gets called from handle_rtp_packet()
     */
    if (!vcp || !msg) {
        return -1;
    }

    VCSession *vc = (VCSession *)vcp;

    const struct RTPHeader *header_v3 = (void *) & (msg->header);
    const struct RTPHeader *header = &msg->header;

    if (msg->header.pt == (rtp_TypeVideo + 2) % 128) {
        LOGGER_WARNING(vc->log, "Got dummy!");
        free(msg);
        return 0;
    }

    if (msg->header.pt != rtp_TypeVideo % 128) {
        LOGGER_WARNING(vc->log, "Invalid payload type! pt=%d", (int)msg->header.pt);
        free(msg);
        return -1;
    }

    // calculate mean "frame incoming every x milliseconds" --------------
    if (vc->incoming_video_frames_gap_last_ts > 0) {
        uint32_t curent_gap = current_time_monotonic() - vc->incoming_video_frames_gap_last_ts;

        vc->incoming_video_frames_gap_ms[vc->incoming_video_frames_gap_ms_index] = curent_gap;
        vc->incoming_video_frames_gap_ms_index = (vc->incoming_video_frames_gap_ms_index + 1) %
                VIDEO_INCOMING_FRAMES_GAP_MS_ENTRIES;

        uint32_t mean_value = 0;

        for (int k = 0; k < VIDEO_INCOMING_FRAMES_GAP_MS_ENTRIES; k++) {
            mean_value = mean_value + vc->incoming_video_frames_gap_ms[k];
        }

        if (mean_value == 0) {
            vc->incoming_video_frames_gap_ms_mean_value = 0;
        } else {
            vc->incoming_video_frames_gap_ms_mean_value = (mean_value * 10) / (VIDEO_INCOMING_FRAMES_GAP_MS_ENTRIES * 10);
        }

#if 0
        LOGGER_DEBUG(vc->log, "FPS:INCOMING=%d ms = %.1f fps mean=%d m=%d",
                     (int)curent_gap,
                     (float)(1000.0f / (curent_gap + 0.00001)),
                     (int)vc->incoming_video_frames_gap_ms_mean_value,
                     (int)mean_value);
#endif
    }

    vc->incoming_video_frames_gap_last_ts = current_time_monotonic();
    // calculate mean "frame incoming every x milliseconds" --------------

    pthread_mutex_lock(vc->queue_mutex);

    LOGGER_DEBUG(vc->log, "TT:queue:V:fragnum=%ld", (long)header_v3->fragment_num);

    // older clients do not send the frame record timestamp
    // compensate by using the frame sennt timestamp
    if (msg->header.frame_record_timestamp == 0) {
        LOGGER_ERROR(vc->log, "old client:001");
        msg->header.frame_record_timestamp = msg->header.timestamp;
    }


    if ((header->flags & RTP_LARGE_FRAME) && header->pt == rtp_TypeVideo % 128) {


        vc->last_incoming_frame_ts = header_v3->frame_record_timestamp;


        // give COMM data to client -------

        if ((vc->network_round_trip_time_last_cb_ts + 2000) < current_time_monotonic()) {
            if (vc->av) {
                if (vc->av->call_comm_cb.first) {
                    vc->av->call_comm_cb.first(vc->av, vc->friend_number,
                                               TOXAV_CALL_COMM_NETWORK_ROUND_TRIP_MS,
                                               (int64_t)vc->rountrip_time_ms,
                                               vc->av->call_comm_cb.second);

                    vc->av->call_comm_cb.first(vc->av, vc->friend_number,
                                               TOXAV_CALL_COMM_PLAY_DELAY,
                                               (int64_t)vc->video_play_delay_real,
                                               vc->av->call_comm_cb.second);

                    vc->av->call_comm_cb.first(vc->av, vc->friend_number,
                                               TOXAV_CALL_COMM_PLAY_BUFFER_ENTRIES,
                                               (int64_t)vc->video_frame_buffer_entries,
                                               vc->av->call_comm_cb.second);

                    if (vc->incoming_video_frames_gap_ms_mean_value == 0) {
                        vc->av->call_comm_cb.first(vc->av, vc->friend_number,
                                                   TOXAV_CALL_COMM_INCOMING_FPS,
                                                   (int64_t)(9999),
                                                   vc->av->call_comm_cb.second);
                    } else {
                        vc->av->call_comm_cb.first(vc->av, vc->friend_number,
                                                   TOXAV_CALL_COMM_INCOMING_FPS,
                                                   (int64_t)(1000 / vc->incoming_video_frames_gap_ms_mean_value),
                                                   vc->av->call_comm_cb.second);
                    }
                }

            }

            vc->network_round_trip_time_last_cb_ts = current_time_monotonic();
        }

        // give COMM data to client -------


        if (vc->show_own_video == 0) {


            if ((vc->incoming_video_bitrate_last_cb_ts + 2000) < current_time_monotonic()) {
                if (vc->incoming_video_bitrate_last_changed != header->encoder_bit_rate_used) {
                    if (vc->av) {
                        if (vc->av->call_comm_cb.first) {
                            vc->av->call_comm_cb.first(vc->av, vc->friend_number,
                                                       TOXAV_CALL_COMM_DECODER_CURRENT_BITRATE,
                                                       (int64_t)header->encoder_bit_rate_used,
                                                       vc->av->call_comm_cb.second);
                        }
                    }

                    vc->incoming_video_bitrate_last_changed = header->encoder_bit_rate_used;
                }

                vc->incoming_video_bitrate_last_cb_ts = current_time_monotonic();
            }


#ifdef USE_TS_BUFFER_FOR_VIDEO
            struct RTPMessage *msg_old = tsb_write((TSBuffer *)vc->vbuf_raw, msg,
                                                   (uint64_t)header->flags,
                                                   (uint32_t)header->frame_record_timestamp);
#else
            struct RTPMessage *msg_old = rb_write((RingBuffer *)vc->vbuf_raw, msg, (uint64_t)header->flags);
#endif

            if (msg_old) {

                // HINT: tell sender to turn down video FPS -------------
#if 0
                uint32_t pkg_buf_len = 3;
                uint8_t pkg_buf[pkg_buf_len];
                pkg_buf[0] = PACKET_TOXAV_COMM_CHANNEL;
                pkg_buf[1] = PACKET_TOXAV_COMM_CHANNEL_LESS_VIDEO_FPS;
                pkg_buf[2] = 3; // skip every 3rd video frame and dont encode and dont sent it

                int result = send_custom_lossless_packet(vc->av->m, vc->friend_number, pkg_buf, pkg_buf_len);
                // HINT: tell sender to turn down video FPS -------------
#endif
                LOGGER_DEBUG(vc->log, "FPATH:%d kicked out", (int)msg_old->header.sequnum);

                free(msg_old);
            }
        } else {
            // discard incoming frame, we want to see our outgoing frames instead
            if (msg) {
                free(msg);
            }
        }
    } else {
#ifdef USE_TS_BUFFER_FOR_VIDEO
        free(tsb_write((TSBuffer *)vc->vbuf_raw, msg, 0, current_time_monotonic()));
#else
        free(rb_write((RingBuffer *)vc->vbuf_raw, msg, 0));
#endif
    }


    /* Calculate time since we received the last video frame */
    // use 5ms less than the actual time, to give some free room
    uint32_t t_lcfd = (current_time_monotonic() - vc->linfts) - 5;
    vc->lcfd = t_lcfd > 100 ? vc->lcfd : t_lcfd;

#ifdef VIDEO_DECODER_SOFT_DEADLINE_AUTOTUNE

    // Autotune decoder softdeadline here ----------
    if (vc->last_decoded_frame_ts > 0) {
        long decode_time_auto_tune = (current_time_monotonic() - vc->last_decoded_frame_ts) * 1000;

        if (decode_time_auto_tune == 0) {
            decode_time_auto_tune = 1; // 0 means infinite long softdeadline!
        }

        vc->decoder_soft_deadline[vc->decoder_soft_deadline_index] = decode_time_auto_tune;
        vc->decoder_soft_deadline_index = (vc->decoder_soft_deadline_index + 1) % VIDEO_DECODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES;

#if 0
        LOGGER_DEBUG(vc->log, "AUTOTUNE:INCOMING=%ld us = %.1f fps", (long)decode_time_auto_tune,
                     (float)(1000000.0f / decode_time_auto_tune));
#endif

    }

    vc->last_decoded_frame_ts = current_time_monotonic();
    // Autotune decoder softdeadline here ----------
#endif

    vc->linfts = current_time_monotonic();

    pthread_mutex_unlock(vc->queue_mutex);

    return 0;
}



int vc_reconfigure_encoder(Logger *log, VCSession *vc, uint32_t bit_rate, uint16_t width, uint16_t height,
                           int16_t kf_max_dist)
{
    if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP8) {
        return vc_reconfigure_encoder_vpx(log, vc, bit_rate, width, height, kf_max_dist);
    } else {
#ifdef RASPBERRY_PI_OMX
        return vc_reconfigure_encoder_h264_omx_raspi(log, vc, bit_rate, width, height, kf_max_dist);
#else
        return vc_reconfigure_encoder_h264(log, vc, bit_rate, width, height, kf_max_dist);
#endif
    }
}

