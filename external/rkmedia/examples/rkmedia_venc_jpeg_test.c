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

static RK_CHAR *g_pOutPath = "/tmp/";
static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void video_packet_cb(MEDIA_BUFFER mb) {
  static RK_U32 jpeg_id = 0;
  printf("Get JPEG packet[%d]:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
         "timestamp:%lld\n",
         jpeg_id, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
         RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb),
         RK_MPI_MB_GetChannelID(mb), RK_MPI_MB_GetTimestamp(mb));

  char jpeg_path[128];
  sprintf(jpeg_path, "%s/test_jpeg%d.jpeg", g_pOutPath, jpeg_id);
  FILE *file = fopen(jpeg_path, "w");
  if (file) {
    fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), file);
    fclose(file);
  }

  RK_MPI_MB_ReleaseBuffer(mb);
  jpeg_id++;
}

#define TEST_ARGB32_YELLOW 0xFFFFFF00
#define TEST_ARGB32_RED 0xFFFF0033
#define TEST_ARGB32_BLUE 0xFF003399
#define TEST_ARGB32_TRANS 0x00000000

static void set_argb8888_buffer(RK_U32 *buf, RK_U32 size, RK_U32 color) {
  for (RK_U32 i = 0; buf && (i < size); i++)
    *(buf + i) = color;
}

static RK_CHAR optstr[] = "?::a::w:h:W:H:o:I:M:r:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"Width", required_argument, NULL, 'W'},
    {"Height", required_argument, NULL, 'H'},
    {"output", required_argument, NULL, 'o'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"rotation", required_argument, NULL, 'r'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]]"
         "[-I 0] "
         "[-M 0] "
         "[-H 1920] "
         "[-W 1080] "
         "[-h 720] "
         "[-w 480] "
         "[-o output_dir] "
         "\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
#else
  printf("\t%s"
         "[-I 0] "
         "[-W 1080] "
         "[-H 1920] "
         "[-w 480] "
         "[-h 720] "
         "[-o output_dir] "
         "\n",
         name);
#endif
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-W | --Width: source width, Default:1920\n");
  printf("\t-H | --Height: source height, Default:1080\n");
  printf("\t-w | --width: destination width, Default:720\n");
  printf("\t-h | --height: destination height, Default:480\n");
  printf("\t-o | --output: output dirpath, Default:/tmp/\n");
  printf(
      "\t-r | --rotation: rotation control, Default:0. Values:0/90/180/270\n");
  printf("\t tip: destination resolution should not over 4096*4096\n");
}

int main(int argc, char *argv[]) {
  RK_S32 ret;
  RK_U32 u32SrcWidth = 1920;
  RK_U32 u32SrcHeight = 1080;
  RK_U32 u32DstWidth = 720;
  RK_U32 u32DstHeight = 480;
  IMAGE_TYPE_E enPixFmt = IMAGE_TYPE_NV12;
  const RK_CHAR *pcVideoNode = "rkispp_scale0";
  const RK_CHAR *pcIqFileDir = NULL;
  RK_S32 s32CamId = 0;
  RK_U32 u32Rotation = 0;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
#endif
  int c;

  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    const char *tmp_optarg = optarg;
    switch (c) {
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }
      if (tmp_optarg) {
        pcIqFileDir = (char *)tmp_optarg;
      } else {
        pcIqFileDir = "/oem/etc/iqfiles/";
      }
      break;
    case 'w':
      u32DstWidth = (RK_U32)atoi(optarg);
      break;
    case 'h':
      u32DstHeight = (RK_U32)atoi(optarg);
      break;
    case 'W':
      u32SrcWidth = (RK_U32)atoi(optarg);
      break;
    case 'H':
      u32SrcHeight = (RK_U32)atoi(optarg);
      break;
    case 'o':
      g_pOutPath = optarg;
      break;
    case 'I':
      s32CamId = atoi(optarg);
      break;
    case 'r':
      u32Rotation = (RK_U32)atoi(optarg);
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

  printf("#SrcWidth: %d\n", u32SrcWidth);
  printf("#u32SrcHeight: %d\n", u32SrcHeight);
  printf("#u32DstWidth: %d\n", u32DstWidth);
  printf("#u32DstHeight: %d\n", u32DstHeight);
  printf("#u32Rotation: %d\n", u32Rotation);
  printf("#OutPath: %s\n", g_pOutPath);
  printf("#CameraIdx: %d\n\n", s32CamId);

  if (pcIqFileDir) {
#ifdef RKAIQ
    printf("#Rkaiq XML DirPath: %s\n", pcIqFileDir);
    printf("#bMultictx: %d\n\n", bMultictx);
    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    int fps = 30;
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pcIqFileDir);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
#endif
  }

  VENC_ROTATION_E enRotation;
  switch (u32Rotation) {
  case 0:
    enRotation = VENC_ROTATION_0;
    break;
  case 90:
    enRotation = VENC_ROTATION_90;
    break;
  case 180:
    enRotation = VENC_ROTATION_180;
    break;
  case 270:
    enRotation = VENC_ROTATION_270;
    break;
  default:
    printf("Invalid rotation(%d), should be 0/90/180/270\n", u32Rotation);
    return -1;
  }

  ret = RK_MPI_SYS_Init();
  if (ret) {
    printf("Sys Init failed! ret=%d\n", ret);
    return -1;
  }

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = pcVideoNode;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = u32SrcWidth;
  vi_chn_attr.u32Height = u32SrcHeight;
  vi_chn_attr.enPixFmt = enPixFmt;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 1, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 1);
  if (ret) {
    printf("Create Vi failed! ret=%d\n", ret);
    return -1;
  }

  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
  venc_chn_attr.stVencAttr.imageType = enPixFmt;
  venc_chn_attr.stVencAttr.u32PicWidth = u32SrcWidth;
  venc_chn_attr.stVencAttr.u32PicHeight = u32SrcHeight;
  venc_chn_attr.stVencAttr.u32VirWidth = u32SrcWidth;
  venc_chn_attr.stVencAttr.u32VirHeight = u32SrcHeight;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomWidth = u32DstWidth;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomHeight = u32DstHeight;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomVirWidth = u32DstWidth;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomVirHeight = u32DstHeight;
  venc_chn_attr.stVencAttr.enRotation = enRotation;
  venc_chn_attr.stVencAttr.stAttrJpege.bSupportDCF = RK_TRUE;
  venc_chn_attr.stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 2;
  venc_chn_attr.stVencAttr.stAttrJpege.stMPFCfg.astLargeThumbNailSize[0]
      .u32Width = 164;
  venc_chn_attr.stVencAttr.stAttrJpege.stMPFCfg.astLargeThumbNailSize[0]
      .u32Height = 128;
  venc_chn_attr.stVencAttr.stAttrJpege.stMPFCfg.astLargeThumbNailSize[1]
      .u32Width = 128;
  venc_chn_attr.stVencAttr.stAttrJpege.stMPFCfg.astLargeThumbNailSize[1]
      .u32Height = 164;
  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("Create Venc failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32ChnId = 0;
  ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
  if (ret) {
    printf("Register Output callback failed! ret=%d\n", ret);
    return -1;
  }

  // The encoder defaults to continuously receiving frames from the previous
  // stage. Before performing the bind operation, set s32RecvPicNum to 0 to
  // make the encoding enter the pause state.
  VENC_RECV_PIC_PARAM_S stRecvParam;
  stRecvParam.s32RecvPicNum = 0;
  RK_MPI_VENC_StartRecvFrame(0, &stRecvParam);

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind VI[1] to VENC[0]::JPEG failed! ret=%d\n", ret);
    return -1;
  }

  RK_MPI_VENC_RGN_Init(0, NULL);

  BITMAP_S BitMap;
  BitMap.enPixelFormat = PIXEL_FORMAT_ARGB_8888;
  BitMap.u32Width = 64;
  BitMap.u32Height = 64;
  BitMap.pData = malloc(BitMap.u32Width * 4 * BitMap.u32Height);
  RK_U8 *ColorData = (RK_U8 *)BitMap.pData;
  RK_U16 ColorBlockSize = BitMap.u32Height * BitMap.u32Width;
  set_argb8888_buffer((RK_U32 *)ColorData, ColorBlockSize / 4,
                      TEST_ARGB32_YELLOW);
  set_argb8888_buffer((RK_U32 *)(ColorData + ColorBlockSize),
                      ColorBlockSize / 4, TEST_ARGB32_TRANS);
  set_argb8888_buffer((RK_U32 *)(ColorData + 2 * ColorBlockSize),
                      ColorBlockSize / 4, TEST_ARGB32_RED);
  set_argb8888_buffer((RK_U32 *)(ColorData + 3 * ColorBlockSize),
                      ColorBlockSize / 4, TEST_ARGB32_BLUE);

  // Case 1: Canvas and bitmap are equal in size
  OSD_REGION_INFO_S RngInfo;
  RngInfo.enRegionId = REGION_ID_0;
  RngInfo.u32PosX = 0;
  RngInfo.u32PosY = 0;
  RngInfo.u32Width = 64;
  RngInfo.u32Height = 64;
  RngInfo.u8Enable = 1;
  RngInfo.u8Inverse = 0;
  RK_MPI_VENC_RGN_SetBitMap(0, &RngInfo, &BitMap);

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  char cmd[64];
  int qfactor = 0;
  VENC_JPEG_PARAM_S stJpegParam;
  printf("#Usage: input 'quit' to exit programe!\n"
         "peress any other key to capture one picture to file\n");
  while (!quit) {
    fgets(cmd, sizeof(cmd), stdin);
    printf("#Input cmd:%s\n", cmd);
    if (strstr(cmd, "quit")) {
      printf("#Get 'quit' cmd!\n");
      break;
    }

    if (qfactor >= 99)
      qfactor = 1;
    else
      qfactor = ((qfactor + 10) > 99) ? 99 : (qfactor + 10);
    stJpegParam.u32Qfactor = qfactor;
    RK_MPI_VENC_SetJpegParam(0, &stJpegParam);

    VENC_CHN_PARAM_S venc_chn_param;
    memset(&venc_chn_param, 0, sizeof(venc_chn_param));
    venc_chn_param.stCropCfg.bEnable = 1;
    venc_chn_param.stCropCfg.stRect.s32X = (qfactor / 2) * 2;
    venc_chn_param.stCropCfg.stRect.s32Y = (qfactor / 2) * 2;
    venc_chn_param.stCropCfg.stRect.u32Width = ((200 + qfactor) / 2) * 2;
    venc_chn_param.stCropCfg.stRect.u32Height = ((200 + qfactor) / 2) * 2;
    ret = RK_MPI_VENC_SetChnParam(0, &venc_chn_param);
    if (ret) {
      printf("Set Venc Channel failed! ret=%d\n", ret);
      return -1;
    }

    stRecvParam.s32RecvPicNum = 1;

    ret = RK_MPI_VENC_StartRecvFrame(0, &stRecvParam);
    if (ret) {
      printf("RK_MPI_VENC_StartRecvFrame failed!\n");
      break;
    }
    usleep(30000); // sleep 30 ms.
  }

  RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  RK_MPI_VENC_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 1);

  if (pcIqFileDir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }

  printf("%s exit!\n", __func__);
  return 0;
}
