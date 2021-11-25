// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_RK_AUDIO_H_
#define EASYMEDIA_RK_AUDIO_H_

#include <cstddef>

#include "alsa/alsa_utils.h"
#include "filter.h"
#include "sound.h"

typedef struct rkAUDIO_VQE_S AUDIO_VQE_S;

bool RK_AUDIO_VQE_Support();
AUDIO_VQE_S *RK_AUDIO_VQE_Init(const SampleInfo &sample_info,
                               AI_LAYOUT_E layout, VQE_CONFIG_S *config);
void RK_AUDIO_VQE_Deinit(AUDIO_VQE_S *handle);
int RK_AUDIO_VQE_Handle(AUDIO_VQE_S *handle, void *buffer, int bytes);
// void rk_audio_vqe_bind(AUDIO_VQE_S *tx, AUDIO_VQE_S *rx); //for
// aec tx rx

#endif // EASYMEDIA_RK_AUDIO_H_
