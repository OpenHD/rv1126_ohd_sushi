// Copyright 2021 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
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

static void *ReadMic0Thread(void *path) {
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

    printf("#%s: Get Frame: idx:%d, ptr:%p, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld\n",
           __func__, cnt++, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb),
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

static void *ReadMic1Thread(void *path) {
  char *save_path = (char *)path;
  printf("#Start %s thread, arg:%s\n", __func__, save_path);
  FILE *save_file = fopen(save_path, "w");
  if (!save_file)
    printf("ERROR: Open %s failed!\n", save_path);

  MEDIA_BUFFER mb = NULL;
  int cnt = 0;
  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_AI, 1, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }

    printf("#%s: Get Frame: idx:%d, ptr:%p, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld\n",
           __func__, cnt++, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb),
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

int main() {
  RK_U32 u32SampleRate = 16000;
  RK_U32 u32ChnCnt = 1;
  RK_U32 u32FrameCnt = 1024;
  RK_CHAR *pMic0Node = "plug:cap_chn0";
  RK_CHAR *pMic1Node = "plug:cap_chn1";
  RK_CHAR *pOutPath0 = "/tmp/ai_chn0.pcm";
  RK_CHAR *pOutPath1 = "/tmp/ai_chn1.pcm";
  int ret = 0;

  printf("#Mic0: AudioNode:%s, OutPath: %s\n", pMic0Node, pOutPath0);
  printf("#Mic1: AudioNode:%s, OutPath: %s\n", pMic1Node, pOutPath1);

  RK_MPI_SYS_Init();

  AI_CHN_ATTR_S ai_attr;

  /*****************************
   * Open AI[0] <--- Mic0
   * ***************************/
  memset(&ai_attr, 0, sizeof(ai_attr));
  ai_attr.pcAudioNode = pMic0Node;
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

  /*****************************
   * Open AI[1] <--- Mic1
   * ***************************/
  memset(&ai_attr, 0, sizeof(ai_attr));
  ai_attr.pcAudioNode = pMic1Node;
  ai_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ai_attr.u32NbSamples = u32FrameCnt;
  ai_attr.u32SampleRate = u32SampleRate;
  ai_attr.u32Channels = u32ChnCnt;
  ai_attr.enAiLayout = AI_LAYOUT_NORMAL;
  ret = RK_MPI_AI_SetChnAttr(1, &ai_attr);
  ret |= RK_MPI_AI_EnableChn(1);
  if (ret) {
    printf("Enable AI[1] failed! ret=%d\n", ret);
    return -1;
  }

  pthread_t read_thread0;
  pthread_create(&read_thread0, NULL, ReadMic0Thread, pOutPath0);
  pthread_t read_thread1;
  pthread_create(&read_thread1, NULL, ReadMic1Thread, pOutPath1);

  ret = RK_MPI_AI_StartStream(0);
  if (ret) {
    printf("Start AI[0] failed! ret=%d\n", ret);
    return -1;
  }

  ret = RK_MPI_AI_StartStream(1);
  if (ret) {
    printf("Start AI[1] failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  while (!quit) {
    usleep(500000);
  }

  pthread_join(read_thread0, NULL);
  pthread_join(read_thread1, NULL);

  printf("%s exit!\n", __func__);
  RK_MPI_AI_DisableChn(0);
  RK_MPI_AI_DisableChn(1);

  return 0;
}
