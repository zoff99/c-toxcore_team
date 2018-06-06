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
#include "rtp.h"

#include "../toxcore/logger.h"
#include "../toxcore/network.h"

#include <assert.h>
#include <stdlib.h>

/*
VPX_DL_REALTIME       (1)
deadline parameter analogous to VPx REALTIME mode.

VPX_DL_GOOD_QUALITY   (1000000)
deadline parameter analogous to VPx GOOD QUALITY mode.

VPX_DL_BEST_QUALITY   (0)
deadline parameter analogous to VPx BEST QUALITY mode.
*/


// initialize encoder with this value. Target bandwidth to use for this stream, in kilobits per second.
#define VIDEO_BITRATE_INITIAL_VALUE 400
#define VIDEO_BITRATE_INITIAL_VALUE_VP9 400

uint32_t MaxIntraTarget(uint32_t optimalBuffersize)
{
    // Set max to the optimal buffer level (normalized by target BR),
    // and scaled by a scalePar.
    // Max target size = scalePar * optimalBufferSize * targetBR[Kbps].
    // This values is presented in percentage of perFrameBw:
    // perFrameBw = targetBR[Kbps] * 1000 / frameRate.
    // The target in % is as follows:
    float scalePar = 0.5;
    float codec_maxFramerate = 30;
    uint32_t targetPct = optimalBuffersize * scalePar * codec_maxFramerate / 10;

    // Don't go below 3 times the per frame bandwidth.
    const uint32_t minIntraTh = 300;
    return (targetPct < minIntraTh) ? minIntraTh : targetPct;
}


void vc__init_encoder_cfg(Logger *log, vpx_codec_enc_cfg_t *cfg, int16_t kf_max_dist, int32_t quality,
                          int32_t rc_max_quantizer, int32_t rc_min_quantizer, int32_t encoder_codec,
                          int32_t video_keyframe_method)
{
    vpx_codec_err_t rc;

    if (encoder_codec != TOXAV_ENCODER_CODEC_USED_VP9) {
        LOGGER_WARNING(log, "Using VP8 codec for encoder (1)");
        rc = vpx_codec_enc_config_default(VIDEO_CODEC_ENCODER_INTERFACE_VP8, cfg, 0);
    } else {
        LOGGER_WARNING(log, "Using VP9 codec for encoder (1)");
        rc = vpx_codec_enc_config_default(VIDEO_CODEC_ENCODER_INTERFACE_VP9, cfg, 0);
    }

    if (rc != VPX_CODEC_OK) {
        LOGGER_ERROR(log, "vc__init_encoder_cfg:Failed to get config: %s", vpx_codec_err_to_string(rc));
    }

    if (encoder_codec == TOXAV_ENCODER_CODEC_USED_VP9) {
        cfg->rc_target_bitrate = VIDEO_BITRATE_INITIAL_VALUE_VP9;
    } else {
        cfg->rc_target_bitrate =
            VIDEO_BITRATE_INITIAL_VALUE; /* Target bandwidth to use for this stream, in kilobits per second */
    }

    cfg->g_w = VIDEO_CODEC_DECODER_MAX_WIDTH;
    cfg->g_h = VIDEO_CODEC_DECODER_MAX_HEIGHT;
    cfg->g_pass = VPX_RC_ONE_PASS;



    /* zoff (in 2017) */
    cfg->g_error_resilient = VPX_ERROR_RESILIENT_DEFAULT; // | VPX_ERROR_RESILIENT_PARTITIONS;
    cfg->g_lag_in_frames = 0;
    /* Allow lagged encoding
     *
     * If set, this value allows the encoder to consume a number of input
     * frames before producing output frames. This allows the encoder to
     * base decisions for the current frame on future frames. This does
     * increase the latency of the encoding pipeline, so it is not appropriate
     * in all situations (ex: realtime encoding).
     *
     * Note that this is a maximum value -- the encoder may produce frames
     * sooner than the given limit. Set this value to 0 to disable this
     * feature.
     */

    if (video_keyframe_method == TOXAV_ENCODER_KF_METHOD_PATTERN) {
        cfg->kf_min_dist = 0;
        cfg->kf_mode = VPX_KF_DISABLED;
    } else {
        cfg->kf_min_dist = 0;
        cfg->kf_mode = VPX_KF_AUTO; // Encoder determines optimal placement automatically
    }

    cfg->rc_end_usage = VPX_VBR; // VPX_VBR; // what quality mode?
    /*
     VPX_VBR    Variable Bit Rate (VBR) mode
     VPX_CBR    Constant Bit Rate (CBR) mode
     VPX_CQ     Constrained Quality (CQ) mode -> give codec a hint that we may be on low bandwidth connection
     VPX_Q      Constant Quality (Q) mode
     */

    if (kf_max_dist > 1) {
        cfg->kf_max_dist = kf_max_dist; // a full frame every x frames minimum (can be more often, codec decides automatically)
        LOGGER_WARNING(log, "kf_max_dist=%d (1)", cfg->kf_max_dist);
    } else {
        cfg->kf_max_dist = VPX_MAX_DIST_START;
        LOGGER_WARNING(log, "kf_max_dist=%d (2)", cfg->kf_max_dist);
    }

    if (encoder_codec == TOXAV_ENCODER_CODEC_USED_VP9) {
        cfg->kf_max_dist = VIDEO__VP9_KF_MAX_DIST;
        LOGGER_WARNING(log, "kf_max_dist=%d (3)", cfg->kf_max_dist);
    }

    cfg->g_threads = VPX_MAX_ENCODER_THREADS; // Maximum number of threads to use

    cfg->g_timebase.num = 1; // 0.1 ms = timebase units = (1/10000)s
    cfg->g_timebase.den = 10000; // 0.1 ms = timebase units = (1/10000)s

    if (encoder_codec == TOXAV_ENCODER_CODEC_USED_VP9) {
        cfg->rc_dropframe_thresh = 0;
        cfg->rc_resize_allowed = 0;
    } else {
        if (quality == TOXAV_ENCODER_VP8_QUALITY_HIGH) {
            /* Highest-resolution encoder settings */
            cfg->rc_dropframe_thresh = 0; // 0
            cfg->rc_resize_allowed = 0; // 0
            cfg->rc_min_quantizer = rc_min_quantizer; // 0
            cfg->rc_max_quantizer = rc_max_quantizer; // 40
            cfg->rc_resize_up_thresh = 29;
            cfg->rc_resize_down_thresh = 5;
            cfg->rc_undershoot_pct = 100; // 100
            cfg->rc_overshoot_pct = 15; // 15
            cfg->rc_buf_initial_sz = 500; // 500 in ms
            cfg->rc_buf_optimal_sz = 600; // 600 in ms
            cfg->rc_buf_sz = 1000; // 1000 in ms
        } else { // TOXAV_ENCODER_VP8_QUALITY_NORMAL
            cfg->rc_dropframe_thresh = 0;
            cfg->rc_resize_allowed = 0; // allow encoder to resize to smaller resolution
            cfg->rc_min_quantizer = rc_min_quantizer; // 2
            cfg->rc_max_quantizer = rc_max_quantizer; // 63
            cfg->rc_resize_up_thresh = TOXAV_ENCODER_VP_RC_RESIZE_UP_THRESH;
            cfg->rc_resize_down_thresh = TOXAV_ENCODER_VP_RC_RESIZE_DOWN_THRESH;
            cfg->rc_undershoot_pct = 100; // 100
            cfg->rc_overshoot_pct = 15; // 15
            cfg->rc_buf_initial_sz = 500; // 500 in ms
            cfg->rc_buf_optimal_sz = 600; // 600 in ms
            cfg->rc_buf_sz = 1000; // 1000 in ms
        }

    }
}


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

    // options ---
    vc->video_encoder_cpu_used = VP8E_SET_CPUUSED_VALUE;
    vc->video_encoder_cpu_used_prev = vc->video_encoder_cpu_used;
    vc->video_encoder_vp8_quality = TOXAV_ENCODER_VP8_QUALITY_NORMAL;
    vc->video_encoder_vp8_quality_prev = vc->video_encoder_vp8_quality;
    vc->video_rc_max_quantizer = TOXAV_ENCODER_VP8_RC_MAX_QUANTIZER_NORMAL;
    vc->video_rc_max_quantizer_prev = vc->video_rc_max_quantizer;
    vc->video_rc_min_quantizer = TOXAV_ENCODER_VP8_RC_MIN_QUANTIZER_NORMAL;
    vc->video_rc_min_quantizer_prev = vc->video_rc_min_quantizer;
    //vc->video_encoder_coded_used = TOXAV_ENCODER_CODEC_USED_VP8; // DEBUG: H264 !!
    vc->video_encoder_coded_used = TOXAV_ENCODER_CODEC_USED_H264; // DEBUG: H264 !!
    vc->video_encoder_coded_used_prev = vc->video_encoder_coded_used;
    vc->video_keyframe_method = TOXAV_ENCODER_KF_METHOD_NORMAL;
    vc->video_keyframe_method_prev = vc->video_keyframe_method;
    vc->video_decoder_error_concealment = VIDEO__VP8_DECODER_ERROR_CONCEALMENT;
    vc->video_decoder_error_concealment_prev = vc->video_decoder_error_concealment;
    //vc->video_decoder_codec_used = TOXAV_ENCODER_CODEC_USED_VP8; // DEBUG: H264 !!
    vc->video_encoder_coded_used = TOXAV_ENCODER_CODEC_USED_H264; // DEBUG: H264 !!
    // options ---


    if (!(vc->vbuf_raw = rb_new(VIDEO_RINGBUFFER_BUFFER_ELEMENTS))) {
        goto BASE_CLEANUP;
    }

    // HINT: initialize the H264 encoder
    vc = vc_new_h264(log, av, friend_number, cb, cb_data, vc);

#ifdef RASPBERRY_PI
    vc = vc_new_h264_omx(log, av, friend_number, cb, cb_data, vc);
#endif

    // HINT: initialize VP8 encoder
    return vc_new_vpx(log, av, friend_number, cb, cb_data, vc);

BASE_CLEANUP:
    pthread_mutex_destroy(vc->queue_mutex);
    rb_kill((RingBuffer *)vc->vbuf_raw);
    free(vc);
    return NULL;
}


void vc_kill(VCSession *vc)
{
    if (!vc) {
        return;
    }

    vc_kill_h264(vc);
    vc_kill_vpx(vc);

#ifdef RASPBERRY_PI
    vc_kill_h264_omx(vc);
#endif 

    void *p;
    uint64_t dummy;

    while (rb_read((RingBuffer *)vc->vbuf_raw, &p, &dummy)) {
        free(p);
    }

    rb_kill((RingBuffer *)vc->vbuf_raw);

    pthread_mutex_destroy(vc->queue_mutex);

    LOGGER_DEBUG(vc->log, "Terminated video handler: %p", vc);
    free(vc);
}

void video_switch_decoder(VCSession *vc, TOXAV_ENCODER_CODEC_USED_VALUE decoder_to_use)
{
    if (vc->video_decoder_codec_used != decoder_to_use) {
        if ((decoder_to_use == TOXAV_ENCODER_CODEC_USED_VP8)
                || (decoder_to_use == TOXAV_ENCODER_CODEC_USED_VP9)
                || (decoder_to_use == TOXAV_ENCODER_CODEC_USED_H264)) {

            // ** DISABLED ** // video_switch_decoder_vpx(vc, decoder_to_use);
            vc->video_decoder_codec_used = decoder_to_use;
            LOGGER_ERROR(vc->log, "**switching DECODER to **:%d",
                         (int)vc->video_decoder_codec_used);

        }
    }
}

uint8_t vc_iterate(VCSession *vc, Messenger *m, uint8_t skip_video_flag, uint64_t *a_r_timestamp,
                   uint64_t *a_l_timestamp,
                   uint64_t *v_r_timestamp, uint64_t *v_l_timestamp)
{

    if (!vc) {
        return 0;
    }

    uint8_t ret_value = 0;
    struct RTPMessage *p;
    bool have_requested_index_frame = false;

    pthread_mutex_lock(vc->queue_mutex);

    uint64_t frame_flags;
    uint8_t data_type;
    uint8_t h264_encoded_video_frame;

    uint32_t full_data_len;


    if (rb_read((RingBuffer *)vc->vbuf_raw, (void **)&p, &frame_flags)) {
        const struct RTPHeader *header_v3_0 = (void *) & (p->header);

        data_type = (uint8_t)((frame_flags & RTP_KEY_FRAME) != 0);
        h264_encoded_video_frame = (uint8_t)((frame_flags & RTP_ENCODER_IS_H264) != 0);


        if ((int32_t)header_v3_0->sequnum < (int32_t)vc->last_seen_fragment_seqnum) {
            // drop frame with too old sequence number
            LOGGER_WARNING(vc->log, "skipping incoming video frame (0) with sn=%d lastseen=%d old_frames_count=%d",
                           (int)header_v3_0->sequnum,
                           (int)vc->last_seen_fragment_seqnum,
                           (int)vc->count_old_video_frames_seen);

            vc->count_old_video_frames_seen++;

            if (vc->count_old_video_frames_seen > 6) {
                // if we see more than 6 old video frames in a row, then either there was
                // a seqnum rollover or something else. just play those frames then
                vc->last_seen_fragment_seqnum = (int32_t)header_v3_0->sequnum;
                vc->count_old_video_frames_seen = 0;
            }

            // if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
            // rc = vpx_codec_decode(vc->decoder, NULL, 0, NULL, VPX_DL_REALTIME);
            // }

            free(p);
            pthread_mutex_unlock(vc->queue_mutex);
            return 0;
        }

        if ((int32_t)header_v3_0->sequnum != (int32_t)(vc->last_seen_fragment_seqnum + 1)) {
            int32_t missing_frames_count = (int32_t)header_v3_0->sequnum -
                                           (int32_t)(vc->last_seen_fragment_seqnum + 1);
            LOGGER_DEBUG(vc->log, "missing %d video frames (m1)", (int)missing_frames_count);

            if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
                // TODO: what was this good for?
                //rc = vpx_codec_decode(vc->decoder, NULL, 0, NULL, VPX_DL_REALTIME);
            }

            if (missing_frames_count > 5) {
                if ((vc->last_requested_keyframe_ts + VIDEO_MIN_REQUEST_KEYFRAME_INTERVAL_MS_FOR_NF)
                        < current_time_monotonic()) {
                    uint32_t pkg_buf_len = 2;
                    uint8_t pkg_buf[pkg_buf_len];
                    pkg_buf[0] = PACKET_REQUEST_KEYFRAME;
                    pkg_buf[1] = 0;

                    if (-1 == send_custom_lossless_packet(m, vc->friend_number, pkg_buf, pkg_buf_len)) {
                        LOGGER_WARNING(vc->log,
                                       "PACKET_REQUEST_KEYFRAME:RTP send failed (2)");
                    } else {
                        LOGGER_WARNING(vc->log,
                                       "PACKET_REQUEST_KEYFRAME:RTP Sent. (2)");
                        have_requested_index_frame = true;
                        vc->last_requested_keyframe_ts = current_time_monotonic();
                    }
                }
            }
        }

        // TODO: check for seqnum rollover!!
        vc->count_old_video_frames_seen = 0;
        vc->last_seen_fragment_seqnum = header_v3_0->sequnum;

        if (skip_video_flag == 1) {
#if 1

            if ((int)data_type != (int)video_frame_type_KEYFRAME) {
                free(p);
                LOGGER_DEBUG(vc->log, "skipping incoming video frame (1)");

                if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
                    // TODO: What does this do?
                    //rc = vpx_codec_decode(vc->decoder, NULL, 0, NULL, VPX_DL_REALTIME);
                }

                pthread_mutex_unlock(vc->queue_mutex);
                return 0;
            }

#endif
        } else {
#if 0

            if ((int)rb_size((RingBuffer *)vc->vbuf_raw) > (int)VIDEO_RINGBUFFER_DROP_THRESHOLD) {
                // LOGGER_WARNING(vc->log, "skipping:002 data_type=%d", (int)data_type);
                if ((int)data_type != (int)video_frame_type_KEYFRAME) {
                    // LOGGER_WARNING(vc->log, "skipping:003");
                    free(p);
                    LOGGER_WARNING(vc->log, "skipping all incoming video frames (2)");

                    if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
                    rc = vpx_codec_decode(vc->decoder, NULL, 0, NULL, VPX_DL_REALTIME);
                    }

                    pthread_mutex_unlock(vc->queue_mutex);
                    return 0;
                }
            }

#endif
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

#if 1

        if ((int)data_type == (int)video_frame_type_KEYFRAME) {
            int percent_recvd = (int)(((float)header_v3->received_length_full / (float)full_data_len) * 100.0f);

            if (percent_recvd < 100) {
                LOGGER_WARNING(vc->log, "RTP_RECV:sn=%ld fn=%ld pct=%d%% *I* len=%ld recv_len=%ld",
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

            if ((percent_recvd < 100) && (have_requested_index_frame == false)) {
                if ((vc->last_requested_keyframe_ts + VIDEO_MIN_REQUEST_KEYFRAME_INTERVAL_MS_FOR_KF)
                        < current_time_monotonic()) {
                    // if keyframe received has less than 100% of the data, request a new keyframe
                    // from the sender
                    uint32_t pkg_buf_len = 2;
                    uint8_t pkg_buf[pkg_buf_len];
                    pkg_buf[0] = PACKET_REQUEST_KEYFRAME;
                    pkg_buf[1] = 0;

                    if (-1 == send_custom_lossless_packet(m, vc->friend_number, pkg_buf, pkg_buf_len)) {
                        LOGGER_WARNING(vc->log,
                                       "PACKET_REQUEST_KEYFRAME:RTP send failed");
                    } else {
                        LOGGER_WARNING(vc->log,
                                       "PACKET_REQUEST_KEYFRAME:RTP Sent.");
                        vc->last_requested_keyframe_ts = current_time_monotonic();
                    }
                }
            }
        } else {
            LOGGER_DEBUG(vc->log, "RTP_RECV:sn=%ld fn=%ld pct=%d%% len=%ld recv_len=%ld",
                         (long)header_v3->sequnum,
                         (long)header_v3->fragment_num,
                         (int)(((float)header_v3->received_length_full / (float)full_data_len) * 100.0f),
                         (long)full_data_len,
                         (long)header_v3->received_length_full);
        }
#endif

        LOGGER_DEBUG(vc->log, "h264_encoded_video_frame=%d", (int)h264_encoded_video_frame);

        if ((vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264)
                && (h264_encoded_video_frame == 1)) {
            LOGGER_DEBUG(vc->log, "h264_encoded_video_frame:AA");
            video_switch_decoder(vc, TOXAV_ENCODER_CODEC_USED_H264);
        } else if ((vc->video_decoder_codec_used == TOXAV_ENCODER_CODEC_USED_H264)
                   && (h264_encoded_video_frame == 0)) {
            LOGGER_DEBUG(vc->log, "h264_encoded_video_frame:BB");
            video_switch_decoder(vc, TOXAV_ENCODER_CODEC_USED_VP8);
        }

        if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_H264) {
            ret_value = vc_decode_frame_vpx(vc, header_v3, p->data, full_data_len);
        } else {
            ret_value = vc_decode_frame_h264(vc, header_v3, p->data, full_data_len);
        }

        free(p);

        return ret_value;
    } else {
        // no frame data available
        // LOGGER_WARNING(vc->log, "Error decoding video: rb_read");
    }

    pthread_mutex_unlock(vc->queue_mutex);

    return ret_value;
}


int vc_queue_message(void *vcp, struct RTPMessage *msg)
{
    /* This function is called with complete messages
     * they have already been assembled.
     * this function gets called from handle_rtp_packet() and handle_rtp_packet_v3()
     */
    if (!vcp || !msg) {
        return -1;
    }

    VCSession *vc = (VCSession *)vcp;

    const struct RTPHeader *header_v3 = (void *) & (msg->header);
    const struct RTPHeader *const header = &msg->header;

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

    pthread_mutex_lock(vc->queue_mutex);

    LOGGER_DEBUG(vc->log, "TT:queue:V:fragnum=%ld", (long)header_v3->fragment_num);


    if ((header->flags & RTP_LARGE_FRAME) && header->pt == rtp_TypeVideo % 128) {
        // LOGGER_WARNING(vc->log, "rb_write msg->len=%d b0=%d b1=%d rb_size=%d", (int)msg->len, (int)msg->data[0], (int)msg->data[1], (int)rb_size((RingBuffer *)vc->vbuf_raw));
        free(rb_write((RingBuffer *)vc->vbuf_raw, msg, (uint64_t)header->flags));
    } else {
        free(rb_write((RingBuffer *)vc->vbuf_raw, msg, 0));
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

        LOGGER_DEBUG(vc->log, "AUTOTUNE:INCOMING=%ld us = %.1f fps", (long)decode_time_auto_tune,
                     (float)(1000000.0f / decode_time_auto_tune));

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
#ifndef RASPBERRY_PI
        return vc_reconfigure_encoder_h264(log, vc, bit_rate, width, height, kf_max_dist);
#else
        return vc_reconfigure_encoder_h264_omx(log, vc, bit_rate, width, height, kf_max_dist);
#endif
    }
}

