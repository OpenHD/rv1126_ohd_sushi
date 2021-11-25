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
static FILE *g_output_file;
static RK_S32 g_s32FrameCnt = -1;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void video_packet_cb(MEDIA_BUFFER mb) {
  static RK_S32 packet_cnt = 0;
  if (quit)
    return;

  const char *nalu_type = "Jpeg data";
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

  if (g_output_file) {
    fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), g_output_file);
    printf("#Write packet-%d, %s, size %zu\n", packet_cnt, nalu_type,
           RK_MPI_MB_GetSize(mb));
  } else {
    printf("#Get packet-%d, %s, size %zu\n", packet_cnt, nalu_type,
           RK_MPI_MB_GetSize(mb));
  }
  RK_MPI_MB_TsNodeDump(mb);
  RK_MPI_MB_ReleaseBuffer(mb);

  packet_cnt++;
  if ((g_s32FrameCnt >= 0) && (packet_cnt > g_s32FrameCnt))
    quit = true;
}

static RK_CHAR optstr[] = "?::a::w:h:c:o:e:d:I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"device_name", required_argument, NULL, 'd'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"frame_cnt", required_argument, NULL, 'c'},
    {"output_path", required_argument, NULL, 'o'},
    {"encode", required_argument, NULL, 'e'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"fps", required_argument, NULL, 'f'},
    {"hdr_mode", required_argument, NULL, 'h' + 'm'},
    {"vi_buf_cnt", required_argument, NULL, 'b' + 'c'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]] [-w 1920] "
         "[-h 1080]"
         "[-c 150] "
         "[-d rkispp_scale0] "
         "[-e 0] "
         "[-I 0] "
         "[-M 0] "
         "[-o output.h264] \n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath emtpty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
  printf("\t--fps fps of vi.\n");
  printf("\t--hdr_mode [normal hdr2 hdr3].\n");
  printf("\t--vi_buf_cnt buffer count of vi.\n");
#else
  printf("\t%s [-w 1920] "
         "[-h 1080]"
         "[-c 150] "
         "[-I 0] "
         "[-d rkispp_scale0] "
         "[-e 0] "
         "[-o output.h264] \n",
         name);
#endif
  printf("\t-w | --width: VI width, Default:1920\n");
  printf("\t-h | --heght: VI height, Default:1080\n");
  printf("\t-c | --frame_cnt: frame number of output, Default:150\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-d | --device_name set pcDeviceName, Default:rkispp_scale0, "
         "Option:[rkispp_scale0, rkispp_scale1, rkispp_scale2]\n");
  printf(
      "\t-e | --encode: encode type, Default:h264, Value:h264, h265, mjpeg\n");
  printf("\t-o | --output_path: output path, Default:NULL\n");
}

int main(int argc, char *argv[]) {
  RK_U32 u32Width = 1920;
  RK_U32 u32Height = 1080;
  RK_CHAR *pDeviceName = "rkispp_scale0";
  RK_CHAR *pOutPath = NULL;
  RK_CHAR *pIqfilesPath = NULL;
  CODEC_TYPE_E enCodecType = RK_CODEC_TYPE_H264;
  RK_CHAR *pCodecName = "H264";
  RK_S32 s32CamId = 0;
  RK_U32 u32BufCnt = 3;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
  RK_U32 u32Fps = 30;
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
#endif

  int c;
  int ret = 0;
  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    const char *tmp_optarg = optarg;
    switch (c) {
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }
      if (tmp_optarg) {
        pIqfilesPath = (char *)tmp_optarg;
      } else {
        pIqfilesPath = "/oem/etc/iqfiles";
      }
      break;
    case 'w':
      u32Width = atoi(optarg);
      break;
    case 'h':
      u32Height = atoi(optarg);
      break;
    case 'c':
      g_s32FrameCnt = atoi(optarg);
      break;
    case 'o':
      pOutPath = optarg;
      break;
    case 'd':
      pDeviceName = optarg;
      break;
    case 'e':
      if (!strcmp(optarg, "h264")) {
        enCodecType = RK_CODEC_TYPE_H264;
        pCodecName = "H264";
      } else if (!strcmp(optarg, "h265")) {
        enCodecType = RK_CODEC_TYPE_H265;
        pCodecName = "H265";
      } else if (!strcmp(optarg, "mjpeg")) {
        enCodecType = RK_CODEC_TYPE_MJPEG;
        pCodecName = "MJPEG";
      } else {
        printf("ERROR: Invalid encoder type.\n");
        return 0;
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
    case 'f':
      u32Fps = atoi(optarg);
      printf("#u32Fps = %u.\n", u32Fps);
      break;
    case 'h' + 'm':
      if (strcmp(optarg, "normal") == 0) {
        hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
      } else if (strcmp(optarg, "hdr2") == 0) {
        hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
      } else if (strcmp(optarg, "hdr3") == 0) {
        hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR3;
      } else {
        print_usage(argv[0]);
        return 0;
      }
      printf("#hdr_mode = %u.\n", hdr_mode);
      break;
#endif
    case 'b' + 'c':
      u32BufCnt = atoi(optarg);
      printf("#vi buffer conunt = %u.\n", u32BufCnt);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#Device: %s\n", pDeviceName);
  printf("#CodecName:%s\n", pCodecName);
  printf("#Resolution: %dx%d\n", u32Width, u32Height);
  printf("#Frame Count to save: %d\n", g_s32FrameCnt);
  printf("#Output Path: %s\n", pOutPath);
  printf("#CameraIdx: %d\n\n", s32CamId);
#ifdef RKAIQ
  printf("#bMultictx: %d\n\n", bMultictx);
  printf("#Aiq xml dirpath: %s\n\n", pIqfilesPath);
#endif

  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pIqfilesPath);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, u32Fps);
#endif
  }

  if (pOutPath) {
    g_output_file = fopen(pOutPath, "w");
    if (!g_output_file) {
      printf("ERROR: open file: %s fail, exit\n", pOutPath);
      return 0;
    }
  }

  RK_MPI_SYS_Init();
  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = pDeviceName;
  vi_chn_attr.u32BufCnt = u32BufCnt;
  vi_chn_attr.u32Width = u32Width;
  vi_chn_attr.u32Height = u32Height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("ERROR: create VI[0] error! ret=%d\n", ret);
    return 0;
  }

  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  switch (enCodecType) {
  case RK_CODEC_TYPE_H265:
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H265;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
    venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = 30;
    venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = u32Width * u32Height;
    // frame rate: in 30/1, out 30/1.
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = 30;
    break;
  case RK_CODEC_TYPE_MJPEG:
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_MJPEG;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
    venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stMjpegCbr.u32BitRate = u32Width * u32Height * 8;
    break;
  case RK_CODEC_TYPE_H264:
  default:
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = u32Width * u32Height;
    // frame rate: in 30/1, out 30/1.
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;
    break;
  }
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
  venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
  venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
  venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
  venc_chn_attr.stVencAttr.u32Profile = 77;
  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("ERROR: create VENC[0] error! ret=%d\n", ret);
    return 0;
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = 0;
  ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
  if (ret) {
    printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
    return 0;
  }

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
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
  signal(SIGINT, sigterm_handler);

  while (!quit) {
    usleep(500000);
  }

  if (g_output_file)
    fclose(g_output_file);

  printf("%s exit!\n", __func__);
  // unbind first
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: UnBind VI[0] and VENC[0] error! ret=%d\n", ret);
    return 0;
  }
  // destroy venc before vi
  ret = RK_MPI_VENC_DestroyChn(0);
  if (ret) {
    printf("ERROR: Destroy VENC[0] error! ret=%d\n", ret);
    return 0;
  }
  // destroy vi
  ret = RK_MPI_VI_DisableChn(s32CamId, 0);
  if (ret) {
    printf("ERROR: Destroy VI[0] error! ret=%d\n", ret);
    return 0;
  }

  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }
  return 0;
}
