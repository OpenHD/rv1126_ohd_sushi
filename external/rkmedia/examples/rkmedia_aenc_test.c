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

typedef struct AudioParams_ {
  RK_U32 u32SampleRate;
  RK_U32 u32ChnCnt;
  SAMPLE_FORMAT_E enSampleFmt;
  const RK_CHAR *pOutPath;
} AudioParams;

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

#define MP3_PROFILE_LOW 1

typedef struct Mp3FreqIdx_ {
  RK_S32 u32SmpRate;
  RK_U8 u8FreqIdx;
} Mp3FreqIdx;

Mp3FreqIdx Mp3FreqIdxTbl[13] = {{96000, 0}, {88200, 1}, {64000, 2},  {48000, 3},
                                {44100, 4}, {32000, 5}, {24000, 6},  {22050, 7},
                                {16000, 8}, {12000, 9}, {11025, 10}, {8000, 11},
                                {7350, 12}};

static void GetMp3Header(RK_U8 *pu8Mp3Hdr, RK_S32 u32SmpRate, RK_U8 u8Channel,
                         RK_U32 u32DataLen) {
  RK_U8 u8FreqIdx = 0;
  for (int i = 0; i < 13; i++) {
    if (u32SmpRate == Mp3FreqIdxTbl[i].u32SmpRate) {
      u8FreqIdx = Mp3FreqIdxTbl[i].u8FreqIdx;
      break;
    }
  }

  RK_U32 u32PacketLen = u32DataLen + 7;
  pu8Mp3Hdr[0] = 0xFF;
  pu8Mp3Hdr[1] = 0xF1;
  pu8Mp3Hdr[2] = ((MP3_PROFILE_LOW) << 6) + (u8FreqIdx << 2) + (u8Channel >> 2);
  pu8Mp3Hdr[3] = (((u8Channel & 3) << 6) + (u32PacketLen >> 11));
  pu8Mp3Hdr[4] = ((u32PacketLen & 0x7FF) >> 3);
  pu8Mp3Hdr[5] = (((u32PacketLen & 7) << 5) + 0x1F);
  pu8Mp3Hdr[6] = 0xFC;
}

static void *GetMediaBuffer(void *params) {
  AudioParams *pstAudioParams = (AudioParams *)params;
  RK_U8 mp3_header[7];
  MEDIA_BUFFER mb = NULL;
  int cnt = 0;
  FILE *save_file = NULL;

  printf("#Start %s thread, SampleRate:%u, Channel:%u, Fmt:%x, Path:%s\n",
         __func__, pstAudioParams->u32SampleRate, pstAudioParams->u32ChnCnt,
         pstAudioParams->enSampleFmt, pstAudioParams->pOutPath);

  if (pstAudioParams->pOutPath) {
    save_file = fopen(pstAudioParams->pOutPath, "w");
    if (!save_file)
      printf("ERROR: Open %s failed!\n", pstAudioParams->pOutPath);
  }

  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_AENC, 0, 1000);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      continue;
    }

    printf("#%d Get Frame:ptr:%p, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld\n",
           cnt++, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb));

    if (save_file) {
      GetMp3Header(mp3_header, pstAudioParams->u32SampleRate,
                   pstAudioParams->u32ChnCnt, RK_MPI_MB_GetSize(mb));
      fwrite(mp3_header, 1, 7, save_file);
      fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), save_file);
    }
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  if (save_file)
    fclose(save_file);

  return NULL;
}

static RK_CHAR optstr[] = "?::d:c:r:s:i:";
static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-d default] [-r 16000] [-c 2] [-s 1024] -i /tmp/ai.pcm\n",
         name);
  printf("\t-d: device name, Default:\"default\"\n");
  printf("\t-r: sample rate, Default:16000\n");
  printf("\t-c: channel count, Default:2\n");
  printf("\t-s: frames cnt, Default:1024\n");
  printf("\t-i: input path.\n");
  printf("Notice: fmt always s16_le\n");
}

int main(int argc, char *argv[]) {
  RK_U32 u32SampleRate = 16000;
  RK_U32 u32BitRate = 64000; // 64kbps
  RK_U32 u32ChnCnt = 2;
  RK_U32 u32FrameCnt = 1024;
  // default:CARD=rockchiprk809co
  RK_CHAR *pDeviceName = "default";
  RK_CHAR *pInputPath = NULL;
  SAMPLE_FORMAT_E enSampleFmt = RK_SAMPLE_FMT_S16;
  RK_CHAR *pOutPath = "/tmp/aenc.mp3";
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
  printf("#Input Path: %s\n", pInputPath);
  printf("#Output Path: %s\n", pOutPath);

  FILE *file = fopen(pInputPath, "r");
  if (!file) {
    printf("ERROR: open %s failed!\n", pInputPath);
    return -1;
  }

  AudioParams stAudioParams;
  stAudioParams.u32SampleRate = u32SampleRate;
  stAudioParams.u32ChnCnt = u32ChnCnt;
  stAudioParams.enSampleFmt = enSampleFmt;
  stAudioParams.pOutPath = pOutPath;

  RK_MPI_SYS_Init();

  AENC_CHN_ATTR_S aenc_attr;
  aenc_attr.enCodecType = RK_CODEC_TYPE_MP3;
  aenc_attr.u32Bitrate = u32BitRate;
  aenc_attr.u32Quality = 1;
  aenc_attr.stAencMP3.u32Channels = u32ChnCnt;
  aenc_attr.stAencMP3.u32SampleRate = u32SampleRate;
  ret = RK_MPI_AENC_CreateChn(0, &aenc_attr);
  if (ret) {
    printf("Create AENC[0] failed! ret=%d\n", ret);
    return -1;
  }

  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, &stAudioParams);

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  MEDIA_BUFFER mb = NULL;
  RK_U32 u32Timeval = u32FrameCnt * 1000000 / u32SampleRate; // us
  RK_U32 u32ReadSize = u32FrameCnt * u32ChnCnt * 2; // RK_SAMPLE_FMT_S16:2Bytes
  printf("# TimeVal:%dus, ReadSize:%d\n", u32Timeval, u32ReadSize);
  MB_AUDIO_INFO_S stSampleInfo = {u32ChnCnt, u32SampleRate, u32FrameCnt,
                                  enSampleFmt};
  int i = 0;
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
    RK_MPI_MB_SetTimestamp(mb, u32Timeval * i);
    ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_AENC, 0, mb);
    if (ret) {
      printf("ERROR: RK_MPI_SYS_SendMediaBuffer failed! ret = %d\n", ret);
      break;
    }
    usleep(u32Timeval);
    RK_MPI_MB_ReleaseBuffer(mb);
    mb = NULL;
    i++;
  }

  if (mb)
    RK_MPI_MB_ReleaseBuffer(mb);

  printf("%s exit!\n", __func__);
  pthread_join(read_thread, NULL);
  RK_MPI_AENC_DestroyChn(0);

  return 0;
}
