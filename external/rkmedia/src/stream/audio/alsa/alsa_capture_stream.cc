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

class AlsaCaptureStream : public Stream {
public:
  AlsaCaptureStream(const char *param);
  virtual ~AlsaCaptureStream();
  static const char *GetStreamName() { return "alsa_capture_stream"; }
  virtual std::shared_ptr<MediaBuffer> Read();
  virtual size_t Read(void *ptr, size_t size, size_t nmemb) final;
  virtual int Seek(int64_t offset _UNUSED, int whence _UNUSED) final {
    return -1;
  }
  virtual long Tell() final { return -1; }
  virtual size_t Write(const void *ptr _UNUSED, size_t size _UNUSED,
                       size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int Open() final;
  virtual int Close() final;
  virtual int IoCtrl(unsigned long int request, ...) final;

private:
  size_t Readi(void *ptr, size_t size, size_t nmemb);
  size_t Readn(void *ptr, size_t size, size_t nmemb);

private:
  SampleInfo alsa_sample_info;   // for capture
  SampleInfo output_sample_info; // for output
  std::string device;
  snd_pcm_t *alsa_handle;
  size_t frame_size;
  int interleaved;
  int64_t buffer_time;
  int buffer_duration;
  AI_LAYOUT_E layout;

#ifdef AUDIO_ALGORITHM_ENABLE
  // for audio process, like aec/anr
  bool bVqeEnable;
  VQE_CONFIG_S stVqeConfig;
  AUDIO_VQE_S *pstVqeHandle;
#endif
};

AlsaCaptureStream::AlsaCaptureStream(const char *param)
    : alsa_handle(NULL), frame_size(0), buffer_time(-1), buffer_duration(-1),
      layout(AI_LAYOUT_NORMAL)
#ifdef AUDIO_ALGORITHM_ENABLE
      ,
      bVqeEnable(false), pstVqeHandle(NULL)
#endif
{
  memset(&output_sample_info, 0, sizeof(output_sample_info));
  output_sample_info.fmt = SAMPLE_FMT_NONE;
  std::map<std::string, std::string> params;
  int ret = ParseAlsaParams(param, params, device, output_sample_info, layout);
  UNUSED(ret);
  if (device.empty())
    device = "default";
  if (SampleInfoIsValid(output_sample_info))
    SetReadable(true);
  else
    RKMEDIA_LOGI("missing some necessary param\n");
  interleaved = SampleFormatToInterleaved(output_sample_info.fmt);

  alsa_sample_info = output_sample_info;
  if (layout == AI_LAYOUT_MIC_REF || layout == AI_LAYOUT_REF_MIC) {
    alsa_sample_info.channels = 2;
  }

#ifdef AUDIO_ALGORITHM_ENABLE
  memset(&stVqeConfig, 0, sizeof(stVqeConfig));
  stVqeConfig.u32VQEMode = VQE_MODE_BUTT;

  ParseVQEParams(param, params, &bVqeEnable, &stVqeConfig);
  RKMEDIA_LOGD("VqeEnable is %d\n", bVqeEnable);
  RKMEDIA_LOGD("VqeConfig.u32VQEMode is %d\n", stVqeConfig.u32VQEMode);
  RKMEDIA_LOGD("OpenMask is %d\n", stVqeConfig.stAiTalkConfig.u32OpenMask);
  RKMEDIA_LOGD("WorkSampleRate is %d\n",
               stVqeConfig.stAiTalkConfig.s32WorkSampleRate);
  RKMEDIA_LOGD("FrameSample is %d\n",
               stVqeConfig.stAiTalkConfig.s32FrameSample);
  RKMEDIA_LOGD("ParamFilePath is %s\n",
               stVqeConfig.stAiTalkConfig.aParamFilePath);
  if (bVqeEnable)
    pstVqeHandle = RK_AUDIO_VQE_Init(alsa_sample_info, layout, &stVqeConfig);
#endif

  RKMEDIA_LOGI("%s: Layout %d, output chan %d, alsa chan %d\n", __func__,
               layout, output_sample_info.channels, alsa_sample_info.channels);
}

AlsaCaptureStream::~AlsaCaptureStream() {
  if (alsa_handle)
    AlsaCaptureStream::Close();
  buffer_time = -1;
  buffer_duration = -1;
}

size_t AlsaCaptureStream::Read(void *ptr, size_t size, size_t nmemb) {
  if (interleaved)
    return Readi(ptr, size, nmemb);
  else
    return Readn(ptr, size, nmemb);
}

size_t AlsaCaptureStream::Readi(void *vptr, size_t size, size_t nmemb) {
  uint8_t *ptr = (uint8_t *)vptr;
  size_t buffer_len = size * nmemb;
  snd_pcm_sframes_t gotten = 0;
  snd_pcm_sframes_t nb_samples =
      (size == frame_size ? nmemb : buffer_len / frame_size);
  while (nb_samples > 0) {
    int status = snd_pcm_readi(alsa_handle, ptr, nb_samples);
    if (status < 0) {
      if (status == -EAGAIN) {
        /* Apparently snd_pcm_recover() doesn't handle this case - does it
         * assume snd_pcm_wait() above? */
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        errno = EAGAIN;
        break;
      }
      status = snd_pcm_recover(alsa_handle, status, 0);
      if (status < 0) {
        /* Hmm, not much we can do - abort */
        RKMEDIA_LOGI("ALSA write failed (unrecoverable): %s\n",
                     snd_strerror(status));
        errno = EIO;
        break;
      }
      errno = EIO;
      break;
    }
    nb_samples -= status;
    gotten += status;
    ptr += status * frame_size;
  }

  return gotten * frame_size / size;
}

size_t AlsaCaptureStream::Readn(void *ptr, size_t size, size_t nmemb) {
  uint8_t *bufs[32];
  int channels = alsa_sample_info.channels;
  size_t sample_size = frame_size / channels;
  size_t buffer_len = size * nmemb;
  snd_pcm_sframes_t gotten = 0;
  snd_pcm_sframes_t nb_samples =
      (size == frame_size ? nmemb : buffer_len / frame_size);

  for (int channel = 0; channel < channels; channel++)
    bufs[channel] = (uint8_t *)ptr + nb_samples * sample_size * channel;

  while (nb_samples > 0) {
    int status = snd_pcm_readn(alsa_handle, (void **)bufs, nb_samples);
    if (status < 0) {
      if (status == -EAGAIN) {
        /* Apparently snd_pcm_recover() doesn't handle this case - does it
         * assume snd_pcm_wait() above? */
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        errno = EAGAIN;
        break;
      }
      status = snd_pcm_recover(alsa_handle, status, 0);
      if (status < 0) {
        /* Hmm, not much we can do - abort */
        RKMEDIA_LOGI("ALSA write failed (unrecoverable): %s\n",
                     snd_strerror(status));
        errno = EIO;
        break;
      }
      errno = EIO;
      break;
    }
    nb_samples -= status;
    gotten += status;
    for (int channel = 0; channel < channels; channel++)
      bufs[channel] += sample_size * status;
  }

  return gotten * frame_size / size;
}

std::shared_ptr<MediaBuffer> AlsaCaptureStream::Read() {
  int buffer_size = frame_size * alsa_sample_info.nb_samples;
  int read_cnt = -1;
  int output_frame_size = frame_size;

  auto sample_buffer = std::make_shared<easymedia::SampleBuffer>(
      MediaBuffer::Alloc2(buffer_size), alsa_sample_info);

  if (!sample_buffer) {
    RKMEDIA_LOGI("Alloc audio frame buffer failed:%d,%zu!\n", buffer_size,
                 frame_size);
    return nullptr;
  }
  if (buffer_time == -1 || buffer_duration == -1) {
    struct timespec crt_tm = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &crt_tm);
    buffer_time = crt_tm.tv_sec * 1000000LL + crt_tm.tv_nsec / 1000;
    buffer_duration =
        alsa_sample_info.nb_samples * 1000000LL / alsa_sample_info.sample_rate;
  }

#ifdef AUDIO_ALGORITHM_ENABLE
read_one_frame:
#endif

  read_cnt =
      Read(sample_buffer->GetPtr(), frame_size, alsa_sample_info.nb_samples);

#ifdef AUDIO_ALGORITHM_ENABLE
  // dynamic close audio vqe
  if (pstVqeHandle && !bVqeEnable) {
    RK_AUDIO_VQE_Deinit(pstVqeHandle);
    pstVqeHandle = NULL;
  }

  if (pstVqeHandle && read_cnt > 0) {
    int ret = RK_AUDIO_VQE_Handle(pstVqeHandle, sample_buffer->GetPtr(),
                                  read_cnt * frame_size);
    if (ret < 0)
      goto read_one_frame;
  }
#endif

  if (read_cnt > 0 &&
      (layout == AI_LAYOUT_MIC_REF && output_sample_info.channels == 1)) {
    int16_t *in = (int16_t *)sample_buffer->GetPtr();
    int16_t *out = in;
    for (int j = 0; j < read_cnt; j++) {
      *out++ = *in++;
      in++;
    }
    sample_buffer->SetChannels(1);
    output_frame_size = frame_size / 2;
    sample_buffer->SetSize(buffer_size / 2); // fix for MP3
  } else if (read_cnt > 0 && (layout == AI_LAYOUT_REF_MIC &&
                              output_sample_info.channels == 1)) {
    int16_t *in = (int16_t *)sample_buffer->GetPtr();
    int16_t *out = in;
    for (int j = 0; j < read_cnt; j++) {
      in++;
      *out++ = *in++;
    }
    sample_buffer->SetChannels(1);
    output_frame_size = frame_size / 2;
    sample_buffer->SetSize(buffer_size / 2); // fix for MP3
  }

  sample_buffer->SetValidSize(read_cnt * output_frame_size);
  sample_buffer->SetSamples(read_cnt);
  sample_buffer->SetUSTimeStamp(buffer_time);
  buffer_time += buffer_duration;

  return sample_buffer;
}

int AlsaCaptureStream::Open() {
  snd_pcm_t *pcm_handle = NULL;
  snd_pcm_hw_params_t *hwparams = NULL;
  if (!Readable())
    return -1;
  int status = snd_pcm_hw_params_malloc(&hwparams);
  if (status < 0) {
    RKMEDIA_LOGI("snd_pcm_hw_params_malloc failed\n");
    return -1;
  }
  pcm_handle = AlsaCommonOpenSetHwParams(device.c_str(), SND_PCM_STREAM_CAPTURE,
                                         0, alsa_sample_info, hwparams);
  if (!pcm_handle)
    goto err;
  if ((status = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
    RKMEDIA_LOGI("cannot set parameters (%s)\n", snd_strerror(status));
    goto err;
  }
#ifndef NDEBUG
  /* This is useful for debugging */
  do {
    unsigned int periods = 0;
    snd_pcm_uframes_t period_size = 0;
    snd_pcm_uframes_t bufsize = 0;
    snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);
    snd_pcm_hw_params_get_period_size(hwparams, &period_size, NULL);
    snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    RKMEDIA_LOGI("ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
                 period_size, periods, bufsize);
  } while (0);
#endif
  if ((status = snd_pcm_prepare(pcm_handle)) < 0) {
    RKMEDIA_LOGI("cannot prepare audio interface for use (%s)\n",
                 snd_strerror(status));
    goto err;
  }
  /* Switch to blocking mode for capture */
  // snd_pcm_nonblock(pcm_handle, 0);

  snd_pcm_hw_params_free(hwparams);
  frame_size = snd_pcm_frames_to_bytes(pcm_handle, 1);
  alsa_handle = pcm_handle;
  return 0;

err:
  RKMEDIA_LOGI("AlsaCaptureStream::Open() failed\n");
  if (hwparams)
    snd_pcm_hw_params_free(hwparams);
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }
  return -1;
}

int AlsaCaptureStream::Close() {
  buffer_time = -1;
  buffer_duration = -1;
  if (alsa_handle) {
    snd_pcm_drop(alsa_handle);
    snd_pcm_close(alsa_handle);
    alsa_handle = NULL;
    RKMEDIA_LOGI("audio capture close done\n");
    return 0;
  }
  return -1;
}

int AlsaCaptureStream::IoCtrl(unsigned long int request, ...) {
  va_list vl;
  va_start(vl, request);
  void *arg = va_arg(vl, void *);
  va_end(vl);
  if (!arg)
    return -1;
  int ret = 0;
  switch (request) {
  case S_ALSA_VOLUME:
    ret = SetCaptureVolume(device, *((int *)arg));
    break;
  case G_ALSA_VOLUME:
    int volume;
    ret = GetCaptureVolume(device, volume);
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
      if (stVqeConfig.u32VQEMode == VQE_MODE_BUTT ||
          stVqeConfig.u32VQEMode == VQE_MODE_AO) {
        RKMEDIA_LOGI("wrong u32VQEMode\n");
        return -1;
      }
      pstVqeHandle = RK_AUDIO_VQE_Init(alsa_sample_info, layout, &stVqeConfig);
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

DEFINE_STREAM_FACTORY(AlsaCaptureStream, Stream)

const char *FACTORY(AlsaCaptureStream)::ExpectedInputDataType() {
  return nullptr;
}

const char *FACTORY(AlsaCaptureStream)::OutPutDataType() { return ALSA_PCM; }

} // namespace easymedia
