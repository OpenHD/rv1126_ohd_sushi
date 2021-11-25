// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include "buffer.h"

#include "media_type.h"
#include "mpp_encoder.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_VENC

#define ENCODER_CFG_INVALID 0x7FFFFFFF
#define ENCODER_CFG_CHECK(VALUE, MIN, MAX, DEF_VALUE, TAG)                     \
  if (!VALUE) {                                                                \
    if (DEF_VALUE != ENCODER_CFG_INVALID) {                                    \
      RKMEDIA_LOGI("MPP Encoder: %s use default value:%d\n", TAG, DEF_VALUE);  \
      VALUE = DEF_VALUE;                                                       \
    } else {                                                                   \
      RKMEDIA_LOGE("MPP Encoder: %s invalid value(%d)\n", TAG, VALUE);         \
      return false;                                                            \
    }                                                                          \
  } else if ((VALUE > MAX) || (VALUE < MIN)) {                                 \
    RKMEDIA_LOGE("MPP Encoder: %s invalid value(%d)\n", TAG, VALUE);           \
    return false;                                                              \
  }

namespace easymedia {

#if 0
static float smart_enc_mode_get_bps_factor(int bps, int w, int h) {
  float den = 1.0;
  //Reference 1080p resolution:
  //5Mb:    [bpsMax / 4,   bpsMax]
  //4Mb:    [bpsMax / 3,   bpsMax]
  //3Mb:    [bpsMax / 2,   bpsMax]
  //Others: [bpsMax * 0.8, bpsMax]
  int relatively_bps = (int)(bps * (2073600.0) / (w * h));
  if (relatively_bps >= 5242880) //5Mb
    den = 0.25; //1/4
  else if (relatively_bps >= 4194304) //4Mb
    den = 0.333; //1/3
  else if (relatively_bps >= 3145728) //3Mb
    den = 0.5; //1/2
  else
    den = 0.8;

  return den;
}
#endif

#define MPP_MIN_BPS_VALUE 2000      // 2Kb
#define MPP_MAX_BPS_VALUE 100000000 // 100Mb

// According to bps_max, automatically calculate bps_target and bps_min.
static int CalcMppBpsWithMax(MppEncRcMode rc_mode, int &bps_max, int &bps_min,
                             int &bps_target) {
  if ((rc_mode != MPP_ENC_RC_MODE_FIXQP) &&
      ((bps_max > MPP_MAX_BPS_VALUE) || (bps_max < MPP_MIN_BPS_VALUE))) {
    RKMEDIA_LOGE("MPP Encoder: bps max <%d> is not valid!\n", bps_max);
    return -1;
  }
  RKMEDIA_LOGI("MPP Encoder: automatically calculate bsp with bps_max\n");
  switch (rc_mode) {
  case MPP_ENC_RC_MODE_CBR:
    // constant bitrate has very small bps range of 1/10 bps
    bps_target = bps_max * 9 / 10;
    bps_min = bps_max * 8 / 10;
    break;
  case MPP_ENC_RC_MODE_VBR:
    // variable bitrate has large bps range
    bps_target = bps_max * 9 / 10;
    bps_min = bps_max * 1 / 4;
    break;
  case MPP_ENC_RC_MODE_FIXQP:
    bps_target = bps_min = bps_max;
    return 0; // FIXQP mode bps is invalid!
  case MPP_ENC_RC_MODE_AVBR:
    bps_target = bps_max * 9 / 10;
    bps_min = bps_max * 1 / 4;
    break;
  default:
    RKMEDIA_LOGE("MPP Encoder: rc_mode=%d is invalid!\n", rc_mode);
    return -1;
  }

  if (bps_min < MPP_MIN_BPS_VALUE)
    bps_min = MPP_MIN_BPS_VALUE;
  if (bps_target < bps_min)
    bps_target = (bps_min + bps_max) / 2;

  return 0;
}

// According to bps_target, automatically calculate bps_max and bps_min.
static int CalcMppBpsWithTarget(MppEncRcMode rc_mode, int &bps_max,
                                int &bps_min, int &bps_target) {
  if ((rc_mode != MPP_ENC_RC_MODE_FIXQP) &&
      ((bps_target > MPP_MAX_BPS_VALUE) || (bps_target < MPP_MIN_BPS_VALUE))) {
    RKMEDIA_LOGE("MPP Encoder: bps <%d> is not valid!\n", bps_target);
    return -1;
  }
  RKMEDIA_LOGI("MPP Encoder: automatically calculate bsp with bps_target\n");
  switch (rc_mode) {
  case MPP_ENC_RC_MODE_CBR:
    // constant bitrate has very small bps range of 1/10 bps
    bps_max = bps_target * 10 / 9;
    bps_min = bps_target * 8 / 9;
    break;
  case MPP_ENC_RC_MODE_VBR:
    // variable bitrate has large bps range
    bps_max = bps_target * 10 / 9;
    bps_min = bps_target * 10 / 36; // bsp_max * 1/4
    break;
  case MPP_ENC_RC_MODE_FIXQP:
    bps_target = bps_min = bps_max;
    return 0; // FIXQP mode bps is invalid!
  case MPP_ENC_RC_MODE_AVBR:
    // variable bitrate has large bps range
    bps_max = bps_target * 10 / 9;
    bps_min = bps_target * 10 / 36; // bsp_max * 1/4
    break;
  default:
    RKMEDIA_LOGE("MPP Encoder: rc_mode=%d is invalid!\n", rc_mode);
    return -1;
  }

  if (bps_min < MPP_MIN_BPS_VALUE)
    bps_min = MPP_MIN_BPS_VALUE;
  if (bps_max > MPP_MAX_BPS_VALUE)
    bps_max = MPP_MAX_BPS_VALUE;

  return 0;
}

class MPPConfig {
public:
  MPPConfig();
  virtual ~MPPConfig();
  virtual bool InitConfig(MPPEncoder &mpp_enc, MediaConfig &cfg) = 0;
  virtual bool CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                 std::shared_ptr<ParameterBuffer> val) = 0;
  MppEncCfg enc_cfg;
};

MPPConfig::MPPConfig() {
  enc_cfg = NULL;
  if (mpp_enc_cfg_init(&enc_cfg)) {
    RKMEDIA_LOGE("MPP Encoder: MPPConfig: cfg init failed!");
    enc_cfg = NULL;
    return;
  }
  RKMEDIA_LOGI("MPP Encoder: MPPConfig: cfg init sucess!\n");
}

MPPConfig::~MPPConfig() {
  if (enc_cfg) {
    mpp_enc_cfg_deinit(enc_cfg);
    RKMEDIA_LOGI("MPP Encoder: MPPConfig: cfg deinit done!\n");
  }
}

class MPPMJPEGConfig : public MPPConfig {
public:
  MPPMJPEGConfig() {}
  ~MPPMJPEGConfig() = default;
  virtual bool InitConfig(MPPEncoder &mpp_enc, MediaConfig &cfg) override;
  virtual bool CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                 std::shared_ptr<ParameterBuffer> val) override;
};

bool MPPMJPEGConfig::InitConfig(MPPEncoder &mpp_enc, MediaConfig &cfg) {
  // for jpeg/mjpeg common.
  ImageConfig img_cfg;
  ImageInfo img_info;
  ImageRect rect_info;
  // for mjpeg.
  VideoConfig vid_cfg;
  int bps_max = 0, bps_min = 0, bps_target = 0;
  int fps_in_num = 0, fps_in_den = 0;
  int fps_out_num = 0, fps_out_den = 0;
  MppEncRcMode rc_mode = MPP_ENC_RC_MODE_FIXQP;
  int ret = 0;

  if (!enc_cfg) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: mpp enc cfg is null!\n");
    return false;
  }

  if (cfg.type == Type::Image) {
    RKMEDIA_LOGI("MPP Encoder[JPEG]: config for JPEG...\n");
    vid_cfg = cfg.vid_cfg;
    img_cfg = cfg.img_cfg;
    img_info = cfg.img_cfg.image_info;
    rect_info = cfg.img_cfg.rect_info;
    ENCODER_CFG_CHECK(img_cfg.qfactor, 1, 99, 70, "qfactor");
    ENCODER_CFG_CHECK(img_cfg.dcf, 0, 1, 0, "dcf");
    ENCODER_CFG_CHECK(img_cfg.mpf_cnt, 0, 2, 0, "mpf_cnt");
  } else if (cfg.type == Type::Video) {
    RKMEDIA_LOGI("MPP Encoder[JPEG]: config for MJPEG...\n");
    vid_cfg = cfg.vid_cfg;
    img_cfg = vid_cfg.image_cfg;
    img_info = cfg.img_cfg.image_info;
    rect_info = cfg.img_cfg.rect_info;

    // Encoder param check.
    ENCODER_CFG_CHECK(vid_cfg.frame_rate, 1, 60, ENCODER_CFG_INVALID, "fpsNum");
    ENCODER_CFG_CHECK(vid_cfg.frame_rate_den, 1, 16, 1, "fpsDen");
    ENCODER_CFG_CHECK(vid_cfg.frame_in_rate, 1, 60, ENCODER_CFG_INVALID,
                      "fpsInNum");
    ENCODER_CFG_CHECK(vid_cfg.frame_in_rate_den, 1, 16, 1, "fpsInDen");

    if (!vid_cfg.rc_mode) {
      RKMEDIA_LOGI("MPP Encoder[JPEG]: rcMode use defalut value: vbr\n");
      vid_cfg.rc_mode = KEY_VBR;
    }

    rc_mode = GetMPPRCMode(vid_cfg.rc_mode);
    if (rc_mode == MPP_ENC_RC_MODE_BUTT) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: Invalid rc mode %s\n", vid_cfg.rc_mode);
      return false;
    }
    bps_max = vid_cfg.bit_rate_max;
    bps_min = vid_cfg.bit_rate_min;
    bps_target = vid_cfg.bit_rate;
    fps_in_num = vid_cfg.frame_in_rate;
    fps_in_den = vid_cfg.frame_in_rate_den;
    fps_out_num = vid_cfg.frame_rate;
    fps_out_den = vid_cfg.frame_rate_den;

    if (CalcMppBpsWithMax(rc_mode, bps_max, bps_min, bps_target)) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: Invalid bps:[%d, %d, %d]\n",
                   vid_cfg.bit_rate_min, vid_cfg.bit_rate,
                   vid_cfg.bit_rate_max);
      return false;
    }
  } else {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: mpp enc cfg type(%d) is invalid!\n",
                 (int)cfg.type);
    return false;
  }
  if (img_cfg.dcf) {
    mpp_enc.thumbnail_width[0] = 160;
    mpp_enc.thumbnail_height[0] = 120;
    mpp_enc.thumbnail_type[0] = THUMBNAIL_TYPE_APP1;
  }
  if (img_cfg.mpf_cnt > 0) {
    mpp_enc.thumbnail_width[1] = img_cfg.mpfw[0];
    mpp_enc.thumbnail_height[1] = img_cfg.mpfh[0];
    mpp_enc.thumbnail_type[1] = THUMBNAIL_TYPE_APP2;
  }
  if (img_cfg.mpf_cnt > 1) {
    mpp_enc.thumbnail_width[2] = img_cfg.mpfw[1];
    mpp_enc.thumbnail_height[2] = img_cfg.mpfh[1];
    mpp_enc.thumbnail_type[2] = THUMBNAIL_TYPE_APP2;
  }

  ENCODER_CFG_CHECK(rect_info.x, 0, img_cfg.image_info.width, 0, "rect_x");
  ENCODER_CFG_CHECK(rect_info.y, 0, img_cfg.image_info.height, 0, "rect_y");
  ENCODER_CFG_CHECK(rect_info.w, 0, (img_cfg.image_info.width - rect_info.x),
                    (img_cfg.image_info.width - rect_info.x), "rect_w");
  ENCODER_CFG_CHECK(rect_info.h, 0, (img_cfg.image_info.height - rect_info.y),
                    (img_cfg.image_info.height - rect_info.y), "rect_h");

  MppEncRotationCfg rotation = MPP_ENC_ROT_0;
  switch (vid_cfg.rotation) {
  case 0:
    rotation = MPP_ENC_ROT_0;
    RKMEDIA_LOGI("MPP Encoder[JPEG]: rotaion = 0\n");
    break;
  case 90:
    RKMEDIA_LOGI("MPP Encoder[JPEG]: rotaion = 90\n");
    rotation = MPP_ENC_ROT_90;
    break;
  case 270:
    RKMEDIA_LOGI("MPP Encoder[JPEG]: rotaion = 270\n");
    rotation = MPP_ENC_ROT_270;
    break;
  default:
    RKMEDIA_LOGE("MPP Encoder[JPEG]: rotaion(%d) is invalid!\n",
                 vid_cfg.rotation);
    return false;
  }

  ENCODER_CFG_CHECK(img_info.width, 1, 8192, ENCODER_CFG_INVALID, "width");
  ENCODER_CFG_CHECK(img_info.height, 1, 8192, ENCODER_CFG_INVALID, "height");
  ENCODER_CFG_CHECK(img_info.vir_width, 1, 8192, img_info.width, "virWidth");
  ENCODER_CFG_CHECK(img_info.vir_height, 1, 8192, img_info.height, "virHeight");
  MppFrameFormat pic_type = ConvertToMppPixFmt(img_info.pix_fmt);
  if (pic_type == -1) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: invalid pixel format!\n");
    return false;
  }

  MppPollType timeout = MPP_POLL_BLOCK;
  RKMEDIA_LOGI("MPP Encoder[JPEG]: Set output block mode.\n");
  ret = mpp_enc.EncodeControl(MPP_SET_OUTPUT_TIMEOUT, &timeout);
  if (ret != 0) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: set output block failed! ret %d\n", ret);
    return false;
  }
  RKMEDIA_LOGI("MPP Encoder[JPEG]: Set input block mode.\n");
  ret = mpp_enc.EncodeControl(MPP_SET_INPUT_TIMEOUT, &timeout);
  if (ret != 0) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: set input block failed! ret %d\n", ret);
    return false;
  }

  int line_size = img_info.vir_width;
  if (pic_type == MPP_FMT_YUV422_YUYV || pic_type == MPP_FMT_YUV422_UYVY)
    line_size *= 2;

  // precfg set.
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:width", rect_info.w);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:height", rect_info.h);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride", line_size);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:ver_stride", img_info.vir_height);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:format", pic_type);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:rotation", rotation);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", rc_mode); // default fixqp.
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "jpeg:q_factor", img_cfg.qfactor);
  if (cfg.type == Type::Video) {
    RKMEDIA_LOGI("MPP Encoder[JPEG]: set rc cfg for MJPEG...\n");
    // mjpeg rc confg set.
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_flex", 0);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_num", fps_in_num);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_denorm", fps_in_den);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_flex", 0);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_num", fps_out_num);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_denorm", fps_out_den);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", 1); // no use, default 300.
    RKMEDIA_LOGI(
        "MPP Encoder[JPEG]: bps:[%d, %d, %d], fps_in:%d/%d, fps_out:%d/%d, "
        "gop:1\n",
        bps_min, bps_target, bps_max, fps_in_num, fps_in_den, fps_out_num,
        fps_out_den);
    // for mjpeg only
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "jpeg:qf_max", 99);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "jpeg:qf_min", 10);
  }

  if (ret) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: cfg set s32 failed ret %d\n", ret);
    return false;
  }

  ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: encoder set cfg failed! ret=%d\n", ret);
    return false;
  }

  RKMEDIA_LOGI("MPP Encoder[JPEG]: w x h(%d[%d] x %d[%d]), qfactor:%d\n",
               img_info.width, line_size, img_info.height, img_info.vir_height,
               img_cfg.qfactor);

  mpp_enc.GetConfig() = cfg;
  return true;
}

bool MPPMJPEGConfig::CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                       std::shared_ptr<ParameterBuffer> val) {
  ImageConfig &iconfig = mpp_enc.GetConfig().img_cfg;
  VideoConfig &vconfig = mpp_enc.GetConfig().vid_cfg;
  int ret = 0;

  if (!enc_cfg) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: mpp enc cfg is null!\n");
    return false;
  }

  if (change & VideoEncoder::kFrameRateChange) {
    uint8_t *values = (uint8_t *)val->GetPtr();
    if (val->GetSize() < 4) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: fps should be uint8_t array[4]:"
                   "{inFpsNum, inFpsDen, outFpsNum, outFpsDen}");
      return false;
    }
    uint8_t in_fps_num = values[0];
    uint8_t in_fps_den = values[1];
    uint8_t out_fps_num = values[2];
    uint8_t out_fps_den = values[3];

    if (!out_fps_num || !out_fps_den || (out_fps_num > 60)) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: invalid out fps: [%d/%d]\n", out_fps_num,
                   out_fps_den);
      return false;
    }

    if (in_fps_num && in_fps_den) {
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_num", in_fps_num);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_denorm", in_fps_den);
    }
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_num", out_fps_num);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_denorm", out_fps_den);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: fps: cfg set s32 failed ret %d\n", ret);
      return false;
    }
    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: change fps cfg failed!\n");
      return false;
    }
    if (in_fps_num && in_fps_den)
      RKMEDIA_LOGI("MPP Encoder[JPEG]: new fps: [%d/%d]->[%d/%d]\n", in_fps_num,
                   in_fps_den, out_fps_num, out_fps_den);
    else
      RKMEDIA_LOGI("MPP Encoder[JPEG]: new out fps: [%d/%d]\n", out_fps_num,
                   out_fps_den);

    vconfig.frame_rate = out_fps_num;
  } else if (change & VideoEncoder::kBitRateChange) {
    int *values = (int *)val->GetPtr();
    if (val->GetSize() < 3 * sizeof(int)) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: fps should be int array[3]:"
                   "{bpsMin, bpsTarget, bpsMax}");
      return false;
    }
    int bps_min = values[0];
    int bps_target = values[1];
    int bps_max = values[2];
    MppEncRcMode rc_mode = GetMPPRCMode(vconfig.rc_mode);
    if (rc_mode == MPP_ENC_RC_MODE_BUTT) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: bps: invalid rc mode %s\n",
                   vconfig.rc_mode);
      return false;
    } else if (rc_mode == MPP_ENC_RC_MODE_FIXQP) {
      RKMEDIA_LOGW("MPP Encoder[JPEG]: bps: FIXQP no need bps!\n");
      return true;
    }

    if (CalcMppBpsWithMax(rc_mode, bps_max, bps_min, bps_target)) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: Invalid bps:[%d, %d, %d]\n", bps_min,
                   bps_target, bps_max);
      return false;
    }

    RKMEDIA_LOGI("MPP Encoder[JPEG]: new bps:[%d, %d, %d]\n", bps_min,
                 bps_target, bps_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: bps: cfg set s32 failed ret %d\n", ret);
      return false;
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: change bps cfg failed!\n");
      return false;
    }
    // save new value to config.
    vconfig.bit_rate = bps_target;
    vconfig.bit_rate_max = bps_max;
    vconfig.bit_rate_min = bps_min;
  } else if (change & VideoEncoder::kRcModeChange) {
    char *new_mode = (char *)val->GetPtr();
    RKMEDIA_LOGI("MPP Encoder[JPEG]: new rc_mode:%s\n", new_mode);
    MppEncRcMode rc_mode = GetMPPRCMode(new_mode);
    if (rc_mode == MPP_ENC_RC_MODE_BUTT) {
      RKMEDIA_LOGE(
          "MPP Encoder[JPEG]: rc_mode is invalid! should be cbr/vbr.\n");
      return false;
    }

    // Recalculate bps
    int bps_max = vconfig.bit_rate_max;
    int bps_min = bps_max;
    int bps_target = bps_max;
    if (CalcMppBpsWithMax(rc_mode, bps_max, bps_min, bps_target) < 0)
      return false;

    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", rc_mode);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: rc mode: cfg set s32 failed ret %d\n",
                   ret);
      return false;
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: change rc_mode cfg failed!\n");
      return false;
    }
    // save new value to encoder->vconfig.
    vconfig.rc_mode = ConvertRcMode(new_mode);
  } else if (change & VideoEncoder::kQPChange) {
    int qfactor = val->GetValue();
    RKMEDIA_LOGD("MPP Encoder[JPEG]: new qfactor:%d\n", qfactor);
    ENCODER_CFG_CHECK(qfactor, 1, 99, ENCODER_CFG_INVALID, "JPEG:qfactor");
    ret = mpp_enc_cfg_set_s32(enc_cfg, "jpeg:q_factor", qfactor);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: cfg set s32 failed! ret=%d\n", ret);
      return false;
    }

    ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: qfactor: set cfg failed! ret=%d\n", ret);
      return false;
    }
    iconfig.qfactor = qfactor;
  } else if (change & VideoEncoder::kResolutionChange) {
    if (val->GetSize() < sizeof(VideoResolutionCfg)) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: Incomplete Resolution params\n");
      return false;
    }
    VideoResolutionCfg *vid_cfg = (VideoResolutionCfg *)val->GetPtr();
    RKMEDIA_LOGD("MPP Encoder[JPEG]: width = %d, height = %d, vwidth = %d, "
                 "vheight = %d.\n",
                 vid_cfg->width, vid_cfg->height, vid_cfg->vir_width,
                 vid_cfg->vir_height);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:width", vid_cfg->w);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:height", vid_cfg->h);
    MppFrameFormat pic_type = ConvertToMppPixFmt(iconfig.image_info.pix_fmt);
    int line_size = vid_cfg->vir_width;
    if (pic_type == MPP_FMT_YUV422_YUYV || pic_type == MPP_FMT_YUV422_UYVY)
      line_size = vid_cfg->vir_width * 2;
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride", line_size);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:ver_stride", vid_cfg->vir_height);
    ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: set resolution cfg failed ret %d\n",
                   ret);
      return false;
    }
    iconfig.image_info.width = vid_cfg->width;
    iconfig.image_info.height = vid_cfg->height;
    iconfig.image_info.vir_width = vid_cfg->vir_width;
    iconfig.image_info.vir_height = vid_cfg->vir_height;
    iconfig.rect_info.x = vid_cfg->x;
    iconfig.rect_info.y = vid_cfg->y;
    iconfig.rect_info.w = vid_cfg->w;
    iconfig.rect_info.h = vid_cfg->h;
  } else if (change & VideoEncoder::kRotationChange) {
    // unsupport return
    RKMEDIA_LOGE("MPP Encoder[JPEG]: Unsupport set rotation dynamically\n");
    return false;
    if (val->GetSize() < sizeof(int)) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: Incomplete Rotation params\n");
      return false;
    }
    int *rotation = (int *)val->GetPtr();
    MppEncRotationCfg rotation_enum = MPP_ENC_ROT_0;
    switch (*rotation) {
    case 0:
      rotation_enum = MPP_ENC_ROT_0;
      break;
    case 90:
      rotation_enum = MPP_ENC_ROT_90;
      break;
    case 270:
      rotation_enum = MPP_ENC_ROT_270;
      break;
    default:
      RKMEDIA_LOGE("MPP Encoder[JPEG]: set rotaion(%d) is invalid!\n",
                   *rotation);
      return false;
    }
    RKMEDIA_LOGI("MPP Encoder[JPEG]: rotation set(%d), old rotation(%d).\n",
                 *rotation, vconfig.rotation);
    if (*rotation == vconfig.rotation) {
      return true;
    }
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:rotation", rotation_enum);
    ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: set rotation cfg failed ret %d\n", ret);
      return false;
    }
    vconfig.rotation = *rotation;
  }
#ifdef RGA_OSD_ENABLE
  else if (change & VideoEncoder::kOSDDataChange) {
    // type: OsdRegionData*
    RKMEDIA_LOGD("MPP Encoder[JPEG]: config osd regions\n");
    if (val->GetSize() < sizeof(OsdRegionData)) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: invalid osd data\n");
      return false;
    }
    if (!val->GetPtr()) {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: osd data is nullptr\n");
      return false;
    }
    OsdRegionData *newRegion = (OsdRegionData *)val->GetPtr();
    if (newRegion->region_type == REGION_TYPE_OVERLAY_EX ||
        newRegion->region_type == REGION_TYPE_COVER_EX) {
      if (mpp_enc.RgaOsdRegionSet(newRegion, vconfig))
        return false;
    } else {
      RKMEDIA_LOGE("MPP Encoder[JPEG]: osd type not support!\n");
      return false;
    }
  }
#endif // RGA_OSD_ENABLE
  else {
    RKMEDIA_LOGI("MPP Encoder[JPEG]: Unsupport request change 0x%08x!\n",
                 change);
    return false;
  }

  return true;
}

class MPPCommonConfig : public MPPConfig {
public:
  MPPCommonConfig(MppCodingType type) : code_type(type) {}
  ~MPPCommonConfig() = default;
  virtual bool InitConfig(MPPEncoder &mpp_enc, MediaConfig &cfg) override;
  virtual bool CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                 std::shared_ptr<ParameterBuffer> val) override;

private:
  MppCodingType code_type;
};

static int CalcQpWithRcQuality(const char *level, VideoEncoderQp &qps) {
  VideoEncoderQp qp_table[7];
  int RcQulaitQpArray[7][6] = {
      {-1, 6, 20, 51, 24, 51}, // Highest
      {-1, 6, 24, 51, 24, 51}, // Higher
      {-1, 6, 26, 51, 24, 51}, // High
      {-1, 6, 29, 51, 24, 51}, // Medium
      {-1, 6, 30, 51, 24, 51}, // Low
      {-1, 6, 35, 51, 24, 51}, // Lower
      {-1, 6, 38, 51, 24, 51}  // Lowest
  };

  memset(qp_table, 0, 7 * sizeof(VideoEncoderQp));
  for (int i = 0; i < 7; i++) {
    qp_table[i].qp_init = RcQulaitQpArray[i][0];
    qp_table[i].qp_step = RcQulaitQpArray[i][1];
    qp_table[i].qp_min = RcQulaitQpArray[i][2];
    qp_table[i].qp_max = RcQulaitQpArray[i][3];
    qp_table[i].qp_min_i = RcQulaitQpArray[i][4];
    qp_table[i].qp_max_i = RcQulaitQpArray[i][5];
  }

  // Read From qp cfg file.
  // To Do...

  if (!strcmp(KEY_HIGHEST, level))
    qps = qp_table[0];
  else if (!strcmp(KEY_HIGHER, level))
    qps = qp_table[1];
  else if (!strcmp(KEY_HIGH, level))
    qps = qp_table[2];
  else if (!strcmp(KEY_MEDIUM, level))
    qps = qp_table[3];
  else if (!strcmp(KEY_LOW, level))
    qps = qp_table[4];
  else if (!strcmp(KEY_LOWER, level))
    qps = qp_table[5];
  else if (!strcmp(KEY_LOWEST, level))
    qps = qp_table[6];
  else {
    RKMEDIA_LOGE("MPP Encoder: invalid rcQualit:%s\n", level);
    return -1;
  }

  return 0;
}

bool MPPCommonConfig::InitConfig(MPPEncoder &mpp_enc, MediaConfig &cfg) {
  VideoConfig vconfig = cfg.vid_cfg;
  VideoEncoderQp &qp = vconfig.encode_qp;
  ImageConfig &img_cfg = vconfig.image_cfg;
  ImageInfo &image_info = cfg.img_cfg.image_info;
  ImageRect &rect_info = cfg.img_cfg.rect_info;
  MppPollType timeout = MPP_POLL_BLOCK;
  MppFrameFormat pic_type = ConvertToMppPixFmt(image_info.pix_fmt);

  int ret = 0;
  if (!enc_cfg) {
    RKMEDIA_LOGE("MPP Encoder: mpp enc cfg is null!\n");
    return false;
  }

  if (!vconfig.rc_mode) {
    RKMEDIA_LOGI("MPP Encoder: rcMode use defalut value: vbr\n");
    vconfig.rc_mode = KEY_VBR;
  }

  MppEncRcMode rc_mode = GetMPPRCMode(vconfig.rc_mode);
  if (rc_mode == MPP_ENC_RC_MODE_BUTT) {
    RKMEDIA_LOGE("MPP Encoder: Invalid rc mode %s\n", vconfig.rc_mode);
    return false;
  }

  // In VBR mode, and the user has not set qp,
  // at this time, the qp value is obtained according to RcQuality.
  if (vconfig.rc_quality && (!strcmp(vconfig.rc_mode, KEY_VBR)) &&
      (!qp.qp_max || !qp.qp_min)) {
    VideoEncoderQp qps;
    if (CalcQpWithRcQuality(vconfig.rc_quality, qps))
      return false;
    RKMEDIA_LOGI("MPP Encoder: [%s:%s] init:%d, setp:%d, min:%d, "
                 "max:%d, min_i:%d, max_i:%d\n",
                 vconfig.rc_mode, vconfig.rc_quality, qps.qp_init, qps.qp_step,
                 qps.qp_min, qps.qp_max, qps.qp_min_i, qps.qp_max_i);
    memcpy(&qp, &qps, sizeof(VideoEncoderQp));
  }

  // Encoder param check.
  ENCODER_CFG_CHECK(vconfig.frame_rate, 1, 60, ENCODER_CFG_INVALID, "fpsNum");
  ENCODER_CFG_CHECK(vconfig.frame_rate_den, 1, 16, 1, "fpsDen");
  ENCODER_CFG_CHECK(vconfig.frame_in_rate, 1, 60, ENCODER_CFG_INVALID,
                    "fpsInNum");
  ENCODER_CFG_CHECK(vconfig.frame_in_rate_den, 1, 16, 1, "fpsInDen");
  ENCODER_CFG_CHECK(vconfig.gop_size, 1, 3000,
                    (vconfig.frame_rate > 10) ? vconfig.frame_rate : 30,
                    "gopSize");

  if (rc_mode == MPP_ENC_RC_MODE_AVBR) {
    ENCODER_CFG_CHECK(qp.qp_max_i, 8, 51, 48, "qpMaxi");
    ENCODER_CFG_CHECK(qp.qp_min_i, 1, VALUE_MIN(qp.qp_max_i, 48),
                      VALUE_MIN(qp.qp_max_i, 28), "qpMini");
    ENCODER_CFG_CHECK(qp.qp_max, 8, 51, 48, "qpMax");
    ENCODER_CFG_CHECK(qp.qp_min, 1, VALUE_MIN(qp.qp_max, 48),
                      VALUE_MIN(qp.qp_max, 32), "qpMin");
  } else {
    ENCODER_CFG_CHECK(qp.qp_max_i, 8, 51, 48, "qpMaxi");
    ENCODER_CFG_CHECK(qp.qp_min_i, 1, VALUE_MIN(qp.qp_max_i, 48),
                      VALUE_MIN(qp.qp_max_i, 8), "qpMini");
    ENCODER_CFG_CHECK(qp.qp_max, 8, 51, 48, "qpMax");
    ENCODER_CFG_CHECK(qp.qp_min, 1, VALUE_MIN(qp.qp_max, 48),
                      VALUE_MIN(qp.qp_max, 8), "qpMin");
  }

  if (qp.qp_init <= 0) {
    // qp_init = -1: mpp encoder automatically generate
    // a value for qp_init.
    qp.qp_init = -1;
    RKMEDIA_LOGI("MPP Encoder: qpInit use default value:-1\n");
  } else {
    ENCODER_CFG_CHECK(qp.qp_init, qp.qp_min, qp.qp_max, ENCODER_CFG_INVALID,
                      "qpInit");
  }
  ENCODER_CFG_CHECK(qp.qp_step, 1, (qp.qp_max - qp.qp_min),
                    VALUE_MIN((qp.qp_max - qp.qp_min), 2), "qpStep");
  ENCODER_CFG_CHECK(img_cfg.image_info.width, 1, 8192, ENCODER_CFG_INVALID,
                    "width");
  ENCODER_CFG_CHECK(img_cfg.image_info.height, 1, 8192, ENCODER_CFG_INVALID,
                    "height");
  ENCODER_CFG_CHECK(img_cfg.image_info.vir_width, 1, 8192,
                    img_cfg.image_info.width, "virWidth");
  ENCODER_CFG_CHECK(img_cfg.image_info.vir_height, 1, 8192,
                    img_cfg.image_info.height, "virHeight");

  ENCODER_CFG_CHECK(rect_info.x, 0, img_cfg.image_info.width, 0, "rect_x");
  ENCODER_CFG_CHECK(rect_info.y, 0, img_cfg.image_info.height, 0, "rect_y");
  ENCODER_CFG_CHECK(rect_info.w, 0, (img_cfg.image_info.width - rect_info.x),
                    (img_cfg.image_info.width - rect_info.x), "rect_w");
  ENCODER_CFG_CHECK(rect_info.h, 0, (img_cfg.image_info.height - rect_info.y),
                    (img_cfg.image_info.height - rect_info.y), "rect_h");

  if (pic_type == -1) {
    RKMEDIA_LOGI("error input pixel format\n");
    return false;
  }

  int bps_max = vconfig.bit_rate_max;
  int bps_min = vconfig.bit_rate_min;
  int bps_target = vconfig.bit_rate;
  int fps_in_num = vconfig.frame_in_rate;
  int fps_in_den = vconfig.frame_in_rate_den;
  int fps_out_num = vconfig.frame_rate;
  int fps_out_den = vconfig.frame_rate_den;
  int gop = vconfig.gop_size;
  int full_range = vconfig.full_range;
  MppEncRotationCfg rotation = MPP_ENC_ROT_0;

  switch (vconfig.rotation) {
  case 0:
    rotation = MPP_ENC_ROT_0;
    RKMEDIA_LOGI("MPP Encoder: rotaion = 0\n");
    break;
  case 90:
    RKMEDIA_LOGI("MPP Encoder: rotaion = 90\n");
    rotation = MPP_ENC_ROT_90;
    break;
  case 180:
    RKMEDIA_LOGI("MPP Encoder: rotaion = 180\n");
    rotation = MPP_ENC_ROT_180;
    break;
  case 270:
    RKMEDIA_LOGI("MPP Encoder: rotaion = 270\n");
    rotation = MPP_ENC_ROT_270;
    break;
  default:
    RKMEDIA_LOGE("MPP Encoder: rotaion(%d) is invalid!\n", vconfig.rotation);
    return false;
  }

  // encoder not support fbc rotation.
  if ((image_info.pix_fmt == PIX_FMT_FBC0) ||
      (image_info.pix_fmt == PIX_FMT_FBC2)) {
    if ((rotation == MPP_ENC_ROT_90) || (rotation == MPP_ENC_ROT_270)) {
      int tmp_value = image_info.width;
      image_info.width = image_info.height;
      image_info.height = tmp_value;
      tmp_value = image_info.vir_width;
      image_info.vir_width = image_info.vir_height;
      image_info.vir_height = tmp_value;
      RKMEDIA_LOGI("MPP Encoder: rotation in fbc mode, Resolution "
                   "from %d(%d)x%d(%d) to %d(%d)x%d(%d)\n",
                   image_info.height, image_info.vir_height, image_info.width,
                   image_info.vir_width, image_info.width, image_info.vir_width,
                   image_info.height, image_info.vir_height);
    }
    rotation = MPP_ENC_ROT_0;
  }

  // Three bit rate configuration methods:
  //  1. Straight-through mode: all three code rate values must be valid.
  //  2. Only bps_max: Generate three values based on bps_max.
  //  3. Only bps_target: Generate three values based on bps_target.
  if (bps_max && bps_min && bps_target) {
    if ((bps_max < MPP_MIN_BPS_VALUE) || (bps_max > MPP_MAX_BPS_VALUE) ||
        (bps_min < MPP_MIN_BPS_VALUE) || (bps_min > MPP_MAX_BPS_VALUE) ||
        (bps_target < bps_min) || (bps_target > bps_max))
      ret = -1;
  } else if (bps_max && !bps_target && !bps_min)
    ret = CalcMppBpsWithMax(rc_mode, bps_max, bps_min, bps_target);
  else if (bps_target && !bps_max && !bps_min)
    ret = CalcMppBpsWithTarget(rc_mode, bps_max, bps_min, bps_target);
  else
    ret = -1;
  if (ret < 0) {
    RKMEDIA_LOGE("MPP Encoder: Invalid bps:[%d, %d, %d]\n",
                 vconfig.bit_rate_min, vconfig.bit_rate, vconfig.bit_rate_max);
    return false;
  }

  int line_size = image_info.vir_width;
  if (pic_type == MPP_FMT_YUV422_YUYV || pic_type == MPP_FMT_YUV422_UYVY)
    line_size *= 2;

  RKMEDIA_LOGI("MPP Encoder: Set output block mode.\n");
  ret = mpp_enc.EncodeControl(MPP_SET_OUTPUT_TIMEOUT, &timeout);
  if (ret != 0) {
    RKMEDIA_LOGE("MPP Encoder: set output block failed ret %d\n", ret);
    return false;
  }
  RKMEDIA_LOGI("MPP Encoder: Set input block mode.\n");
  ret = mpp_enc.EncodeControl(MPP_SET_INPUT_TIMEOUT, &timeout);
  if (ret != 0) {
    RKMEDIA_LOGE("MPP Encoder: set input block failed ret %d\n", ret);
    return false;
  }

  // precfg set.
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:width", rect_info.w);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:height", rect_info.h);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride", line_size);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:ver_stride", image_info.vir_height);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:format", pic_type);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:rotation", rotation);
  if (full_range)
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:range", MPP_FRAME_RANGE_JPEG);

  // rccfg set.
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", rc_mode);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_flex", 0);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_num", fps_in_num);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_denorm", fps_in_den);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_flex", 0);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_num", fps_out_num);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_denorm", fps_out_den);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", gop);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_init", qp.qp_init);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max", qp.qp_max);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min", qp.qp_min);
  // ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_step", vconfig.qp_step);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max_i", qp.qp_max_i);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min_i", qp.qp_min_i);

  vconfig.frame_rate = fps_in_num;
  RKMEDIA_LOGI("MPP Encoder: bps:[%d,%d,%d] fps: [%d/%d]->[%d/%d], gop:%d "
               "qpInit:%d, qpMin:%d, qpMax:%d, qpMinI:%d, qpMaxI:%d.\n",
               bps_max, bps_target, bps_min, fps_in_num, fps_in_den,
               fps_out_num, fps_out_den, gop, qp.qp_init, qp.qp_min, qp.qp_max,
               qp.qp_min_i, qp.qp_max_i);

  // codeccfg set.
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "codec:type", code_type);
  if (code_type == MPP_VIDEO_CodingAVC) {
    // H.264 profile_idc parameter
    // 66  - Baseline profile
    // 77  - Main profile
    // 100 - High profile
    if (vconfig.profile != 66 && vconfig.profile != 77) {
      RKMEDIA_LOGI("MPP Encoder: H264 profile use defalut value: 100");
      vconfig.profile = 100; // default PROFILE_HIGH 100
    }
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "h264:profile", vconfig.profile);

    // H.264 level_idc parameter
    // 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
    // 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
    // 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
    // 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
    // 50 / 51 / 52         - 4K@30fps
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "h264:level", vconfig.level);
    ret |= mpp_enc_cfg_set_s32(
        enc_cfg, "h264:cabac_en",
        (vconfig.profile == 100 || vconfig.profile == 77) ? 1 : 0);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "h264:cabac_idc", 0);
    // trans8x8 should set to 1 if profile=100
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "h264:trans8x8",
                               (vconfig.profile == 100) ? 1 : 0);
    RKMEDIA_LOGI("MPP Encoder: AVC: encode profile %d level %d\n",
                 vconfig.profile, vconfig.level);
  } else if (code_type == MPP_VIDEO_CodingHEVC) {
    // H.265 parameter
    if (vconfig.scaling_list)
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "h265:scaling_list", 1);
  }

  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: cfg set s32 failed ret %d\n", ret);
    return false;
  }

  ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: set cfg failed ret %d\n", ret);
    return false;
  }

  // save bps to vconfig.
  vconfig.bit_rate_max = bps_max;
  vconfig.bit_rate_min = bps_min;
  vconfig.bit_rate = bps_target;

  RKMEDIA_LOGI("MPP Encoder: w x h(%d[%d] x %d[%d])\n", image_info.width,
               line_size, image_info.height, image_info.vir_height);

#if 0
  MppPacket packet = nullptr;
  ret = mpp_enc.EncodeControl(MPP_ENC_GET_EXTRA_INFO, &packet);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: get extra info failed\n");
    return false;
  }

  // Get and write sps/pps for H.264/5
  if (packet) {
    void *ptr = mpp_packet_get_pos(packet);
    size_t len = mpp_packet_get_length(packet);
    if (!mpp_enc.SetExtraData(ptr, len)) {
      RKMEDIA_LOGE("MPP Encoder: SetExtraData failed\n");
      return false;
    }
    mpp_enc.GetExtraData()->SetUserFlag(MediaBuffer::kExtraIntra);
    packet = NULL;
  }
#endif

  if (vconfig.ref_frm_cfg) {
    MppEncRefCfg ref = NULL;

    RKMEDIA_LOGI("MPP Encoder: enable tsvc mode...\n");
    if (mpp_enc_ref_cfg_init(&ref)) {
      RKMEDIA_LOGE("MPP Encoder: ref cfg init failed!\n");
      return false;
    }
    if (mpi_enc_gen_ref_cfg(ref)) {
      RKMEDIA_LOGE("MPP Encoder: ref cfg gen failed!\n");
      mpp_enc_ref_cfg_deinit(&ref);
      return false;
    }
    ret = mpp_enc.EncodeControl(MPP_ENC_SET_REF_CFG, ref);
    mpp_enc_ref_cfg_deinit(&ref);
  }

  // ALL IDR whith (VPS)/SPS/PPS
  int header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
  ret = mpp_enc.EncodeControl(MPP_ENC_SET_HEADER_MODE, &header_mode);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: set heder mode failed ret %d\n", ret);
    return false;
  }

  // Disable RockChip self sei info
  MppEncSeiMode sei_mode = MPP_ENC_SEI_MODE_DISABLE;
  ret = mpp_enc.EncodeControl(MPP_ENC_SET_SEI_CFG, &sei_mode);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: set sei cfg failed ret %d\n", ret);
    return false;
  }

  mpp_enc.GetConfig().vid_cfg = vconfig;
  mpp_enc.GetConfig().type = Type::Video;
  return true;
}

bool MPPCommonConfig::CheckConfigChange(MPPEncoder &mpp_enc, uint32_t change,
                                        std::shared_ptr<ParameterBuffer> val) {
  VideoConfig &vconfig = mpp_enc.GetConfig().vid_cfg;
  ImageConfig &iconfig = mpp_enc.GetConfig().img_cfg;
  VideoEncoderQp &qp = vconfig.encode_qp;
  int ret = 0;

  if (!enc_cfg) {
    RKMEDIA_LOGE("MPP Encoder: mpp enc cfg is null!\n");
    return false;
  }

  if (change & VideoEncoder::kFrameRateChange) {
    uint8_t *values = (uint8_t *)val->GetPtr();
    if (val->GetSize() < 4) {
      RKMEDIA_LOGE("MPP Encoder: fps should be uint8_t array[4]:"
                   "{inFpsNum, inFpsDen, outFpsNum, outFpsDen}");
      return false;
    }
    uint8_t in_fps_num = values[0];
    uint8_t in_fps_den = values[1];
    uint8_t out_fps_num = values[2];
    uint8_t out_fps_den = values[3];

    if (!out_fps_num || !out_fps_den || (out_fps_num > 60)) {
      RKMEDIA_LOGE("MPP Encoder: invalid out fps: [%d/%d]\n", out_fps_num,
                   out_fps_den);
      return false;
    }

    if (in_fps_num && in_fps_den) {
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_num", in_fps_num);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_denorm", in_fps_den);
    }
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_num", out_fps_num);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_denorm", out_fps_den);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: fps: cfg set s32 failed ret %d\n", ret);
      return false;
    }
    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder: change fps cfg failed!\n");
      return false;
    }
    if (in_fps_num && in_fps_den) {
      RKMEDIA_LOGI("MPP Encoder: new fps: [%d/%d]->[%d/%d]\n", in_fps_num,
                   in_fps_den, out_fps_num, out_fps_den);
    } else
      RKMEDIA_LOGI("MPP Encoder: new out fps: [%d/%d]\n", out_fps_num,
                   out_fps_den);

    vconfig.frame_rate = out_fps_num;
  } else if (change & VideoEncoder::kBitRateChange) {
    int *values = (int *)val->GetPtr();
    if (val->GetSize() < 3 * sizeof(int)) {
      RKMEDIA_LOGE("MPP Encoder: fps should be int array[3]:"
                   "{bpsMin, bpsTarget, bpsMax}");
      return false;
    }
    int bps_min = values[0];
    int bps_target = values[1];
    int bps_max = values[2];
    MppEncRcMode rc_mode = GetMPPRCMode(vconfig.rc_mode);
    if (rc_mode == MPP_ENC_RC_MODE_BUTT) {
      RKMEDIA_LOGE("MPP Encoder: bps: invalid rc mode %s\n", vconfig.rc_mode);
      return false;
    }

    // Three bit rate configuration methods:
    //  1. Straight-through mode: all three code rate values must be valid.
    //  2. Only bps_max: Generate three values based on bps_max.
    //  3. Only bps_target: Generate three values based on bps_target.
    if (bps_max && bps_min && bps_target) {
      if ((bps_max > MPP_MAX_BPS_VALUE) || (bps_max < MPP_MIN_BPS_VALUE) ||
          (bps_min > MPP_MAX_BPS_VALUE) || (bps_min < MPP_MIN_BPS_VALUE) ||
          (bps_target > bps_max) || (bps_target < bps_min))
        ret = -1;
    } else if (bps_max && !bps_target && !bps_min)
      ret = CalcMppBpsWithMax(rc_mode, bps_max, bps_min, bps_target);
    else if (bps_target && !bps_max && !bps_min)
      ret = CalcMppBpsWithTarget(rc_mode, bps_max, bps_min, bps_target);
    else
      ret = -1;
    if (ret < 0) {
      RKMEDIA_LOGE("MPP Encoder: Invalid bps:[%d, %d, %d]\n", bps_min,
                   bps_target, bps_max);
      return false;
    }

    ret = 0;
    RKMEDIA_LOGI("MPP Encoder: new bps:[%d, %d, %d]\n", bps_min, bps_target,
                 bps_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: bps: cfg set s32 failed ret %d\n", ret);
      return false;
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder: change bps cfg failed!\n");
      return false;
    }
    // save new value to config.
    vconfig.bit_rate = bps_target;
    vconfig.bit_rate_max = bps_max;
    vconfig.bit_rate_min = bps_min;
  } else if (change & VideoEncoder::kRcModeChange) {
    char *new_mode = (char *)val->GetPtr();
    RKMEDIA_LOGI("MPP Encoder: new rc_mode:%s\n", new_mode);
    MppEncRcMode rc_mode = GetMPPRCMode(new_mode);
    if (rc_mode == MPP_ENC_RC_MODE_BUTT) {
      RKMEDIA_LOGE(
          "MPP Encoder: rc_mode is invalid! should be cbr/vbr/avbr.\n");
      return false;
    }

    // Recalculate bps
    int bps_max = vconfig.bit_rate_max;
    int bps_min = bps_max;
    int bps_target = bps_max;
    if (CalcMppBpsWithMax(rc_mode, bps_max, bps_min, bps_target) < 0)
      return false;
    // Reset qp values
    int qp_max = 48;
    int qp_min = 8;
    int qp_max_i = 48;
    int qp_min_i = 8;
    if (rc_mode == MPP_ENC_RC_MODE_AVBR) {
      qp_max = 48;
      qp_min = 32;
      qp_max_i = 48;
      qp_max_i = 28;
    }

    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", rc_mode);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_init", -1);
    // ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_step", 2);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max", qp_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min", qp_min);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max_i", qp_max_i);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min_i", qp_min_i);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: rc mode: cfg set s32 failed ret %d\n", ret);
      return false;
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder: change rc_mode cfg failed!\n");
      return false;
    }
    // save new value to encoder->vconfig.
    vconfig.rc_mode = ConvertRcMode(new_mode);
    ;
  } else if (change & VideoEncoder::kProfileChange) {
    if (val->GetSize() < 2 * sizeof(int)) {
      RKMEDIA_LOGE("MPP Encoder: fps should be int array[2]:"
                   "{profile_idc, level}");
      return false;
    }
    int profile_idc = *((int *)val->GetPtr());
    int level = *((int *)val->GetPtr() + 1);
    RKMEDIA_LOGI("MPP Encoder: new profile_idc:%d, level:%d\n", profile_idc,
                 level);

    if (vconfig.image_cfg.codec_type != CODEC_TYPE_H264) {
      RKMEDIA_LOGE(
          "MPP Encoder: Current codec:%d not support Profile change.\n",
          vconfig.image_cfg.codec_type);
      return false;
    }

    // H.264 profile_idc parameter
    // 66  - Baseline profile
    // 77  - Main profile
    // 100 - High profile
    if ((profile_idc != 66) && (profile_idc != 77) && (profile_idc != 100)) {
      RKMEDIA_LOGI("MPP Encoder: Invalid H264 profile(%d)\n", profile_idc);
      return false;
    }
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "h264:profile", profile_idc);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "h264:cabac_en",
                               (profile_idc == 100) ? 1 : 0);

    // H.264 level_idc parameter
    // 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
    // 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
    // 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
    // 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
    // 50 / 51 / 52         - 4K@30fps
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "h264:level", level);

    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: profile: cfg set s32 failed ret %d\n", ret);
      return false;
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder: change rc_mode cfg failed!\n");
      return false;
    }
    // save new value to encoder->vconfig.
    vconfig.profile = profile_idc;
    vconfig.level = level;
  } else if (change & VideoEncoder::kRcQualityChange) {
    char *rc_quality = (char *)val->GetPtr();
    VideoEncoderQp qps;

    if (strcmp(vconfig.rc_mode, KEY_VBR)) {
      RKMEDIA_LOGE("MPP Encoder: only vbr mode support rcQuality changes!\n");
      return false;
    }
    memcpy(&qps, &qp, sizeof(VideoEncoderQp));
    if (CalcQpWithRcQuality(rc_quality, qps))
      return false;

    RKMEDIA_LOGI("MPP Encoder: [%s:%s->%s] init:%d, setp:%d, min:%d, "
                 "max:%d, min_i:%d, max_i:%d\n",
                 vconfig.rc_mode, vconfig.rc_quality, rc_quality, qps.qp_init,
                 qps.qp_step, qps.qp_min, qps.qp_max, qps.qp_min_i,
                 qps.qp_max_i);

    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_init", qps.qp_init);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max", qps.qp_max);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min", qps.qp_min);
    // ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_step", qps.qp_step);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max_i", qps.qp_max_i);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min_i", qps.qp_min_i);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: qp: cfg set s32 failed ret %d\n", ret);
      return false;
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder: change qp cfg failed!\n");
      return false;
    }
    memcpy(&qp, &qps, sizeof(VideoEncoderQp));
    vconfig.rc_quality = ConvertRcQuality(rc_quality);
  } else if (change & VideoEncoder::kGopChange) {
    int new_gop_size = val->GetValue();
    if (new_gop_size < 0) {
      RKMEDIA_LOGE("MPP Encoder: gop size invalid!\n");
      return false;
    }
    RKMEDIA_LOGI("MPP Encoder: gop change frome %d to %d\n", vconfig.gop_size,
                 new_gop_size);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", new_gop_size);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: gop: cfg set s32 failed ret %d\n", ret);
      return false;
    }
    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder: change gop cfg failed!\n");
      return false;
    }
    // save to vconfig
    vconfig.gop_size = new_gop_size;
    // gop change restart userata status
    mpp_enc.RestartUserData();
  } else if (change & VideoEncoder::kQPChange) {
    VideoEncoderQp *qps = (VideoEncoderQp *)val->GetPtr();
    if (val->GetSize() < sizeof(VideoEncoderQp)) {
      RKMEDIA_LOGE("MPP Encoder: Incomplete VideoEncoderQp information\n");
      return false;
    }
    if (change & VideoEncoder::kGetFlag) {
      if (mpp_enc.EncodeControl(MPP_ENC_GET_CFG, enc_cfg) != 0) {
        RKMEDIA_LOGE("MPP Encoder: change qp cfg failed!\n");
        return false;
      }
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "rc:qp_init", &qps->qp_init);
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "rc:qp_max", &qps->qp_max);
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "rc:qp_min", &qps->qp_min);
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "rc:qp_max_i", &qps->qp_max_i);
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "rc:qp_min_i", &qps->qp_min_i);
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "hw:qp_row_i", &qps->row_qp_delta_i);
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "hw:qp_row", &qps->row_qp_delta_p);
      ret |= mpp_enc_cfg_get_s32(enc_cfg, "rc:hier_qp_en", &qps->hier_qp_en);
      ret |=
          mpp_enc_cfg_get_st(enc_cfg, "rc:hier_qp_delta", &qps->hier_qp_delta);
      ret |= mpp_enc_cfg_get_st(enc_cfg, "rc:hier_frame_num",
                                &qps->hier_frame_num);
      ret |= mpp_enc_cfg_get_st(enc_cfg, "hw:aq_thrd_i", qps->thrd_i);
      ret |= mpp_enc_cfg_get_st(enc_cfg, "hw:aq_thrd_p", qps->thrd_p);
      if (ret) {
        RKMEDIA_LOGE("MPP Encoder: qp: cfg get s32 failed ret %d\n", ret);
        return false;
      }
    } else {
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_init", qps->qp_init);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max", qps->qp_max);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min", qps->qp_min);
      // ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_step", qps->qp_step);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max_i", qps->qp_max_i);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min_i", qps->qp_min_i);
      // hardware cfg. If the hardware does not support it,
      // the interface will do nothing
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "hw:qp_row_i", qps->row_qp_delta_i);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "hw:qp_row", qps->row_qp_delta_p);
      // hierachy qp cfg
      if (qps->hier_qp_en) {
        ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:hier_qp_en", 1);
        ret |=
            mpp_enc_cfg_set_st(enc_cfg, "rc:hier_qp_delta", qps->hier_qp_delta);
        ret |= mpp_enc_cfg_set_st(enc_cfg, "rc:hier_frame_num",
                                  qps->hier_frame_num);
      }

      ret |= mpp_enc_cfg_set_st(enc_cfg, "hw:aq_thrd_i", qps->thrd_i);
      ret |= mpp_enc_cfg_set_st(enc_cfg, "hw:aq_thrd_p", qps->thrd_p);

      if (ret) {
        RKMEDIA_LOGE("MPP Encoder: qp: cfg set s32 failed ret %d\n", ret);
        return false;
      }

      if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
        RKMEDIA_LOGE("MPP Encoder: change qp cfg failed!\n");
        return false;
      }
    }
    RKMEDIA_LOGI("MPP Encoder: cur qp value:\n"
                 "\tQpInit:%d\n"
                 "\tQpMin:%d\n"
                 "\tQpMax:%d\n"
                 "\tQpMinI:%d\n"
                 "\tQpMaxI:%d\n"
                 "\tQpRowDeltaI:%d\n"
                 "\tQpRowDeltaP:%d\n"
                 "\tthri[0]:%d, p= %p\n"
                 "\tthrp[0]:%d\n",
                 qps->qp_init, qps->qp_min, qps->qp_max, qps->qp_min_i,
                 qps->qp_max_i, qps->row_qp_delta_i, qps->row_qp_delta_p,
                 qps->thrd_i[0], qps->thrd_i, qps->thrd_p[0]);
    memcpy(&qp, qps, sizeof(VideoEncoderQp));
  } else if (change & VideoEncoder::kROICfgChange) {
    EncROIRegion *regions = (EncROIRegion *)val->GetPtr();
    if (val->GetSize() && (val->GetSize() < sizeof(EncROIRegion))) {
      RKMEDIA_LOGE("MPP Encoder: ParameterBuffer size is invalid!\n");
      return false;
    }
    int region_cnt = val->GetSize() / sizeof(EncROIRegion);
    mpp_enc.RoiUpdateRegions(regions, region_cnt);
  } else if (change & VideoEncoder::kForceIdrFrame) {
    RKMEDIA_LOGI("MPP Encoder: force idr frame...\n");
    if (mpp_enc.EncodeControl(MPP_ENC_SET_IDR_FRAME, nullptr) != 0) {
      RKMEDIA_LOGE("MPP Encoder: force idr frame control failed!\n");
      return false;
    }
    // force idr frame, restart userata status
    mpp_enc.RestartUserData();
  } else if (change & VideoEncoder::kSplitChange) {
    if (val->GetSize() < (2 * sizeof(int))) {
      RKMEDIA_LOGE("MPP Encoder: Incomplete split information\n");
      return false;
    }
    RK_U32 split_mode = *((unsigned int *)val->GetPtr());
    RK_U32 split_arg = *((unsigned int *)val->GetPtr() + 1);

    RKMEDIA_LOGI("MPP Encoder: split_mode:%u, split_arg:%u\n", split_mode,
                 split_arg);
    ret |= mpp_enc_cfg_set_u32(enc_cfg, "split:mode", split_mode);
    ret |= mpp_enc_cfg_set_u32(enc_cfg, "split:arg", split_arg);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: split: cfg set s32 failed ret %d\n", ret);
      return false;
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
      RKMEDIA_LOGE("MPP Encoder: set split mode failed!\n");
      return false;
    }
  } else if (change & VideoEncoder::kGopModeChange) {
    if (val->GetSize() < sizeof(EncGopModeParam)) {
      RKMEDIA_LOGE("MPP Encoder: Incomplete gop mode params\n");
      return false;
    }
    EncGopModeParam *gop_param = (EncGopModeParam *)val->GetPtr();
    EncGopMode gop_mode = gop_param->mode;
    MppEncRefCfg ref = NULL;

    switch (gop_mode) {
    case GOP_MODE_TSVC2:
    case GOP_MODE_TSVC3:
    case GOP_MODE_TSVC4: {
      RKMEDIA_LOGI("MPP Encoder: Set GopMode to \"TSVC\" mode...\n");
      if (mpp_enc_ref_cfg_init(&ref)) {
        RKMEDIA_LOGE("MPP Encoder: ref cfg init failed!\n");
        return false;
      }
      if (mpi_enc_gen_ref_cfg(ref, gop_mode)) {
        RKMEDIA_LOGE("MPP Encoder: ref cfg gen failed!\n");
        mpp_enc_ref_cfg_deinit(&ref);
        return false;
      }
      ret = mpp_enc.EncodeControl(MPP_ENC_SET_REF_CFG, ref);
      mpp_enc_ref_cfg_deinit(&ref);
    } break;
    case GOP_MODE_SMARTP: {
      /*********************************************************
       * Set Gop Mode
       * *******************************************************/
      // virtual intra frame gap.
      int smartp_vi_len = gop_param->gop_size;
      int smartp_gop_len = gop_param->interval;
      int smartp_ip_qp_delta = gop_param->ip_qp_delta;
      int smartp_vi_qp_delta = gop_param->vi_qp_delta;

      RKMEDIA_LOGI("MPP Encoder: Set GopMode to \"SMARTP\" mode. "
                   "gop_size:%d, interval:%d, ip_qp_delta:%d, "
                   "vi_qp_delta:%d\n",
                   gop_param->gop_size, gop_param->interval,
                   gop_param->ip_qp_delta, gop_param->vi_qp_delta);

      if (mpp_enc_ref_cfg_init(&ref)) {
        RKMEDIA_LOGE("MPP Encoder: ref cfg init failed!\n");
        return false;
      }
      if (mpi_enc_gen_ref_cfg(ref, gop_mode, smartp_gop_len, smartp_vi_len)) {
        RKMEDIA_LOGE("MPP Encoder: ref cfg gen failed!\n");
        mpp_enc_ref_cfg_deinit(&ref);
        return false;
      }
      ret = mpp_enc.EncodeControl(MPP_ENC_SET_REF_CFG, ref);
      mpp_enc_ref_cfg_deinit(&ref);
      if (ret) {
        RKMEDIA_LOGE("MPP Encoder: SMARTP: set ref cfg failed!\n");
        return false;
      }

      ret = mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", smartp_gop_len);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_ip", smartp_ip_qp_delta);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_vi", smartp_vi_qp_delta);
      // Set a special qp value to protect the effect of this mode
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max", 48);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min", 30);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max_i", 48);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min_i", 30);
      if (ret) {
        RKMEDIA_LOGE("MPP Encoder: gop mode: cfg set s32 failed ret %d\n", ret);
        return false;
      }

      if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
        RKMEDIA_LOGE("MPP Encoder: change gop cfg failed!\n");
        return false;
      }
    } break;
    case GOP_MODE_NORMALP: {
      RKMEDIA_LOGI("MPP Encoder: Set GopMode to \"NORMALP\" mode...\n");
      ret = mpp_enc.EncodeControl(MPP_ENC_SET_REF_CFG, NULL);
      if (ret) {
        RKMEDIA_LOGE("MPP Encoder: gop mode: reset ref cfg failed ret %d\n",
                     ret);
        return false;
      }

      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", vconfig.gop_size);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_ip", gop_param->ip_qp_delta);
      // Reset qp value frome smartp
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max", qp.qp_max);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min", qp.qp_min);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_max_i", qp.qp_max_i);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:qp_min_i", qp.qp_min_i);
      if (ret) {
        RKMEDIA_LOGE("MPP Encoder: gop mode: cfg set s32 failed ret %d\n", ret);
        return false;
      }

      if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
        RKMEDIA_LOGE("MPP Encoder: change gop cfg failed!\n");
        return false;
      }
    } break;
    default:
      RKMEDIA_LOGE("MPP Encoder: gop mode: unsupport mode value(%d)!\n",
                   gop_mode);
      return false;
    }

    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: set gop mode failed!\n");
      return false;
    }
  } else if (change & VideoEncoder::kOSDDataChange) {
    // type: OsdRegionData*
    RKMEDIA_LOGD("MPP Encoder: config osd regions\n");
    if (val->GetSize() < sizeof(OsdRegionData)) {
      RKMEDIA_LOGE("MPP Encoder: palette buff should be OsdRegionData type\n");
      return false;
    }
    if (!val->GetPtr()) {
      RKMEDIA_LOGE("MPP Encoder: osd data is nullptr\n");
      return false;
    }
#ifdef MPP_SUPPORT_HW_OSD
    OsdRegionData *param = (OsdRegionData *)val->GetPtr();
    if (param->region_type == REGION_TYPE_OVERLAY ||
        param->region_type == REGION_TYPE_COVER ||
        param->region_type == REGION_TYPE_MOSAIC) {
      if (mpp_enc.OsdRegionSet(param)) {
        RKMEDIA_LOGE("MPP Encoder: set osd regions error!\n");
        return false;
      }
    }
#endif
#ifdef RGA_OSD_ENABLE
    OsdRegionData *newRegion = (OsdRegionData *)val->GetPtr();
    if (newRegion->region_type == REGION_TYPE_OVERLAY_EX ||
        newRegion->region_type == REGION_TYPE_COVER_EX) {
      if (mpp_enc.RgaOsdRegionSet(newRegion, vconfig)) {
        RKMEDIA_LOGE("MPP Encoder: set rga osd regions error!\n");
        return false;
      }
    }
#endif
  }
#ifdef MPP_SUPPORT_HW_OSD
  else if (change & VideoEncoder::kOSDPltChange) {
    // type: 265 * U32 array.
    RKMEDIA_LOGI("MPP Encoder: config osd palette\n");
    if (val->GetSize() < (sizeof(int) * 4)) {
      RKMEDIA_LOGE("MPP Encoder: palette buff should be U32 * 256\n");
      return false;
    }
    uint32_t *param = (uint32_t *)val->GetPtr();
    if (mpp_enc.OsdPaletteSet(param)) {
      RKMEDIA_LOGE("MPP Encoder: set Palette error!\n");
      return false;
    }
  }
#endif
  else if (change & VideoEncoder::kUserDataChange) {
    // type: OsdRegionData*
    RKMEDIA_LOGD("MPP Encoder: config userdata\n");
    if (val->GetSize() <= 0) {
      RKMEDIA_LOGE("MPP Encoder: invalid userdata size\n");
      return false;
    }
    uint8_t enable_all_frames = *(uint8_t *)val->GetPtr();
    mpp_enc.EnableUserDataAllFrame(enable_all_frames ? true : false);

    const char *data = (char *)val->GetPtr() + 1;
    uint16_t data_len = val->GetSize() - 1;
    if (mpp_enc.SetUserData(data, data_len)) {
      RKMEDIA_LOGE("MPP Encoder: set userdata error!\n");
      return false;
    }
  } else if (change & VideoEncoder::kMoveDetectionFlow) {
    RcApiBrief brief;

    if (val->GetPtr()) {
#if 0
      int bps_max = vconfig.bit_rate_max;
      int bps_min = vconfig.bit_rate_min;
      int bps_target = vconfig.bit_rate;
      int w = vconfig.image_cfg.image_info.vir_width;
      int h = vconfig.image_cfg.image_info.vir_height;
      float bps_factor = smart_enc_mode_get_bps_factor(vconfig.bit_rate_max, w, h);
      bps_min = (int)(vconfig.bit_rate_max * bps_factor);

      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", MPP_ENC_RC_MODE_VBR);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", 300);

      RKMEDIA_LOGI("MPP Encoder: smart enc mode: factor:%f, bps:[%d,%d,%d] gop:%d\n",
        bps_factor, bps_max, bps_target, bps_min, 300);
      if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
        RKMEDIA_LOGE("MPP Encoder: rc control for smart enc failed!\n");
        return false;
      }

      //save to vconfig
      vconfig.gop_size = 300;
      vconfig.rc_mode = KEY_VBR;
      vconfig.bit_rate_min = bps_min;
#endif
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", MPP_ENC_RC_MODE_VBR);
      ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", 300);
      if (mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg) != 0) {
        RKMEDIA_LOGE("MPP Encoder: rc control for smart enc failed!\n");
        return false;
      }
      // save to vconfig
      vconfig.gop_size = 300;
      vconfig.rc_mode = KEY_VBR;
      // gop change restart userata status
      mpp_enc.RestartUserData();

      // Enable smart mode.
      brief.name = "smart";
      brief.type = code_type;
      RKMEDIA_LOGI("MPP Encoder: enable smart enc mode...\n");
    } else {
      brief.name = "defalut";
      brief.type = code_type;
      RKMEDIA_LOGI("MPP Encoder: disable smart enc mode...\n");
    }

    if (mpp_enc.EncodeControl(MPP_ENC_SET_RC_API_CURRENT, &brief) != 0) {
      RKMEDIA_LOGE("MPP Encoder: enable smart enc control failed!\n");
      return false;
    }
    mpp_enc.rc_api_brief_name = brief.name;
  } else if (change & VideoEncoder::kResolutionChange) {
    if (val->GetSize() < sizeof(VideoResolutionCfg)) {
      RKMEDIA_LOGE("MPP Encoder: Incomplete Resolution params\n");
      return false;
    }
    VideoResolutionCfg *vid_cfg = (VideoResolutionCfg *)val->GetPtr();
    RKMEDIA_LOGI(
        "MPP Encoder: width = %d, height = %d, vwidth = %d, vheight = %d.\n",
        vid_cfg->width, vid_cfg->height, vid_cfg->vir_width,
        vid_cfg->vir_height);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:width", vid_cfg->w);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:height", vid_cfg->h);
    MppFrameFormat pic_type = ConvertToMppPixFmt(iconfig.image_info.pix_fmt);
    int line_size = vid_cfg->vir_width;
    if (pic_type == MPP_FMT_YUV422_YUYV || pic_type == MPP_FMT_YUV422_UYVY)
      line_size = vid_cfg->vir_width * 2;
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride", line_size);
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:ver_stride", vid_cfg->vir_height);
    ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: set resolution cfg failed ret %d\n", ret);
      return false;
    }
    iconfig.image_info.width = vid_cfg->width;
    iconfig.image_info.height = vid_cfg->height;
    iconfig.image_info.vir_width = vid_cfg->vir_width;
    iconfig.image_info.vir_height = vid_cfg->vir_height;
    iconfig.rect_info.x = vid_cfg->x;
    iconfig.rect_info.y = vid_cfg->y;
    iconfig.rect_info.w = vid_cfg->w;
    iconfig.rect_info.h = vid_cfg->h;
  } else if (change & VideoEncoder::kRotationChange) {
    if (val->GetSize() < sizeof(int)) {
      RKMEDIA_LOGE("MPP Encoder: Incomplete Rotation params\n");
      return false;
    }
    int *rotation = (int *)val->GetPtr();
    MppEncRotationCfg rotation_enum = MPP_ENC_ROT_0;
    switch (*rotation) {
    case 0:
      rotation_enum = MPP_ENC_ROT_0;
      break;
    case 90:
      rotation_enum = MPP_ENC_ROT_90;
      break;
    case 180:
      rotation_enum = MPP_ENC_ROT_180;
      break;
    case 270:
      rotation_enum = MPP_ENC_ROT_270;
      break;
    default:
      RKMEDIA_LOGE("MPP Encoder: set rotaion(%d) is invalid!\n", *rotation);
      return false;
    }
    RKMEDIA_LOGI("MPP Encoder: rotation set(%d), old rotation(%d).\n",
                 *rotation, vconfig.rotation);
    if (*rotation == vconfig.rotation) {
      return true;
    }
    ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:rotation", rotation_enum);
    ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: set rotation cfg failed ret %d\n", ret);
      return false;
    }
    vconfig.rotation = *rotation;
  } else if (change & VideoEncoder::kSuperFrmChange) {
    if (val->GetSize() != sizeof(VencSuperFrmCfg)) {
      RKMEDIA_LOGE("MPP Encoder: invalid super frame cfg size\n");
      return false;
    }
    VencSuperFrmCfg *super_cfg = (VencSuperFrmCfg *)val->GetPtr();
    MppEncRcSuperFrameMode new_mode;
    switch (super_cfg->SuperFrmMode) {
    case RKMEDIA_SUPERFRM_NONE:
      new_mode = MPP_ENC_RC_SUPER_FRM_NONE;
      break;
    case RKMEDIA_SUPERFRM_DISCARD:
      new_mode = MPP_ENC_RC_SUPER_FRM_DROP;
      break;
    case RKMEDIA_SUPERFRM_REENCODE:
      new_mode = MPP_ENC_RC_SUPER_FRM_REENC;
      break;
    default:
      RKMEDIA_LOGE("MPP Encoder: invalid super fram mode:%d\n",
                   super_cfg->SuperFrmMode);
      return false;
    }
    MppEncRcPriority new_rc_priority;
    switch (super_cfg->RcPriority) {
    case RKMEDIA_VENC_RC_PRIORITY_BITRATE_FIRST:
      new_rc_priority = MPP_ENC_RC_BY_BITRATE_FIRST;
      break;
    case RKMEDIA_VENC_RC_PRIORITY_FRAMEBITS_FIRST:
      new_rc_priority = MPP_ENC_RC_BY_FRM_SIZE_FIRST;
      break;
    default:
      RKMEDIA_LOGE("MPP Encoder: invalid rc priortiy:%d\n",
                   super_cfg->SuperFrmMode);
      return false;
    }
    RKMEDIA_LOGI("MPP Encoder: super frame: mode:%d, priority:%d, "
                 "IThd:%d, PThd:%d\n",
                 super_cfg->SuperFrmMode, super_cfg->RcPriority,
                 super_cfg->SuperIFrmBitsThr, super_cfg->SuperPFrmBitsThr);
    ret |= mpp_enc_cfg_set_u32(enc_cfg, "rc:super_mode", new_mode);
    ret |= mpp_enc_cfg_set_u32(enc_cfg, "rc:priority", new_rc_priority);
    ret |= mpp_enc_cfg_set_u32(enc_cfg, "rc:super_i_thd",
                               super_cfg->SuperIFrmBitsThr);
    ret |= mpp_enc_cfg_set_u32(enc_cfg, "rc:super_p_thd",
                               super_cfg->SuperPFrmBitsThr);
    ret = mpp_enc.EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: set super frm cfg failed ret %d\n", ret);
      return false;
    }
  } else {
    RKMEDIA_LOGI("Unsupport request change 0x%08x!\n", change);
    return false;
  }

  return true;
}

class MPPFinalEncoder : public MPPEncoder {
public:
  MPPFinalEncoder(const char *param);
  virtual ~MPPFinalEncoder() {
    if (mpp_config)
      delete mpp_config;
  }

  static const char *GetCodecName() { return "rkmpp"; }
  virtual bool InitConfig(const MediaConfig &cfg) override;

protected:
  // Change configs which are not contained in sps/pps.
  virtual bool CheckConfigChange(
      std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>) override;

  MPPConfig *mpp_config;
};

MPPFinalEncoder::MPPFinalEncoder(const char *param) : mpp_config(nullptr) {
  std::string output_data_type =
      get_media_value_by_key(param, KEY_OUTPUTDATATYPE);
  SetMppCodeingType(output_data_type.empty()
                        ? MPP_VIDEO_CodingUnused
                        : GetMPPCodingType(output_data_type));
}

bool MPPFinalEncoder::InitConfig(const MediaConfig &cfg) {
  assert(!mpp_config);
  MediaConfig new_cfg = cfg;
  switch (coding_type) {
  case MPP_VIDEO_CodingMJPEG:
    mpp_config = new MPPMJPEGConfig();
    new_cfg.img_cfg.codec_type = codec_type;
    break;
  case MPP_VIDEO_CodingAVC:
  case MPP_VIDEO_CodingHEVC:
    new_cfg.vid_cfg.image_cfg.codec_type = codec_type;
    mpp_config = new MPPCommonConfig(coding_type);
    break;
  default:
    RKMEDIA_LOGI("Unsupport mpp encode type: %d\n", coding_type);
    return false;
  }
  if (!mpp_config) {
    LOG_NO_MEMORY();
    return false;
  }
  return mpp_config->InitConfig(*this, new_cfg);
}

bool MPPFinalEncoder::CheckConfigChange(
    std::pair<uint32_t, std::shared_ptr<ParameterBuffer>> change_pair) {
  // common ConfigChange process
  if (change_pair.first & VideoEncoder::kEnableStatistics) {
    bool value = (change_pair.second->GetValue()) ? true : false;
    set_statistics_switch(value);
    return true;
  }

  assert(mpp_config);
  if (!mpp_config)
    return false;
  return mpp_config->CheckConfigChange(*this, change_pair.first,
                                       change_pair.second);
}

DEFINE_VIDEO_ENCODER_FACTORY(MPPFinalEncoder)
const char *FACTORY(MPPFinalEncoder)::ExpectedInputDataType() {
  return MppAcceptImageFmts();
}

#define VIDEO_ENC_OUTPUT                                                       \
  TYPENEAR(VIDEO_H264)                                                         \
  TYPENEAR(VIDEO_H265) TYPENEAR(VIDEO_MJPEG) TYPENEAR(IMAGE_JPEG)

const char *FACTORY(MPPFinalEncoder)::OutPutDataType() {
  return VIDEO_ENC_OUTPUT;
}

} // namespace easymedia
