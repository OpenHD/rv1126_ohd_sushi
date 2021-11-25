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
#include <time.h>
#include <unistd.h>

#include "common/sample_common.h"
#include "rkmedia_api.h"

typedef struct rkOcclusionDetection {
  RK_U32 u32ModeIdx;
  RK_U32 u32ChnIdx;
} OcclusionDetection;

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void occlusion_detection_cb(RK_VOID *pHandle, RK_VOID *pstEvent) {
  OcclusionDetection *pstMDHandle = (OcclusionDetection *)pHandle;
  OD_EVENT_S *pstOdEvent = (OD_EVENT_S *)pstEvent;
  RK_S32 s32ModeIdx = -1;
  RK_S32 s32ChnIdx = -1;
  if (pstMDHandle) {
    s32ModeIdx = (RK_S32)pstMDHandle->u32ModeIdx;
    s32ChnIdx = (RK_S32)pstMDHandle->u32ChnIdx;
  }

  if (pstEvent) {
    printf("@@@ OD: ModeID:%d, ChnID:%d, ODInfoCnt[%d], ORI:%dx%d\n",
           s32ModeIdx, s32ChnIdx, pstOdEvent->u16Cnt, pstOdEvent->u32Width,
           pstOdEvent->u32Height);
    for (int i = 0; i < pstOdEvent->u16Cnt; i++) {
      printf("--> %d rect:(%d, %d, %d, %d), Occlusion:%d\n", i,
             pstOdEvent->stRects[i].s32X, pstOdEvent->stRects[i].s32Y,
             pstOdEvent->stRects[i].u32Width, pstOdEvent->stRects[i].u32Height,
             pstOdEvent->u16Occlusion[i]);
    }
  }
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
  RK_S32 ret = 0;
  RK_S32 video_width = 1920;
  RK_S32 video_height = 1080;
  RK_S32 disp_width = 720;
  RK_S32 disp_height = 1280;
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
        iq_file_dir = "/oem/etc/iqfiles/";
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
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }
  RGA_ATTR_S stRgaAttr;
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 2;
  stRgaAttr.u16Rotaion = 90;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_height;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_RGB888;
  stRgaAttr.stImgOut.u32Width = disp_width;
  stRgaAttr.stImgOut.u32Height = disp_height;
  stRgaAttr.stImgOut.u32HorStride = disp_width;
  stRgaAttr.stImgOut.u32VirStride = disp_height;
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
    printf("Create rga[0] falied! ret=%d\n", ret);
    return -1;
  }

  VO_CHN_ATTR_S stVoAttr = {0};
  stVoAttr.enImgType = IMAGE_TYPE_RGB888;
  stVoAttr.u16Zpos = 0;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = disp_width;
  stVoAttr.stDispRect.u32Height = disp_height;
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  ALGO_OD_ATTR_S stOdChnAttr;
  stOdChnAttr.enImageType = IMAGE_TYPE_NV12;
  stOdChnAttr.u32Width = 1920;
  stOdChnAttr.u32Height = 1080;
  stOdChnAttr.u16RoiCnt = 1;
  stOdChnAttr.stRoiRects[0].s32X = 0;
  stOdChnAttr.stRoiRects[0].s32Y = 0;
  stOdChnAttr.stRoiRects[0].u32Width = 1920;
  stOdChnAttr.stRoiRects[0].u32Height = 1080;
  stOdChnAttr.u16Sensitivity = 30;
  ret = RK_MPI_ALGO_OD_CreateChn(0, &stOdChnAttr);
  if (ret) {
    printf("ERROR: OcclusionDetection Create failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stOdChn;
  stOdChn.enModId = RK_ID_ALGO_OD;
  stOdChn.s32DevId = 0;
  stOdChn.s32ChnId = 0;

  OcclusionDetection stODHandle;
  stODHandle.u32ChnIdx = 0;
  stODHandle.u32ModeIdx = RK_ID_ALGO_OD;

  ret =
      RK_MPI_SYS_RegisterEventCb(&stOdChn, &stODHandle, occlusion_detection_cb);
  if (ret) {
    printf("ERROR: MoveDetection register event failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn = {0};
  MPP_CHN_S stDestChn = {0};

  // RGA -> VO
  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  // VI -> RGA
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  // VI -> OD
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_ALGO_OD;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: Bind vi and od failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  while (!quit) {
    printf("\n##Keep OD working on 10s...\n");
    if (RK_MPI_ALGO_OD_EnableSwitch(0, RK_TRUE)) {
      printf("ERROR: Enable od error!\n");
      break;
    }
    sleep(20);
    printf("\n##Keep OD closing on 10s...\n");
    if (RK_MPI_ALGO_OD_EnableSwitch(0, RK_FALSE)) {
      printf("ERROR: Disable od error!\n");
      break;
    }
    sleep(10);
  }
  printf("%s exit!\n", __func__);

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
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_ALGO_OD;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[0] to od[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  RK_MPI_ALGO_OD_DestroyChn(0);
  RK_MPI_VO_DestroyChn(0);
  RK_MPI_RGA_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 0);

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(s32CamId);
#endif

  return 0;
}
