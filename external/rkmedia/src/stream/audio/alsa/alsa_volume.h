// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_ALSA_VOLUME_H_
#define EASYMEDIA_ALSA_VOLUME_H_

#include <string>

/* Volume Limit range (0 - 100) */

int SetCaptureVolume(std::string card, int volume);

int GetCaptureVolume(std::string card, int &volume);

int SetPlaybackVolume(std::string card, int volume);

int GetPlaybackVolume(std::string card, int &volume);

#endif // EASYMEDIA_ALSA_VOLUME_H_
