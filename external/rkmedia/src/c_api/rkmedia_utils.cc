// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_type.h"
#include "rkmedia_common.h"
#include "rkmedia_venc.h"
#include "utils.h"

std::string ImageTypeToString(IMAGE_TYPE_E type) {
  switch (type) {
  case IMAGE_TYPE_GRAY8:
    return IMAGE_GRAY8;
  case IMAGE_TYPE_GRAY16:
    return IMAGE_GRAY16;
  case IMAGE_TYPE_YUV420P:
    return IMAGE_YUV420P;
  case IMAGE_TYPE_NV12:
    return IMAGE_NV12;
  case IMAGE_TYPE_NV21:
    return IMAGE_NV21;
  case IMAGE_TYPE_YV12:
    return IMAGE_YV12;
  case IMAGE_TYPE_FBC2:
    return IMAGE_FBC2;
  case IMAGE_TYPE_FBC0:
    return IMAGE_FBC0;
  case IMAGE_TYPE_YUV422P:
    return IMAGE_YUV422P;
  case IMAGE_TYPE_NV16:
    return IMAGE_NV16;
  case IMAGE_TYPE_NV61:
    return IMAGE_NV61;
  case IMAGE_TYPE_YV16:
    return IMAGE_YV16;
  case IMAGE_TYPE_YUYV422:
    return IMAGE_YUYV422;
  case IMAGE_TYPE_UYVY422:
    return IMAGE_UYVY422;
  case IMAGE_TYPE_YUV444SP:
    return IMAGE_YUV444SP;
  case IMAGE_TYPE_RGB332:
    return IMAGE_RGB332;
  case IMAGE_TYPE_RGB565:
    return IMAGE_RGB565;
  case IMAGE_TYPE_BGR565:
    return IMAGE_BGR565;
  case IMAGE_TYPE_RGB888:
    return IMAGE_RGB888;
  case IMAGE_TYPE_BGR888:
    return IMAGE_BGR888;
  case IMAGE_TYPE_ARGB8888:
    return IMAGE_ARGB8888;
  case IMAGE_TYPE_ABGR8888:
    return IMAGE_ABGR8888;
  case IMAGE_TYPE_RGBA8888:
    return IMAGE_RGBA8888;
  case IMAGE_TYPE_BGRA8888:
    return IMAGE_BGRA8888;
  case IMAGE_TYPE_JPEG:
    return IMAGE_JPEG;
  default:
    RKMEDIA_LOGE("%s: not support image type:%d", __func__, type);
    return "";
  }
}

IMAGE_TYPE_E StringToImageType(std::string type) {
  if (type == IMAGE_GRAY8)
    return IMAGE_TYPE_GRAY8;
  else if (type == IMAGE_GRAY16)
    return IMAGE_TYPE_GRAY16;
  else if (type == IMAGE_YUV420P)
    return IMAGE_TYPE_YUV420P;
  else if (type == IMAGE_NV12)
    return IMAGE_TYPE_NV12;
  else if (type == IMAGE_NV21)
    return IMAGE_TYPE_NV21;
  else if (type == IMAGE_YV12)
    return IMAGE_TYPE_YV12;
  else if (type == IMAGE_FBC2)
    return IMAGE_TYPE_FBC2;
  else if (type == IMAGE_FBC0)
    return IMAGE_TYPE_FBC0;
  else if (type == IMAGE_YUV422P)
    return IMAGE_TYPE_YUV422P;
  else if (type == IMAGE_NV16)
    return IMAGE_TYPE_NV16;
  else if (type == IMAGE_NV61)
    return IMAGE_TYPE_NV61;
  else if (type == IMAGE_YV16)
    return IMAGE_TYPE_YV16;
  else if (type == IMAGE_YUYV422)
    return IMAGE_TYPE_YUYV422;
  else if (type == IMAGE_UYVY422)
    return IMAGE_TYPE_UYVY422;
  else if (type == IMAGE_YUV444SP)
    return IMAGE_TYPE_YUV444SP;
  else if (type == IMAGE_RGB332)
    return IMAGE_TYPE_RGB332;
  else if (type == IMAGE_RGB565)
    return IMAGE_TYPE_RGB565;
  else if (type == IMAGE_BGR565)
    return IMAGE_TYPE_BGR565;
  else if (type == IMAGE_RGB888)
    return IMAGE_TYPE_RGB888;
  else if (type == IMAGE_BGR888)
    return IMAGE_TYPE_BGR888;
  else if (type == IMAGE_ARGB8888)
    return IMAGE_TYPE_ARGB8888;
  else if (type == IMAGE_ABGR8888)
    return IMAGE_TYPE_ABGR8888;
  else if (type == IMAGE_RGBA8888)
    return IMAGE_TYPE_RGBA8888;
  else if (type == IMAGE_BGRA8888)
    return IMAGE_TYPE_BGRA8888;
  else if (type == IMAGE_JPEG)
    return IMAGE_TYPE_JPEG;
  else
    RKMEDIA_LOGE("%s: unknown image type:%s", __func__, type.c_str());
  return IMAGE_TYPE_UNKNOW;
}

std::string CodecToString(CODEC_TYPE_E type) {
  switch (type) {
  case RK_CODEC_TYPE_MP3:
    return AUDIO_MP3;
  case RK_CODEC_TYPE_MP2:
    return AUDIO_MP2;
  case RK_CODEC_TYPE_VORBIS:
    return AUDIO_VORBIS;
  case RK_CODEC_TYPE_G711A:
    return AUDIO_G711A;
  case RK_CODEC_TYPE_G711U:
    return AUDIO_G711U;
  case RK_CODEC_TYPE_G726:
    return AUDIO_G726;
  case RK_CODEC_TYPE_H264:
    return VIDEO_H264;
  case RK_CODEC_TYPE_H265:
    return VIDEO_H265;
  case RK_CODEC_TYPE_JPEG:
  case RK_CODEC_TYPE_MJPEG:
    return IMAGE_JPEG;
  default:
    return "";
  }
}

std::string SampleFormatToString(SAMPLE_FORMAT_E type) {
  switch (type) {
  case RK_SAMPLE_FMT_U8:
    return AUDIO_PCM_U8;
  case RK_SAMPLE_FMT_S16:
    return AUDIO_PCM_S16;
  case RK_SAMPLE_FMT_S32:
    return AUDIO_PCM_S32;
  case RK_SAMPLE_FMT_FLT:
    return AUDIO_PCM_FLT;
  case RK_SAMPLE_FMT_U8P:
    return AUDIO_PCM_U8P;
  case RK_SAMPLE_FMT_S16P:
    return AUDIO_PCM_S16P;
  case RK_SAMPLE_FMT_S32P:
    return AUDIO_PCM_S32P;
  case RK_SAMPLE_FMT_FLTP:
    return AUDIO_PCM_FLTP;
  default:
    return "";
  }
}

const char *ModIdToString(MOD_ID_E mod_id) {
  if ((mod_id < RK_ID_UNKNOW) || (mod_id > RK_ID_BUTT)) {
    RKMEDIA_LOGE("%s: mod_id is incorrect\n", __func__);
    return mod_tag_list[RK_ID_UNKNOW];
  }

  return mod_tag_list[mod_id];
}
