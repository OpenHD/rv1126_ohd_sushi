// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/sample_common.h"
#include "rkmedia_api.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static RK_CHAR optstr[] = "?::a::I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]]"
         "[-I 0] "
         "[-M 0] "
         "\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
#else
  printf("\t%s [-I 0]\n", name);
#endif
  printf("\t-I | --camid: camera ctx id, Default 0\n");
}

int main(int argc, char *argv[]) {
  int ret = 0;
  RK_U32 u32DispWidth = 720;
  RK_U32 u32DispHeight = 1280;
  RK_U32 u32VideWidth = 640;
  RK_U32 u32VideoHeight = 480;
  // Chn layout: 2x4
  RK_U8 u8LayoutHor = 2;
  RK_U8 u8LayoutVer = 4;
  RK_S32 s32CamId = 0;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
#endif
  int c;
  char *iq_file_dir = NULL;
  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    const char *tmp_optarg = optarg;
    switch (c) {
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }
      if (tmp_optarg) {
        iq_file_dir = (char *)tmp_optarg;
      } else {
        iq_file_dir = "/oem/etc/iqfiles";
      }
      break;
    case 'I':
      s32CamId = atoi(optarg);
      break;
#ifdef RKAIQ
    case 'M':
      if (atoi(optarg)) {
        bMultictx = RK_TRUE;
      }
      break;
#endif
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#CameraIdx: %d\n\n", s32CamId);
  if (iq_file_dir) {
#ifdef RKAIQ
    printf("#Rkaiq XML DirPath: %s\n", iq_file_dir);
    printf("#bMultictx: %d\n\n", bMultictx);
    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    int fps = 30;
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, iq_file_dir);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
#endif
  }

  RK_MPI_SYS_Init();

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.u32BufCnt = 20;
  vi_chn_attr.u32Width = u32VideWidth;
  vi_chn_attr.u32Height = u32VideoHeight;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }

  // Chn layout: u8LayoutHor x u8LayoutVer
  RK_U32 u32ChnWidth = u32DispWidth / u8LayoutHor;
  RK_U32 u32ChnHeight = u32DispHeight / u8LayoutVer;
  RK_U16 u16ChnCnt = u8LayoutHor * u8LayoutVer;
  RK_U16 u16ChnIdx = 0;

  VMIX_DEV_INFO_S stDevInfo;
  stDevInfo.enImgType = IMAGE_TYPE_NV12;
  stDevInfo.u16ChnCnt = u16ChnCnt;
  stDevInfo.u16Fps = 30;
  stDevInfo.u32ImgWidth = u32DispWidth;
  stDevInfo.u32ImgHeight = u32DispHeight;
  stDevInfo.bEnBufPool = RK_TRUE;
  stDevInfo.u16BufPoolCnt = 2;

  for (RK_U8 u8VerIdx = 0; u8VerIdx < u8LayoutVer; u8VerIdx++) {
    for (RK_U8 u8HorIdx = 0; u8HorIdx < u8LayoutHor; u8HorIdx++) {
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.s32X = 0;
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.s32Y = 0;
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.u32Width = u32VideWidth;
      stDevInfo.stChnInfo[u16ChnIdx].stInRect.u32Height = u32VideoHeight;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.s32X = u8HorIdx * u32ChnWidth;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.s32Y = u8VerIdx * u32ChnHeight;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.u32Width = u32ChnWidth;
      stDevInfo.stChnInfo[u16ChnIdx].stOutRect.u32Height = u32ChnHeight;
      printf("#CHN[%d]:IN<0,0,%d,%d> --> Out<%d,%d,%d,%d>\n", u16ChnIdx,
             u32VideWidth, u32VideoHeight, u8HorIdx * u32ChnWidth,
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

  VMIX_LINE_INFO_S stVmLine;
  memset(&stVmLine, 0, sizeof(stVmLine));
  for (RK_U8 u8HorIdx = 1; u8HorIdx < u8LayoutHor; u8HorIdx++) {
    stVmLine.stLines[stVmLine.u32LineCnt].s32X =
        u32DispWidth * u8HorIdx / u8LayoutHor;
    stVmLine.stLines[stVmLine.u32LineCnt].s32Y = 0;
    stVmLine.stLines[stVmLine.u32LineCnt].u32Width = 2;
    stVmLine.stLines[stVmLine.u32LineCnt].u32Height = u32DispHeight;
    stVmLine.u8Enable[stVmLine.u32LineCnt] = 1;
    stVmLine.u32LineCnt++;
  }
  for (RK_U8 u8VerIdx = 1; u8VerIdx < u8LayoutVer; u8VerIdx++) {
    stVmLine.stLines[stVmLine.u32LineCnt].s32X = 0;
    stVmLine.stLines[stVmLine.u32LineCnt].s32Y =
        u32DispHeight * u8VerIdx / u8LayoutVer;
    stVmLine.stLines[stVmLine.u32LineCnt].u32Width = u32DispWidth;
    stVmLine.stLines[stVmLine.u32LineCnt].u32Height = 2;
    stVmLine.u8Enable[stVmLine.u32LineCnt] = 1;
    stVmLine.u32LineCnt++;
  }
  /* only last channel need line info */
  ret = RK_MPI_VMIX_SetLineInfo(0, stDevInfo.u16ChnCnt - 1, stVmLine);
  if (ret) {
    printf("Set VM[0] line info failed! ret=%d\n", ret);
    return -1;
  }

  // VO[1] for overlay plane
  VO_CHN_ATTR_S stVoAttr;
  memset(&stVoAttr, 0, sizeof(stVoAttr));
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
  stVoAttr.enImgType = IMAGE_TYPE_NV12;
  stVoAttr.u16Zpos = 1;
  stVoAttr.stImgRect.s32X = 0;
  stVoAttr.stImgRect.s32Y = 0;
  stVoAttr.stImgRect.u32Width = u32DispWidth;
  stVoAttr.stImgRect.u32Height = u32DispHeight;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = u32DispWidth;
  stVoAttr.stDispRect.u32Height = u32DispHeight;
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
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
    printf("#Bind VI[0] to VM[0]:Chn[%u]....\n", i);
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_VMIX;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = i;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("Bind vi[0] to vmix[0]:Chn[%u] failed! ret=%d\n", i, ret);
      return -1;
    }
    //    RK_MPI_SYS_DumpChn(RK_ID_VMIX);
    //    getchar();
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    /* test hide and show channel */
    RK_MPI_VMIX_HideChn(0, 0);
    usleep(500000);
    RK_MPI_VMIX_ShowChn(0, 0);
    usleep(500000);
  }

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
    printf("#UnBind VI[0] to VM[0]:Chn[%u]....\n", i);
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_VMIX;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = i;
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("UnBind vi[0] to vmix[0]:Chn[%u] failed! ret=%d\n", i, ret);
      return -1;
    }
  }

  ret = RK_MPI_VO_DestroyChn(0);
  if (ret)
    printf("Destroy VO[0] failed! ret=%d\n", ret);

  for (RK_U16 i = 0; i < u16ChnCnt; i++) {
    ret = RK_MPI_VMIX_DisableChn(0, i);
    if (ret)
      printf("Disable VIMX[0]:Chn[%u] failed! ret=%d\n", i, ret);
  }

  ret = RK_MPI_VMIX_DestroyDev(0);
  if (ret) {
    printf("DeInit VIMX[0] failed! ret=%d\n", ret);
  }

  ret = RK_MPI_VI_DisableChn(s32CamId, 0);
  if (ret)
    printf("Disable VI[0] failed! ret=%d\n", ret);

  if (iq_file_dir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }
  return 0;
}
