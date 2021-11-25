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

#define TEST_ARGB32_YELLOW 0xFFFFFF00
#define TEST_ARGB32_RED 0xFFFF0033
#define TEST_ARGB32_BLUE 0xFF003399
#define TEST_ARGB32_TRANS 0x00000000

static FILE *g_save_file;
static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static void set_argb8888_buffer(RK_U32 *buf, RK_U32 size, RK_U32 color) {
  for (RK_U32 i = 0; buf && (i < size); i++)
    *(buf + i) = color;
}

void video_packet_cb(MEDIA_BUFFER mb) {
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

static void print_usage(char *name) {
  printf("#Function description:\n");
  printf("In the case of multiple streams, verify the effect of roi and osd at "
         "the same time.\n");
  printf(
      "  MainStream: rkispp_scale0: 1920x1080 NV12 -> /<output>/main.h264\n");
  printf("  SubStream0: rkispp_scale1: 720x480 NV12 -> /<output>/sub0.h264\n");
  printf("  SubStream1: rkispp_scale2: 1280x720 NV12 -> /<output>/sub1.h264\n");
  printf("#Usage Example: \n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]] "
         "[-I 0] "
         "[-M 0] "
         "[-o output_dir]\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
#else
  printf("\t%s [-o output_dir] [-I 0]\n", name);
#endif
  printf("\t-o | --output: output dirpath, Default:/tmp/\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
}

int main(int argc, char *argv[]) {
  char *iq_file_dir = NULL;
  RK_CHAR *pOutPath = NULL;
  RK_U32 u32Width = 1920;
  RK_U32 u32Height = 1080;
  RK_U32 u32RectX = 0;
  RK_U32 u32RectY = 0;
  RK_U32 u32RectW = (u32Width / 2) & (~0x0000000F); // 16 align
  RK_U32 u32RectH = (u32Height / 2) & (~0x0000000F);
  const RK_CHAR *pDeviceNode = "rkispp_scale0";
  RK_S32 s32CamId = 0;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
#endif
  int c = 0, ret = 0;

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

  printf("#output path:%s\n", pOutPath);
  printf("#CameraIdx: %d\n\n", s32CamId);
  g_save_file = fopen(pOutPath, "w");
  if (!g_save_file)
    printf("ERROR: open %s fail\n", pOutPath);

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
  vi_chn_attr.pcVideoNode = pDeviceNode;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = u32Width;
  vi_chn_attr.u32Height = u32Height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("Create Vi failed! ret=%d\n", ret);
    return -1;
  }

  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
  venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
  venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
  venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
  venc_chn_attr.stVencAttr.u32Profile = 77;
  venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
  venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 75;
  venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = u32Width * u32Height;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;
  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("Create avc failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = 0;
  RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);

  RK_MPI_VENC_RGN_Init(0, NULL);

  // Generate bitmap
  BITMAP_S BitMap;
  BitMap.enPixelFormat = PIXEL_FORMAT_ARGB_8888;
  BitMap.u32Width = u32Width / 2;
  BitMap.u32Height = u32Height / 2;
  BitMap.pData = malloc(BitMap.u32Width * 4 * BitMap.u32Height);
  set_argb8888_buffer((RK_U32 *)BitMap.pData,
                      BitMap.u32Height * BitMap.u32Width, TEST_ARGB32_YELLOW);

  // Create Canvas
  OSD_REGION_INFO_S RngInfo;
  RngInfo.enRegionId = REGION_ID_0;
  RngInfo.u32PosX = u32RectX;
  RngInfo.u32PosY = u32RectY;
  RngInfo.u32Width = u32RectW;
  RngInfo.u32Height = u32RectH;
  RngInfo.u8Enable = 1;
  RngInfo.u8Inverse = 0;
  ret = RK_MPI_VENC_RGN_SetBitMap(0, &RngInfo, &BitMap);
  if (ret) {
    printf("Venc SetBitMap failed! ret=%d\n", ret);
    return -1;
  }

  VENC_ROI_ATTR_S stRoiAttr = {0};
  stRoiAttr.bAbsQp = RK_TRUE;
  stRoiAttr.bEnable = RK_TRUE;
  stRoiAttr.stRect.s32X = u32RectX;
  stRoiAttr.stRect.s32Y = u32RectY;
  stRoiAttr.stRect.u32Width = u32RectW;
  stRoiAttr.stRect.u32Height = u32RectH;
  stRoiAttr.u32Index = 0;
  stRoiAttr.s32Qp = 18;
  stRoiAttr.bIntra = RK_FALSE;
  ret = RK_MPI_VENC_SetRoiAttr(0, &stRoiAttr, 1);
  if (ret) {
    printf("Venc SetRoi failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 0;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Create RK_MPI_SYS_Bind0 failed! ret=%d\n", ret);
    return -1;
  }

  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: unbind vi[0] -> venc[0] failed! ret=%d\n", ret);
    return -1;
  }

  ret = RK_MPI_VI_DisableChn(s32CamId, 0);
  if (ret)
    printf("ERROR: destroy vi[0] failed! ret=%d\n", ret);

  ret = RK_MPI_VENC_DestroyChn(0);
  if (ret)
    printf("ERROR: destroy venc[0] failed! ret=%d\n", ret);

  if (iq_file_dir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }

  fclose(g_save_file);
  return 0;
}
