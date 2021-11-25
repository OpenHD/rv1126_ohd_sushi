// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stream.h"

#include <assert.h>
#include <errno.h>

#include "../rk_audio.h"
#include "alsa_utils.h"
#include "alsa_volume.h"
#include "buffer.h"
#include "media_type.h"
#include "utils.h"

namespace easymedia {

class AlsaPlayBackStream : public Stream {
public:
  static const int kStartDelays;
  static const int kPresetFrames;
  static const int kPresetSampleRate;
  static const int kPresetMinBufferSize;
  AlsaPlayBackStream(const char *param);
  virtual ~AlsaPlayBackStream();
  static const char *GetStreamName() { return "alsa_playback_stream"; }
  virtual size_t Read(void *ptr _UNUSED, size_t size _UNUSED,
                      size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int Seek(int64_t offset _UNUSED, int whence _UNUSED) final {
    return -1;
  }
  virtual long Tell() final { return -1; }
  virtual size_t Write(const void *ptr, size_t size, size_t nmemb) final;
  virtual bool Write(std::shared_ptr<MediaBuffer>);
  virtual int Open() final;
  virtual int Close() final;
  virtual int IoCtrl(unsigned long int request, ...) final;

private:
  size_t Writei(const void *ptr, size_t size, size_t nmemb);
  size_t Writen(const void *ptr, size_t size, size_t nmemb);

private:
  SampleInfo sample_info;
  std::string device;
  snd_pcm_t *alsa_handle;
  size_t frame_size;
  int interleaved;
  AI_LAYOUT_E layout;

#ifdef AUDIO_ALGORITHM_ENABLE
  bool bVqeEnable;
  VQE_CONFIG_S stVqeConfig;
  AUDIO_VQE_S *pstVqeHandle;
#endif
};

const int AlsaPlayBackStream::kStartDelays = 2; // number delays of periods
const int AlsaPlayBackStream::kPresetFrames = 1024;
const int AlsaPlayBackStream::kPresetSampleRate =
    48000; // the same to asound.conf
const int AlsaPlayBackStream::kPresetMinBufferSize = 8192;
AlsaPlayBackStream::AlsaPlayBackStream(const char *param)
    : alsa_handle(NULL), frame_size(0)
#ifdef AUDIO_ALGORITHM_ENABLE
      ,
      bVqeEnable(false), pstVqeHandle(NULL)
#endif
{
  memset(&sample_info, 0, sizeof(sample_info));
  sample_info.fmt = SAMPLE_FMT_NONE;
  std::map<std::string, std::string> params;
  int ret = ParseAlsaParams(param, params, device, sample_info, layout);
  UNUSED(ret);
  if (device.empty())
    device = "default";
  if (SampleInfoIsValid(sample_info))
    SetWriteable(true);
  else
    RKMEDIA_LOGI("missing some necessary param\n");
  interleaved = SampleFormatToInterleaved(sample_info.fmt);

#ifdef AUDIO_ALGORITHM_ENABLE
  memset(&stVqeConfig, 0, sizeof(stVqeConfig));
  stVqeConfig.u32VQEMode = VQE_MODE_BUTT;
#endif
}

AlsaPlayBackStream::~AlsaPlayBackStream() {
  if (alsa_handle)
    AlsaPlayBackStream::Close();
}

size_t AlsaPlayBackStream::Write(const void *ptr, size_t size, size_t nmemb) {
  if (interleaved)
    return Writei(ptr, size, nmemb);
  else
    return Writen(ptr, size, nmemb);
}

size_t AlsaPlayBackStream::Writei(const void *ptr, size_t size, size_t nmemb) {
  size_t buffer_len = size * nmemb;
  snd_pcm_sframes_t frames =
      (size == frame_size ? nmemb : buffer_len / frame_size);
  while (frames > 0) {
    // SND_PCM_ACCESS_RW_INTERLEAVED
    int status = snd_pcm_writei(alsa_handle, ptr, frames);
    if (status < 0) {
      if (status == -EAGAIN) {
        /* Apparently snd_pcm_recover() doesn't handle this case - does it
         * assume snd_pcm_wait() above? */
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        errno = EAGAIN;
        RKMEDIA_LOGI("ALSA write failed : %s\n", snd_strerror(status));
        return 0;
      }
      status = snd_pcm_recover(alsa_handle, status, 0);
      if (status < 0) {
        /* Hmm, not much we can do - abort */
        RKMEDIA_LOGI("ALSA write failed (unrecoverable): %s\n",
                     snd_strerror(status));
        errno = EIO;
        goto out;
      }
      errno = EIO;
      goto out;
    }
    frames -= status;
  }

out:
  return (buffer_len - frames * frame_size) / size;
}

size_t AlsaPlayBackStream::Writen(const void *ptr, size_t size, size_t nmemb) {
  uint8_t *bufs[32];
  int channels = sample_info.channels;
  size_t sample_size = frame_size / channels;
  size_t buffer_len = size * nmemb;
  snd_pcm_sframes_t frames =
      (size == frame_size ? nmemb : buffer_len / frame_size);

  for (int channel = 0; channel < channels; channel++)
    bufs[channel] = (uint8_t *)ptr + frames * sample_size * channel;

  while (frames > 0) {
    // SND_PCM_ACCESS_RW_NONINTERLEAVED
    int status = snd_pcm_writen(alsa_handle, (void **)bufs, frames);
    if (status < 0) {
      if (status == -EAGAIN) {
        /* Apparently snd_pcm_recover() doesn't handle this case - does it
         * assume snd_pcm_wait() above? */
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        errno = EAGAIN;
        return 0;
      }
      status = snd_pcm_recover(alsa_handle, status, 0);
      if (status < 0) {
        /* Hmm, not much we can do - abort */
        RKMEDIA_LOGI("ALSA write failed (unrecoverable): %s\n",
                     snd_strerror(status));
        errno = EIO;
        goto out;
      }
      errno = EIO;
      goto out;
    }
    frames -= status;
    for (int channel = 0; channel < channels; channel++)
      bufs[channel] += sample_size * status;
  }

out:
  return (buffer_len - frames * frame_size) / size;
}

bool AlsaPlayBackStream::Write(std::shared_ptr<MediaBuffer> mb) {
  if (mb->IsValid()) {
#ifdef AUDIO_ALGORITHM_ENABLE
    if (pstVqeHandle && !bVqeEnable) {
      RK_AUDIO_VQE_Deinit(pstVqeHandle);
      pstVqeHandle = NULL;
    }
    if (pstVqeHandle) {
      int ret =
          RK_AUDIO_VQE_Handle(pstVqeHandle, mb->GetPtr(), mb->GetValidSize());
      if (ret < 0)
        return 0;
    }
#endif
    Write(mb->GetPtr(), 1, mb->GetValidSize());
    return 0;
  }
  return -1;
  /*
  if (mb->IsValid()) {
    auto in = std::static_pointer_cast<SampleBuffer>(mb);
    Write(in->GetPtr(), in->GetSampleSize(), in->GetSamples());
    return 0;
  }*/
  return -1;
}

static int ALSA_finalize_hardware(snd_pcm_t *pcm_handle, uint32_t samples,
                                  int sample_size,
                                  snd_pcm_hw_params_t *hwparams,
                                  snd_pcm_uframes_t *period_size) {
  int status;
  snd_pcm_uframes_t bufsize;
  int ret = 0;

  status = snd_pcm_hw_params(pcm_handle, hwparams);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set pcm_hw_params: %s\n", snd_strerror(status));
    ret = -1;
    goto err;
  }

  /* Get samples for the actual buffer size */
  status = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't get buffer size: %s\n", snd_strerror(status));
    ret = -1;
    goto err;
  }

#ifndef NDEBUG
  if (bufsize != samples * sample_size) {
    RKMEDIA_LOGW("bufsize != samples * %d; %lu != %u * %d\n", sample_size,
                 bufsize, samples, sample_size);
  }
#else
  UNUSED(samples);
  UNUSED(sample_size);
#endif

err:
  snd_pcm_hw_params_get_period_size(hwparams, period_size, NULL);
#ifndef NDEBUG
  /* This is useful for debugging */
  do {
    unsigned int periods = 0;
    snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);
    RKMEDIA_LOGI("ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
                 *period_size, periods, bufsize);
  } while (0);
#endif
  return ret;
}

static int ALSA_set_period_size(snd_pcm_t *pcm_handle, uint32_t samples,
                                int sample_size, snd_pcm_hw_params_t *params,
                                snd_pcm_uframes_t *period_size) {
  int status;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_uframes_t frames;
  unsigned int periods;

  /* Copy the hardware parameters for this setup */
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_hw_params_copy(hwparams, params);

  frames = samples;
  status = snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &frames,
                                                  NULL);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set period size<%d> : %s\n", (int)frames,
                 snd_strerror(status));
    return -1;
  }

  // when enable dmixer in asound.conf, rv1108 need large buffersize
  if (samples * sample_size < AlsaPlayBackStream::kPresetMinBufferSize)
    periods = (AlsaPlayBackStream::kPresetMinBufferSize + frames - 1) / frames;
  else
    periods = (samples * sample_size + frames - 1) / frames;

  status =
      snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, NULL);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set periods<%d> : %s\n", periods,
                 snd_strerror(status));
    return -1;
  }

  return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams,
                                period_size);
}

static int ALSA_set_buffer_size(snd_pcm_t *pcm_handle, uint32_t samples,
                                int sample_size, snd_pcm_hw_params_t *params,
                                snd_pcm_uframes_t *period_size) {
  int status;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_uframes_t frames;
  /* Copy the hardware parameters for this setup */
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_hw_params_copy(hwparams, params);

  frames = samples * sample_size;
  if (frames < AlsaPlayBackStream::kPresetMinBufferSize)
    frames = AlsaPlayBackStream::kPresetMinBufferSize;
  status =
      snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &frames);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set buffer size<%d> : %s\n", (int)frames,
                 snd_strerror(status));
    return -1;
  }

  return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams,
                                period_size);
}

int AlsaPlayBackStream::Open() {
  if (device.empty())
    device = "default";
  snd_pcm_t *pcm_handle = NULL;
  snd_pcm_hw_params_t *hwparams = NULL;
  snd_pcm_sw_params_t *swparams = NULL;
  snd_pcm_uframes_t period_size = 0;
  uint32_t frames;
  if (!Writeable())
    return -1;
  int status = snd_pcm_hw_params_malloc(&hwparams);
  if (status < 0) {
    RKMEDIA_LOGI("snd_pcm_hw_params_malloc failed\n");
    return -1;
  }
  status = snd_pcm_sw_params_malloc(&swparams);
  if (status < 0) {
    RKMEDIA_LOGI("snd_pcm_sw_params_malloc failed\n");
    goto err;
  }
  pcm_handle = AlsaCommonOpenSetHwParams(
      device.c_str(), SND_PCM_STREAM_PLAYBACK, 0, sample_info, hwparams);
  if (!pcm_handle)
    goto err;
  frames = std::min<int>(kPresetFrames,
                         2 << (MATH_LOG2(sample_info.sample_rate *
                                         kPresetFrames / kPresetSampleRate) -
                               1));
  /* fix underrun, set period size not less than transport chunk size */
  frames = std::max<int>(sample_info.nb_samples, frames);
  frame_size = GetSampleSize(sample_info);
  if (frame_size == 0)
    goto err;
  if (ALSA_set_period_size(pcm_handle, frames, frame_size, hwparams,
                           &period_size) < 0 &&
      ALSA_set_buffer_size(pcm_handle, frames, frame_size, hwparams,
                           &period_size) < 0) {
    goto err;
  }
  status = snd_pcm_sw_params_current(pcm_handle, swparams);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't get alsa software config: %s\n",
                 snd_strerror(status));
    goto err;
  }
  status = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, period_size);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set minimum available period_size <%d>: %s\n",
                 (int)period_size, snd_strerror(status));
    goto err;
  }
  status = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams,
                                                 period_size * kStartDelays);
  if (status < 0) {
    RKMEDIA_LOGI("Unable to set start threshold mode for playback: %s\n",
                 snd_strerror(status));
    goto err;
  }
  status = snd_pcm_sw_params(pcm_handle, swparams);
  if (status < 0) {
    RKMEDIA_LOGI("Couldn't set software audio parameters: %s\n",
                 snd_strerror(status));
    goto err;
  }
  /* Switch to blocking mode for playback */
  // snd_pcm_nonblock(pcm_handle, 0);

  snd_pcm_hw_params_free(hwparams);
  snd_pcm_sw_params_free(swparams);
  alsa_handle = pcm_handle;
  return 0;

err:
  if (hwparams)
    snd_pcm_hw_params_free(hwparams);
  if (swparams)
    snd_pcm_sw_params_free(swparams);
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }
  return -1;
}

int AlsaPlayBackStream::Close() {
  if (alsa_handle) {
    snd_pcm_drop(alsa_handle);
    snd_pcm_close(alsa_handle);
    alsa_handle = NULL;
    RKMEDIA_LOGI("audio playback close done\n");
    return 0;
  }
  return -1;
}

int AlsaPlayBackStream::IoCtrl(unsigned long int request, ...) {
  va_list vl;
  va_start(vl, request);
  void *arg = va_arg(vl, void *);
  va_end(vl);
  if (!arg)
    return -1;
  int ret = 0;
  switch (request) {
  case S_ALSA_VOLUME:
    ret = SetPlaybackVolume(device, *((int *)arg));
    break;
  case G_ALSA_VOLUME:
    int volume;
    ret = GetPlaybackVolume(device, volume);
    *((int *)arg) = volume;
    break;
#ifdef AUDIO_ALGORITHM_ENABLE
  case S_VQE_ENABLE:
    bVqeEnable = *((int *)arg);
    if (bVqeEnable) {
      if (pstVqeHandle) {
        RKMEDIA_LOGI("already enabled\n");
        return -1;
      }
      if (stVqeConfig.u32VQEMode != VQE_MODE_AO) {
        RKMEDIA_LOGI("wrong u32VQEMode\n");
        return -1;
      }
      pstVqeHandle = RK_AUDIO_VQE_Init(sample_info, layout, &stVqeConfig);
      if (!pstVqeHandle)
        return -1;
    }
    break;
  case S_VQE_ATTR:
    if (bVqeEnable) {
      RKMEDIA_LOGI(
          "bVqeEnable already enable, please disable it before set attr");
      return -1;
    }
    stVqeConfig = *((VQE_CONFIG_S *)arg);
    break;
  case G_VQE_ATTR:
    *((VQE_CONFIG_S *)arg) = stVqeConfig;
    break;
#endif
  default:
    ret = -1;
    break;
  }
  return ret;
}

DEFINE_STREAM_FACTORY(AlsaPlayBackStream, Stream)

const char *FACTORY(AlsaPlayBackStream)::ExpectedInputDataType() {
  return ALSA_PCM;
}

const char *FACTORY(AlsaPlayBackStream)::OutPutDataType() { return nullptr; }

} // namespace easymedia
