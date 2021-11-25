// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <string.h>

#include "alsa/alsa_utils.h"
#include "rk_audio.h"
#include "sound.h"

#include "utils.h"

#ifdef AUDIO_ALGORITHM_ENABLE
extern "C" {
#include <RKAP_3A.h>
#include <RKAP_ANR.h>
}

#define RK_AUDIO_BUFFER_MAX_SIZE 12288
#define ALGO_FRAME_TIMS_MS 20 // 20ms
typedef struct rkAUDIO_QUEUE_S {
  int buffer_size;
  char *buffer;
  int roffset;
} AUDIO_QUEUE_S;
struct rkAUDIO_VQE_S {
  AUDIO_QUEUE_S *in_queue;  /* for before process */
  AUDIO_QUEUE_S *out_queue; /* for after process */
  VQE_CONFIG_S stVqeConfig;
  SampleInfo sample_info;
  RKAP_Handle ap_handle;
  AI_LAYOUT_E layout;
};

static int16_t get_bit_width(SampleFormat fmt) {
  int16_t bit_width = -1;

  switch (fmt) {
  case SAMPLE_FMT_U8:
  case SAMPLE_FMT_U8P:
    bit_width = 8;
    break;
  case SAMPLE_FMT_S16:
  case SAMPLE_FMT_S16P:
    bit_width = 16;
    break;
  case SAMPLE_FMT_S32:
  case SAMPLE_FMT_S32P:
    bit_width = 32;
    break;
  case SAMPLE_FMT_FLT:
  case SAMPLE_FMT_FLTP:
    bit_width = sizeof(float);
    break;
  case SAMPLE_FMT_G711A:
  case SAMPLE_FMT_G711U:
    bit_width = 8;
    break;
  default:
    break;
  }

  return bit_width;
}

static AUDIO_QUEUE_S *queue_create(int buf_size) {
  AUDIO_QUEUE_S *queue = (AUDIO_QUEUE_S *)calloc(sizeof(AUDIO_QUEUE_S), 1);
  if (!queue)
    return NULL;
  queue->buffer = (char *)malloc(buf_size);
  return queue;
}
static void queue_free(AUDIO_QUEUE_S *queue) {
  if (queue) {
    if (queue->buffer)
      free(queue->buffer);
    free(queue);
  }
}

static int queue_size(AUDIO_QUEUE_S *queue) { return queue->buffer_size; }

static int queue_write(AUDIO_QUEUE_S *queue, const unsigned char *buffer,
                       int bytes) {
  if ((buffer == NULL) || (bytes <= 0)) {
    RKMEDIA_LOGI("queue_capture_buffer buffer error!");
    return -1;
  }
  if (queue->buffer_size + bytes > RK_AUDIO_BUFFER_MAX_SIZE) {
    RKMEDIA_LOGI("unexpected cap buffer size too big!! return!");
    return -1;
  }

  memcpy((char *)queue->buffer + queue->buffer_size, (char *)buffer, bytes);
  queue->buffer_size += bytes;
  return bytes;
}

static int queue_read(AUDIO_QUEUE_S *queue, unsigned char *buffer, int bytes) {
  if ((buffer == NULL) || (bytes <= 0)) {
    RKMEDIA_LOGI("queue_capture_buffer buffer error!");
    return -1;
  }
  if ((queue->roffset + bytes) > queue->buffer_size) {
    RKMEDIA_LOGI("queue_read  error!");
    return -1;
  }
  memcpy(buffer, queue->buffer + queue->roffset, bytes);
  queue->roffset += bytes;
  return bytes;
}

static void queue_tune(AUDIO_QUEUE_S *queue) {
  /* Copy the rest of the sample to the beginning of the Buffer */
  memcpy(queue->buffer, queue->buffer + queue->roffset,
         queue->buffer_size - queue->roffset);
  queue->buffer_size = queue->buffer_size - queue->roffset;
  queue->roffset = 0;
}

int AI_TALKVQE_Init(AUDIO_VQE_S *handle, VQE_CONFIG_S *config) {
  SampleInfo sample_info = handle->sample_info;

  // 1. check params
  if (!(sample_info.fmt == SAMPLE_FMT_S16 &&
        (sample_info.sample_rate == 8000 ||
         sample_info.sample_rate == 16000))) {
    RKMEDIA_LOGI("check failed: sample_info.fmt == SAMPLE_FMT_S16 && \
			(sample_info.sample_rate == 8000 || sample_info.sample_rate == 16000))");
    return -1;
  }
  if (sample_info.channels != 2) {
    RKMEDIA_LOGI("check failed: aec channels must equal 2");
    return -1;
  }
  if (handle->layout != AI_LAYOUT_MIC_REF &&
      handle->layout != AI_LAYOUT_REF_MIC) {
    RKMEDIA_LOGI("check failed: enAiLayout must be AI_LAYOUT_MIC_REF or "
                 "AI_LAYOUT_REF_MIC\n");
    return -1;
  }

  RKAP_AEC_State state;
  state.swSampleRate = sample_info.sample_rate; // 8k|16k
  state.swFrameLen =
      ALGO_FRAME_TIMS_MS * sample_info.sample_rate / 1000; // hard code 20ms
  state.pathPara = config->stAiTalkConfig.aParamFilePath;
  RKMEDIA_LOGI("AEC: param file = %s\n", state.pathPara);
  RKAP_3A_DumpVersion();
  handle->ap_handle = RKAP_3A_Init(&state, AEC_TX_TYPE);
  if (!handle->ap_handle) {
    RKMEDIA_LOGI("AEC: init failed\n");
    return -1;
  }
  return 0;
}

int AI_TALKVQE_Deinit(AUDIO_VQE_S *handle) {
  RKAP_3A_Destroy(handle->ap_handle);
  return 0;
}

static int AI_TALKVQE_Process(AUDIO_VQE_S *handle, unsigned char *in,
                              unsigned char *out) {
  // for hardware refs signal
  int16_t *sigin;
  int16_t *sigref;
  int16_t bytes =
      handle->stVqeConfig.stAiTalkConfig.s32FrameSample * 4; // S16 && 2 channel

  unsigned char prebuf[bytes] = {0};
  unsigned char sigout[bytes / 2] = {0};
  int nb_samples = bytes / 4;

  sigin = (int16_t *)prebuf;
  sigref = (int16_t *)prebuf + nb_samples;
  if (handle->layout == AI_LAYOUT_MIC_REF) {
    for (int i = 0; i < nb_samples; i++) {
      sigin[i] = *((int16_t *)in + i * 2);
      sigref[i] = *((int16_t *)in + i * 2 + 1);
    }
  } else if (handle->layout == AI_LAYOUT_REF_MIC) {
    for (int i = 0; i < nb_samples; i++) {
      sigref[i] = *((int16_t *)in + i * 2);
      sigin[i] = *((int16_t *)in + i * 2 + 1);
    }
  }
  RKAP_3A_Process(handle->ap_handle, sigin, sigref, (int16_t *)sigout);

  int16_t *tmp2 = (int16_t *)sigout;
  int16_t *tmp1 = (int16_t *)out;
  for (int j = 0; j < nb_samples; j++) {
    *tmp1++ = *tmp2;
    *tmp1++ = *tmp2++;
  }
  return 0;
}

int AI_RECORDVQE_Init(AUDIO_VQE_S *handle, VQE_CONFIG_S *config) {
  SampleInfo sample_info = handle->sample_info;

  if (!(sample_info.fmt == SAMPLE_FMT_S16 && sample_info.sample_rate >= 8000 &&
        sample_info.sample_rate <= 48000)) {
    RKMEDIA_LOGI("check failed: fmt == SAMPLE_FMT_S16 && sample_rate >= 8000 "
                 "&& sample_rate <= 48000");
    return -1;
  }
  if (handle->layout == AI_LAYOUT_NORMAL) {
    if (sample_info.channels != 1) {
      RKMEDIA_LOGI(
          "ANR check failed: layout == AI_LAYOUT_NORMAL, channels must be 1\n");
      return -1;
    }
  } else {
    if (sample_info.channels != 2) {
      RKMEDIA_LOGI(
          "ANR check failed: layout != AI_LAYOUT_NORMAL, channels must be 2\n");
      return -1;
    }
  }

  RKAP_ANR_State state;
  state.swSampleRate = sample_info.sample_rate; // 8k|16k; // 8-48k
  state.swFrameLen =
      ALGO_FRAME_TIMS_MS * sample_info.sample_rate / 1000; // hard code 20ms
  state.fGmin = config->stAiRecordConfig.stAnrConfig.fGmin;
  state.fPostAddGain = config->stAiRecordConfig.stAnrConfig.fPostAddGain;
  state.fNoiseFactor = config->stAiRecordConfig.stAnrConfig.fNoiseFactor;
  state.enHpfSwitch = config->stAiRecordConfig.stAnrConfig.enHpfSwitch;
  state.fHpfFc = config->stAiRecordConfig.stAnrConfig.fHpfFc;
  state.enLpfSwitch = config->stAiRecordConfig.stAnrConfig.enLpfSwitch;
  state.fLpfFc = config->stAiRecordConfig.stAnrConfig.fLpfFc;

  RKAP_ANR_DumpVersion();
  handle->ap_handle = RKAP_ANR_Init(&state);
  if (!handle->ap_handle) {
    RKMEDIA_LOGI("ANR: init failed\n");
    return -1;
  }
  return 0;
}

int AI_RECORDVQE_Deinit(AUDIO_VQE_S *handle) {
  RKAP_ANR_Destroy(handle->ap_handle);
  return 0;
}

static int AI_RECORDVQE_Process(AUDIO_VQE_S *handle, unsigned char *in,
                                unsigned char *out) {
  // for hardware refs signal
  int16_t *sigin;
  int16_t byte_width;
  SampleInfo sample_info = handle->sample_info;

  byte_width = get_bit_width(sample_info.fmt) / 8;
  if (byte_width < 0) {
    RKMEDIA_LOGE("get byte width(%d) failed, SampleFormat: %d\n", byte_width,
                 sample_info.fmt);
    return -1;
  }

  int nb_samples = handle->stVqeConfig.stAiTalkConfig.s32FrameSample;
  int16_t bytes = nb_samples * byte_width * sample_info.channels;
  unsigned char prebuf[bytes] = {0};
  unsigned char sigout[bytes / sample_info.channels] = {0};

  sigin = (int16_t *)prebuf;
  if (handle->layout == AI_LAYOUT_MIC_REF) {
    for (int i = 0; i < nb_samples; i++)
      sigin[i] = *((int16_t *)in + i * 2);
  } else if (handle->layout == AI_LAYOUT_REF_MIC) {
    for (int i = 0; i < nb_samples; i++)
      sigin[i] = *((int16_t *)in + i * 2 + 1);
  } else if (handle->layout == AI_LAYOUT_NORMAL) {
    return RKAP_ANR_Process(handle->ap_handle, (int16_t *)in, (int16_t *)out);
  }

  RKAP_ANR_Process(handle->ap_handle, sigin, (int16_t *)sigout);

  int16_t *tmp2 = (int16_t *)sigout;
  int16_t *tmp1 = (int16_t *)out;
  for (int j = 0; j < nb_samples; j++) {
    *tmp1++ = *tmp2;
    *tmp1++ = *tmp2++;
  }

  return 0;
}

int AO_VQE_Init(AUDIO_VQE_S *handle, VQE_CONFIG_S *config) {
  SampleInfo sample_info = handle->sample_info;

  if (!(sample_info.fmt == SAMPLE_FMT_S16 &&
        (sample_info.sample_rate == 8000 ||
         sample_info.sample_rate == 16000))) {
    RKMEDIA_LOGI("check failed: sample_info.fmt == SAMPLE_FMT_S16 && \
		 (sample_info.sample_rate == 8000 || sample_info.sample_rate == 16000))");
    return -1;
  }

  RKAP_AEC_State state;
  state.swSampleRate = sample_info.sample_rate; // 8k|16k
  state.swFrameLen =
      ALGO_FRAME_TIMS_MS * sample_info.sample_rate / 1000; // hard code 20ms
  state.pathPara = config->stAoConfig.aParamFilePath;
  RKMEDIA_LOGI("AEC: param file = %s\n", state.pathPara);
  handle->ap_handle = RKAP_3A_Init(&state, AEC_RX_TYPE);
  if (!handle->ap_handle) {
    RKMEDIA_LOGI("AEC: init failed\n");
    return -1;
  }
  return 0;
}

int AO_VQE_Deinit(AUDIO_VQE_S *handle) {
  RKAP_3A_Destroy(handle->ap_handle);
  return 0;
}

static int AO_VQE_Process(AUDIO_VQE_S *handle, unsigned char *in,
                          unsigned char *out) {
  SampleInfo sample_info = handle->sample_info;

  if (sample_info.channels == 1) {
    RKAP_3A_Process(handle->ap_handle, (int16_t *)in, NULL, (int16_t *)out);
    return 0;
  }

  int16_t *left;
  int16_t bytes =
      handle->stVqeConfig.stAiTalkConfig.s32FrameSample * 4; // S16 && 2 channel

  unsigned char prebuf[bytes] = {0};
  unsigned char sigout[bytes / 2] = {0};
  int nb_samples = bytes / 4;

  left = (int16_t *)prebuf;
  for (int i = 0; i < nb_samples; i++) {
    left[i] = *((int16_t *)in + i * 2);
  }
  RKAP_3A_Process(handle->ap_handle, left, NULL, (int16_t *)sigout);

  int16_t *tmp2 = (int16_t *)sigout;
  int16_t *tmp1 = (int16_t *)out;
  for (int j = 0; j < nb_samples; j++) {
    *tmp1++ = *tmp2;
    *tmp1++ = *tmp2++;
  }
  return 0;
}

static int VQE_Process(AUDIO_VQE_S *handle, unsigned char *in,
                       unsigned char *out) {
  int ret;
  switch (handle->stVqeConfig.u32VQEMode) {
  case VQE_MODE_AI_TALK:
    ret = AI_TALKVQE_Process(handle, in, out);
    break;
  case VQE_MODE_AO:
    ret = AO_VQE_Process(handle, in, out);
    break;
  case VQE_MODE_AI_RECORD:
    ret = AI_RECORDVQE_Process(handle, in, out);
    break;
  default:
    ret = -1;
    break;
  }
  return ret;
}

bool RK_AUDIO_VQE_Support() { return true; }

AUDIO_VQE_S *RK_AUDIO_VQE_Init(const SampleInfo &sample_info,
                               AI_LAYOUT_E layout, VQE_CONFIG_S *config) {
  int ret = -1;
  AUDIO_VQE_S *handle = (AUDIO_VQE_S *)calloc(sizeof(AUDIO_VQE_S), 1);
  if (!handle)
    return NULL;

  handle->in_queue = queue_create(RK_AUDIO_BUFFER_MAX_SIZE);
  handle->out_queue = queue_create(RK_AUDIO_BUFFER_MAX_SIZE);

  handle->sample_info = sample_info;
  handle->layout = layout;
  handle->stVqeConfig = *config;

  switch (config->u32VQEMode) {
  case VQE_MODE_AI_TALK:
    ret = AI_TALKVQE_Init(handle, config);
    break;
  case VQE_MODE_AO:
    ret = AO_VQE_Init(handle, config);
    break;
  case VQE_MODE_AI_RECORD:
    ret = AI_RECORDVQE_Init(handle, config);
    break;
  case VQE_MODE_BUTT:
  default:
    ret = -1;
    break;
  }
  if (ret == 0)
    return handle;

  if (handle) {
    free(handle->in_queue);
    free(handle->out_queue);
    free(handle);
  }
  return NULL;
}

int RK_AUDIO_VQE_Handle(AUDIO_VQE_S *handle, void *buffer, int bytes) {
  int16_t nm_samples = ALGO_FRAME_TIMS_MS * handle->sample_info.sample_rate /
                       1000; // hard code 20ms
  int16_t frame_bytes = nm_samples * 2 * handle->sample_info.channels;

  // 1. data in queue
  queue_write(handle->in_queue, (unsigned char *)buffer, bytes);

  // 2. peek data from in queue, do audio process, data out queue
  for (int i = 0; i < queue_size(handle->in_queue) / frame_bytes; i++) {
    unsigned char in[frame_bytes] = {0};
    unsigned char out[frame_bytes] = {0};
    queue_read(handle->in_queue, in, frame_bytes);
    VQE_Process(handle, in, out);
    queue_write(handle->out_queue, out, frame_bytes);
  }
  /* Copy the rest of the sample to the beginning of the Buffer */
  queue_tune(handle->in_queue);

  // 2. peek data from out queue
  if (queue_size(handle->out_queue) >= bytes) {
    queue_read(handle->out_queue, (unsigned char *)buffer, bytes);
    queue_tune(handle->out_queue);
    return 0;
  } else {
    RKMEDIA_LOGI("%s: queue size %d less than %d\n", __func__,
                 queue_size(handle->out_queue), bytes);
    return -1;
  }
}

void RK_AUDIO_VQE_Deinit(AUDIO_VQE_S *handle) {
  queue_free(handle->in_queue);
  queue_free(handle->out_queue);

  switch (handle->stVqeConfig.u32VQEMode) {
  case VQE_MODE_AI_TALK:
    AI_TALKVQE_Deinit(handle);
    break;
  case VQE_MODE_AO:
    AO_VQE_Deinit(handle);
    break;
  case VQE_MODE_AI_RECORD:
    AI_RECORDVQE_Deinit(handle);
    break;
  case VQE_MODE_BUTT:
  default:
    break;
  }
}

#else

struct rkAUDIO_VQE_S {};
bool RK_AUDIO_VQE_Support() { return false; }
AUDIO_VQE_S *RK_AUDIO_VQE_Init(const SampleInfo &sample_info _UNUSED,
                               AI_LAYOUT_E layout _UNUSED,
                               VQE_CONFIG_S *config _UNUSED) {
  return NULL;
}
void RK_AUDIO_VQE_Deinit(AUDIO_VQE_S *handle _UNUSED) {}
int RK_AUDIO_VQE_Handle(AUDIO_VQE_S *handle _UNUSED, void *buffer _UNUSED,
                        int bytes _UNUSED) {
  return 0;
}

#endif
