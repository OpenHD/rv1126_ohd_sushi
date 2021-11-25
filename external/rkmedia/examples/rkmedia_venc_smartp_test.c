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
static FILE *g_save_file;

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

  if (RK_MPI_MB_IsViFrame(mb))
    nalu_type = "VI Slice";

  printf("Get Video Encoded packet[%s]:ptr:%p, fd:%d, size:%zu, mode:%d, "
         "level:%d\n",
         nalu_type, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
         RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb),
         RK_MPI_MB_GetTsvcLevel(mb));
  if (g_save_file)
    fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), g_save_file);
  RK_MPI_MB_ReleaseBuffer(mb);
}

static RK_CHAR optstr[] = "?::a::o:I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"help", optional_argument, NULL, '?'},
    {"output", required_argument, NULL, 'o'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("#Usage Example: \n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]] "
         "[-I 0] "
         "[-M 0] "
         "[-o output.h264]\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
#else
  printf("\t%s [-o output.h264] [-I 0]\n", name);
#endif
  printf("\t-o | --output: output path, Default:/tmp/output.h264\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
}

int main(int argc, char *argv[]) {
  int ret = 0;
  int c;
  RK_CHAR *pOutPath = "/tmp/output.h264";
  RK_CHAR *iq_file_dir = NULL;
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
    case 'o':
      pOutPath = optarg;
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

  g_save_file = fopen(pOutPath, "w");
  if (!g_save_file)
    printf("#VENC OSD TEST:: Open %s failed!\n", pOutPath);
  printf("#output file: %s\n\n", pOutPath);
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
  vi_chn_attr.u32Width = 1920;
  vi_chn_attr.u32Height = 1080;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 1, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 1);
  if (ret) {
    printf("TEST: ERROR: Create vi[1] error! code:%d\n", ret);
    return -1;
  }

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
  venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = 2000000; // 2Mb
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;
  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("TEST: ERROR: Create venc[0] error! code:%d\n", ret);
    return -1;
  }

  printf("### Start smartp mode ....\n");
  VENC_GOP_ATTR_S stGopModeAttr;
  stGopModeAttr.enGopMode = VENC_GOPMODE_SMARTP;
  stGopModeAttr.s32IPQpDelta = 3;
  stGopModeAttr.s32ViQpDelta = 3;
  stGopModeAttr.u32BgInterval = 300;
  stGopModeAttr.u32GopSize = 30;
  ret = RK_MPI_VENC_SetGopMode(0, &stGopModeAttr);
  if (ret) {
    printf("TEST: ERROR: Set gop mode error! code:%d\n", ret);
    return -1;
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = 0;
  ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
  if (ret) {
    printf("TEST: ERROR: Register cb for venc[0] error! code:%d\n", ret);
    return -1;
  }

  // Bind VI and VENC
  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 1;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("TEST: ERROR: Bind vi[0] to venc[0] error! code:%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);

  sleep(15);
  printf("### Change to normalp mode ....\n");
  memset(&stGopModeAttr, 0, sizeof(stGopModeAttr));
  stGopModeAttr.enGopMode = VENC_GOPMODE_NORMALP;
  stGopModeAttr.s32IPQpDelta = 3;
  ret = RK_MPI_VENC_SetGopMode(0, &stGopModeAttr);
  if (ret) {
    printf("TEST: ERROR: Set gop mode error! code:%d\n", ret);
    return -1;
  }

  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  if (g_save_file) {
    printf("#VENC OSD TEST:: Close save file!\n");
    fclose(g_save_file);
  }

  RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  RK_MPI_VENC_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 1);

  if (iq_file_dir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }
  printf("%s exit!\n", __func__);
  return 0;
}
