// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RKMEDIA_AI_
#define __RKMEDIA_AI_
#ifdef __cplusplus
extern "C" {
#endif
#include "rkmedia_common.h"

typedef enum rk_AI_LAYOUT_E {
  AI_LAYOUT_NORMAL = 0, /* Normal      */
  AI_LAYOUT_MIC_REF,    /* chanel layout: [mic:ref];*/
  AI_LAYOUT_REF_MIC,    /* chanel layout: [ref:mic];*/
  AI_LAYOUT_BUTT
} AI_LAYOUT_E;

typedef struct rkAI_CHN_ATTR_S {
  RK_CHAR *pcAudioNode;
  SAMPLE_FORMAT_E enSampleFormat;
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
  RK_U32 u32NbSamples;
  AI_LAYOUT_E enAiLayout;
} AI_CHN_ATTR_S;

#define AI_TALKVQE_MASK_AEC 0x1
#define AI_TALKVQE_MASK_ANR 0x2
#define AI_TALKVQE_MASK_AGC 0x4
typedef struct rkAI_TALKVQE_CONFIG_S {
  RK_U32 u32OpenMask;
  RK_S32 s32WorkSampleRate; /* aec sample rate: 8k /16k*/
  RK_S32 s32FrameSample;    /* aec samples per frame */
  RK_CHAR aParamFilePath[MAX_FILE_PATH_LEN];
} AI_TALKVQE_CONFIG_S;

#define AI_RECORDVQE_MASK_ANR 0x1
typedef struct rkAI_RECORDVQE_CONFIG_S {
  RK_U32 u32OpenMask;
  RK_S32 s32WorkSampleRate;
  RK_S32 s32FrameSample;
  struct {
    RK_FLOAT fPostAddGain; /* post-gain 0*/
    RK_FLOAT fGmin;        /* spectral gain floor,unit:(dB),default:-30dB */
    RK_FLOAT fNoiseFactor; /* noise suppression factor,default:0.98 */
    RK_FLOAT fHpfFc;       /* High Pass Filter cut-off frequency */
    RK_FLOAT fLpfFc;       /* Low Pass Filter cut-off frequency */
    RK_S32 enHpfSwitch;    /* HighPass Filter Enable */
    RK_S32 enLpfSwitch;    /* LowPass Filter Enable */
  } stAnrConfig;
} AI_RECORDVQE_CONFIG_S;

#ifdef __cplusplus
}
#endif
#endif // #ifndef __RKMEDIA_AI_
