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

typedef struct _PlayBackHandle {
  RK_U32 u32ChnIdx;
  FILE *pFileHandle;
  RK_U32 u32Timeval; // us
  RK_U32 u32ReadSize;
} PlayBackHandle;

static void *Spk0PlayThread(void *pHandle) {
  MEDIA_BUFFER mb = NULL;
  PlayBackHandle *pPlayHandle = (PlayBackHandle *)pHandle;
  RK_U32 u32Timeval = pPlayHandle->u32Timeval;
  RK_U32 u32ReadSize = pPlayHandle->u32ReadSize;
  RK_U32 u32ChnIdx = pPlayHandle->u32ChnIdx;
  FILE *pFileHandle = pPlayHandle->pFileHandle;
  int ret = 0;

  printf("#%s:: VOChn[%d]: TimeVal:%dus, ReadSize:%d\n", __func__, u32ChnIdx,
         u32Timeval, u32ReadSize);
  while (!quit) {
    mb = RK_MPI_MB_CreateAudioBuffer(u32ReadSize, RK_FALSE);
    if (!mb) {
      printf("ERROR: %s: malloc failed! size:%d\n", __func__, u32ReadSize);
      break;
    }
    ret = fread(RK_MPI_MB_GetPtr(mb), 1, u32ReadSize, pFileHandle);
    if (!ret) {
      printf("#%s: Get end of file!\n", __func__);
      break;
    }
    RK_MPI_MB_SetSize(mb, u32ReadSize);
    ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_AO, u32ChnIdx, mb);
    if (ret) {
      printf("ERROR: %s: SendMediaBuffer failed! ret = %d\n", __func__, ret);
      break;
    }
    usleep(u32Timeval);
    RK_MPI_MB_ReleaseBuffer(mb);
    mb = NULL;
  }

  if (mb)
    RK_MPI_MB_ReleaseBuffer(mb);

  return NULL;
}

static void *Spk1PlayThread(void *pHandle) {
  MEDIA_BUFFER mb = NULL;
  PlayBackHandle *pPlayHandle = (PlayBackHandle *)pHandle;
  RK_U32 u32Timeval = pPlayHandle->u32Timeval;
  RK_U32 u32ReadSize = pPlayHandle->u32ReadSize;
  RK_U32 u32ChnIdx = pPlayHandle->u32ChnIdx;
  FILE *pFileHandle = pPlayHandle->pFileHandle;
  int ret = 0;

  printf("#%s:: VOChn[%d]: TimeVal:%dus, ReadSize:%d\n", __func__, u32ChnIdx,
         u32Timeval, u32ReadSize);
  while (!quit) {
    mb = RK_MPI_MB_CreateAudioBuffer(u32ReadSize, RK_FALSE);
    if (!mb) {
      printf("ERROR: %s: malloc failed! size:%d\n", __func__, u32ReadSize);
      break;
    }
    ret = fread(RK_MPI_MB_GetPtr(mb), 1, u32ReadSize, pFileHandle);
    if (!ret) {
      printf("#%s: Get end of file!\n", __func__);
      break;
    }
    RK_MPI_MB_SetSize(mb, u32ReadSize);
    ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_AO, u32ChnIdx, mb);
    if (ret) {
      printf("ERROR: %s: SendMediaBuffer failed! ret = %d\n", __func__, ret);
      break;
    }
    usleep(u32Timeval);
    RK_MPI_MB_ReleaseBuffer(mb);
    mb = NULL;
  }

  if (mb)
    RK_MPI_MB_ReleaseBuffer(mb);

  return NULL;
}

int main() {
  RK_U32 u32SampleRate = 16000;
  RK_U32 u32ChnCnt = 1;
  RK_U32 u32FrameCnt = 1024;
  RK_CHAR *pPlay0Node = "plug:play_chn0";
  RK_CHAR *pPlay1Node = "plug:play_chn1";
  RK_CHAR *pInPath0 = "/tmp/ao_chn0.pcm";
  RK_CHAR *pInPath1 = "/tmp/ao_chn1.pcm";
  FILE *pInFile0 = NULL;
  FILE *pInFile1 = NULL;
  int ret = 0;

  printf("#Spk0: AudioNode:%s, OutPath: %s\n", pPlay0Node, pInPath0);
  printf("#Spk1: AudioNode:%s, OutPath: %s\n", pPlay1Node, pInPath1);

  pInFile0 = fopen(pInPath0, "r");
  if (!pInFile0) {
    printf("ERROR: open %s failed!\n", pInPath0);
    return -1;
  }

  pInFile1 = fopen(pInPath1, "r");
  if (!pInFile1) {
    printf("ERROR: open %s failed!\n", pInPath0);
    return -1;
  }

  RK_MPI_SYS_Init();

  AO_CHN_ATTR_S ao_attr;
  /*******************************************
   * AOChn[0] --> Spk0
   * *****************************************/
  memset(&ao_attr, 0, sizeof(ao_attr));
  ao_attr.pcAudioNode = pPlay0Node;
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

  /*******************************************
   * AOChn[1] --> Spk1
   * *****************************************/
  memset(&ao_attr, 0, sizeof(ao_attr));
  ao_attr.pcAudioNode = pPlay1Node;
  ao_attr.enSampleFormat = RK_SAMPLE_FMT_S16;
  ao_attr.u32NbSamples = u32FrameCnt;
  ao_attr.u32SampleRate = u32SampleRate;
  ao_attr.u32Channels = u32ChnCnt;
  ret = RK_MPI_AO_SetChnAttr(1, &ao_attr);
  ret |= RK_MPI_AO_EnableChn(1);
  if (ret) {
    printf("ERROR: create ao[1] failed! ret=%d\n", ret);
    return -1;
  }

  RK_U32 u32ReadSize = u32FrameCnt * u32ChnCnt * 2; // RK_SAMPLE_FMT_S16:2Bytes
  RK_U32 u32Timeval = u32FrameCnt * 1000000 / u32SampleRate; // us
  PlayBackHandle stPlayBackHdl0;
  stPlayBackHdl0.u32ChnIdx = 0;
  stPlayBackHdl0.pFileHandle = pInFile0;
  stPlayBackHdl0.u32Timeval = u32Timeval;
  stPlayBackHdl0.u32ReadSize = u32ReadSize;
  pthread_t play_thread0;
  pthread_create(&play_thread0, NULL, Spk0PlayThread, &stPlayBackHdl0);

  PlayBackHandle stPlayBackHdl1;
  stPlayBackHdl1.u32ChnIdx = 1;
  stPlayBackHdl1.pFileHandle = pInFile1;
  stPlayBackHdl1.u32Timeval = u32Timeval;
  stPlayBackHdl1.u32ReadSize = u32ReadSize;
  pthread_t play_thread1;
  pthread_create(&play_thread1, NULL, Spk1PlayThread, &stPlayBackHdl1);

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  while (!quit) {
    usleep(500000);
  }

  pthread_join(play_thread0, NULL);
  pthread_join(play_thread1, NULL);

  printf("%s exit!\n", __func__);
  RK_MPI_AO_DisableChn(0);
  RK_MPI_AO_DisableChn(1);

  return 0;
}
