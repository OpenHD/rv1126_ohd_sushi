// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RKMEDIA_AENC_
#define __RKMEDIA_AENC_
#ifdef __cplusplus
extern "C" {
#endif
#include "rkmedia_common.h"
typedef struct rkAENC_ATTR_MP3_S {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate; // 96000, 88200, 64000, 48000, 44100, 32000,
                        // 24000, 22050, 16000, 12000, 11025, 8000, 7350
} AENC_ATTR_MP3_S;

typedef struct rkAENC_ATTR_MP2_S {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate; // 44100, 48000,  32000, 22050, 24000, 16000, 0
} AENC_ATTR_MP2_S;

typedef struct rkAENC_ATTR_G711A_S {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
  RK_U32 u32NbSample;
} AENC_ATTR_G711A_S;

typedef struct rkANEC_ATTR_G711U_S {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
  RK_U32 u32NbSample;
} AENC_ATTR_G711U_S;

typedef struct rkAENC_ATTR_G726_S {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
} AENC_ATTR_G726_S;

typedef struct rkAENC_CHN_ATTR_S {
  CODEC_TYPE_E enCodecType; /*payload type ()*/
  RK_U32 u32Bitrate;
  RK_U32 u32Quality;
  union {
    AENC_ATTR_MP3_S stAencMP3;
    AENC_ATTR_MP2_S stAencMP2;
    AENC_ATTR_G711A_S stAencG711A;
    AENC_ATTR_G711U_S stAencG711U;
    AENC_ATTR_G726_S stAencG726;
  };

} AENC_CHN_ATTR_S;
#ifdef __cplusplus
}
#endif
#endif // #ifndef __RKMEDIA_AENC_
