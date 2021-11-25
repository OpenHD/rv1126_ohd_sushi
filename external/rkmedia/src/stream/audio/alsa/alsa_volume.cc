// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "alsa_volume.h"
#include "alsa_utils.h"
#include "utils.h"

typedef enum {
  GET,
  SET,
} OPS;

typedef enum {
  CAPTURE_DEV,
  PLAYBACK_DEV,
} SUB_DEVICE;

static int mixer_control(const char *device, SUB_DEVICE subdev, OPS ops,
                         int &volume) {
  long volMin, volMax, leftVal, rightVal;
  int ret;
  snd_mixer_t *handle;
  snd_mixer_elem_t *elem;
  // snd_mixer_selem_id_t *sid;

  // snd_mixer_selem_id_alloca(&sid);
  volMin = 0;
  volMax = 0;
  leftVal = 0;
  rightVal = 0;
  handle = NULL;

  if ((ret = snd_mixer_open(&handle, 0)) < 0) {
    RKMEDIA_LOGI("snd_mixer_open failed\n");
    goto exit;
  }

  ret = snd_mixer_attach(handle, device);
  if (ret < 0) {
    RKMEDIA_LOGI("snd_mixer_attach failed\n");
    goto exit;
  }
  snd_mixer_selem_register(handle, NULL, NULL);
  snd_mixer_load(handle);

  elem = snd_mixer_first_elem(handle);
  while (elem) {
    if (snd_mixer_selem_is_active(elem)) {
      if ((subdev == CAPTURE_DEV) && snd_mixer_selem_has_capture_volume(elem))
        break;
      if ((subdev == PLAYBACK_DEV) && snd_mixer_selem_has_playback_volume(elem))
        break;
    }
    elem = snd_mixer_elem_next(elem);
  }

  if (!elem) {
    RKMEDIA_LOGI("snd_mixer_find_selem Err\n");
    ret = -ENOENT;
    goto exit;
  }

  if (subdev == CAPTURE_DEV) {
    snd_mixer_selem_get_capture_volume_range(elem, &volMin, &volMax);
    if (ops == GET) {
      snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT,
                                         &leftVal);
      snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT,
                                         &rightVal);
      volume = ((leftVal + rightVal) >> 1) * 100 / (volMax - volMin);
    } else {
      leftVal = volume * (volMax - volMin) / 100;
      snd_mixer_selem_set_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT,
                                         leftVal);
      snd_mixer_selem_set_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT,
                                         leftVal);
    }
  } else {
    snd_mixer_selem_get_playback_volume_range(elem, &volMin, &volMax);
    if (ops == GET) {
      snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT,
                                          &leftVal);
      snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT,
                                          &rightVal);
      volume = ((leftVal + rightVal) >> 1) * 100 / (volMax - volMin);
    } else {
      leftVal = volume * (volMax - volMin) / 100;
      snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT,
                                          leftVal);
      snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT,
                                          leftVal);
    }
  }
exit:
  if (handle)
    snd_mixer_close(handle);
  return ret;
}

int SetCaptureVolume(std::string card, int volume) {
  return mixer_control(card.c_str(), CAPTURE_DEV, SET, volume);
}

int GetCaptureVolume(std::string card, int &volume) {
  return mixer_control(card.c_str(), CAPTURE_DEV, GET, volume);
}

int SetPlaybackVolume(std::string card, int volume) {
  return mixer_control(card.c_str(), PLAYBACK_DEV, SET, volume);
}

int GetPlaybackVolume(std::string card, int &volume) {
  return mixer_control(card.c_str(), PLAYBACK_DEV, GET, volume);
}
