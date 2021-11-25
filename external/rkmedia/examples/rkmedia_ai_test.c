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

static void *GetMediaBuffer(void *path) {
  char *save_path = (char *)path;
  printf("#Start %s thread, arg:%s\n", __func__, save_path);
  FILE *save_file = fopen(save_path, "w");
  if (!save_file)
    printf("ERROR: Open %s failed!\n", save_path);

  MEDIA_BUFFER mb = NULL;
  int cnt = 0;
  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_AI, 0, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }

    printf("#%d Get Frame:ptr:%p, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld\n",
           cnt++, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb));

    if (save_file)
      fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), save_file);

    RK_MPI_MB_ReleaseBuffer(mb);
  }

  if (save_file)
    fclose(save_file);

  return NULL;
}

static RK_CHAR optstr[] = "?::d:c:r:s:o:v:";
static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-d default] [-r 16000] [-c 2] [-s 1024] -o /tmp/ai.pcm\n",
         name);
  printf("\t-d: device name, Default:\"default\"\n");
  printf("\t-r: sample rate, Default:16000\n");
  printf("\t-c: channel count, Default:2\n");
  printf("\t-s: frames cnt, Default:1024\n");
  printf("\t-v: volume, Default:50 (0-100)\n");
  printf("\t-o: output path, Default:\"/tmp/ai.pcm\"\n");
  printf("Notice: fmt always s16_le\n");
}

int main(int argc, char *argv[]) {
  RK_U32 u32SampleRate = 16000;
  RK_U32 u32ChnCnt = 2;
  RK_U32 u32FrameCnt = 1024;
  RK_S32 s32Volume = 50;
  // default:CARD=rockchiprk809co
  RK_CHAR *pDeviceName = "default";
  RK_CHAR *pOutPath = "/tmp/ai.pcm";
  int c;
  int ret = 0;

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
    case 'o':
      pOutPath = optarg;
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#Device: %s\n", pDeviceName);
  printf("#SampleRate: %u\n", u32SampleRate);
  printf("#Channel Count: %u\n", u32ChnCnt);
  printf("#Frame Count: %u\n", u32FrameCnt);
  printf("#Volume: %d\n", s32Volume);
  printf("#Output Path: %s\n", pOutPath);

  RK_MPI_SYS_Init();

  AI_CHN_ATTR_S ai_attr;
  ai_attr.pcAudioNode = pDeviceName;
  ai_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ai_attr.u32NbSamples = u32FrameCnt;
  ai_attr.u32SampleRate = u32SampleRate;
  ai_attr.u32Channels = u32ChnCnt;
  ai_attr.enAiLayout = AI_LAYOUT_NORMAL;

  ret = RK_MPI_AI_SetChnAttr(0, &ai_attr);
  ret |= RK_MPI_AI_EnableChn(0);
  if (ret) {
    printf("Enable AI[0] failed! ret=%d\n", ret);
    return -1;
  }

  RK_S32 s32CurrentVolmue = 0;
  ret = RK_MPI_AI_GetVolume(0, &s32CurrentVolmue);
  if (ret) {
    printf("Get Volume(before) failed! ret=%d\n", ret);
    return -1;
  }
  printf("#Before Volume set, volume=%d\n", s32CurrentVolmue);

  ret = RK_MPI_AI_SetVolume(0, s32Volume);
  if (ret) {
    printf("Set Volume failed! ret=%d\n", ret);
    return -1;
  }

  s32CurrentVolmue = 0;
  ret = RK_MPI_AI_GetVolume(0, &s32CurrentVolmue);
  if (ret) {
    printf("Get Volume(after) failed! ret=%d\n", ret);
    return -1;
  }
  printf("#After Volume set, volume=%d\n", s32CurrentVolmue);

  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, pOutPath);
  ret = RK_MPI_AI_StartStream(0);
  if (ret) {
    printf("Start AI failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);
  RK_MPI_AI_DisableChn(0);

  return 0;
}
