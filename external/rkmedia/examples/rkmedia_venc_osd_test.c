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

static bool quit = false;
static FILE *g_save_file;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

// for argb8888
#define TEST_ARGB32_PIX_SIZE 4
#define TEST_ARGB32_GREEN 0xFF00FF00
#define TEST_ARGB32_RED 0xFFFF0000
#define TEST_ARGB32_BLUE 0xFF0000FF
#define TEST_ARGB32_TRANS 0x00000000

static void set_argb8888_buffer(RK_U32 *buf, RK_U32 size, RK_U32 color) {
  for (RK_U32 i = 0; buf && (i < size); i++)
    *(buf + i) = color;
}

static int get_random_value(int mode, int align) {
  int rand_value = 0;
  srand((unsigned)time(NULL));
  rand_value = rand() % mode;

  if (align && (rand_value % align))
    rand_value = (rand_value / align + 1) * align;

  if (rand_value > mode)
    rand_value = align ? (rand_value - align) : (mode - 1);

  if (rand_value < 0)
    rand_value = 0;

  return rand_value;
}

static void *GetMediaBuffer(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);

  MEDIA_BUFFER mb = NULL;
  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, 0, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }
#if 0
    printf("Get packet:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld\n",
           RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb));
#endif
    if (g_save_file)
      fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), g_save_file);
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  return NULL;
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
  printf("usage example:\n");
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
  RK_CHAR *pOutPath = "/tmp/output.h264";
  int video_width = 1920;
  int video_height = 1080;
  int c;
  RK_S32 s32CamId = 0;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
#endif
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

  signal(SIGINT, sigterm_handler);

  printf("#output path: %s\n", pOutPath);
  printf("#CameraIdx: %d\n\n", s32CamId);
  g_save_file = fopen(pOutPath, "w");
  if (!g_save_file) {
    printf("WARN: open %s failed!\n", pOutPath);
  }
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
    printf("ERROR: create VI[0] error! ret=%d\n", ret);
    return 0;
  }

  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  venc_chn_attr.stVencAttr.u32PicWidth = video_width;
  venc_chn_attr.stVencAttr.u32PicHeight = video_height;
  venc_chn_attr.stVencAttr.u32VirWidth = video_width;
  venc_chn_attr.stVencAttr.u32VirHeight = video_height;
  venc_chn_attr.stVencAttr.u32Profile = 77;
  venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
  venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = 2000000; // 2Mb
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 0;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 0;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;
  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("ERROR: create VENC[0] error! ret=%d\n", ret);
    return 0;
  }

  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, NULL);

  RK_MPI_VENC_RGN_Init(0, NULL);

  // Bind VI and VENC
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
    printf("ERROR: Bind VI[0] and VENC[0] error! ret=%d\n", ret);
    return 0;
  }
  printf("%s initial finish\n", __func__);

  int test_cnt = 0;
  int bitmap_width = 0;
  int bitmap_height = 0;
  int wxh_size = 0;

  while (!quit) {
    bitmap_width = get_random_value(video_width, 16);
    bitmap_height = get_random_value(video_height, 16);
    if (bitmap_width < 64)
      bitmap_width = 64;
    if (bitmap_height < 64)
      bitmap_height = 64;

    wxh_size = bitmap_width * bitmap_height;
    BITMAP_S BitMap;

    BitMap.enPixelFormat = PIXEL_FORMAT_ARGB_8888;
    BitMap.u32Width = bitmap_width;
    BitMap.u32Height = bitmap_height;
    BitMap.pData = malloc(wxh_size * TEST_ARGB32_PIX_SIZE);
    if (!BitMap.pData) {
      printf("ERROR: no mem left for argb8888(%d)!\n",
             wxh_size * TEST_ARGB32_PIX_SIZE);
      break;
    }
    set_argb8888_buffer((RK_U32 *)BitMap.pData, wxh_size, TEST_ARGB32_GREEN);
    OSD_REGION_INFO_S RngInfo;
    RngInfo.enRegionId = get_random_value(REGION_ID_7 + 1, 0);
    RngInfo.u32PosX = get_random_value(video_width - bitmap_width, 16);
    RngInfo.u32PosY = get_random_value(video_height - bitmap_height, 16);
    RngInfo.u32Width = bitmap_width;
    RngInfo.u32Height = bitmap_height;
    RngInfo.u8Enable = 1;
    RngInfo.u8Inverse = 0;
    printf("#%03d ENABLE RGN[%d]: <%d, %d, %d, %d> for 100ms...\n", test_cnt,
           RngInfo.enRegionId, RngInfo.u32PosX, RngInfo.u32PosY,
           RngInfo.u32Width, RngInfo.u32Height);

    ret = RK_MPI_VENC_RGN_SetBitMap(0, &RngInfo, &BitMap);
    if (ret) {
      printf("ERROR: set rgn bitmap(enable) failed! ret=%d\n", ret);
      if (BitMap.pData)
        free(BitMap.pData);
      break;
    }

    // free data
    free(BitMap.pData);
    BitMap.pData = NULL;

    usleep(100000);
    printf("     DISABLE RGN[%d]!\n", RngInfo.enRegionId);
    RngInfo.u8Enable = 0;
    ret = RK_MPI_VENC_RGN_SetBitMap(0, &RngInfo, &BitMap);
    if (ret) {
      printf("ERROR: set rgn bitmap(disable) failed! ret=%d\n", ret);
      break;
    }
    test_cnt++;
  }
  printf("%s exit!\n", __func__);
  RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  RK_MPI_VENC_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 0);

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(s32CamId);
#endif

  if (g_save_file)
    fclose(g_save_file);

  return 0;
}
