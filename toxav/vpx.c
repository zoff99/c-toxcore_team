#include "video.h"
#include "ring_buffer.h"
#include "rtp.h"

#define VIDEO_MIN_SEND_KEYFRAME_INTERVAL 5000

struct vpx_frame_user_data {
    uint64_t record_timestamp;
};


VCSession *vc_new_vpx(Logger *log, ToxAV *av, uint32_t friend_number, toxav_video_receive_frame_cb *cb, void *cb_data,
                      VCSession *vc)
{

    vpx_codec_err_t rc;
    vc->send_keyframe_request_received = 0;

    /*
    VPX_CODEC_USE_FRAME_THREADING
       Enable frame-based multi-threading

    VPX_CODEC_USE_ERROR_CONCEALMENT
       Conceal errors in decoded frames
    */
    vpx_codec_dec_cfg_t  dec_cfg;
    dec_cfg.threads = VPX_MAX_DECODER_THREADS; // Maximum number of threads to use
    dec_cfg.w = VIDEO_CODEC_DECODER_MAX_WIDTH;
    dec_cfg.h = VIDEO_CODEC_DECODER_MAX_HEIGHT;

    if (VPX_DECODER_USED != TOXAV_ENCODER_CODEC_USED_VP9) {
        LOGGER_WARNING(log, "Using VP8 codec for decoder (0)");

        vpx_codec_flags_t dec_flags_ = 0;

        if (vc->video_decoder_error_concealment == 1) {
            vpx_codec_caps_t decoder_caps = vpx_codec_get_caps(VIDEO_CODEC_DECODER_INTERFACE_VP8);

            if (decoder_caps & VPX_CODEC_CAP_ERROR_CONCEALMENT) {
                dec_flags_ = VPX_CODEC_USE_ERROR_CONCEALMENT;
                LOGGER_WARNING(log, "Using VP8 VPX_CODEC_USE_ERROR_CONCEALMENT (0)");
            }
        }

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS
        rc = vpx_codec_dec_init(vc->decoder, VIDEO_CODEC_DECODER_INTERFACE_VP8, &dec_cfg,
                                dec_flags_ | VPX_CODEC_USE_FRAME_THREADING
                                | VPX_CODEC_USE_POSTPROC | VPX_CODEC_USE_INPUT_FRAGMENTS);
        LOGGER_WARNING(log, "Using VP8 using input fragments (0) rc=%d", (int)rc);
#else
        rc = vpx_codec_dec_init(vc->decoder, VIDEO_CODEC_DECODER_INTERFACE_VP8, &dec_cfg,
                                dec_flags_ | VPX_CODEC_USE_FRAME_THREADING
                                | VPX_CODEC_USE_POSTPROC);
#endif

        if (rc == VPX_CODEC_INCAPABLE) {
            LOGGER_WARNING(log, "Postproc not supported by this decoder (0)");
            rc = vpx_codec_dec_init(vc->decoder, VIDEO_CODEC_DECODER_INTERFACE_VP8, &dec_cfg,
                                    dec_flags_ | VPX_CODEC_USE_FRAME_THREADING);
        }

    } else {
        LOGGER_WARNING(log, "Using VP9 codec for decoder (0)");
        rc = vpx_codec_dec_init(vc->decoder, VIDEO_CODEC_DECODER_INTERFACE_VP9, &dec_cfg,
                                VPX_CODEC_USE_FRAME_THREADING);
    }

    if (rc != VPX_CODEC_OK) {
        LOGGER_ERROR(log, "Init video_decoder failed: %s", vpx_codec_err_to_string(rc));
        goto BASE_CLEANUP;
    }


    if (VPX_DECODER_USED != TOXAV_ENCODER_CODEC_USED_VP9) {
        if (VIDEO__VP8_DECODER_POST_PROCESSING_ENABLED == 1) {
            LOGGER_WARNING(log, "turn on postproc: OK");
        } else if (VIDEO__VP8_DECODER_POST_PROCESSING_ENABLED == 2) {
            vp8_postproc_cfg_t pp = {VP8_MFQE, 1, 0};
            vpx_codec_err_t cc_res = vpx_codec_control(vc->decoder, VP8_SET_POSTPROC, &pp);

            if (cc_res != VPX_CODEC_OK) {
                LOGGER_WARNING(log, "Failed to turn on postproc");
            } else {
                LOGGER_WARNING(log, "turn on postproc: OK");
            }
        } else if (VIDEO__VP8_DECODER_POST_PROCESSING_ENABLED == 3) {
            vp8_postproc_cfg_t pp = {VP8_DEBLOCK | VP8_DEMACROBLOCK | VP8_MFQE, 1, 0};
            vpx_codec_err_t cc_res = vpx_codec_control(vc->decoder, VP8_SET_POSTPROC, &pp);

            if (cc_res != VPX_CODEC_OK) {
                LOGGER_WARNING(log, "Failed to turn on postproc");
            } else {
                LOGGER_WARNING(log, "turn on postproc: OK");
            }
        } else {
            vp8_postproc_cfg_t pp = {0, 0, 0};
            vpx_codec_err_t cc_res = vpx_codec_control(vc->decoder, VP8_SET_POSTPROC, &pp);

            if (cc_res != VPX_CODEC_OK) {
                LOGGER_WARNING(log, "Failed to turn OFF postproc");
            } else {
                LOGGER_WARNING(log, "Disable postproc: OK");
            }
        }
    }










    /* Set encoder to some initial values
     */
    vpx_codec_enc_cfg_t  cfg;
    vc__init_encoder_cfg(log, &cfg, 1,
                         vc->video_encoder_vp8_quality,
                         vc->video_rc_max_quantizer,
                         vc->video_rc_min_quantizer,
                         vc->video_encoder_coded_used,
                         vc->video_keyframe_method);

    if (vc->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
        LOGGER_WARNING(log, "Using VP8 codec for encoder (0.1)");

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS
        rc = vpx_codec_enc_init(vc->encoder, VIDEO_CODEC_ENCODER_INTERFACE_VP8, &cfg,
                                VPX_CODEC_USE_FRAME_THREADING | VPX_CODEC_USE_OUTPUT_PARTITION);
#else
        rc = vpx_codec_enc_init(vc->encoder, VIDEO_CODEC_ENCODER_INTERFACE_VP8, &cfg,
                                VPX_CODEC_USE_FRAME_THREADING);
#endif

    } else {
        LOGGER_WARNING(log, "Using VP9 codec for encoder (0.1)");
        rc = vpx_codec_enc_init(vc->encoder, VIDEO_CODEC_ENCODER_INTERFACE_VP9, &cfg, VPX_CODEC_USE_FRAME_THREADING);
    }

    if (rc != VPX_CODEC_OK) {
        LOGGER_ERROR(log, "Failed to initialize encoder: %s", vpx_codec_err_to_string(rc));
        goto BASE_CLEANUP_1;
    }


#if 1
    rc = vpx_codec_control(vc->encoder, VP8E_SET_ENABLEAUTOALTREF, 0);

    if (rc != VPX_CODEC_OK) {
        LOGGER_ERROR(log, "Failed to set encoder VP8E_SET_ENABLEAUTOALTREF setting: %s value=%d", vpx_codec_err_to_string(rc),
                     (int)0);
        vpx_codec_destroy(vc->encoder);
        goto BASE_CLEANUP_1;
    } else {
        LOGGER_WARNING(log, "set encoder VP8E_SET_ENABLEAUTOALTREF setting: %s value=%d", vpx_codec_err_to_string(rc),
                       (int)0);
    }

#endif


#if 1
    uint32_t rc_max_intra_target = MaxIntraTarget(600);
    rc_max_intra_target = 102;
    rc = vpx_codec_control(vc->encoder, VP8E_SET_MAX_INTRA_BITRATE_PCT, rc_max_intra_target);

    if (rc != VPX_CODEC_OK) {
        LOGGER_ERROR(log, "Failed to set encoder VP8E_SET_MAX_INTRA_BITRATE_PCT setting: %s value=%d",
                     vpx_codec_err_to_string(rc),
                     (int)rc_max_intra_target);
        vpx_codec_destroy(vc->encoder);
        goto BASE_CLEANUP_1;
    } else {
        LOGGER_WARNING(log, "set encoder VP8E_SET_MAX_INTRA_BITRATE_PCT setting: %s value=%d", vpx_codec_err_to_string(rc),
                       (int)rc_max_intra_target);
    }

#endif



    /*
    Codec control function to set encoder internal speed settings.
    Changes in this value influences, among others, the encoder's selection of motion estimation methods.
    Values greater than 0 will increase encoder speed at the expense of quality.

    Note:
      Valid range for VP8: -16..16
      Valid range for VP9: -8..8
    */

    int cpu_used_value = vc->video_encoder_cpu_used;

    if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9) {
        if (cpu_used_value < -8) {
            cpu_used_value = -8;
        } else if (cpu_used_value > 8) {
            cpu_used_value = 8; // set to default (fastest) value
        }
    }

    rc = vpx_codec_control(vc->encoder, VP8E_SET_CPUUSED, cpu_used_value);

    if (rc != VPX_CODEC_OK) {
        LOGGER_ERROR(log, "Failed to set encoder VP8E_SET_CPUUSED setting: %s value=%d", vpx_codec_err_to_string(rc),
                     (int)cpu_used_value);
        vpx_codec_destroy(vc->encoder);
        goto BASE_CLEANUP_1;
    } else {
        LOGGER_WARNING(log, "set encoder VP8E_SET_CPUUSED setting: %s value=%d", vpx_codec_err_to_string(rc),
                       (int)cpu_used_value);
    }

    vc->video_encoder_cpu_used = cpu_used_value;
    vc->video_encoder_cpu_used_prev = cpu_used_value;

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS

    if (vc->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
        rc = vpx_codec_control(vc->encoder, VP8E_SET_TOKEN_PARTITIONS, VIDEO_CODEC_FRAGMENT_VPX_NUMS);

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(log, "Failed to set encoder token partitions: %s", vpx_codec_err_to_string(rc));
        }
    }

#endif

    /*
    VP9E_SET_TILE_COLUMNS

    Codec control function to set number of tile columns.

    In encoding and decoding, VP9 allows an input image frame be partitioned
    into separated vertical tile columns, which can be encoded or decoded independently.
    This enables easy implementation of parallel encoding and decoding. This control requests
    the encoder to use column tiles in encoding an input frame, with number of tile columns
    (in Log2 unit) as the parameter: 0 = 1 tile column 1 = 2 tile columns
    2 = 4 tile columns ..... n = 2**n tile columns The requested tile columns will
    be capped by encoder based on image size limitation (The minimum width of a
    tile column is 256 pixel, the maximum is 4096).

    By default, the value is 0, i.e. one single column tile for entire image.

    Supported in codecs: VP9
     */

    if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9) {
        rc = vpx_codec_control(vc->encoder, VP9E_SET_TILE_COLUMNS, VIDEO__VP9E_SET_TILE_COLUMNS);

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
            vpx_codec_destroy(vc->encoder);
            goto BASE_CLEANUP_1;
        }

        rc = vpx_codec_control(vc->encoder, VP9E_SET_TILE_ROWS, VIDEO__VP9E_SET_TILE_ROWS);

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
            vpx_codec_destroy(vc->encoder);
            goto BASE_CLEANUP_1;
        }
    }


#if 0

    if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9) {
        if (1 == 2) {
            rc = vpx_codec_control(vc->encoder, VP9E_SET_LOSSLESS, 1);

            LOGGER_WARNING(vc->log, "setting VP9 lossless video quality(2): ON");

            if (rc != VPX_CODEC_OK) {
                LOGGER_ERROR(log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
                vpx_codec_destroy(vc->encoder);
                goto BASE_CLEANUP_1;
            }
        } else {
            rc = vpx_codec_control(vc->encoder, VP9E_SET_LOSSLESS, 0);

            LOGGER_WARNING(vc->log, "setting VP9 lossless video quality(2): OFF");

            if (rc != VPX_CODEC_OK) {
                LOGGER_ERROR(log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
                vpx_codec_destroy(vc->encoder);
                goto BASE_CLEANUP_1;
            }
        }
    }

#endif


    /*
    VPX_CTRL_USE_TYPE(VP8E_SET_NOISE_SENSITIVITY,  unsigned int)
    control function to set noise sensitivity
      0: off, 1: OnYOnly, 2: OnYUV, 3: OnYUVAggressive, 4: Adaptive
    */
    /*
      rc = vpx_codec_control(vc->encoder, VP8E_SET_NOISE_SENSITIVITY, 2);

      if (rc != VPX_CODEC_OK) {
          LOGGER_ERROR(log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
          vpx_codec_destroy(vc->encoder);
          goto BASE_CLEANUP_1;
      }
     */

    // VP8E_SET_STATIC_THRESHOLD


    vc->linfts = current_time_monotonic();
    vc->lcfd = 10; // initial value in ms for av_iterate sleep
    vc->vcb.first = cb;
    vc->vcb.second = cb_data;
    vc->friend_number = friend_number;
    vc->av = av;
    vc->log = log;
    vc->last_decoded_frame_ts = 0;
    vc->last_encoded_frame_ts = 0;
    vc->flag_end_video_fragment = 0;
    vc->last_seen_fragment_num = 0;
    vc->count_old_video_frames_seen = 0;
    vc->last_seen_fragment_seqnum = -1;
    vc->fragment_buf_counter = 0;

    for (int k = 0; k < VIDEO_DECODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES; k++) {
        vc->decoder_soft_deadline[k] = MAX_DECODE_TIME_US;
    }

    vc->decoder_soft_deadline_index = 0;

    for (int k = 0; k < VIDEO_ENCODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES; k++) {
        vc->encoder_soft_deadline[k] = MAX_ENCODE_TIME_US;
    }

    vc->encoder_soft_deadline_index = 0;

    uint16_t jk = 0;

    for (jk = 0; jk < (uint16_t)VIDEO_MAX_FRAGMENT_BUFFER_COUNT; jk++) {
        vc->vpx_frames_buf_list[jk] = NULL;
    }

    return vc;

BASE_CLEANUP_1:
    vpx_codec_destroy(vc->decoder);
BASE_CLEANUP:
    pthread_mutex_destroy(vc->queue_mutex);
    rb_kill((RingBuffer *)vc->vbuf_raw);
    free(vc);
    return NULL;
}


void vc_kill_vpx(VCSession *vc)
{
    int jk;

    for (jk = 0; jk < vc->fragment_buf_counter; jk++) {
        free(vc->vpx_frames_buf_list[jk]);
        vc->vpx_frames_buf_list[jk] = NULL;
    }

    vc->fragment_buf_counter = 0;

    vpx_codec_destroy(vc->encoder);
    vpx_codec_destroy(vc->decoder);
}



bool vc_encode_frame_vpx(VCSession* vc, struct RTPSession* rtp, uint16_t width, uint16_t height, const uint8_t *y,
                            const uint8_t *u, const uint8_t *v, TOXAV_ERR_SEND_FRAME *error)
{
    uint64_t video_frame_record_timestamp = current_time_monotonic();

    int vpx_encode_flags = 0;
    unsigned long max_encode_time_in_us = MAX_ENCODE_TIME_US;

    if (vc->video_keyframe_method == TOXAV_ENCODER_KF_METHOD_NORMAL) {
        if (rtp->ssrc < VIDEO_SEND_X_KEYFRAMES_FIRST) {

            if (vc->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
                // Key frame flag for first frames
                vpx_encode_flags = VPX_EFLAG_FORCE_KF;
                vpx_encode_flags |= VP8_EFLAG_FORCE_GF;
                // vpx_encode_flags |= VP8_EFLAG_FORCE_ARF;

                max_encode_time_in_us = VPX_DL_REALTIME;
                // uint32_t lowered_bitrate = (300 * 1000);
                // vc_reconfigure_encoder_bitrate_only(vc, lowered_bitrate);
                // HINT: Zoff: this does not seem to work
                // vpx_codec_control(vc->encoder, VP8E_SET_FRAME_FLAGS, vpx_encode_flags);
                // LOGGER_ERROR(vc->log, "I_FRAME_FLAG:%d only-i-frame mode", rtp->ssrc);
            }


            rtp->ssrc++;
        } else if (rtp->ssrc == VIDEO_SEND_X_KEYFRAMES_FIRST) {
            if (vc->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
                // normal keyframe placement
                vpx_encode_flags = 0;
                max_encode_time_in_us = MAX_ENCODE_TIME_US;
                LOGGER_INFO(vc->log, "I_FRAME_FLAG:%d normal mode", rtp->ssrc);
            }

            rtp->ssrc++;
        }
    }

    // we start with I-frames (full frames) and then switch to normal mode later

    vc->last_encoded_frame_ts = current_time_monotonic();

    if (vc->send_keyframe_request_received == 1) {
        vpx_encode_flags = VPX_EFLAG_FORCE_KF;
        vpx_encode_flags |= VP8_EFLAG_FORCE_GF;
        // vpx_encode_flags |= VP8_EFLAG_FORCE_ARF;
        vc->send_keyframe_request_received = 0;
    } else {
        if ((vc->last_sent_keyframe_ts + VIDEO_MIN_SEND_KEYFRAME_INTERVAL)
                < current_time_monotonic()) {
            // it's been x seconds without a keyframe, send one now
            vpx_encode_flags = VPX_EFLAG_FORCE_KF;
            vpx_encode_flags |= VP8_EFLAG_FORCE_GF;
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

    vpx_image_t img;
    img.w = img.h = img.d_w = img.d_h = 0;
    vpx_img_alloc(&img, VPX_IMG_FMT_I420, width, height, 0);

    /* I420 "It comprises an NxM Y plane followed by (N/2)x(M/2) V and U planes."
        * http://fourcc.org/yuv.php#IYUV
        */
    memcpy(img.planes[VPX_PLANE_Y], y, width * height);
    memcpy(img.planes[VPX_PLANE_U], u, (width / 2) * (height / 2));
    memcpy(img.planes[VPX_PLANE_V], v, (width / 2) * (height / 2));

#if 0
    uint32_t duration = (ms_to_last_frame * 10) + 1;

    if (duration > 10000) {
        duration = 10000;
    }

#else
    // set to hardcoded 24fps (this is only for vpx internal calculations!!)
    uint32_t duration = (41 * 10); // HINT: 24fps ~= 41ms
#endif

    vpx_codec_err_t vrc = vpx_codec_encode(vc->encoder, &img,
                                            (int64_t)video_frame_record_timestamp, duration,
                                            vpx_encode_flags,
                                            VPX_DL_REALTIME);

    vpx_img_free(&img);

    if (vrc != VPX_CODEC_OK) {
        return TOXAV_ERR_SEND_FRAME_INVALID;
    }


    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt;

    while ((pkt = vpx_codec_get_cx_data(vc->encoder, &iter)) != NULL) {
        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
            const int keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;

            if (keyframe) {
                vc->last_sent_keyframe_ts = current_time_monotonic();
            }

            if ((pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT) != 0) {
                LOGGER_DEBUG(vc->log, "VPXENC:VPX_FRAME_IS_FRAGMENT:*yes* size=%lld pid=%d\n",
                                (long long)pkt->data.frame.sz, (int)pkt->data.frame.partition_id);
            } else {
                LOGGER_DEBUG(vc->log, "VPXENC:VPX_FRAME_IS_FRAGMENT:-no- size=%lld pid=%d\n",
                                (long long)pkt->data.frame.sz, (int)pkt->data.frame.partition_id);
            }

            // use the record timestamp that was actually used for this frame
            video_frame_record_timestamp = (uint64_t)pkt->data.frame.pts;
            // LOGGER_DEBUG(vc->log, "video packet record time: %llu", video_frame_record_timestamp);

            // https://www.webmproject.org/docs/webm-sdk/structvpx__codec__cx__pkt.html
            // pkt->data.frame.sz -> size_t
            const uint32_t frame_length_in_bytes = pkt->data.frame.sz;


            int res = rtp_send_data
                        (
                            rtp,
                            (const uint8_t *)pkt->data.frame.buf,
                            frame_length_in_bytes,
                            keyframe,
                            video_frame_record_timestamp,
                            (int32_t)pkt->data.frame.partition_id,
                            TOXAV_ENCODER_CODEC_USED_VP8,
                            vc->log
                        );

            LOGGER_DEBUG(vc->log, "+ _sending_FRAME_TYPE_==%s bytes=%d frame_len=%d", keyframe ? "K" : ".",
                            (int)pkt->data.frame.sz, (int)frame_length_in_bytes);
            LOGGER_DEBUG(vc->log, "+ _sending_FRAME_ b0=%d b1=%d", ((const uint8_t *)pkt->data.frame.buf)[0] ,
                            ((const uint8_t *)pkt->data.frame.buf)[1]);

            video_frame_record_timestamp++;

            if (res < 0) {
                LOGGER_WARNING(vc->log, "Could not send video frame: %s", strerror(errno));
                return TOXAV_ERR_SEND_FRAME_RTP_FAILED;
            }
        }
    }
}

int vc_decode_frame_vpx(VCSession *vc, struct RTPHeader* header_v3, uint8_t *data, uint32_t data_len)
{
    vpx_codec_err_t rc;
    int ret_value = 0;
    // LOGGER_ERROR(vc->log, "DEC:VP8------------");

    long decoder_soft_dealine_value_used = VPX_DL_REALTIME;
    void *user_priv = NULL;

    if (header_v3->frame_record_timestamp > 0) {
#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS
        // --- //
#else

#ifdef VIDEO_PTS_TIMESTAMPS
        struct vpx_frame_user_data *vpx_u_data = calloc(1, sizeof(struct vpx_frame_user_data));
        vpx_u_data->record_timestamp = header_v3->frame_record_timestamp;
        user_priv = vpx_u_data;
#endif

#endif
    }

    if ((int)rb_size((RingBuffer *)vc->vbuf_raw) > (int)VIDEO_RINGBUFFER_FILL_THRESHOLD) {
        rc = vpx_codec_decode(vc->decoder, data, data_len, user_priv, VPX_DL_REALTIME);
        LOGGER_DEBUG(vc->log, "skipping:REALTIME");
    }

#ifdef VIDEO_DECODER_SOFT_DEADLINE_AUTOTUNE
    else {
        long decode_time_auto_tune = MAX_DECODE_TIME_US;

        if (vc->last_decoded_frame_ts > 0) {

            // calc mean value
            decode_time_auto_tune = 0;

            for (int k = 0; k < VIDEO_DECODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES; k++) {
                decode_time_auto_tune = decode_time_auto_tune + vc->decoder_soft_deadline[k];
            }

            decode_time_auto_tune = decode_time_auto_tune / VIDEO_DECODER_SOFT_DEADLINE_AUTOTUNE_ENTRIES;

            if (decode_time_auto_tune > (1000000 / VIDEO_DECODER_MINFPS_AUTOTUNE)) {
                decode_time_auto_tune = (1000000 / VIDEO_DECODER_MINFPS_AUTOTUNE);
            }

            if (decode_time_auto_tune > (VIDEO_DECODER_LEEWAY_IN_MS_AUTOTUNE * 1000)) {
                decode_time_auto_tune = decode_time_auto_tune - (VIDEO_DECODER_LEEWAY_IN_MS_AUTOTUNE * 1000); // give x ms more room
            }
        }

        decoder_soft_dealine_value_used = decode_time_auto_tune;
        rc = vpx_codec_decode(vc->decoder, data, data_len, user_priv, (long)decode_time_auto_tune);

        LOGGER_DEBUG(vc->log, "AUTOTUNE:MAX_DECODE_TIME_US=%ld us = %.1f fps", (long)decode_time_auto_tune,
                        (float)(1000000.0f / decode_time_auto_tune));
    }

#else
    else {
        decoder_soft_dealine_value_used = MAX_DECODE_TIME_US;
        rc = vpx_codec_decode(vc->decoder, data, data_len, user_priv, MAX_DECODE_TIME_US);
        // LOGGER_WARNING(vc->log, "NORMAL:MAX_DECODE_TIME_US=%d", (int)MAX_DECODE_TIME_US);
    }

#endif



#ifdef VIDEO_DECODER_AUTOSWITCH_CODEC

    if (rc != VPX_CODEC_OK) {
        if ((rc == VPX_CODEC_CORRUPT_FRAME) || (rc == VPX_CODEC_UNSUP_BITSTREAM)) {
            // LOGGER_ERROR(vc->log, "Error decoding video: VPX_CODEC_CORRUPT_FRAME or VPX_CODEC_UNSUP_BITSTREAM");
        } else {
            // LOGGER_ERROR(vc->log, "Error decoding video: err-num=%d err-str=%s", (int)rc, vpx_codec_err_to_string(rc));
        }
    }

#else

    if (rc != VPX_CODEC_OK) {
        if (rc == VPX_CODEC_CORRUPT_FRAME) {
            LOGGER_WARNING(vc->log, "Corrupt frame detected: data size=%d start byte=%d end byte=%d",
                            (int)data_len, (int)data[0], (int)data[data_len - 1]);
        } else {
            // LOGGER_ERROR(vc->log, "Error decoding video: err-num=%d err-str=%s", (int)rc, vpx_codec_err_to_string(rc));
        }
    }

#endif

    if (rc == VPX_CODEC_OK) {

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS

        int save_current_buf = 0;

        if (header_v3->fragment_num < vc->last_seen_fragment_num) {
            if (vc->flag_end_video_fragment == 0) {
                //LOGGER_WARNING(vc->log, "endframe:x:%d", (int)header_v3->fragment_num);
                vc->flag_end_video_fragment = 1;
                save_current_buf = 1;
                vpx_codec_decode(vc->decoder, NULL, 0, user_priv, decoder_soft_dealine_value_used);
            } else {
                vc->flag_end_video_fragment = 0;
                //LOGGER_WARNING(vc->log, "reset:flag:%d", (int)header_v3->fragment_num);
            }
        } else {
            if ((long)header_v3->fragment_num == (long)(VIDEO_CODEC_FRAGMENT_NUMS - 1)) {
                //LOGGER_WARNING(vc->log, "endframe:N:%d", (int)(VIDEO_CODEC_FRAGMENT_NUMS - 1));
                vc->flag_end_video_fragment = 1;
                vpx_codec_decode(vc->decoder, NULL, 0, user_priv, decoder_soft_dealine_value_used);
            }
        }

        // push buffer to list
        if (vc->fragment_buf_counter < (uint16_t)(VIDEO_MAX_FRAGMENT_BUFFER_COUNT - 1)) {
            vc->vpx_frames_buf_list[vc->fragment_buf_counter] = p;
            vc->fragment_buf_counter++;
        } else {
            LOGGER_WARNING(vc->log, "mem leak: VIDEO_MAX_FRAGMENT_BUFFER_COUNT");
        }

        vc->last_seen_fragment_num = header_v3->fragment_num;
#endif

        /* Play decoded images */
        vpx_codec_iter_t iter = NULL;
        vpx_image_t *dest = NULL;

        while ((dest = vpx_codec_get_frame(vc->decoder, &iter)) != NULL) {
            // we have a frame, set return code
            ret_value = 1;

            if (vc->vcb.first) {

                // what is the audio to video latency?
                //
                if (dest->user_priv != NULL) {
                    uint64_t frame_record_timestamp_vpx = ((struct vpx_frame_user_data *)(dest->user_priv))->record_timestamp;

                    //LOGGER_ERROR(vc->log, "VIDEO:TTx: %llu now=%llu", frame_record_timestamp_vpx, current_time_monotonic());
                    if (frame_record_timestamp_vpx > 0) {
                        ret_value = 1;

// TODO: Understand what this timestamping does, does it belong into the codec?
#if 0
                        if (*v_r_timestamp < frame_record_timestamp_vpx) {
                            // LOGGER_ERROR(vc->log, "VIDEO:TTx:2: %llu", frame_record_timestamp_vpx);
                            *v_r_timestamp = frame_record_timestamp_vpx;
                            *v_l_timestamp = current_time_monotonic();
                        } else {
                            // TODO: this should not happen here!
                            LOGGER_DEBUG(vc->log, "VIDEO: remote timestamp older");
                        }
#endif
                    }

                    //
                    // what is the audio to video latency?
                    free(dest->user_priv);
                }

                LOGGER_DEBUG(vc->log, "VIDEO: -FRAME OUT- %p %p %p",
                                (const uint8_t *)dest->planes[0],
                                (const uint8_t *)dest->planes[1],
                                (const uint8_t *)dest->planes[2]);

                vc->vcb.first(vc->av, vc->friend_number, dest->d_w, dest->d_h,
                                (const uint8_t *)dest->planes[0],
                                (const uint8_t *)dest->planes[1],
                                (const uint8_t *)dest->planes[2],
                                dest->stride[0], dest->stride[1], dest->stride[2], vc->vcb.second);
            }

            vpx_img_free(dest); // is this needed? none of the VPx examples show that
        }

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS

        if (vc->flag_end_video_fragment == 1) {
            //LOGGER_ERROR(vc->log, "free vpx_frames_buf_list:count=%d", (int)vc->fragment_buf_counter);
            uint16_t jk = 0;

            if (save_current_buf == 1) {
                for (jk = 0; jk < (vc->fragment_buf_counter - 1); jk++) {
                    free(vc->vpx_frames_buf_list[jk]);
                    vc->vpx_frames_buf_list[jk] = NULL;
                }

                vc->vpx_frames_buf_list[0] = vc->vpx_frames_buf_list[vc->fragment_buf_counter];
                vc->vpx_frames_buf_list[vc->fragment_buf_counter] = NULL;
                vc->fragment_buf_counter = 1;
            } else {
                for (jk = 0; jk < vc->fragment_buf_counter; jk++) {
                    free(vc->vpx_frames_buf_list[jk]);
                    vc->vpx_frames_buf_list[jk] = NULL;
                }

                vc->fragment_buf_counter = 0;
            }
        }
#endif

    }
}

void video_switch_decoder_vpx(VCSession *vc, TOXAV_ENCODER_CODEC_USED_VALUE decoder_to_use)
{

    /*
    vpx_codec_err_t vpx_codec_peek_stream_info  (   vpx_codec_iface_t *     iface,
            const uint8_t *     data,
            unsigned int    data_sz,
            vpx_codec_stream_info_t *   si
        )

    Parse stream info from a buffer.
    Performs high level parsing of the bitstream. Construction of a decoder context is not necessary.
    Can be used to determine if the bitstream is of the proper format, and to extract information from the stream.
    */


    // toggle decoder codec between VP8 and VP9
    // TODO: put codec into header flags at encoder side, then use this at decoder side!
    if (vc->video_decoder_codec_used == TOXAV_ENCODER_CODEC_USED_VP8) {
        vc->video_decoder_codec_used = TOXAV_ENCODER_CODEC_USED_VP9;
    } else {
        vc->video_decoder_codec_used = TOXAV_ENCODER_CODEC_USED_VP8;
    }

    vpx_codec_err_t rc;
    vpx_codec_ctx_t new_d;

    LOGGER_WARNING(vc->log, "Switch:Re-initializing DEcoder to: %d", (int)vc->video_decoder_codec_used);

    vpx_codec_dec_cfg_t dec_cfg;
    dec_cfg.threads = VPX_MAX_DECODER_THREADS; // Maximum number of threads to use
    dec_cfg.w = VIDEO_CODEC_DECODER_MAX_WIDTH;
    dec_cfg.h = VIDEO_CODEC_DECODER_MAX_HEIGHT;

    if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_VP9) {

        vpx_codec_flags_t dec_flags_ = 0;

        if (vc->video_decoder_error_concealment == 1) {
            vpx_codec_caps_t decoder_caps = vpx_codec_get_caps(VIDEO_CODEC_DECODER_INTERFACE_VP8);

            if (decoder_caps & VPX_CODEC_CAP_ERROR_CONCEALMENT) {
                dec_flags_ = VPX_CODEC_USE_ERROR_CONCEALMENT;
                LOGGER_WARNING(vc->log, "Using VP8 VPX_CODEC_USE_ERROR_CONCEALMENT (1)");
            }
        }

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS
        rc = vpx_codec_dec_init(&new_d, VIDEO_CODEC_DECODER_INTERFACE_VP8, &dec_cfg,
                                dec_flags_ | VPX_CODEC_USE_FRAME_THREADING
                                | VPX_CODEC_USE_POSTPROC | VPX_CODEC_USE_INPUT_FRAGMENTS);
        LOGGER_WARNING(vc->log, "Using VP8 using input fragments (1) rc=%d", (int)rc);
#else
        rc = vpx_codec_dec_init(&new_d, VIDEO_CODEC_DECODER_INTERFACE_VP8, &dec_cfg,
                                dec_flags_ | VPX_CODEC_USE_FRAME_THREADING
                                | VPX_CODEC_USE_POSTPROC);
#endif

        if (rc == VPX_CODEC_INCAPABLE) {
            LOGGER_WARNING(vc->log, "Postproc not supported by this decoder");
            rc = vpx_codec_dec_init(&new_d, VIDEO_CODEC_DECODER_INTERFACE_VP8, &dec_cfg,
                                    dec_flags_ | VPX_CODEC_USE_FRAME_THREADING);
        }

    } else {
        rc = vpx_codec_dec_init(&new_d, VIDEO_CODEC_DECODER_INTERFACE_VP9, &dec_cfg,
                                VPX_CODEC_USE_FRAME_THREADING);
    }

    if (rc != VPX_CODEC_OK) {
        LOGGER_ERROR(vc->log, "Failed to Re-initialize decoder: %s", vpx_codec_err_to_string(rc));
        vpx_codec_destroy(&new_d);
        return;
    }


    if (vc->video_decoder_codec_used != TOXAV_ENCODER_CODEC_USED_VP9) {
        if (VIDEO__VP8_DECODER_POST_PROCESSING_ENABLED == 1) {
            LOGGER_WARNING(vc->log, "turn on postproc: OK");
        } else if (VIDEO__VP8_DECODER_POST_PROCESSING_ENABLED == 2) {
            vp8_postproc_cfg_t pp = {VP8_DEBLOCK, 1, 0};
            vpx_codec_err_t cc_res = vpx_codec_control(&new_d, VP8_SET_POSTPROC, &pp);

            if (cc_res != VPX_CODEC_OK) {
                LOGGER_WARNING(vc->log, "Failed to turn on postproc");
            } else {
                LOGGER_WARNING(vc->log, "turn on postproc: OK");
            }
        } else if (VIDEO__VP8_DECODER_POST_PROCESSING_ENABLED == 3) {
            vp8_postproc_cfg_t pp = {VP8_DEBLOCK | VP8_DEMACROBLOCK | VP8_MFQE, 1, 0};
            vpx_codec_err_t cc_res = vpx_codec_control(&new_d, VP8_SET_POSTPROC, &pp);

            if (cc_res != VPX_CODEC_OK) {
                LOGGER_WARNING(vc->log, "Failed to turn on postproc");
            } else {
                LOGGER_WARNING(vc->log, "turn on postproc: OK");
            }
        } else {
            vp8_postproc_cfg_t pp = {0, 0, 0};
            vpx_codec_err_t cc_res = vpx_codec_control(&new_d, VP8_SET_POSTPROC, &pp);

            if (cc_res != VPX_CODEC_OK) {
                LOGGER_WARNING(vc->log, "Failed to turn OFF postproc");
            } else {
                LOGGER_WARNING(vc->log, "Disable postproc: OK");
            }
        }
    }

    // now replace the current decoder
    vpx_codec_destroy(vc->decoder);
    memcpy(vc->decoder, &new_d, sizeof(new_d));

    LOGGER_ERROR(vc->log, "Re-initialize decoder OK: %s", vpx_codec_err_to_string(rc));

}


int vc_reconfigure_encoder_vpx(Logger *log, VCSession *vc, uint32_t bit_rate, uint16_t width, uint16_t height,
                               int16_t kf_max_dist)
{
    if (!vc) {
        return -1;
    }

    vpx_codec_enc_cfg_t cfg2 = *vc->encoder->config.enc;
    vpx_codec_err_t rc;

    if (cfg2.rc_target_bitrate == bit_rate && cfg2.g_w == width && cfg2.g_h == height && kf_max_dist == -1
            && vc->video_encoder_cpu_used == vc->video_encoder_cpu_used_prev
            && vc->video_encoder_vp8_quality == vc->video_encoder_vp8_quality_prev
            && vc->video_rc_max_quantizer == vc->video_rc_max_quantizer_prev
            && vc->video_rc_min_quantizer == vc->video_rc_min_quantizer_prev
            && vc->video_encoder_coded_used == vc->video_encoder_coded_used_prev
            && vc->video_keyframe_method == vc->video_keyframe_method_prev
       ) {
        return 0; /* Nothing changed */
    }

    if (cfg2.g_w == width && cfg2.g_h == height && kf_max_dist == -1
            && vc->video_encoder_cpu_used == vc->video_encoder_cpu_used_prev
            && vc->video_encoder_vp8_quality == vc->video_encoder_vp8_quality_prev
            && vc->video_rc_max_quantizer == vc->video_rc_max_quantizer_prev
            && vc->video_rc_min_quantizer == vc->video_rc_min_quantizer_prev
            && vc->video_encoder_coded_used == vc->video_encoder_coded_used_prev
            && vc->video_keyframe_method == vc->video_keyframe_method_prev
       ) {
        /* Only bit rate changed */

        LOGGER_INFO(vc->log, "bitrate change from: %u to: %u", (uint32_t)(cfg2.rc_target_bitrate / 1000),
                    (uint32_t)(bit_rate / 1000));

        cfg2.rc_target_bitrate = bit_rate;

        rc = vpx_codec_enc_config_set(vc->encoder, &cfg2);

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(vc->log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
            return -1;
        }
    } else {
        /* Resolution is changed, must reinitialize encoder since libvpx v1.4 doesn't support
         * reconfiguring encoder to use resolutions greater than initially set.
         */
        /*
         * TODO: Zoff in 2018: i wonder if this is still the case with libvpx 1.7.x ?
         */

        LOGGER_DEBUG(vc->log, "Have to reinitialize vpx encoder on session %p", vc);


        vpx_codec_ctx_t new_c;
        vpx_codec_enc_cfg_t  cfg;
        vc__init_encoder_cfg(vc->log, &cfg, kf_max_dist,
                             vc->video_encoder_vp8_quality,
                             vc->video_rc_max_quantizer,
                             vc->video_rc_min_quantizer,
                             vc->video_encoder_coded_used,
                             vc->video_keyframe_method);

        vc->video_encoder_coded_used_prev = vc->video_encoder_coded_used;
        vc->video_encoder_vp8_quality_prev = vc->video_encoder_vp8_quality;
        vc->video_rc_max_quantizer_prev = vc->video_rc_max_quantizer;
        vc->video_rc_min_quantizer_prev = vc->video_rc_min_quantizer;
        vc->video_keyframe_method_prev = vc->video_keyframe_method;

        cfg.rc_target_bitrate = bit_rate;
        cfg.g_w = width;
        cfg.g_h = height;


        if (vc->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
            LOGGER_WARNING(vc->log, "Using VP8 codec for encoder");

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS
            rc = vpx_codec_enc_init(&new_c, VIDEO_CODEC_ENCODER_INTERFACE_VP8, &cfg,
                                    VPX_CODEC_USE_FRAME_THREADING | VPX_CODEC_USE_OUTPUT_PARTITION);
#else
            rc = vpx_codec_enc_init(&new_c, VIDEO_CODEC_ENCODER_INTERFACE_VP8, &cfg,
                                    VPX_CODEC_USE_FRAME_THREADING);
#endif
        } else {
            LOGGER_WARNING(vc->log, "Using VP9 codec for encoder");
            rc = vpx_codec_enc_init(&new_c, VIDEO_CODEC_ENCODER_INTERFACE_VP9, &cfg, VPX_CODEC_USE_FRAME_THREADING);
        }

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(vc->log, "Failed to initialize encoder: %s", vpx_codec_err_to_string(rc));
            return -1;
        }


#if 0
        rc = vpx_codec_control(&new_c, VP8E_SET_ENABLEAUTOALTREF, 0);

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(vc->log, "(b)Failed to set encoder VP8E_SET_ENABLEAUTOALTREF setting: %s value=%d",
                         vpx_codec_err_to_string(rc),
                         (int)1);
            vpx_codec_destroy(&new_c);
            return -1;
        } else {
            LOGGER_WARNING(vc->log, "(b)set encoder VP8E_SET_ENABLEAUTOALTREF setting: %s value=%d", vpx_codec_err_to_string(rc),
                           (int)1);
        }

#endif

        /*
        encoder->Control(VP8E_SET_ARNR_MAXFRAMES, 7);
        encoder->Control(VP8E_SET_ARNR_STRENGTH, 5);
        encoder->Control(VP8E_SET_ARNR_TYPE, 3);
        */

#if 1
        /*
        Codec control function to set Max data rate for Intra frames.
        This value controls additional clamping on the maximum size of a keyframe. It is expressed as a percentage of the average per-frame bitrate, with the special (and default) value 0 meaning unlimited, or no additional clamping beyond the codec's built-in algorithm.
        For example, to allocate no more than 4.5 frames worth of bitrate to a keyframe, set this to 450.
        Supported in codecs: VP8, VP9
        */
        uint32_t rc_max_intra_target = MaxIntraTarget(600);
        rc_max_intra_target = 102;
        rc = vpx_codec_control(&new_c, VP8E_SET_MAX_INTRA_BITRATE_PCT, rc_max_intra_target);

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(vc->log, "(b)Failed to set encoder VP8E_SET_MAX_INTRA_BITRATE_PCT setting: %s value=%d",
                         vpx_codec_err_to_string(rc),
                         (int)rc_max_intra_target);
            vpx_codec_destroy(&new_c);
            return -1;
        } else {
            LOGGER_WARNING(vc->log, "(b)set encoder VP8E_SET_MAX_INTRA_BITRATE_PCT setting: %s value=%d",
                           vpx_codec_err_to_string(rc),
                           (int)rc_max_intra_target);
        }

#endif

        /*
        Codec control function to set max data rate for Inter frames.
        This value controls additional clamping on the maximum size of an inter frame. It is expressed as a percentage of the average per-frame bitrate, with the special (and default) value 0 meaning unlimited, or no additional clamping beyond the codec's built-in algorithm.
        For example, to allow no more than 4.5 frames worth of bitrate to an inter frame, set this to 450.
        Supported in codecs: VP9

        VP9E_SET_MAX_INTER_BITRATE_PCT
        */


        int cpu_used_value = vc->video_encoder_cpu_used;

        if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9) {
            if (cpu_used_value < -8) {
                cpu_used_value = -8;
            } else if (cpu_used_value > 8) {
                cpu_used_value = 8; // set to default (fastest) value
            }
        }

        rc = vpx_codec_control(&new_c, VP8E_SET_CPUUSED, cpu_used_value);

        if (rc != VPX_CODEC_OK) {
            LOGGER_ERROR(vc->log, "(b)Failed to set encoder VP8E_SET_CPUUSED setting: %s value=%d", vpx_codec_err_to_string(rc),
                         (int)cpu_used_value);
            vpx_codec_destroy(&new_c);
            return -1;
        } else {
            LOGGER_WARNING(vc->log, "(b)set encoder VP8E_SET_CPUUSED setting: %s value=%d", vpx_codec_err_to_string(rc),
                           (int)cpu_used_value);
        }

        vc->video_encoder_cpu_used = cpu_used_value;
        vc->video_encoder_cpu_used_prev = cpu_used_value;

#ifdef VIDEO_CODEC_ENCODER_USE_FRAGMENTS

        if (vc->video_encoder_coded_used != TOXAV_ENCODER_CODEC_USED_VP9) {
            rc = vpx_codec_control(&new_c, VP8E_SET_TOKEN_PARTITIONS, VIDEO_CODEC_FRAGMENT_VPX_NUMS);

            if (rc != VPX_CODEC_OK) {
                LOGGER_ERROR(vc->log, "Failed to set encoder token partitions: %s", vpx_codec_err_to_string(rc));
            }
        }

#endif

        if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9) {
            rc = vpx_codec_control(&new_c, VP9E_SET_TILE_COLUMNS, VIDEO__VP9E_SET_TILE_COLUMNS);

            if (rc != VPX_CODEC_OK) {
                LOGGER_ERROR(vc->log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
                vpx_codec_destroy(&new_c);
                return -1;
            }

            rc = vpx_codec_control(&new_c, VP9E_SET_TILE_ROWS, VIDEO__VP9E_SET_TILE_ROWS);

            if (rc != VPX_CODEC_OK) {
                LOGGER_ERROR(vc->log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
                vpx_codec_destroy(&new_c);
                return -1;
            }
        }

#if 0

        if (vc->video_encoder_coded_used == TOXAV_ENCODER_CODEC_USED_VP9) {
            if (1 == 2) {
                LOGGER_WARNING(vc->log, "setting VP9 lossless video quality: ON");

                rc = vpx_codec_control(&new_c, VP9E_SET_LOSSLESS, 1);

                if (rc != VPX_CODEC_OK) {
                    LOGGER_ERROR(vc->log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
                    vpx_codec_destroy(&new_c);
                    return -1;
                }
            } else {
                LOGGER_WARNING(vc->log, "setting VP9 lossless video quality: OFF");

                rc = vpx_codec_control(&new_c, VP9E_SET_LOSSLESS, 0);

                if (rc != VPX_CODEC_OK) {
                    LOGGER_ERROR(vc->log, "Failed to set encoder control setting: %s", vpx_codec_err_to_string(rc));
                    vpx_codec_destroy(&new_c);
                    return -1;
                }
            }
        }

#endif


        vpx_codec_destroy(vc->encoder);
        memcpy(vc->encoder, &new_c, sizeof(new_c));
    }

    return 0;
}

