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
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static RK_CHAR optstr[] = "?::d:c:r:s:i:v:";
static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-d default] [-r 16000] [-c 2] [-s 1024] -i /tmp/ao.pcm\n",
         name);
  printf("\t-d: device name, Default:\"default\"\n");
  printf("\t-r: sample rate, Default:16000\n");
  printf("\t-c: channel count, Default:2\n");
  printf("\t-s: frames cnt, Default:1024\n");
  printf("\t-v: volume, Default:50 (0-100)\n");
  printf("\t-i: input path.\n");
  printf("Notice: fmt always s16_le\n");
}

int main(int argc, char *argv[]) {
  RK_U32 u32SampleRate = 16000;
  RK_U32 u32ChnCnt = 2;
  RK_U32 u32FrameCnt = 1024;
  RK_S32 s32Volume = 50;
  // default:CARD=rockchiprk809co
  RK_CHAR *pDeviceName = "default";
  RK_CHAR *pInputPath = NULL;
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
    case 's':
      u32FrameCnt = atoi(optarg);
      break;
    case 'v':
      s32Volume = atoi(optarg);
      break;
    case 'i':
      pInputPath = optarg;
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  if (!pInputPath) {
    printf("ERROR: loss input file!\n");
    return -1;
  }

  printf("#Device: %s\n", pDeviceName);
  printf("#SampleRate: %u\n", u32SampleRate);
  printf("#Channel Count: %u\n", u32ChnCnt);
  printf("#Frame Count: %u\n", u32FrameCnt);
  printf("#Volume: %d\n", s32Volume);
  printf("#Output Path: %s\n", pInputPath);

  FILE *file = fopen(pInputPath, "r");
  if (!file) {
    printf("ERROR: open %s failed!\n", pInputPath);
    return -1;
  }

  RK_MPI_SYS_Init();

  AO_CHN_ATTR_S ao_attr;
  ao_attr.pcAudioNode = pDeviceName;
  ao_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ao_attr.u32NbSamples = u32FrameCnt;
  ao_attr.u32SampleRate = u32SampleRate;
  ao_attr.u32Channels = u32ChnCnt;
  ret = RK_MPI_AO_SetChnAttr(0, &ao_attr);
  ret |= RK_MPI_AO_EnableChn(0);
  if (ret) {
    printf("ERROR: create ao[0] failed! ret=%d\n", ret);
    return -1;
  }

  RK_S32 s32CurrentVolmue = 0;
  ret = RK_MPI_AO_GetVolume(0, &s32CurrentVolmue);
  if (ret) {
    printf("Get Volume(before) failed! ret=%d\n", ret);
    return -1;
  }
  printf("#Before Volume set, volume=%d\n", s32CurrentVolmue);

  ret = RK_MPI_AO_SetVolume(0, s32Volume);
  if (ret) {
    printf("Set Volume failed! ret=%d\n", ret);
    return -1;
  }

  s32CurrentVolmue = 0;
  ret = RK_MPI_AO_GetVolume(0, &s32CurrentVolmue);
  if (ret) {
    printf("Get Volume(after) failed! ret=%d\n", ret);
    return -1;
  }
  printf("#After Volume set, volume=%d\n", s32CurrentVolmue);

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  MEDIA_BUFFER mb = NULL;
  RK_U32 u32Timeval = u32FrameCnt * 1000000 / u32SampleRate; // us
  RK_U32 u32ReadSize = u32FrameCnt * u32ChnCnt * 2; // RK_SAMPLE_FMT_S16:2Bytes
  printf("# TimeVal:%dus, ReadSize:%d\n", u32Timeval, u32ReadSize);
  MB_AUDIO_INFO_S stSampleInfo = {ao_attr.u32Channels, ao_attr.u32SampleRate,
                                  ao_attr.u32NbSamples, ao_attr.enSampleFormat};
  while (!quit) {
    mb = RK_MPI_MB_CreateAudioBufferExt(&stSampleInfo, RK_FALSE, 0);
    if (!mb) {
      printf("ERROR: malloc failed! size:%d\n", u32ReadSize);
      break;
    }
    ret = fread(RK_MPI_MB_GetPtr(mb), 1, u32ReadSize, file);
    if (!ret) {
      printf("# Get end of file!\n");
      break;
    }
    RK_MPI_MB_SetSize(mb, u32ReadSize);
    ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_AO, 0, mb);
    if (ret) {
      printf("ERROR: RK_MPI_SYS_SendMediaBuffer failed! ret = %d\n", ret);
      break;
    }
    usleep(u32Timeval);
    RK_MPI_MB_ReleaseBuffer(mb);
    mb = NULL;
  }

  if (mb)
    RK_MPI_MB_ReleaseBuffer(mb);

  printf("%s exit!\n", __func__);
  RK_MPI_AO_DisableChn(0);

  return 0;
}
