/*
 * Copyright © 2018 zoff@zoff.cc and mail@strfry.org
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



/*
  Soft deadline the decoder should attempt to meet, in "us" (microseconds). Set to zero for unlimited.
  By convention, the value 1 is used to mean "return as fast as possible."
*/
// TODO: don't hardcode this, let the application choose it
#define WANTED_MAX_DECODER_FPS (20)
#define MAX_DECODE_TIME_US (1000000 / WANTED_MAX_DECODER_FPS) // to allow x fps
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

struct vpx_frame_user_data {
    uint64_t record_timestamp;
};





// ----------- VPX  -----------
VCSession *vc_new_vpx(Logger *log, ToxAV *av, uint32_t friend_number, toxav_video_receive_frame_cb *cb, void *cb_data,
                      VCSession *vc);

int vc_reconfigure_encoder_vpx(Logger *log, VCSession *vc, uint32_t bit_rate, uint16_t width, uint16_t height,
                               int16_t kf_max_dist);

void decode_frame_vpx(VCSession *vc, Messenger *m, uint8_t skip_video_flag, uint64_t *a_r_timestamp,
                      uint64_t *a_l_timestamp,
                      uint64_t *v_r_timestamp, uint64_t *v_l_timestamp,
                      const struct RTPHeader *header_v3,
                      struct RTPMessage *p, vpx_codec_err_t rc,
                      uint32_t full_data_len,
                      uint8_t *ret_value);

uint32_t encode_frame_vpx(ToxAV *av, uint32_t friend_number, uint16_t width, uint16_t height,
                          const uint8_t *y,
                          const uint8_t *u, const uint8_t *v, ToxAVCall *call,
                          uint64_t *video_frame_record_timestamp,
                          int vpx_encode_flags,
                          TOXAV_ERR_SEND_FRAME *error);

void vc_kill_vpx(VCSession *vc);


// ----------- H264 -----------
VCSession *vc_new_h264(Logger *log, ToxAV *av, uint32_t friend_number, toxav_video_receive_frame_cb *cb, void *cb_data,
                       VCSession *vc);

int vc_reconfigure_encoder_h264(Logger *log, VCSession *vc, uint32_t bit_rate, uint16_t width, uint16_t height,
                                int16_t kf_max_dist);

void decode_frame_h264(VCSession *vc, Messenger *m, uint8_t skip_video_flag, uint64_t *a_r_timestamp,
                       uint64_t *a_l_timestamp,
                       uint64_t *v_r_timestamp, uint64_t *v_l_timestamp,
                       const struct RTPHeader *header_v3,
                       struct RTPMessage *p, vpx_codec_err_t rc,
                       uint32_t full_data_len,
                       uint8_t *ret_value);

void vc_kill_h264(VCSession *vc);



// ----------- H264 OMX RaspberryPi -----------




