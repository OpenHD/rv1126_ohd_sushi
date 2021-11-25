// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "rkmedia_api.h"
#define MP3_NB_SAMPLES 1024
#define MP2_NB_SAMPLES 1152
//#define ALSA_PATH "default:CARD=rockchiprk809co" // get from "arecord -L"
#define ALSA_PATH "default" // get from "arecord -L"
#define VQEFILE "/usr/share/rkap_3a/para/16k/RKAP_3A_Para.bin"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

FILE *fp = NULL;
static RK_U32 g_enWorkSampleRate = 16000;
static RK_U32 g_s32VqeFrameSample = 320; // 20ms;

static void audio_packet_cb(MEDIA_BUFFER mb) {
  printf("Get Audio Encoded packet:ptr:%p, fd:%d, size:%zu, mode:%d\n",
         RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
         RK_MPI_MB_GetModeID(mb));
  fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), fp);
  RK_MPI_MB_ReleaseBuffer(mb);
}

static RK_VOID AI_AO() {
  RK_MPI_SYS_Init();
  MPP_CHN_S mpp_chn_ai, mpp_chn_ao;
  mpp_chn_ai.enModId = RK_ID_AI;
  mpp_chn_ai.s32ChnId = 0;
  mpp_chn_ao.enModId = RK_ID_AO;
  mpp_chn_ao.s32ChnId = 0;

  AI_CHN_ATTR_S ai_attr;
  ai_attr.pcAudioNode = ALSA_PATH;
  ai_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ai_attr.u32NbSamples = 1152;
  ai_attr.u32SampleRate = g_enWorkSampleRate;
  ai_attr.u32Channels = 1;
  ai_attr.enAiLayout = AI_LAYOUT_MIC_REF; // chanel layout: [ref:mic]; remove
                                          // ref, output mic mono

  AO_CHN_ATTR_S ao_attr;
  ao_attr.pcAudioNode = ALSA_PATH;
  ao_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ao_attr.u32NbSamples = 1152;
  ao_attr.u32SampleRate = g_enWorkSampleRate;
  ao_attr.u32Channels = 1;

  // 1. create AI
  RK_MPI_AI_SetChnAttr(mpp_chn_ai.s32ChnId, &ai_attr);
  RK_MPI_AI_EnableChn(mpp_chn_ai.s32ChnId);

  // 2. create AO
  RK_MPI_AO_SetChnAttr(mpp_chn_ao.s32ChnId, &ao_attr);
  RK_MPI_AO_EnableChn(mpp_chn_ao.s32ChnId);

  // 3. bind AI-AO
  RK_MPI_SYS_Bind(&mpp_chn_ai, &mpp_chn_ao);

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);
  RK_MPI_SYS_UnBind(&mpp_chn_ai, &mpp_chn_ao);
  RK_MPI_AI_DisableChn(mpp_chn_ai.s32ChnId);
  RK_MPI_AO_DisableChn(mpp_chn_ao.s32ChnId);
}

static RK_VOID AI_AENC_FILE(char *file_path) {
  fp = fopen(file_path, "w+");
  RK_MPI_SYS_Init();
  MPP_CHN_S mpp_chn_ai, mpp_chn_aenc;
  mpp_chn_ai.enModId = RK_ID_AI;
  mpp_chn_ai.s32ChnId = 0;
  mpp_chn_aenc.enModId = RK_ID_AENC;
  mpp_chn_aenc.s32ChnId = 0;

  AI_CHN_ATTR_S ai_attr;
  ai_attr.pcAudioNode = ALSA_PATH;
  ai_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ai_attr.u32NbSamples = MP3_NB_SAMPLES;
  ai_attr.u32SampleRate = g_enWorkSampleRate;
  ai_attr.u32Channels = 1;
  ai_attr.enAiLayout = AI_LAYOUT_MIC_REF; // chanel layout: [ref:mic]; remove
                                          // ref, output mic mono

  AENC_CHN_ATTR_S aenc_attr;
  aenc_attr.enCodecType = RK_CODEC_TYPE_MP3;
  aenc_attr.u32Bitrate = 64000;
  aenc_attr.u32Quality = 1;
  aenc_attr.stAencMP3.u32Channels = 1;
  aenc_attr.stAencMP3.u32SampleRate = g_enWorkSampleRate;

  // 1. create AI
  RK_MPI_AI_SetChnAttr(mpp_chn_ai.s32ChnId, &ai_attr);
  RK_MPI_AI_EnableChn(mpp_chn_ai.s32ChnId);

  // 2. create AENC
  RK_MPI_AENC_CreateChn(mpp_chn_aenc.s32ChnId, &aenc_attr);
  RK_U32 ret = RK_MPI_SYS_RegisterOutCb(&mpp_chn_aenc, audio_packet_cb);
  printf("ret = %d.\n", ret);

  // 3. bind AI-AENC
  RK_MPI_SYS_Bind(&mpp_chn_ai, &mpp_chn_aenc);

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  RK_MPI_SYS_UnBind(&mpp_chn_ai, &mpp_chn_aenc);
  RK_MPI_AI_DisableChn(mpp_chn_ai.s32ChnId);
  RK_MPI_AENC_DestroyChn(mpp_chn_aenc.s32ChnId);
  fclose(fp);
}

static RK_VOID FILE_ADEC_AO(char *file_path) {
  CODEC_TYPE_E codec_type = RK_CODEC_TYPE_MP3;
  RK_U32 channels = 2;
  RK_U32 sample_rate = g_enWorkSampleRate;
  ADEC_CHN_ATTR_S stAdecAttr;
  AO_CHN_ATTR_S stAoAttr;

  stAdecAttr.enCodecType = codec_type;
  MPP_CHN_S mpp_chn_ao, mpp_chn_adec;
  mpp_chn_ao.enModId = RK_ID_AO;
  mpp_chn_ao.s32ChnId = 0;
  mpp_chn_adec.enModId = RK_ID_ADEC;
  mpp_chn_adec.s32ChnId = 0;

  stAoAttr.u32Channels = channels;
  stAoAttr.u32SampleRate = sample_rate;
  stAoAttr.u32NbSamples = 1024;
  stAoAttr.pcAudioNode = ALSA_PATH;

  switch (codec_type) {
  case RK_CODEC_TYPE_MP3:
    stAoAttr.enSampleFormat = RK_SAMPLE_FMT_S16;
    stAoAttr.u32NbSamples = 1024;
    break;
  case RK_CODEC_TYPE_MP2:
    stAoAttr.enSampleFormat = RK_SAMPLE_FMT_S16;
    stAoAttr.u32NbSamples = 1152;
    break;
  case RK_CODEC_TYPE_G711A:
    stAdecAttr.stAdecG711A.u32Channels = channels;
    stAdecAttr.stAdecG711A.u32SampleRate = sample_rate;
    stAoAttr.enSampleFormat = RK_SAMPLE_FMT_S16;
    break;
  case RK_CODEC_TYPE_G711U:
    stAdecAttr.stAdecG711U.u32Channels = channels;
    stAdecAttr.stAdecG711U.u32SampleRate = sample_rate;
    stAoAttr.enSampleFormat = RK_SAMPLE_FMT_S16;
    break;
  case RK_CODEC_TYPE_G726:
    stAoAttr.enSampleFormat = RK_SAMPLE_FMT_S16;
    break;
  default:
    printf("audio codec type error.\n");
    return;
  }
  // init MPI
  RK_MPI_SYS_Init();
  // create ADEC
  RK_MPI_ADEC_CreateChn(mpp_chn_adec.s32ChnId, &stAdecAttr);
  // create AO
  RK_MPI_AO_SetChnAttr(mpp_chn_ao.s32ChnId, &stAoAttr);
  RK_MPI_AO_EnableChn(mpp_chn_ao.s32ChnId);

  RK_MPI_SYS_Bind(&mpp_chn_adec, &mpp_chn_ao);

  MB_AUDIO_INFO_S stSampleInfo = { stAoAttr.u32Channels, stAoAttr.u32SampleRate,
                                   stAoAttr.u32NbSamples,stAoAttr.enSampleFormat};
  FILE *read_file = fopen(file_path, "r");
  if (!read_file) {
    printf("ERROR: open %s failed!\n", file_path);
    exit(0);
  }
  quit = true;
  while (quit) {
    MEDIA_BUFFER mb = RK_MPI_MB_CreateAudioBufferExt(&stSampleInfo, RK_FALSE, 0);
    if (!mb) {
      printf("ERROR: no space left!\n");
      break;
    }

    RK_S32 buffer_size = RK_MPI_MB_GetSize(mb);
    RK_S32 s32ReadSize = fread(RK_MPI_MB_GetPtr(mb), 1, buffer_size, read_file);

    RK_MPI_MB_SetSize(mb, s32ReadSize);
    RK_MPI_SYS_SendMediaBuffer(RK_ID_ADEC, mpp_chn_adec.s32ChnId, mb);
    RK_MPI_MB_ReleaseBuffer(mb);
    if (s32ReadSize != buffer_size) {
      printf("Get end of file!\n");
      break;
    }
  }
  sleep(2);
  {
    // flush decoder
    printf("start flush decoder.\n");
    MEDIA_BUFFER mb = RK_MPI_MB_CreateAudioBufferExt(&stSampleInfo, RK_FALSE, 0);
    RK_MPI_MB_SetSize(mb, 0);
    RK_MPI_SYS_SendMediaBuffer(RK_ID_ADEC, mpp_chn_adec.s32ChnId, mb);
    RK_MPI_MB_ReleaseBuffer(mb);
    printf("end flush decoder.\n");
  }

  sleep(10);
}

/* 0: close, 1: talk, 2: record */
static RK_U32 u32AiVqeType = 1;
/* 0: close, 1: open */
static RK_U32 u32AoVqeType = 1;

/******************************************************************************
 * function : Ai ->VqeProcess-> Ao
 ******************************************************************************/
RK_S32 AI_VqeProcess_AO(RK_VOID) {
  AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
  AI_RECORDVQE_CONFIG_S stAiVqeRecordAttr;
  AO_VQE_CONFIG_S stAoVqeAttr;
  MPP_CHN_S mpp_chn_ai, mpp_chn_ao;

  if (1 == u32AiVqeType) {
    memset(&stAiVqeTalkAttr, 0, sizeof(AI_TALKVQE_CONFIG_S));
    stAiVqeTalkAttr.s32WorkSampleRate = g_enWorkSampleRate;
    stAiVqeTalkAttr.s32FrameSample = g_s32VqeFrameSample;
    strcpy(stAiVqeTalkAttr.aParamFilePath, VQEFILE);
    stAiVqeTalkAttr.u32OpenMask =
        AI_TALKVQE_MASK_AEC | AI_TALKVQE_MASK_ANR | AI_TALKVQE_MASK_AGC;
  } else if (2 == u32AiVqeType) {
    memset(&stAiVqeRecordAttr, 0, sizeof(AI_RECORDVQE_CONFIG_S));
    stAiVqeRecordAttr.s32WorkSampleRate = g_enWorkSampleRate;
    stAiVqeRecordAttr.s32FrameSample = g_s32VqeFrameSample;
    stAiVqeRecordAttr.stAnrConfig.fPostAddGain = 0;
    stAiVqeRecordAttr.stAnrConfig.fGmin = -30;
    stAiVqeRecordAttr.stAnrConfig.fNoiseFactor = 0.98;
    stAiVqeRecordAttr.u32OpenMask = AI_RECORDVQE_MASK_ANR;
  }

  if (1 == u32AoVqeType) {
    memset(&stAoVqeAttr, 0, sizeof(AO_VQE_CONFIG_S));
    stAoVqeAttr.s32WorkSampleRate = g_enWorkSampleRate;
    stAoVqeAttr.s32FrameSample = g_s32VqeFrameSample;
    strcpy(stAoVqeAttr.aParamFilePath, VQEFILE);
    stAoVqeAttr.u32OpenMask = AO_VQE_MASK_ANR | AO_VQE_MASK_AGC;
  }

  RK_MPI_SYS_Init();
  mpp_chn_ai.enModId = RK_ID_AI;
  mpp_chn_ai.s32ChnId = 0;
  mpp_chn_ao.enModId = RK_ID_AO;
  mpp_chn_ao.s32ChnId = 0;

  AI_CHN_ATTR_S ai_attr;
  ai_attr.pcAudioNode = ALSA_PATH;
  ai_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ai_attr.u32NbSamples = 1024;
  ai_attr.u32SampleRate = g_enWorkSampleRate;
  ai_attr.u32Channels = 1;
  ai_attr.enAiLayout =
      AI_LAYOUT_MIC_REF; // remove ref channel, and output mic mono

  AO_CHN_ATTR_S ao_attr;
  ao_attr.pcAudioNode = ALSA_PATH;
  ao_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ao_attr.u32NbSamples = 1024;
  ao_attr.u32SampleRate = g_enWorkSampleRate;
  ao_attr.u32Channels = 1;

  // 1. create AI
  RK_MPI_AI_SetChnAttr(mpp_chn_ai.s32ChnId, &ai_attr);
  RK_MPI_AI_EnableChn(mpp_chn_ai.s32ChnId);
  if (1 == u32AiVqeType) {
    RK_MPI_AI_SetTalkVqeAttr(mpp_chn_ai.s32ChnId, &stAiVqeTalkAttr);
    RK_MPI_AI_EnableVqe(mpp_chn_ai.s32ChnId);
  } else if (2 == u32AiVqeType) {
    RK_MPI_AI_SetRecordVqeAttr(mpp_chn_ai.s32ChnId, &stAiVqeRecordAttr);
    RK_MPI_AI_EnableVqe(mpp_chn_ai.s32ChnId);
  }
  // 2. create AO
  RK_MPI_AO_SetChnAttr(mpp_chn_ao.s32ChnId, &ao_attr);
  RK_MPI_AO_EnableChn(mpp_chn_ao.s32ChnId);
  if (1 == u32AoVqeType) {
    RK_MPI_AO_SetVqeAttr(mpp_chn_ao.s32ChnId, &stAoVqeAttr);
    RK_MPI_AO_EnableVqe(mpp_chn_ao.s32ChnId);
  }
  // 3. bind AI-AO
  RK_MPI_SYS_Bind(&mpp_chn_ai, &mpp_chn_ao);

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);
  RK_MPI_SYS_UnBind(&mpp_chn_ai, &mpp_chn_ao);
  RK_MPI_AI_DisableChn(mpp_chn_ai.s32ChnId);
  RK_MPI_AO_DisableChn(mpp_chn_ao.s32ChnId);

  return RK_SUCCESS;
}

static RK_VOID RKMEDIA_AUDIO_Usage() {
  printf("\n\n/Usage:./rkmdia_audio <index> <sampleRate> [filePath]/\n");
  printf("\tindex and its function list below\n");
  printf("\t0:  start AI to AO loop\n");
  printf("\t1:  send audio frame to AENC channel from AI, save them\n");
  printf("\t2:  read audio stream from file, decode and send AO\n");
  printf("\t3:  start AI(VQE process), then send to AO\n");
  // printf("\t4:  start AI to Extern Resampler\n");
  printf("\n");
  printf("\tsampleRate list:\n");
  printf("\t0 16000 22050 24000 32000 44100 48000\n");
  printf("\n");
  printf("\tfilePath represents the path of audio file to be decoded, only for "
         "sample 2.\n");
  printf("\tdefault filePath: /userdata/out.mp2\n");
  printf("\n");
  printf("\texample: ./rkmdia_audio 0 48000\n");
}

int main(int argc, char *argv[]) {

  RK_U32 u32Index;
  RK_CHAR *pFilePath = RK_NULL;

  if (argc != 3 && argc != 4) {
    RKMEDIA_AUDIO_Usage();
    return -1;
  }

  if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0) {
    RKMEDIA_AUDIO_Usage();
    return -1;
  }

  u32Index = atoi(argv[1]);
  g_enWorkSampleRate = atoi(argv[2]);
  if (4 == argc) {
    pFilePath = argv[3];
  } else {
    pFilePath = (char *)"/userdata/out.mp2";
  }

  switch (u32Index) {
  case 0:
    AI_AO();
    break;
  case 1:
    AI_AENC_FILE(pFilePath);
    break;
  case 2:
    FILE_ADEC_AO(pFilePath);
    break;
  case 3:
    AI_VqeProcess_AO();
  default:
    break;
  }
  return 0;
}
