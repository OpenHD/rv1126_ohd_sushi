// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
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
#include "rkmedia_venc.h"

typedef struct {
  char *file_path;
  int frame_cnt;
} OutputArgs;

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static void *GetMediaBuffer(void *arg) {
  OutputArgs *outArgs = (OutputArgs *)arg;
  char *save_path = outArgs->file_path;
  int save_cnt = outArgs->frame_cnt;
  int frame_id = 0;
  FILE *save_file = NULL;
  if (save_path) {
    save_file = fopen(save_path, "w");
    if (!save_file)
      printf("ERROR: Open %s failed!\n", save_path);
  }

  MEDIA_BUFFER mb = NULL;
  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, 0, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }

    MB_IMAGE_INFO_S stImageInfo = {0};
    int ret = RK_MPI_MB_GetImageInfo(mb, &stImageInfo);
    if (ret)
      printf("Warn: Get image info failed! ret = %d\n", ret);

    printf("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",
           RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb), stImageInfo.u32Width,
           stImageInfo.u32Height, stImageInfo.enImgType);

    if (save_file) {
      fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), save_file);
      printf("#Save frame-%d to %s\n", frame_id++, save_path);
    }

    RK_MPI_MB_ReleaseBuffer(mb);

    if (save_cnt > 0)
      save_cnt--;

    // exit when complete
    if (!save_cnt) {
      quit = true;
      break;
    }
  }

  if (save_file)
    fclose(save_file);

  return NULL;
}

static RK_CHAR optstr[] = "?::a::o:c:d:I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"output", required_argument, NULL, 'o'},
    {"frame_cnt", required_argument, NULL, 'c'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"help", no_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]] "
         "[-I 0] "
         "[-M 0] "
         " [-o /tmp/rga.nv12]\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
#else
  printf("\t%s [-o /tmp/rga.nv12] [-I 0]\n", name);
#endif
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-d | --device_name: set pcDeviceName, Default:rkispp_scale0\n");
  printf("\t-c | --frame_cnt: frame cnt, Default:-1(unlimit)\n");
  printf("\t-o | --output: output path, Default:NULL\n");
  printf("Notice: this example's fmt always be NV12, resolution:1920x1080\n");
}

int main(int argc, char *argv[]) {
  int ret = 0;
  int c;
  char *iq_file_dir = NULL;
  int frameCnt = -1;
  RK_CHAR *pDeviceName = "rkispp_scale0";
  RK_CHAR *pOutPath = NULL;
  RK_U32 u32Width = 1920;
  RK_U32 u32Height = 1080;
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
        iq_file_dir = "/oem/etc/iqfiles";
      }
      break;
    case 'c':
      frameCnt = atoi(optarg);
      break;
    case 'o':
      pOutPath = optarg;
      break;
    case 'd':
      pDeviceName = optarg;
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

  printf("#DeviceNode: %s\n", pDeviceName);
  printf("#Resolution: %dx%d\n", u32Width, u32Height);
  printf("#FrameCount: %d\n", frameCnt);
  printf("#OutputPath: %s\n", pOutPath);
  printf("#CameraIdx: %d\n\n", s32CamId);
  signal(SIGINT, sigterm_handler);

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
  vi_chn_attr.u32BufCnt = 2;
  vi_chn_attr.u32Width = u32Width;
  vi_chn_attr.u32Height = u32Height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("ERROR: Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }

  RGA_ATTR_S stRgaAttr;
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 2;
  stRgaAttr.u16Rotaion = 0;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = u32Width;
  stRgaAttr.stImgIn.u32Height = u32Height;
  stRgaAttr.stImgIn.u32HorStride = u32Width;
  stRgaAttr.stImgIn.u32VirStride = u32Height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgOut.u32Width = u32Width / 2;
  stRgaAttr.stImgOut.u32Height = u32Height / 2;
  stRgaAttr.stImgOut.u32HorStride = u32Width / 2;
  stRgaAttr.stImgOut.u32VirStride = u32Height / 2;
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
    printf("ERROR: Create rga[0] falied! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 0;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: Bind vi[0] and rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);

  pthread_t read_thread;
  OutputArgs outArgs = {pOutPath, frameCnt};
  pthread_create(&read_thread, NULL, GetMediaBuffer, &outArgs);

#if 1
  /**************************************************
   * RGA only supports dynamic modification for
   * the attributes listed below:
   *   stRgaAttr.stImgIn.u32X
   *   stRgaAttr.stImgIn.u32Y
   *   stRgaAttr.stImgIn.u32Width
   *   stRgaAttr.stImgIn.u32Height
   *   stRgaAttr.stImgOut.u32X
   *   stRgaAttr.stImgOut.u32Y
   *   stRgaAttr.stImgOut.u32Width
   *   stRgaAttr.stImgOut.u32Height
   * ************************************************/
  printf("#Keep the initial configuration for 1s...\n");
  sleep(1);

  printf("#Zoom 1920x1080 to 960x540...\n");
  stRgaAttr.u16Rotaion = 0;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.u32Width = u32Width;
  stRgaAttr.stImgIn.u32Height = u32Height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.u32Width = u32Width / 2;
  stRgaAttr.stImgOut.u32Height = u32Height / 2;
  ret = RK_MPI_RGA_SetChnAttr(0, &stRgaAttr);
  if (ret) {
    printf("ERROR: RGA Set Attr: ImgIn:<%u,%u,%u,%u> "
           "ImgOut:<%u,%u,%u,%u>, rotation=%u failed! ret=%d\n",
           stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
           stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
           stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
           stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
           stRgaAttr.u16Rotaion, ret);
    return -1;
  }
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  ret = RK_MPI_RGA_GetChnAttr(0, &stRgaAttr);
  if (ret) {
    printf("ERROR: RGA Get Attr failed! ret=%d\n", ret);
    return -1;
  }
  printf("#RGA Get Attr: ImgIn:<%u,%u,%u,%u> "
         "ImgOut:<%u,%u,%u,%u>, rotation=%u.\n",
         stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
         stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
         stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
         stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
         stRgaAttr.u16Rotaion);
  sleep(1);

  printf("#Crop (960,540,960,540) to 960x540...\n");
  stRgaAttr.u16Rotaion = 0;
  stRgaAttr.stImgIn.u32X = u32Width / 2;
  stRgaAttr.stImgIn.u32Y = u32Height / 2;
  stRgaAttr.stImgIn.u32Width = u32Width / 2;
  stRgaAttr.stImgIn.u32Height = u32Height / 2;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.u32Width = u32Width / 2;
  stRgaAttr.stImgOut.u32Height = u32Height / 2;
  ret = RK_MPI_RGA_SetChnAttr(0, &stRgaAttr);
  if (ret) {
    printf("ERROR: RGA Set Attr: ImgIn:<%u,%u,%u,%u> "
           "ImgOut:<%u,%u,%u,%u>, rotation=%d failed! ret=%u\n",
           stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
           stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
           stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
           stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
           stRgaAttr.u16Rotaion, ret);
    return -1;
  }
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  ret = RK_MPI_RGA_GetChnAttr(0, &stRgaAttr);
  if (ret) {
    printf("ERROR: RGA Get Attr failed! ret=%d\n", ret);
    return -1;
  }
  printf("#RGA Get Attr: ImgIn:<%u,%u,%u,%u> "
         "ImgOut:<%u,%u,%u,%u>, rotation=%u.\n",
         stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
         stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
         stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
         stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
         stRgaAttr.u16Rotaion);
  sleep(1);

  printf("#Rotation (0,0,540,960) to 960x540...\n");
  stRgaAttr.u16Rotaion = 90;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.u32Width = u32Height / 2;
  stRgaAttr.stImgIn.u32Height = u32Width / 2;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.u32Width = u32Width / 2;
  stRgaAttr.stImgOut.u32Height = u32Height / 2;
  ret = RK_MPI_RGA_SetChnAttr(0, &stRgaAttr);
  if (ret) {
    printf("ERROR: RGA Set Attr: ImgIn:<%u,%u,%u,%u> "
           "ImgOut:<%u,%u,%u,%u>, rotation=%u failed! ret=%d\n",
           stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
           stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
           stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
           stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
           stRgaAttr.u16Rotaion, ret);
    return -1;
  }
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  ret = RK_MPI_RGA_GetChnAttr(0, &stRgaAttr);
  if (ret) {
    printf("ERROR: RGA Get Attr failed! ret=%d\n", ret);
    return -1;
  }
  printf("#RGA Get Attr: ImgIn:<%u,%u,%u,%u> "
         "ImgOut:<%u,%u,%u,%u>, rotation=%u.\n",
         stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
         stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
         stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
         stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
         stRgaAttr.u16Rotaion);
  sleep(1);
#endif

  while (!quit) {
    usleep(50000);
  }

  printf("%s exit!\n", __func__);
  pthread_join(read_thread, NULL);
  RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  RK_MPI_RGA_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 1);

  if (iq_file_dir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }

  return 0;
}
