// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RKMEDIA_AO_
#define __RKMEDIA_AO_
#ifdef __cplusplus
extern "C" {
#endif
#include "rkmedia_common.h"
typedef struct rkAO_CHN_ATTR_S {
  RK_CHAR *pcAudioNode;
  SAMPLE_FORMAT_E enSampleFormat;
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
  RK_U32 u32NbSamples;
} AO_CHN_ATTR_S;

#define AO_VQE_MASK_ANR 0x1
#define AO_VQE_MASK_AGC 0x2

typedef struct rkAO_VQE_CONFIG_S {
  RK_U32 u32OpenMask;
  RK_S32 s32WorkSampleRate;
  RK_S32 s32FrameSample;
  RK_CHAR aParamFilePath[MAX_FILE_PATH_LEN];
} AO_VQE_CONFIG_S;

typedef struct rkAO_CHN_STATE_S {
  RK_U32 u32ChnTotalNum;
  RK_U32 u32ChnFreeNum;
  RK_U32 u32ChnBusyNum;
} AO_CHN_STATE_S;

#ifdef __cplusplus
}
#endif
#endif // #ifndef __RKMEDIA_AO_
