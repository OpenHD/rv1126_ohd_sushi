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

#include "common/sample_common.h"
#include "rkmedia_api.h"
#include "rkmedia_vdec.h"

#define INBUF_SIZE 4096

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 1280

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static void *GetMediaBuffer(void *arg) {
  (void)arg;
  MEDIA_BUFFER mb = NULL;
  int ret = 0;

  MPP_CHN_S VdecChn, VoChn;
  VdecChn.enModId = RK_ID_VDEC;
  VdecChn.s32DevId = 0;
  VdecChn.s32ChnId = 0;
  VoChn.enModId = RK_ID_VO;
  VoChn.s32DevId = 0;
  VoChn.s32ChnId = 0;

  mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VDEC, 0, 5000);
  if (!mb) {
    printf("RK_MPI_SYS_GetMediaBuffer get null buffer in 5s...\n");
    return NULL;
  }

  MB_IMAGE_INFO_S stImageInfo = {0};
  ret = RK_MPI_MB_GetImageInfo(mb, &stImageInfo);
  if (ret) {
    printf("Get image info failed! ret = %d\n", ret);
    RK_MPI_MB_ReleaseBuffer(mb);
    return NULL;
  }

  printf("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
         "timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",
         RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
         RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
         RK_MPI_MB_GetTimestamp(mb), stImageInfo.u32Width,
         stImageInfo.u32Height, stImageInfo.enImgType);
  RK_MPI_MB_ReleaseBuffer(mb);

  VO_CHN_ATTR_S stVoAttr = {0};
  memset(&stVoAttr, 0, sizeof(stVoAttr));
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
  stVoAttr.enImgType = stImageInfo.enImgType;
  stVoAttr.u16Zpos = 1;
  stVoAttr.u32Width = SCREEN_WIDTH;
  stVoAttr.u32Height = SCREEN_HEIGHT;
  stVoAttr.stImgRect.s32X = 0;
  stVoAttr.stImgRect.s32Y = 0;
  stVoAttr.stImgRect.u32Width = stImageInfo.u32Width;
  stVoAttr.stImgRect.u32Height = stImageInfo.u32Height;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = stImageInfo.u32Width;
  stVoAttr.stDispRect.u32Height = stImageInfo.u32Height;
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create VO[0] failed! ret=%d\n", ret);
    quit = false;
    return NULL;
  }

  // VDEC->VO
  ret = RK_MPI_SYS_Bind(&VdecChn, &VoChn);
  if (ret) {
    printf("Bind VDEC[0] to VO[0] failed! ret=%d\n", ret);
    quit = false;
    return NULL;
  }

  while (!quit) {
    usleep(500000);
  }

  ret = RK_MPI_SYS_UnBind(&VdecChn, &VoChn);
  if (ret)
    printf("UnBind VDECp[0] to VO[0] failed! ret=%d\n", ret);

  ret = RK_MPI_VO_DestroyChn(VoChn.s32ChnId);
  if (ret)
    printf("Destroy VO[0] failed! ret=%d\n", ret);

  return NULL;
}

static RK_CHAR optstr[] = "?::i:o:f:w:h:t:l:";
static void print_usage() {
  printf("usage example: rkmedia_vdec_test -w 720 -h 480 -i /userdata/out.jpeg "
         "-f 0 -t JPEG.\n");
  printf("\t-w: DisplayWidth, Default: 720\n");
  printf("\t-h: DisplayHeight, Default: 1280\n");
  printf("\t-i: InputFilePath, Default: NULL\n");
  printf("\t-f: 1:hardware; 0:software. Default:hardware\n");
  printf("\t-l: LoopSwitch; 0:NoLoop; 1:Loop. Default: 0.\n");
  printf("\t-t: codec type, Default H264, support H264/H265/JPEG.\n");
}

int main(int argc, char *argv[]) {
  RK_CHAR *pcFileName = NULL;
  RK_U32 u32DispWidth = 720;
  RK_U32 u32DispHeight = 1280;
  RK_BOOL bIsHardware = RK_TRUE;
  RK_U32 u32Loop = 0;
  CODEC_TYPE_E enCodecType = RK_CODEC_TYPE_H264;
  int c, ret;

  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'w':
      u32DispWidth = atoi(optarg);
      break;
    case 'h':
      u32DispHeight = atoi(optarg);
      break;
    case 'i':
      pcFileName = optarg;
      break;
    case 'f':
      bIsHardware = atoi(optarg) ? RK_TRUE : RK_FALSE;
      break;
    case 'l':
      u32Loop = atoi(optarg);
      break;
    case 't':
      if (strcmp(optarg, "H264") == 0) {
        enCodecType = RK_CODEC_TYPE_H264;
      } else if (strcmp(optarg, "H265") == 0) {
        enCodecType = RK_CODEC_TYPE_H265;
      } else if (strcmp(optarg, "JPEG") == 0) {
        enCodecType = RK_CODEC_TYPE_JPEG;
      }
      break;
    case '?':
    default:
      print_usage();
      return 0;
    }
  }

  printf("#FileName: %s\n", pcFileName);
  printf("#Display wxh: %dx%d\n", u32DispWidth, u32DispHeight);
  printf("#Decode Mode: %s\n", bIsHardware ? "Hardware" : "Software");
  printf("#Loop Cnt: %d\n", u32Loop);

  signal(SIGINT, sigterm_handler);

  FILE *infile = fopen(pcFileName, "rb");
  if (!infile) {
    fprintf(stderr, "Could not open %s\n", pcFileName);
    return 0;
  }

  RK_MPI_SYS_Init();

  // VDEC
  VDEC_CHN_ATTR_S stVdecAttr;
  stVdecAttr.enCodecType = enCodecType;
  stVdecAttr.enMode = VIDEO_MODE_FRAME;
  if (bIsHardware) {
    if (stVdecAttr.enCodecType == RK_CODEC_TYPE_JPEG) {
      stVdecAttr.enMode = VIDEO_MODE_FRAME;
    } else {
      stVdecAttr.enMode = VIDEO_MODE_STREAM;
    }
    stVdecAttr.enDecodecMode = VIDEO_DECODEC_HADRWARE;
  } else {
    stVdecAttr.enMode = VIDEO_MODE_FRAME;
    stVdecAttr.enDecodecMode = VIDEO_DECODEC_SOFTWARE;
  }

  ret = RK_MPI_VDEC_CreateChn(0, &stVdecAttr);
  if (ret) {
    printf("Create Vdec[0] failed! ret=%d\n", ret);
    return -1;
  }

  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, NULL);

  int data_size;
  int read_size;
  if (stVdecAttr.enMode == VIDEO_MODE_STREAM) {
    data_size = INBUF_SIZE;
  } else if (stVdecAttr.enMode == VIDEO_MODE_FRAME) {
    fseek(infile, 0, SEEK_END);
    data_size = ftell(infile);
    fseek(infile, 0, SEEK_SET);
  }

  while (!quit) {
    MEDIA_BUFFER mb = RK_MPI_MB_CreateBuffer(data_size, RK_FALSE, 0);
  RETRY:
    /* read raw data from the input file */
    read_size = fread(RK_MPI_MB_GetPtr(mb), 1, data_size, infile);
    if (!read_size || feof(infile)) {
      if (u32Loop) {
        fseek(infile, 0, SEEK_SET);
        goto RETRY;
      } else {
        RK_MPI_MB_ReleaseBuffer(mb);
        break;
      }
    }
    RK_MPI_MB_SetSize(mb, read_size);
    printf("#Send packet(%p, %zuBytes) to VDEC[0].\n", RK_MPI_MB_GetPtr(mb),
           RK_MPI_MB_GetSize(mb));
    ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VDEC, 0, mb);
    RK_MPI_MB_ReleaseBuffer(mb);

    usleep(30 * 1000);
  }

  quit = true;
  pthread_join(read_thread, NULL);

  RK_MPI_VDEC_DestroyChn(0);
  fclose(infile);

  return 0;
}
