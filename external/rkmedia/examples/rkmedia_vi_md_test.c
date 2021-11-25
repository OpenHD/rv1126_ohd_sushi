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
#include "rkmedia_move_detection.h"

typedef struct rkMoveDetectionHandle {
  RK_U32 u32ModeIdx;
  RK_U32 u32ChnIdx;
} MoveDetectionHandle;

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void move_detection_cb(RK_VOID *pHandle, RK_VOID *pstEvent) {
  MoveDetectionHandle *pstMDHandle = (MoveDetectionHandle *)pHandle;
  MD_EVENT_S *pstMdEvent = (MD_EVENT_S *)pstEvent;
  RK_S32 s32ModeIdx = -1;
  RK_S32 s32ChnIdx = -1;
  if (pstMDHandle) {
    s32ModeIdx = (RK_S32)pstMDHandle->u32ModeIdx;
    s32ChnIdx = (RK_S32)pstMDHandle->u32ChnIdx;
  }

  if (pstMdEvent) {
    printf("@@@ MD: ModeID:%d, ChnID:%d, MDInfoCnt[%d], ORI:%dx%d\n",
           s32ModeIdx, s32ChnIdx, pstMdEvent->u16Cnt, pstMdEvent->u32Width,
           pstMdEvent->u32Height);
    for (int i = 0; i < pstMdEvent->u16Cnt; i++) {
      printf("--> %d rect:(%d, %d, %d, %d)\n", i, pstMdEvent->stRects[i].s32X,
             pstMdEvent->stRects[i].s32Y, pstMdEvent->stRects[i].u32Width,
             pstMdEvent->stRects[i].u32Height);
    }
  }
}

static RK_CHAR optstr[] = "?::a::I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"help", optional_argument, NULL, '?'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
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
  int c;
  char *iq_file_dir = NULL;
  RK_S32 s32CamId = 0;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
#endif

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
  vi_chn_attr.u32Width = 640;
  vi_chn_attr.u32Height = 480;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 1, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 1);
  if (ret) {
    printf("ERROR: Vi Create failed!\n");
    exit(0);
  }

  ALGO_MD_ATTR_S md_chn_attr;
  md_chn_attr.imageType = IMAGE_TYPE_NV12;
  md_chn_attr.u16Sensitivity = 70;
  md_chn_attr.u32Width = 640;
  md_chn_attr.u32Height = 480;
  md_chn_attr.u16RoiCnt = 2;
  md_chn_attr.stRoiRects[0].s32X = 0;
  md_chn_attr.stRoiRects[0].s32Y = 0;
  md_chn_attr.stRoiRects[0].u32Width = 160;
  md_chn_attr.stRoiRects[0].u32Height = 120;
  md_chn_attr.stRoiRects[1].s32X = 160;
  md_chn_attr.stRoiRects[1].s32Y = 120;
  md_chn_attr.stRoiRects[1].u32Width = 160;
  md_chn_attr.stRoiRects[1].u32Height = 120;
  ret = RK_MPI_ALGO_MD_CreateChn(0, &md_chn_attr);
  if (ret) {
    printf("ERROR: MoveDetection Create failed!\n");
    exit(0);
  }

  MPP_CHN_S stMdChn;
  stMdChn.enModId = RK_ID_ALGO_MD;
  stMdChn.s32DevId = 0;
  stMdChn.s32ChnId = 0;

  MoveDetectionHandle stMDHandle;
  stMDHandle.u32ChnIdx = 0;
  stMDHandle.u32ModeIdx = RK_ID_ALGO_MD;

  ret = RK_MPI_SYS_RegisterEventCb(&stMdChn, &stMDHandle, move_detection_cb);
  if (ret) {
    printf("ERROR: MoveDetection register event failed!\n");
    exit(0);
  }

  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 1;

  stDestChn.enModId = RK_ID_ALGO_MD;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: Bind vi and md failed!\n");
    exit(0);
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  printf("Keep MoveDetection enable for 5s...\n");
  sleep(5);
  printf("Keep MoveDetection disable for 5s...\n");
  ret = RK_MPI_ALGO_MD_EnableSwitch(0, RK_FALSE);
  if (ret) {
    printf("ERROR: Disable MD failed!\n");
    exit(0);
  }
  sleep(5);
  printf("Reset MoveDetection to enable...\n");
  ret = RK_MPI_ALGO_MD_EnableSwitch(0, RK_TRUE);
  if (ret) {
    printf("ERROR: Enable MD failed!\n");
    exit(0);
  }

  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);
  RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  RK_MPI_ALGO_MD_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 1);

  if (iq_file_dir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }

  return 0;
}
