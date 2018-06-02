#include "video.h"


// H264 settings -----------
#define x264_param_profile_str "high"
#define VIDEO_BITRATE_INITIAL_VALUE_H264 3000
#define VIDEO_MAX_KF_H264 52
#define VIDEO_BUF_FACTOR_H264 4
#define VIDEO_F_RATE_TOLERANCE_H264 1.2
#define VIDEO_BITRATE_FACTOR_H264 0.7
// H264 settings -----------


VCSession *vc_new_h264(Logger *log, ToxAV *av, uint32_t friend_number, toxav_video_receive_frame_cb *cb, void *cb_data,
                       VCSession *vc)
{

    // ENCODER -------
    x264_param_t param;

    if (x264_param_default_preset(&param, "ultrafast", "zerolatency") < 0) {
        // goto fail;
    }

    /* Configure non-default params */
    // param.i_bitdepth = 8;
    param.i_csp = X264_CSP_I420;
    param.i_width  = 1920;
    param.i_height = 1080;
    vc->h264_enc_width = param.i_width;
    vc->h264_enc_height = param.i_height;
    param.i_threads = 3;
    // param.b_open_gop = 20;
    param.i_keyint_max = VIDEO_MAX_KF_H264;
    // param.rc.i_rc_method = X264_RC_CRF; // X264_RC_ABR;
    // param.i_nal_hrd = X264_NAL_HRD_CBR;

    param.b_vfr_input = 1; /* VFR input.  If 1, use timebase and timestamps for ratecontrol purposes.
                            * If 0, use fps only. */
    param.i_timebase_num = 1;       // 1 ms = timebase units = (1/1000)s
    param.i_timebase_den = 1000;   // 1 ms = timebase units = (1/1000)s
    param.b_repeat_headers = 1;
    param.b_annexb = 1;

    param.rc.f_rate_tolerance = VIDEO_F_RATE_TOLERANCE_H264;
    param.rc.i_vbv_buffer_size = VIDEO_BITRATE_INITIAL_VALUE_H264 * VIDEO_BUF_FACTOR_H264;
    param.rc.i_vbv_max_bitrate = VIDEO_BITRATE_INITIAL_VALUE_H264 * 1;
    // param.rc.i_bitrate = VIDEO_BITRATE_INITIAL_VALUE_H264 * VIDEO_BITRATE_FACTOR_H264;

    vc->h264_enc_bitrate = VIDEO_BITRATE_INITIAL_VALUE_H264 * 1000;

    /* Apply profile restrictions. */
    if (x264_param_apply_profile(&param,
                                 x264_param_profile_str) < 0) { // "baseline", "main", "high", "high10", "high422", "high444"
        // goto fail;
    }

    if (x264_picture_alloc(&(vc->h264_in_pic), param.i_csp, param.i_width, param.i_height) < 0) {
        // goto fail;
    }

    // vc->h264_in_pic.img.plane[0] --> Y
    // vc->h264_in_pic.img.plane[1] --> U
    // vc->h264_in_pic.img.plane[2] --> V

    vc->h264_encoder = x264_encoder_open(&param);

//    goto good;

//fail:
//    vc->h264_encoder = NULL;

//good:

    // ENCODER -------


    // DECODER -------
    AVCodec *codec;
    vc->h264_decoder = NULL;

    avcodec_register_all();

    //codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    codec = avcodec_find_decoder_by_name("h264");


    if (!codec) {
        LOGGER_WARNING(log, "codec not found H264 on decoder");
    }

    vc->h264_decoder = avcodec_alloc_context3(codec);

    if (codec->capabilities & CODEC_CAP_TRUNCATED) {
        vc->h264_decoder->flags |= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */
    }

    vc->h264_decoder->refcounted_frames = 0;
    /*   When AVCodecContext.refcounted_frames is set to 0, the returned
    *             reference belongs to the decoder and is valid only until the
    *             next call to this function or until closing or flushing the
    *             decoder. The caller may not write to it.
    */

    if (avcodec_open2(vc->h264_decoder, codec, NULL) < 0) {
        LOGGER_WARNING(log, "could not open codec H264 on decoder");
    }

    vc->h264_decoder->refcounted_frames = 0;
    /*   When AVCodecContext.refcounted_frames is set to 0, the returned
    *             reference belongs to the decoder and is valid only until the
    *             next call to this function or until closing or flushing the
    *             decoder. The caller may not write to it.
    */

    // DECODER -------

    return vc;
}

void vc_kill_h264(VCSession *vc)
{
    x264_encoder_close(vc->h264_encoder);
    x264_picture_clean(&(vc->h264_in_pic));
    avcodec_free_context(&vc->h264_decoder);
}

bool vc_encode_frame_h264(ToxAV *av, uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t *y,
                            const uint8_t *u, const uint8_t *v, TOXAV_ERR_SEND_FRAME *error)
{
    
}
int vc_decode_frame_h264(VCSession *vc, struct RTPHeader* header_v3, uint8_t *data, uint32_t data_len)
{
    // LOGGER_ERROR(vc->log, "DEC:H264------------");

    /*
        For decoding, call avcodec_send_packet() to give the decoder raw
            compressed data in an AVPacket.


            For decoding, call avcodec_receive_frame(). On success, it will return
            an AVFrame containing uncompressed audio or video data.


        *   Repeat this call until it returns AVERROR(EAGAIN) or an error. The
        *   AVERROR(EAGAIN) return value means that new input data is required to
        *   return new output. In this case, continue with sending input. For each
        *   input frame/packet, the codec will typically return 1 output frame/packet,
        *   but it can also be 0 or more than 1.

        */

    AVPacket *compr_data;
    compr_data = av_packet_alloc();

#if 0
    compr_data->pts = AV_NOPTS_VALUE;
    compr_data->dts = AV_NOPTS_VALUE;

    compr_data->duration = 0;
    compr_data->post = -1;
#endif

    // HINT: dirty hack to add FF_INPUT_BUFFER_PADDING_SIZE bytes!!
    uint8_t *tmp_buf = calloc(1, data_len + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(tmp_buf, data, data_len);

    compr_data->data = tmp_buf; // p->data;
    compr_data->size = (int)data_len; // hmm, "int" again

    avcodec_send_packet(vc->h264_decoder, compr_data);


    int ret_ = 0;

    while (ret_ >= 0) {

        AVFrame *frame = av_frame_alloc();
        ret_ = avcodec_receive_frame(vc->h264_decoder, frame);

        // LOGGER_ERROR(vc->log, "H264:decoder:ret_=%d\n", (int)ret_);


        if (ret_ == AVERROR(EAGAIN) || ret_ == AVERROR_EOF) {
            // error
            break;
        } else if (ret_ < 0) {
            // Error during decoding
            break;
        } else if (ret_ == 0) {

            // LOGGER_ERROR(vc->log, "H264:decoder:fnum=%d\n", (int)vc->h264_decoder->frame_number);
            // LOGGER_ERROR(vc->log, "H264:decoder:linesize=%d\n", (int)frame->linesize[0]);
            // LOGGER_ERROR(vc->log, "H264:decoder:w=%d\n", (int)frame->width);
            // LOGGER_ERROR(vc->log, "H264:decoder:h=%d\n", (int)frame->height);

            vc->vcb.first(vc->av, vc->friend_number, frame->width, frame->height,
                            (const uint8_t *)frame->data[0],
                            (const uint8_t *)frame->data[1],
                            (const uint8_t *)frame->data[2],
                            frame->linesize[0], frame->linesize[1],
                            frame->linesize[2], vc->vcb.second);

        } else {
            // some other error
        }

        av_frame_free(&frame);
    }

    av_packet_free(&compr_data);
    free(tmp_buf);
    // HINT: H264 decode ----------------
}

int vc_reconfigure_encoder_h264(Logger *log, VCSession *vc, uint32_t bit_rate, uint16_t width, uint16_t height,
                                int16_t kf_max_dist)
{
    if (!vc) {
        return -1;
    }

    if ((vc->h264_enc_width != width) ||
            (vc->h264_enc_height != height) ||
            (vc->h264_enc_bitrate != bit_rate)
       ) {
        // input image size changed

        x264_param_t param;

        if (x264_param_default_preset(&param, "ultrafast", "zerolatency") < 0) {
            // goto fail;
        }


        /* Configure non-default params */
        // param.i_bitdepth = 8;
        param.i_csp = X264_CSP_I420;
        param.i_width  = width;
        param.i_height = height;
        vc->h264_enc_width = param.i_width;
        vc->h264_enc_height = param.i_height;
        param.i_threads = 3;
        // param.b_open_gop = 20;
        param.i_keyint_max = VIDEO_MAX_KF_H264;
        // param.rc.i_rc_method = X264_RC_ABR;

        param.b_vfr_input = 1; /* VFR input.  If 1, use timebase and timestamps for ratecontrol purposes.
                            * If 0, use fps only. */
        param.i_timebase_num = 1;       // 1 ms = timebase units = (1/1000)s
        param.i_timebase_den = 1000;   // 1 ms = timebase units = (1/1000)s
        param.b_repeat_headers = 1;
        param.b_annexb = 1;

        param.rc.f_rate_tolerance = VIDEO_F_RATE_TOLERANCE_H264;
        param.rc.i_vbv_buffer_size = (bit_rate / 1000) * VIDEO_BUF_FACTOR_H264;
        param.rc.i_vbv_max_bitrate = (bit_rate / 1000) * 1;

        // param.rc.i_bitrate = (bit_rate / 1000) * VIDEO_BITRATE_FACTOR_H264;
        vc->h264_enc_bitrate = bit_rate;

        /* Apply profile restrictions. */
        if (x264_param_apply_profile(&param,
                                     x264_param_profile_str) < 0) { // "baseline", "main", "high", "high10", "high422", "high444"
            // goto fail;
        }

        LOGGER_DEBUG(log, "H264: reconfigure encoder:001: w:%d h:%d w_new:%d h_new:%d BR:%d\n",
                     vc->h264_enc_width,
                     vc->h264_enc_height,
                     width,
                     height,
                     (int)bit_rate);

        // free old stuff ---------
        x264_encoder_close(vc->h264_encoder);
        x264_picture_clean(&(vc->h264_in_pic));
        // free old stuff ---------

        LOGGER_DEBUG(log, "H264: reconfigure encoder:002\n");

        // alloc with new values -------
        if (x264_picture_alloc(&(vc->h264_in_pic), param.i_csp, param.i_width, param.i_height) < 0) {
            // goto fail;
        }

        LOGGER_DEBUG(log, "H264: reconfigure encoder:003\n");

        vc->h264_encoder = x264_encoder_open(&param);
        // alloc with new values -------


        LOGGER_DEBUG(log, "H264: reconfigure encoder:004\n");

    }

    return 0;
}

int vc_reconfigure_encoder_bitrate_only_h264(VCSession *vc, uint32_t bit_rate)
{
    if (!vc) {
        return -1;
    }

    if (vc->h264_enc_bitrate != bit_rate) {

        x264_param_t param;

        if (x264_param_default_preset(&param, "ultrafast", "zerolatency") < 0) {
            // goto fail;
        }

        /* Configure non-default params */
        // param.i_bitdepth = 8;
        param.i_csp = X264_CSP_I420;
        param.i_width  = vc->h264_enc_width;
        param.i_height = vc->h264_enc_height;
        param.i_threads = 3;
        // param.b_open_gop = 20;
        param.i_keyint_max = VIDEO_MAX_KF_H264;
        // param.rc.i_rc_method = X264_RC_ABR;

        param.b_vfr_input = 1; /* VFR input.  If 1, use timebase and timestamps for ratecontrol purposes.
                            * If 0, use fps only. */
        param.i_timebase_num = 1;       // 1 ms = timebase units = (1/1000)s
        param.i_timebase_den = 1000;   // 1 ms = timebase units = (1/1000)s
        param.b_repeat_headers = 1;
        param.b_annexb = 1;

        param.rc.f_rate_tolerance = VIDEO_F_RATE_TOLERANCE_H264;
        param.rc.i_vbv_buffer_size = (bit_rate / 1000) * VIDEO_BUF_FACTOR_H264;
        param.rc.i_vbv_max_bitrate = (bit_rate / 1000) * 1;

        // param.rc.i_bitrate = (bit_rate / 1000) * VIDEO_BITRATE_FACTOR_H264;
        vc->h264_enc_bitrate = bit_rate;

        /* Apply profile restrictions. */
        if (x264_param_apply_profile(&param,
                                     x264_param_profile_str) < 0) { // "baseline", "main", "high", "high10", "high422", "high444"
            // goto fail;
        }

        // free old stuff ---------
        x264_encoder_close(vc->h264_encoder);
        x264_picture_clean(&(vc->h264_in_pic));
        // free old stuff ---------

        // alloc with new values -------
        if (x264_picture_alloc(&(vc->h264_in_pic), param.i_csp, param.i_width, param.i_height) < 0) {
            // goto fail;
        }

        vc->h264_encoder = x264_encoder_open(&param);
        // alloc with new values -------

    }

    return 0;
}

