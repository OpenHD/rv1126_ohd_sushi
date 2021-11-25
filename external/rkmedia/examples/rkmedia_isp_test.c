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
#include "rkmedia_venc.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void video_packet_cb(MEDIA_BUFFER mb) {
  const char *nalu_type = "Unknow";
  switch (RK_MPI_MB_GetFlag(mb)) {
  case VENC_NALU_IDRSLICE:
    nalu_type = "IDR Slice";
    break;
  case VENC_NALU_PSLICE:
    nalu_type = "P Slice";
    break;
  default:
    break;
  }
  printf("Get Video Encoded packet(%s):ptr:%p, fd:%d, size:%zu, mode:%d\n",
         nalu_type, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
         RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb));
  RK_MPI_MB_ReleaseBuffer(mb);
}

int menu() {
  int menu;
  char choose[5];

  printf("\n***************************************\n");
  printf("©¦²Ëµ¥©¦\n");
  printf("__________________________________\n");
  printf("|    0. quit       \n");
  printf("©¦1. Normal  \n");
  printf("©¦2. HDRX2  \n");
  printf("©¦3. HDRX3  \n");
  printf("©¦4. FEC ON \n");
  printf("©¦5. FEC OFF\n");
  printf("©¦6. LDCH    \n");
  printf("©¸©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤\n");
  printf("**************************************\n\n");

  do {
    printf("Please input your select (0-6):");
    scanf("%s", choose);
    menu = atoi(choose);
  } while (menu < 0 || menu > 6);

  return menu;
}

int menu_ldch() {
  int menu;
  char choose[5];

  do {
    printf("Please input ldch level (0-255 ):");
    scanf("%s", choose);
    menu = atoi(choose);
  } while (menu < 0 || menu > 255);

  return menu;
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
  printf("\t%s [-a iqfiles_dir] "
         "[-I 0] "
         "[-M 0] "
         "\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided,"
         "without this option, default path \"/oem/etc/iqfiles/\" would be "
         "userd.\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
}

int main(int argc, char *argv[]) {

  signal(SIGINT, sigterm_handler);

  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  RK_BOOL fec_enable = RK_FALSE;
  int fps = 30;
  char *iq_file_dir = "/oem/etc/iqfiles/";
  RK_S32 s32CamId = 0;
  RK_BOOL bMultictx = RK_FALSE;
  int c;
  opterr = 1;
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
    case 'M':
      if (atoi(optarg)) {
        bMultictx = RK_TRUE;
      }
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#CameraIdx: %d\n\n", s32CamId);
  printf("#bMultictx: %d\n\n", bMultictx);

  char *tmp = getenv("HDR_MODE");
  if (tmp) {
    if (strstr(tmp, "32")) {
      hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR3;
      fps = 20;
    }
    if (strstr(tmp, "16")) {
      hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
      fps = 25;
    }
  }
  char *fec_mode = getenv("FEC_MODE");
  if (fec_mode)
    if (strstr(fec_mode, "1"))
      fec_enable = RK_TRUE;

  char *sfps = getenv("FPS");
  if (sfps)
    fps = atoi(sfps);

  RK_BOOL running = RK_FALSE;
  RK_MPI_SYS_Init();
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;

restart:
  if (running) {
    RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    RK_MPI_VENC_DestroyChn(0);
    RK_MPI_VI_DisableChn(s32CamId, 0);
    SAMPLE_COMM_ISP_Stop(0);
  }

  printf("#####hdr mode %d, fec mode %d, fps %d\n", hdr_mode, fec_enable, fps);
  SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, iq_file_dir);
  SAMPLE_COMM_ISP_SetFecEn(s32CamId, fec_enable);
  SAMPLE_COMM_ISP_Run(s32CamId);
  SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);

  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  venc_chn_attr.stVencAttr.u32PicWidth = 1920;
  venc_chn_attr.stVencAttr.u32PicHeight = 1080;
  venc_chn_attr.stVencAttr.u32VirWidth = 1920;
  venc_chn_attr.stVencAttr.u32VirHeight = 1080;
  venc_chn_attr.stVencAttr.u32Profile = 77;

  venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;

  venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = 1920 * 1080 * 30 / 14;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 0;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 0;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = 1920;
  vi_chn_attr.u32Height = 1080;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;

  RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  RK_MPI_VI_EnableChn(s32CamId, 0);
  RK_MPI_VENC_CreateChn(0, &venc_chn_attr);

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = 0;
  RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 0;

  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;

  RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);

  running = RK_TRUE;
  /*
       printf(" |   0. quit   \n");
       printf("©¦1. Normal  \n");
        printf("©¦2. HDRX2	\n");
        printf("©¦3. HDRX3	\n");
        printf("©¦4. FEC ON \n");
        printf("©¦5. FEC OFF\n");
       printf("©¦6. LDCH level \n");
  */
  while (!quit) {
    switch (menu()) {
    case 0:
      quit = true;
      break;
    case 1:
      if (hdr_mode == RK_AIQ_WORKING_MODE_ISP_HDR3) {
        hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
        goto restart;
      } else {
        hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
        SAMPLE_COMM_ISP_SetWDRModeDyn(0, hdr_mode);
      }
      break;
    case 2:
      if (hdr_mode == RK_AIQ_WORKING_MODE_ISP_HDR3) {
        hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
        goto restart;
      } else {
        hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
        SAMPLE_COMM_ISP_SetWDRModeDyn(0, hdr_mode);
      }
      break;
    case 3:
      hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR3;
      goto restart;
    case 4:
      fec_enable = 1;
      goto restart;
    case 5:
      fec_enable = 0;
      goto restart;
    case 6:
      SAMPLE_COMM_ISP_SetLDCHLevel(0, menu_ldch());
      continue;
    }
    usleep(30000); // sleep 30 ms.
  }

  printf("%s exit!\n", __func__);

  RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  RK_MPI_VENC_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 0);

  SAMPLE_COMM_ISP_Stop(s32CamId);
  return 0;
}
