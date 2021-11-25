// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_ALSA_UTILS_H_
#define EASYMEDIA_ALSA_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/asoundlib.h>

#ifdef __cplusplus
}
#endif

#include <map>
#include <string>

#include "sound.h"

typedef enum rk_AI_LAYOUT_E {
  AI_LAYOUT_NORMAL = 0, /* Normal      */
  AI_LAYOUT_MIC_REF,    /* MIC + REF, do clear ref*/
  AI_LAYOUT_REF_MIC,    /* REF + MIC, do clear ref*/
  AI_LAYOUT_BUTT
} AI_LAYOUT_E;

#define ALSA_PCM                                                               \
  AUDIO_PCM                                                                    \
  TYPENEAR(AUDIO_G711A)                                                        \
  TYPENEAR(AUDIO_G711U)

snd_pcm_format_t SampleFormatToAlsaFormat(SampleFormat fmt);
int SampleFormatToInterleaved(SampleFormat fmt);
void ShowAlsaAvailableFormats(snd_pcm_t *handle, snd_pcm_hw_params_t *params);
int ParseAlsaParams(const char *param,
                    std::map<std::string, std::string> &params,
                    std::string &device, SampleInfo &sample_info,
                    AI_LAYOUT_E &layout);
#ifdef AUDIO_ALGORITHM_ENABLE
int ParseVQEParams(const char *param,
                   std::map<std::string, std::string> &params, bool *bVqeEnable,
                   VQE_CONFIG_S *stVqeConfig);
#endif

snd_pcm_t *AlsaCommonOpenSetHwParams(const char *device,
                                     snd_pcm_stream_t stream, int mode,
                                     SampleInfo &sample_info,
                                     snd_pcm_hw_params_t *hwparams);

#endif // EASYMEDIA_ALSA_UTILS_H_
