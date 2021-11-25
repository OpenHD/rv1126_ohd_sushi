// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "alsa_utils.h"

#include "key_string.h"
#include "utils.h"

static const struct SampleFormatEntry {
  SampleFormat fmt;
  snd_pcm_format_t alsa_fmt;
  int interleaved;
} sample_format_alsa_map[] = {{SAMPLE_FMT_U8, SND_PCM_FORMAT_U8, 1},
                              {SAMPLE_FMT_S16, SND_PCM_FORMAT_S16_LE, 1},
                              {SAMPLE_FMT_S32, SND_PCM_FORMAT_S32_LE, 1},
                              {SAMPLE_FMT_FLT, SND_PCM_FORMAT_FLOAT_LE, 1},
                              {SAMPLE_FMT_U8P, SND_PCM_FORMAT_U8, 0},
                              {SAMPLE_FMT_S16P, SND_PCM_FORMAT_S16_LE, 0},
                              {SAMPLE_FMT_S32P, SND_PCM_FORMAT_S32_LE, 0},
                              {SAMPLE_FMT_FLTP, SND_PCM_FORMAT_FLOAT_LE, 0},
                              {SAMPLE_FMT_G711A, SND_PCM_FORMAT_A_LAW, 1},
                              {SAMPLE_FMT_G711U, SND_PCM_FORMAT_MU_LAW, 1}};

snd_pcm_format_t SampleFormatToAlsaFormat(SampleFormat fmt) {
  FIND_ENTRY_TARGET(fmt, sample_format_alsa_map, fmt, alsa_fmt)
  return SND_PCM_FORMAT_UNKNOWN;
}

int SampleFormatToInterleaved(SampleFormat fmt) {
  FIND_ENTRY_TARGET(fmt, sample_format_alsa_map, fmt, interleaved)
  return 1;
}

void ShowAlsaAvailableFormats(snd_pcm_t *handle, snd_pcm_hw_params_t *params) {
  snd_pcm_format_t format;
  printf("Available formats:\n");
  for (int i = 0; i <= SND_PCM_FORMAT_LAST; i++) {
    format = static_cast<snd_pcm_format_t>(i);
    if (snd_pcm_hw_params_test_format(handle, params, format) == 0)
      printf("- %s\n", snd_pcm_format_name(format));
  }
}

int ParseAlsaParams(const char *param,
                    std::map<std::string, std::string> &params,
                    std::string &device, SampleInfo &sample_info,
                    AI_LAYOUT_E &layout) {
  int ret = 0;
  if (!easymedia::parse_media_param_map(param, params))
    return 0;
  for (auto &p : params) {
    const std::string &key = p.first;
    if (key == KEY_SAMPLE_FMT) {
      SampleFormat fmt = StringToSampleFmt(p.second.c_str());
      if (fmt == SAMPLE_FMT_NONE) {
        RKMEDIA_LOGI("unknown pcm fmt: %s\n", p.second.c_str());
        return 0;
      }
      sample_info.fmt = fmt;
      ret++;
    } else if (key == KEY_CHANNELS) {
      sample_info.channels = stoi(p.second);
      ret++;
    } else if (key == KEY_SAMPLE_RATE) {
      sample_info.sample_rate = stoi(p.second);
      ret++;
    } else if (key == KEY_DEVICE) {
      device = p.second;
      ret++;
    } else if (key == KEY_FRAMES) {
      sample_info.nb_samples = stoi(p.second);
      ret++;
    } else if (key == KEY_LAYOUT) {
      layout = (AI_LAYOUT_E)stoi(p.second);
      ret++;
    }
  }
  return ret;
}

#ifdef AUDIO_ALGORITHM_ENABLE
int ParseVQEParams(const char *param,
                   std::map<std::string, std::string> &params, bool *bVqeEnable,
                   VQE_CONFIG_S *stVqeConfig) {
  int ret = 0;
  if (!easymedia::parse_media_param_map(param, params))
    return 0;
  for (auto &p : params) {
    const std::string &key = p.first;
    if (key == KEY_VQE_ENABLE) {
      *bVqeEnable = stoi(p.second);
    } else if (key == KEY_VQE_MODE) {
      stVqeConfig->u32VQEMode = (VQE_MODE_E)stoi(p.second);
    }
  }
  if ((*bVqeEnable == 0) || (stVqeConfig->u32VQEMode > VQE_MODE_AO))
    return 0;

  if (stVqeConfig->u32VQEMode == VQE_MODE_AI_TALK) {
    for (auto &p : params) {
      const std::string &key = p.first;
      if (key == KEY_VQE_OPEN_MASK)
        stVqeConfig->stAiTalkConfig.u32OpenMask = stoi(p.second);
      else if (key == KEY_VQE_WORK_SAMPLE_RATE)
        stVqeConfig->stAiTalkConfig.s32WorkSampleRate = stoi(p.second);
      else if (key == KEY_VQE_FRAME_SAMPLE)
        stVqeConfig->stAiTalkConfig.s32FrameSample = stoi(p.second);
      else if (key == KEY_VQE_PARAM_FILE_PATH)
        strcpy(stVqeConfig->stAiTalkConfig.aParamFilePath, p.second.c_str());
    }
  } else if (stVqeConfig->u32VQEMode == VQE_MODE_AI_RECORD) {
    for (auto &p : params) {
      const std::string &key = p.first;
      if (key == KEY_VQE_OPEN_MASK)
        stVqeConfig->stAiRecordConfig.u32OpenMask = stoi(p.second);
      else if (key == KEY_VQE_WORK_SAMPLE_RATE)
        stVqeConfig->stAiRecordConfig.s32WorkSampleRate = stoi(p.second);
      else if (key == KEY_VQE_FRAME_SAMPLE)
        stVqeConfig->stAiRecordConfig.s32FrameSample = stoi(p.second);
      else if (key == KEY_ANR_POST_ADD_GAIN)
        stVqeConfig->stAiRecordConfig.stAnrConfig.fPostAddGain = stoi(p.second);
      else if (key == KEY_ANR_GMIN)
        stVqeConfig->stAiRecordConfig.stAnrConfig.fGmin = stoi(p.second);
      else if (key == KEY_ANR_NOISE_FACTOR)
        stVqeConfig->stAiRecordConfig.stAnrConfig.fNoiseFactor = stoi(p.second);
    }
  } else if (stVqeConfig->u32VQEMode == VQE_MODE_AO) {
    for (auto &p : params) {
      const std::string &key = p.first;
      if (key == KEY_VQE_OPEN_MASK)
        stVqeConfig->stAoConfig.u32OpenMask = stoi(p.second);
      else if (key == KEY_VQE_WORK_SAMPLE_RATE)
        stVqeConfig->stAoConfig.s32WorkSampleRate = stoi(p.second);
      else if (key == KEY_VQE_FRAME_SAMPLE)
        stVqeConfig->stAoConfig.s32FrameSample = stoi(p.second);
      else if (key == KEY_VQE_PARAM_FILE_PATH)
        strcpy(stVqeConfig->stAoConfig.aParamFilePath, p.second.c_str());
    }
  }

  return ret;
}
#endif

// open device, and set format/channel/samplerate.
snd_pcm_t *AlsaCommonOpenSetHwParams(const char *device,
                                     snd_pcm_stream_t stream, int mode,
                                     SampleInfo &sample_info,
                                     snd_pcm_hw_params_t *hwparams) {
  snd_pcm_t *pcm_handle = NULL;
  unsigned int rate = sample_info.sample_rate;
  unsigned int channels;
  snd_pcm_format_t pcm_fmt = SampleFormatToAlsaFormat(sample_info.fmt);
  int interleaved = SampleFormatToInterleaved(sample_info.fmt);
  if (pcm_fmt == SND_PCM_FORMAT_UNKNOWN)
    return NULL;

  int status = snd_pcm_open(&pcm_handle, device, stream, mode);
  if (status < 0 || !pcm_handle) {
    RKMEDIA_LOGI("audio open error: %s\n", snd_strerror(status));
    goto err;
  }

  status = snd_pcm_hw_params_any(pcm_handle, hwparams);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't get hardware config: %s\n", snd_strerror(status));
    goto err;
  }
#ifndef NDEBUG
  {
    snd_output_t *log = NULL;
    snd_output_stdio_attach(&log, stderr, 0);
    // fprintf(stderr, "HW Params of device \"%s\":\n",
    //        snd_pcm_name(pcm_handle));
    RKMEDIA_LOGI("--------------------\n");
    snd_pcm_hw_params_dump(hwparams, log);
    RKMEDIA_LOGI("--------------------\n");
    snd_output_close(log);
  }
#endif

  if (interleaved)
    status = snd_pcm_hw_params_set_access(pcm_handle, hwparams,
                                          SND_PCM_ACCESS_RW_INTERLEAVED);
  else
    status = snd_pcm_hw_params_set_access(pcm_handle, hwparams,
                                          SND_PCM_ACCESS_RW_NONINTERLEAVED);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set access type: %s\n", snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_set_format(pcm_handle, hwparams, pcm_fmt);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't find any hardware audio formats\n");
    ShowAlsaAvailableFormats(pcm_handle, hwparams);
    goto err;
  }
  status = snd_pcm_hw_params_set_channels(pcm_handle, hwparams,
                                          sample_info.channels);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set audio channels<%d>: %s\n", sample_info.channels,
                 snd_strerror(status));
    goto err;
  }
  status = snd_pcm_hw_params_get_channels(hwparams, &channels);
  if (status < 0 || channels != (unsigned int)sample_info.channels) {
    RKMEDIA_LOGI(
        "final channels do not match expected, %d != %d. resample require.\n",
        channels, sample_info.channels);
    goto err;
  }
  status = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, NULL);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set audio frequency<%d>: %s\n",
                 sample_info.sample_rate, snd_strerror(status));
    goto err;
  }
  if (rate != (unsigned int)sample_info.sample_rate) {
    RKMEDIA_LOGI("final sample rate do not match expected, %d != %d. resample "
                 "require.\n",
                 rate, sample_info.sample_rate);
    goto err;
  }

  return pcm_handle;

err:
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }
  return NULL;
}
