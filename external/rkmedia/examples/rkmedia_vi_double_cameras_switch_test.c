// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/sample_common.h"
#include "rkmedia_api.h"
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

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}
static void *GetBuffer(void *arg) {
  MEDIA_BUFFER mb = NULL;
  int *id = (int *)arg;
  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, *id, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }
    RK_MPI_MB_ReleaseBuffer(mb);
  }
  return NULL;
}
void usage(char *name) {
  printf("%s [-a id_dir] [-W video_width] [-H video_height]\n"
         "\t[-w disp_width] [-h disp_height] [-u ui(0/1)]\n"
         "\t[-i id(0/1)]\n",
         name);
  exit(-1);
}
int main(int argc, char *argv[]) {
  int ret = 0;
  int video_width = 1920;
  int video_hegith = 1080;
  int disp_width = 720;
  int disp_height = 1280;
  int id = 0;
  char *iq_dir = NULL;
  int ui = 1;
  int ch;

  while ((ch = getopt(argc, argv, "a:W:H:w:h:u:i:")) != -1) {
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
      video_hegith = atoi(optarg);
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
    case 'i':
      id = atoi(optarg);
      if (id != 0 && id != 1)
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
#endif

  RK_MPI_SYS_Init();
  VI_CHN_ATTR_S vi_chn_attr;
  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_hegith;
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
  vi_chn_attr.u32Height = video_hegith;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(1, 1, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(1, 1);
  if (ret) {
    printf("Create vi[1] failed! ret=%d\n", ret);
    return -1;
  }
  RGA_ATTR_S stRgaAttr;
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 2;
  stRgaAttr.u16Rotaion = 90;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_hegith;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_hegith;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgOut.u32Width = disp_width;
  stRgaAttr.stImgOut.u32Height = disp_height;
  stRgaAttr.stImgOut.u32HorStride = disp_width;
  stRgaAttr.stImgOut.u32VirStride = disp_height;
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
    printf("Create rga[0] falied! ret=%d\n", ret);
    return -1;
  }
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 2;
  stRgaAttr.u16Rotaion = 270;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_hegith;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_hegith;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgOut.u32Width = disp_width;
  stRgaAttr.stImgOut.u32Height = disp_height;
  stRgaAttr.stImgOut.u32HorStride = disp_width;
  stRgaAttr.stImgOut.u32VirStride = disp_height;
  ret = RK_MPI_RGA_CreateChn(1, &stRgaAttr);
  if (ret) {
    printf("Create rga[1] falied! ret=%d\n", ret);
    return -1;
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
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[1] to rga[1] failed! ret=%d\n", ret);
    return -1;
  }
  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = id;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[%d] to rga[0] failed! ret=%d\n", id, ret);
    return -1;
  }
  pthread_t th;
  if (pthread_create(&th, NULL, GetBuffer, &id)) {
    printf("create GetBuffer thread failed!\n");
    return -1;
  }
  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }
  printf("%s exit!\n", __func__);
  pthread_join(th, NULL);
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[1] to rga[1] failed! ret=%d\n", ret);
    return -1;
  }
  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = id;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[%d] to vo[0] failed! ret=%d\n", id, ret);
    return -1;
  }
  RK_MPI_VO_DestroyChn(0);
  RK_MPI_VO_DestroyChn(1);
  RK_MPI_RGA_DestroyChn(0);
  RK_MPI_RGA_DestroyChn(1);
  RK_MPI_VI_DisableChn(0, 0);
  RK_MPI_VI_DisableChn(1, 1);
#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(0);
  SAMPLE_COMM_ISP_Stop(1);
#endif

  return 0;
}
