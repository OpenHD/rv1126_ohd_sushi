// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "rkmedia_api.h"

static bool quit = false;
static RK_CHAR optstr[] = "?::d:c:r:i:";

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-d default] [-r 16000] [-c 2] -i /tmp/aenc.mp3\n", name);
  printf("\t-d: device name, Default:\"default\"\n");
  printf("\t-r: sample rate, Default:16000\n");
  printf("\t-c: channel count, Default:2\n");
  printf("\t-i: input path, Default:\"/tmp/aenc.mp3\"\n");
  printf("Notice: fmt always s16_le\n");
}

int main(int argc, char *argv[]) {
  RK_U32 u32SampleRate = 16000;
  RK_U32 u32ChnCnt = 2;
  RK_U32 u32FrameCnt = 1024;
  // default:CARD=rockchiprk809co
  RK_CHAR *pDeviceName = "default";
  RK_CHAR *pInPath = "/tmp/aenc.mp3";
  int ret = 0;
  int c;

  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'd':
      pDeviceName = optarg;
      break;
    case 'r':
      u32SampleRate = atoi(optarg);
      break;
    case 'c':
      u32ChnCnt = atoi(optarg);
      break;
    case 'i':
      pInPath = optarg;
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#Device: %s\n", pDeviceName);
  printf("#SampleRate: %d\n", u32SampleRate);
  printf("#Channel Count: %d\n", u32ChnCnt);
  printf("#Frame Count: %d\n", u32FrameCnt);
  printf("#Input Path: %s\n", pInPath);

  FILE *file = fopen(pInPath, "r");
  if (!file) {
    printf("ERROR: open %s failed!\n", pInPath);
    return -1;
  }

  // init MPI
  RK_MPI_SYS_Init();

  // create ADEC
  ADEC_CHN_ATTR_S stAdecAttr = {0};
  stAdecAttr.enCodecType = RK_CODEC_TYPE_MP3;
  ret = RK_MPI_ADEC_CreateChn(0, &stAdecAttr);
  if (ret) {
    printf("ERROR: Create ADEC[0] failed! ret=%d\n", ret);
    return -1;
  }

  // create AO
  AO_CHN_ATTR_S stAoAttr;
  stAoAttr.u32Channels = u32ChnCnt;
  stAoAttr.u32SampleRate = u32SampleRate;
  stAoAttr.u32NbSamples = u32FrameCnt;
  stAoAttr.pcAudioNode = pDeviceName;
  stAoAttr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ret = RK_MPI_AO_SetChnAttr(0, &stAoAttr);
  ret |= RK_MPI_AO_EnableChn(0);
  if (ret) {
    printf("ERROR: Create AO[0] failed! ret=%d\n", ret);
    return -1;
  }

  // Bind ADEC to AO
  MPP_CHN_S mpp_chn_ao, mpp_chn_adec;
  mpp_chn_ao.enModId = RK_ID_AO;
  mpp_chn_ao.s32ChnId = 0;
  mpp_chn_adec.enModId = RK_ID_ADEC;
  mpp_chn_adec.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&mpp_chn_adec, &mpp_chn_ao);
  if (ret) {
    printf("ERROR: Bind ADEC[0] to AO[0] failed! ret=%d\n", ret);
    return -1;
  }

  RK_S32 s32BufferSize;
  RK_S32 s32ReadSize = 0;
  MEDIA_BUFFER mb = NULL;
  RK_U32 u32Cnt = 0;
  MB_AUDIO_INFO_S stSampleInfo = {stAoAttr.u32Channels, stAoAttr.u32SampleRate,
                                  stAoAttr.u32NbSamples,
                                  stAoAttr.enSampleFormat};
  while (!quit) {
    mb = RK_MPI_MB_CreateAudioBufferExt(&stSampleInfo, RK_FALSE, 0);
    if (!mb) {
      printf("ERROR: no space left!\n");
      break;
    }
    s32BufferSize = RK_MPI_MB_GetSize(mb);
    s32ReadSize = fread(RK_MPI_MB_GetPtr(mb), 1, s32BufferSize, file);
    if (s32ReadSize <= 0) {
      printf("# Get end of file!\n");
      break;
    }
    RK_MPI_MB_SetSize(mb, s32ReadSize);
    printf("#%d Send %dBytes to ADEC[0]...\n", u32Cnt++, s32ReadSize);
    ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_ADEC, mpp_chn_adec.s32ChnId, mb);
    if (ret) {
      printf("ERROR: RK_MPI_SYS_SendMediaBuffer failed! ret=%d\n", ret);
      break;
    }
    RK_MPI_MB_ReleaseBuffer(mb);
    mb = NULL;
  }

  if (mb)
    RK_MPI_MB_ReleaseBuffer(mb);

  // flush decoder
  printf("# Flush decoder...\n");
  mb = RK_MPI_MB_CreateAudioBufferExt(&stSampleInfo, RK_FALSE, 0);
  RK_MPI_MB_SetSize(mb, 0);
  RK_MPI_SYS_SendMediaBuffer(RK_ID_ADEC, mpp_chn_adec.s32ChnId, mb);
  RK_MPI_MB_ReleaseBuffer(mb);

  sleep(10);
  printf("# decoder sucess!!!\n");
  fclose(file);

  return 0;
}
