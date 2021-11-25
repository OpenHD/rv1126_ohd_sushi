// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RKMEDIA_ADEC_
#define __RKMEDIA_ADEC_
#ifdef __cplusplus
extern "C" {
#endif
#include "rkmedia_common.h"

typedef struct rkADEC_ATTR_MP3_S {
  // reserved
} ADEC_ATTR_MP3_S;

typedef struct rkADEC_ATTR_MP2_S {
  // reserved
} ADEC_ATTR_MP2_S;

typedef struct rkADEC_ATTR_G711A_S {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
} ADEC_ATTR_G711A_S;

typedef struct rkADEC_ATTR_G711U_S {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
} ADEC_ATTR_G711U_S;

typedef struct rkADEC_ATTR_G726_S {
  // reserved
} ADEC_ATTR_G726_S;

typedef struct rkADEC_CHN_ATTR_S {
  CODEC_TYPE_E enCodecType;
  union {
    ADEC_ATTR_MP3_S stAdecMP3;
    ADEC_ATTR_MP2_S stAdecMP2;
    ADEC_ATTR_G711A_S stAdecG711A;
    ADEC_ATTR_G711U_S stAdecG711U;
    ADEC_ATTR_G726_S stAdecG726;
  };
} ADEC_CHN_ATTR_S;

#ifdef __cplusplus
}
#endif
#endif // #ifndef __RKMEDIA_ADEC_
