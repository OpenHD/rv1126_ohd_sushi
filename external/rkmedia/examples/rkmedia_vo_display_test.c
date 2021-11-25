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

#include "common/sample_common.h"
#include "rkmedia_api.h"

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 1280

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static RK_CHAR optstr[] = "?::d:t:s:x:y:w:h:z:f:";
static void print_usage(const RK_CHAR *name) {
  printf("#Usage example:\n");
  printf("\t%s [-d /dev/dri/card0] [-t Primary] [-s 0] [-x 0] [-y 0] [-w 720] "
         "[-w 1280] [-z 0] [-f 60]\n",
         name);
  printf("\t-d: display card node, Default:\"/dev/dri/card0\"\n");
  printf("\t-t: plane type, Default:Primary, Value:Primary, Overlay\n");
  printf("\t-s: Random gen x/y pos, Valud:0,disable; 1,enable. default: 0.\n");
  printf("\t-x: display x pos, Default:0\n");
  printf("\t-y: display y pos, Default:0\n");
  printf("\t-w: display width, Default:720\n");
  printf("\t-h: display height, Default:1280\n");
  printf("\t-f: display fps, Default:60\n");
  printf("\t-z: plane zpos, Default:0, value[0, 2]\n");

  printf("#Note:\n\tPrimary plane format: RGB888\n");
  printf("\tOverlay plane format: NV12\n");
}

int main(int argc, char *argv[]) {
  RK_CHAR *pDeviceName = "/dev/dri/card0";
  RK_CHAR *u32PlaneType = "Primary";
  RK_U8 u8EnableRandom = 0;
  RK_S32 s32Xpos = 0;
  RK_S32 s32Ypos = 0;
  RK_U32 u32Fps = 0;
  RK_U32 u32Zpos = 0;
  RK_U32 u32DispWidth = 720;
  RK_U32 u32DispHeight = 1280;
  RK_FLOAT fltImgRatio = 0.0;
  RK_U32 u32FrameSize = 0;
  int ret = 0;
  int c;

  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'd':
      pDeviceName = optarg;
      break;
    case 't':
      u32PlaneType = optarg;
      break;
    case 's':
      u8EnableRandom = (RK_U8)atoi(optarg);
      break;
    case 'x':
      s32Xpos = atoi(optarg);
      break;
    case 'y':
      s32Ypos = atoi(optarg);
      break;
    case 'w':
      u32DispWidth = (RK_U32)atoi(optarg);
      break;
    case 'h':
      u32DispHeight = (RK_U32)atoi(optarg);
      break;
    case 'z':
      u32Zpos = (RK_U32)atoi(optarg);
      break;
    case 'f':
      u32Fps = (RK_U32)atoi(optarg);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#Device: %s\n", pDeviceName);
  printf("#PlaneType: %s\n", u32PlaneType);
  printf("#Display RandomPos: %s\n", u8EnableRandom ? "Enable" : "Disable");
  printf("#Display Xpos: %d\n", s32Xpos);
  printf("#Display Ypos: %d\n", s32Ypos);
  printf("#Display Width: %u\n", u32DispWidth);
  printf("#Display Height: %u\n", u32DispHeight);
  printf("#Display Zpos: %u\n", u32Zpos);
  printf("#Display Fps: %u\n", u32Fps);
  RK_MPI_SYS_Init();

  VO_CHN_ATTR_S stVoAttr = {0};
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.u32Width = SCREEN_WIDTH;
  stVoAttr.u32Height = SCREEN_HEIGHT;
  stVoAttr.u16Fps = u32Fps;
  stVoAttr.u16Zpos = u32Zpos;
  stVoAttr.stImgRect.s32X = 0;
  stVoAttr.stImgRect.s32Y = 0;
  stVoAttr.stImgRect.u32Width = u32DispWidth;
  stVoAttr.stImgRect.u32Height = u32DispHeight;
  stVoAttr.stDispRect.s32X = s32Xpos;
  stVoAttr.stDispRect.s32Y = s32Ypos;
  stVoAttr.stDispRect.u32Width = u32DispWidth;
  stVoAttr.stDispRect.u32Height = u32DispHeight;
  if (!strcmp(u32PlaneType, "Primary")) {
    // VO[0] for primary plane
    stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
    stVoAttr.enImgType = IMAGE_TYPE_RGB888;
    fltImgRatio = 3.0; // for RGB888
  } else if (!strcmp(u32PlaneType, "Overlay")) {
    // VO[0] for overlay plane
    stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
    stVoAttr.enImgType = IMAGE_TYPE_NV12;
    fltImgRatio = 1.5; // for NV12
  } else {
    printf("ERROR: Unsupport plane type:%s\n", u32PlaneType);
    return -1;
  }

  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  // Create buffer pool. Note that VO only support dma buffer.
  MB_POOL_PARAM_S stBufferPoolParam;
  stBufferPoolParam.u32Cnt = 3;
  stBufferPoolParam.u32Size =
      0; // Automatic calculation using imgInfo internally
  stBufferPoolParam.enMediaType = MB_TYPE_VIDEO;
  stBufferPoolParam.bHardWare = RK_TRUE;
  stBufferPoolParam.u16Flag = MB_FLAG_NOCACHED;
  stBufferPoolParam.stImageInfo.enImgType = stVoAttr.enImgType;
  stBufferPoolParam.stImageInfo.u32Width = SCREEN_WIDTH;
  stBufferPoolParam.stImageInfo.u32Height = SCREEN_HEIGHT;
  stBufferPoolParam.stImageInfo.u32HorStride = SCREEN_WIDTH;
  stBufferPoolParam.stImageInfo.u32VerStride = SCREEN_HEIGHT;

  MEDIA_BUFFER_POOL mbp = RK_MPI_MB_POOL_Create(&stBufferPoolParam);
  if (!mbp) {
    printf("Create buffer pool for vo failed!\n");
    return -1;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  RK_U32 u32FrameId = 0;
  RK_U64 u64TimePeriod = 33333; // us
  MB_IMAGE_INFO_S stImageInfo = {0};
  u32FrameSize = (RK_U32)(u32DispWidth * u32DispHeight * fltImgRatio);

  while (!quit) {
    // Get mb buffer from bufferpool
    MEDIA_BUFFER mb = RK_MPI_MB_POOL_GetBuffer(mbp, RK_TRUE);
    if (!mb) {
      printf("WARN: BufferPool get null buffer...\n");
      usleep(10000);
      continue;
    }

    if (u8EnableRandom) {
      srand((unsigned)time(0));
      u32DispWidth = rand() % (SCREEN_WIDTH);
      u32DispHeight = rand() % (SCREEN_HEIGHT);
      if (u32DispWidth < (SCREEN_WIDTH / 4))
        u32DispWidth = SCREEN_WIDTH / 4;
      if (u32DispHeight < (SCREEN_HEIGHT / 4))
        u32DispHeight = SCREEN_HEIGHT / 4;
      s32Xpos = rand() % (SCREEN_WIDTH - u32DispWidth);
      s32Ypos = rand() % (SCREEN_HEIGHT - u32DispHeight);

      stVoAttr.stImgRect.s32X = 0;
      stVoAttr.stImgRect.s32Y = 0;
      stVoAttr.stImgRect.u32Width = u32DispWidth;
      stVoAttr.stImgRect.u32Height = u32DispHeight;
      stVoAttr.stDispRect.s32X = s32Xpos;
      stVoAttr.stDispRect.s32Y = s32Ypos;
      stVoAttr.stDispRect.u32Width = u32DispWidth;
      stVoAttr.stDispRect.u32Height = u32DispHeight;
      ret = RK_MPI_VO_SetChnAttr(0, &stVoAttr);
      if (ret) {
        printf("Set VO Attr:%d, %d, %d, %d failed!\n", s32Xpos, s32Ypos,
               u32DispWidth, u32DispHeight);
        break;
      }

      // resize image wxh.
      stImageInfo.u32Width = u32DispWidth;
      stImageInfo.u32Height = u32DispHeight;
      stImageInfo.u32HorStride = u32DispWidth;
      stImageInfo.u32VerStride = u32DispHeight;
      stImageInfo.enImgType = stVoAttr.enImgType;
      mb = RK_MPI_MB_ConvertToImgBuffer(mb, &stImageInfo);
      if (!mb) {
        printf("ERROR: convert to img buffer failed!\n");
        break;
      }
      u32FrameSize = (RK_U32)(u32DispWidth * u32DispHeight * fltImgRatio);
    }

    memset(RK_MPI_MB_GetPtr(mb), 0xFF, u32FrameSize);
    RK_MPI_MB_SetSize(mb, u32FrameSize);
    printf("#Display Image[%d]:<%d,%d,%u,%u> to VO[0]:<%d,%d,%u,%u>...\n",
           u32FrameId++, 0, 0, u32DispWidth, u32DispHeight, s32Xpos, s32Ypos,
           u32DispWidth, u32DispHeight);
    ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 0, mb);
    if (ret) {
      printf("ERROR: RK_MPI_SYS_SendMediaBuffer to VO[0] failed! ret=%d\n",
             ret);
      RK_MPI_MB_ReleaseBuffer(mb);
      break;
    }
    // mb must be release. The encoder has internal references to the data sent
    // in. Therefore, mb cannot be reused directly
    RK_MPI_MB_ReleaseBuffer(mb);
    usleep(u64TimePeriod);
  }

  RK_MPI_VO_DestroyChn(0);
  // buffer pool destroy should after destroy vo.
  RK_MPI_MB_POOL_Destroy(mbp);

  return 0;
}
