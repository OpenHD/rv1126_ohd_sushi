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
#include <rga/RgaApi.h>
#include <rga/rga.h>

static int video_width = 1920;
static int video_height = 1080;
static int disp_width = 720;
static int disp_height = 1280;

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void usage(char *name) {
  printf("%s [-a id_dir] [-W video_width] [-H video_height]\n"
         "\t[-w disp_width] [-h disp_height] [-u ui(0/1)]\n",
         name);
  exit(-1);
}

int main(int argc, char *argv[]) {
  int ret = 0;
  int ui = 1;
  char *iq_dir = NULL;
  int ch;

  while ((ch = getopt(argc, argv, "a:W:H:w:h:u:")) != -1) {
    switch (ch) {
    case 'a':
      iq_dir = optarg;
      if (access(iq_dir, R_OK))
        usage(argv[0]);
      break;
    case 'W':
      video_width = atoi(optarg);
      break;
    case 'H':
      video_height = atoi(optarg);
      break;
    case 'w':
      disp_width = atoi(optarg);
      break;
    case 'h':
      disp_height = atoi(optarg);
      break;
    case 'u':
      ui = atoi(optarg);
      if (ui != 0 && ui != 1)
        usage(argv[0]);
      break;
    default:
      usage(argv[0]);
      break;
    }
  }

#ifdef RKAIQ
  ret = SAMPLE_COMM_ISP_Init(0, RK_AIQ_WORKING_MODE_NORMAL, RK_TRUE, iq_dir);
  if (ret)
    return -1;
  SAMPLE_COMM_ISP_Run(0);
  ret = SAMPLE_COMM_ISP_Init(1, RK_AIQ_WORKING_MODE_NORMAL, RK_TRUE, iq_dir);
  if (ret)
    return -1;
  SAMPLE_COMM_ISP_Run(1);
#endif // RKAIQ

  RK_MPI_SYS_Init();
  VI_CHN_ATTR_S vi_chn_attr;
  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(0, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(0, 0);
  if (ret) {
    printf("Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }

  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(1, 1, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(1, 1);
  if (ret) {
    printf("Create vi[1] failed! ret=%d\n", ret);
    return -1;
  }

  // Chn layout: 1x2
  RK_U8 u8LayoutHor = 1;
  RK_U8 u8LayoutVer = 2;
  // Chn layout: u8LayoutHor x u8LayoutVer
  RK_U32 u32ChnWidth = disp_height / u8LayoutHor;
  RK_U32 u32ChnHeight = disp_height / u8LayoutVer;
  RK_U16 u16ChnCnt = u8LayoutHor * u8LayoutVer;
  RK_U16 u16ChnIdx = 0;

  VMIX_DEV_INFO_S stDevInfo;
  stDevInfo.enImgType = IMAGE_TYPE_NV12;
  stDevInfo.u16ChnCnt = u16ChnCnt;
  stDevInfo.u16Fps = 30;
  stDevInfo.u32ImgWidth = disp_height;
  stDevInfo.u32ImgHeight = disp_height;
  stDevInfo.bEnBufPool = RK_TRUE;
  stDevInfo.u16BufPoolCnt = 2;

  for (RK_U8 u8VerIdx = 0; u8VerIdx < u8LayoutVer; u8VerIdx++) {
    for (RK_U8 u8HorIdx = 0; u8HorIdx < u8LayoutHor; u8HorIdx++) {
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.s32X = 0;
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.s32Y = 0;
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.u32Width = video_width;
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.u32Height = video_height;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.s32X = u8HorIdx * u32ChnWidth;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.s32Y = u8VerIdx * u32ChnHeight;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.u32Width = u32ChnWidth;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.u32Height = u32ChnHeight;
      printf("#CHN[%d]:IN<0,0,%d,%d> --> Out<%d,%d,%d,%d>\n", u16ChnIdx,
             video_width, video_height, u8HorIdx * u32ChnWidth,
             u8VerIdx * u32ChnHeight, u32ChnWidth, u32ChnHeight);
      u16ChnIdx++;
    }
  }

  ret = RK_MPI_VMIX_CreateDev(0, &stDevInfo);
  if (ret) {
    printf("Init VMIX device failed! ret=%d\n", ret);
    return -1;
  }

  for (RK_U16 i = 0; i < stDevInfo.u16ChnCnt; i++) {
    ret = RK_MPI_VMIX_EnableChn(0, i);
    if (ret) {
      printf("Enable VM[0]:Chn[%d] failed! ret=%d\n", i, ret);
      return -1;
    }
  }

  VO_CHN_ATTR_S stVoAttr = {0};
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
  stVoAttr.enImgType = IMAGE_TYPE_NV12;
  stVoAttr.u16Zpos = 0;
  stVoAttr.stImgRect.s32X = 0;
  stVoAttr.stImgRect.s32Y = 0;
  stVoAttr.stImgRect.u32Width = disp_width;
  stVoAttr.stImgRect.u32Height = disp_height;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = disp_width;
  stVoAttr.stDispRect.u32Height = disp_height;
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  // VO[1] for primary plane
  memset(&stVoAttr, 0, sizeof(stVoAttr));
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
  stVoAttr.enImgType = IMAGE_TYPE_RGB888;
  stVoAttr.u16Zpos = ui;
  stVoAttr.stImgRect.s32X = 0;
  stVoAttr.stImgRect.s32Y = 0;
  stVoAttr.stImgRect.u32Width = disp_width;
  stVoAttr.stImgRect.u32Height = disp_height;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = disp_width;
  stVoAttr.stDispRect.u32Height = disp_height;
  ret = RK_MPI_VO_CreateChn(1, &stVoAttr);
  if (ret) {
    printf("Create vo[1] failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn = {0};
  MPP_CHN_S stDestChn = {0};

  printf("#Bind VMX[0] to VO[0]....\n");
  stSrcChn.enModId = RK_ID_VMIX;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0; // invalid
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind VMX[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  for (RK_U16 i = 0; i < u16ChnCnt; i++) {
    printf("#Bind VI[%u] to VM[0]:Chn[%u]....\n", i, i);
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = i;
    stSrcChn.s32ChnId = i;
    stDestChn.enModId = RK_ID_VMIX;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = i;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("Bind vi[%u] to vmix[0]:Chn[%u] failed! ret=%d\n", i, i, ret);
      return -1;
    }
    //    RK_MPI_SYS_DumpChn(RK_ID_VMIX);
    //    getchar();
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);
  printf("#UnBind VMX[0] to VO[0]....\n");
  stSrcChn.enModId = RK_ID_VMIX;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0; // invalid
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind VMX[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  for (RK_U16 i = 0; i < u16ChnCnt; i++) {
    printf("#UnBind VI[%u] to VM[0]:Chn[%u]....\n", i, i);
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = i;
    stSrcChn.s32ChnId = i;
    stDestChn.enModId = RK_ID_VMIX;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = i;
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("UnBind vi[%u] to vmix[0]:Chn[%u] failed! ret=%d\n", i, i, ret);
      return -1;
    }
  }

  RK_MPI_VO_DestroyChn(0);
  RK_MPI_VO_DestroyChn(1);

  for (RK_U16 i = 0; i < u16ChnCnt; i++) {
    ret = RK_MPI_VMIX_DisableChn(0, i);
    if (ret)
      printf("Disable VIMX[0]:Chn[%u] failed! ret=%d\n", i, ret);
  }

  ret = RK_MPI_VMIX_DestroyDev(0);
  if (ret) {
    printf("DeInit VIMX[0] failed! ret=%d\n", ret);
  }

  RK_MPI_VI_DisableChn(0, 0);
  RK_MPI_VI_DisableChn(1, 1);

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(0);
  SAMPLE_COMM_ISP_Stop(1);
#endif

  return 0;
}
