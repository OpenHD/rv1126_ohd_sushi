// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "common/sample_common.h"
#include "rkmedia_api.h"
#include "rkmedia_venc.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}
static int g_frame_cnt;

static inline void PrintRss(const char *tag, int seq) {
  pid_t id = getpid();
  char cmd[64] = {0};
  printf("==========[%s]:%d\n", tag, seq);
  sprintf(cmd, "cat /proc/%d/status | grep RSS", id);
  system(cmd);
}

#define MAIN_STREAM_VENC_CHN 0
#define SUB0_STREAM_VENC_CHN 1
#define SUB1_STREAM_VENC_CHN 2
#define MAIN_STREAM_JPEG_CHN 3

#define MAIN_STREAM_VI_CHN 0
#define SUB0_STREAM_VI_CHN 1
#define SUB1_STREAM_VI_CHN 2

void video_packet_cb(MEDIA_BUFFER mb) {
  if (g_frame_cnt++ < 5) {
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

    printf("Get Video Encoded packet(%s):ptr:%p, fd:%d, size:%zu, mode:%d, Chn:%d\n",
          nalu_type, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
          RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb));
  }

  RK_MPI_MB_ReleaseBuffer(mb);
}

int StreamOn(RK_U32 u32StreamIdx) {
  int ret = 0;
  RK_U32 u32Width = 0;
  RK_U32 u32Height = 0;
  RK_U32 u32ViChn = 0;
  RK_U32 u32VencChn = 0;
  const RK_CHAR *pcStreamName = NULL;
  const RK_CHAR *pcVideoNode = NULL;
  IMAGE_TYPE_E enImgType = IMAGE_TYPE_NV12;
  CODEC_TYPE_E enCodecType = RK_CODEC_TYPE_H264;
  const RK_CHAR *pcCodecName = "H264";
  RK_U32 u32CodecRandNum = 0;

  srand((unsigned)time(0));
  u32CodecRandNum = rand() % 10;

  // If the random number is odd, choose H265 encoder
  switch (u32CodecRandNum % 3) {
  case 0: // for h264
    enCodecType = RK_CODEC_TYPE_H264;
    pcCodecName = "H264";
    break;
  case 1: // for h264
    enCodecType = RK_CODEC_TYPE_H265;
    pcCodecName = "H265";
    break;
  case 2:
    enCodecType = RK_CODEC_TYPE_MJPEG;
    pcCodecName = "MJPEG";
    break;
  default:
    break;
  }

  switch (u32StreamIdx) {
    case 0:
      pcStreamName = "MainStream";
      pcVideoNode = "rkispp_m_bypass";
      u32Width = 2688;
      u32Height = 1520;
      u32ViChn = MAIN_STREAM_VI_CHN;
      u32VencChn = MAIN_STREAM_VENC_CHN;
      enImgType = IMAGE_TYPE_FBC0;
      break;
    case 1:
      pcStreamName = "Sub0Stream";
      pcVideoNode = "rkispp_scale0";
      u32Width = 1280;
      u32Height = 720;
      u32ViChn = SUB0_STREAM_VI_CHN;
      u32VencChn = SUB0_STREAM_VENC_CHN;
      break;
    case 2:
      pcStreamName = "Sub1Stream";
      pcVideoNode = "rkispp_scale1";
      u32Width = 720;
      u32Height = 480;
      u32ViChn = SUB1_STREAM_VI_CHN;
      u32VencChn = SUB1_STREAM_VENC_CHN;
      break;
    default:
      printf("#Error: %s invalid stream idx:%d\n", __func__, u32StreamIdx);
      return -1;
  }

  printf("#%s, wxh: %dx%d, CodeType: %s, ImgType: %s Start........\n\n",
         pcStreamName, u32Width, u32Height, pcCodecName,
         (enImgType == IMAGE_TYPE_FBC0) ? "FBC0" : "NV12");

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = pcVideoNode;
  vi_chn_attr.u32BufCnt = 4;
  vi_chn_attr.u32Width = u32Width;
  vi_chn_attr.u32Height = u32Height;
  vi_chn_attr.enPixFmt = enImgType;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(0, u32ViChn, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(0, u32ViChn);
  if (ret) {
    printf("Create Vi failed! ret=%d\n", ret);
    return -1;
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
  ret = RK_MPI_VENC_CreateChn(u32VencChn, &venc_chn_attr);
  if (ret) {
    printf("Create venc[%d] failed! ret=%d\n", u32VencChn, ret);
    return -1;
  }

  if (u32StreamIdx == 0) {
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
    venc_chn_attr.stVencAttr.imageType = enImgType;
    venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
    venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
    venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
    venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
    // venc_chn_attr.stVencAttr.enRotation = VENC_ROTATION_90;
    ret = RK_MPI_VENC_CreateChn(MAIN_STREAM_JPEG_CHN, &venc_chn_attr);
    if (ret) {
      printf("Create jpeg failed! ret=%d\n", ret);
      return -1;
    }

    // The encoder defaults to continuously receiving frames from the previous
    // stage. Before performing the bind operation, set s32RecvPicNum to 0 to
    // make the encoding enter the pause state.
    VENC_RECV_PIC_PARAM_S stRecvParam;
    stRecvParam.s32RecvPicNum = 0;
    RK_MPI_VENC_StartRecvFrame(3, &stRecvParam);
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = u32VencChn;
  RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = u32ViChn;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = u32VencChn;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Create RK_MPI_SYS_Bind0 failed! ret=%d\n", ret);
    return -1;
  }

  if (u32StreamIdx == 0) {
    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32ChnId = MAIN_STREAM_JPEG_CHN;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("ERROR: JPEG: Bind VO[%d] to VENC[%d] failed! ret=%d\n", u32ViChn, MAIN_STREAM_JPEG_CHN, ret);
      return -1;
    }
  }

  return 0;
}

int StreamOff(RK_U32 u32StreamIdx) {
  int ret = 0;
  RK_U32 u32ViChn = 0;
  RK_U32 u32VencChn = 0;
  const RK_CHAR *pcStreamName = NULL;

  switch (u32StreamIdx) {
    case 0:
      pcStreamName = "MainStream";
      u32ViChn = MAIN_STREAM_VI_CHN;
      u32VencChn = MAIN_STREAM_VENC_CHN;
      break;
    case 1:
      pcStreamName = "Sub0Stream";
      u32ViChn = SUB0_STREAM_VI_CHN;
      u32VencChn = SUB0_STREAM_VENC_CHN;
      break;
    case 2:
      pcStreamName = "Sub1Stream";
      u32ViChn = SUB1_STREAM_VI_CHN;
      u32VencChn = SUB1_STREAM_VENC_CHN;
      break;
    default:
      printf("#Error: %s invalid stream idx:%d\n", __func__, u32StreamIdx);
      return -1;
  }

  printf("#%s end ......................................\n", pcStreamName);

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = u32ViChn;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = u32VencChn;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: unbind vi[%d] -> venc[%d] failed!\n", u32ViChn, u32VencChn);
    return -1;
  }

  if (u32StreamIdx == 0) {
    stDestChn.s32ChnId = MAIN_STREAM_JPEG_CHN;
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("ERROR: JPEG: unbind vi[%d] -> venc[%d] failed!\n", u32ViChn, MAIN_STREAM_JPEG_CHN);
      return -1;
    }
    RK_MPI_VENC_DestroyChn(MAIN_STREAM_JPEG_CHN);
  }

  RK_MPI_VENC_DestroyChn(u32VencChn);
  RK_MPI_VI_DisableChn(0, u32ViChn);

  return 0;
}

static RK_CHAR optstr[] = "?::a::w:h:c:s:I:M:f:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"frame_cnt", required_argument, NULL, 'c'},
    {"duration", required_argument, NULL, 's'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(char *name) {
  printf("#Function description:\n");
  printf("There are three streams in total:MainStream/SubStream0/SubStream.\n");
  printf("  #MainStream:rkispp_m_bypass(WxH FBC0) --> Venc(random:H264/H265/MJPEG)\n");
  printf("                                        --> Venc(JPEG: take photos)\n");
  printf("  #SubStream0: rkispp_scale1(1280x720 NV12) --> Venc(random:H264/H265/MJPEG)\n");
  printf("  #SubStream1: rkispp_scale2(720x480 NV12) --> Venc(random:H264/H265/MJPEG)\n");
  printf("#Usage Example: \n");
  printf("  %s [-c 20] [-s 5] [-w 2688] [-h 1520] [-a the path of iqfiles]\n", name);
  printf("  @[-c] Main stream switching times. defalut:20\n");
  printf("  @[-s] The duration of the main stream. default:5\n");
  printf("  @[-w] img width for rkispp_m_bypass. default: 2688\n");
  printf("  @[-h] img height for rkispp_m_bypass. default: 1520\n");
  printf("  @[-a] the path of iqfiles. default: NULL\n");
}

int main(int argc, char *argv[]) {
  RK_U32 u32LoopCnt = 20000;
  RK_U32 u32LoopSeconds = 3; // 3s
  RK_U32 u32Width = 2688;
  RK_U32 u32Height = 1520;
  RK_U32 u32LoopValue = 0;
  RK_CHAR *pIqfilesPath = NULL;

  int c = 0;
  opterr = 1;
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
    case 'c':
      u32LoopCnt = (RK_U32)atoi(optarg);
      break;
    case 's':
      u32LoopSeconds = (RK_U32)atoi(optarg);
      break;
    case 'w':
      u32Width = atoi(optarg);
      break;
    case 'h':
      u32Height = atoi(optarg);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  printf(">>>>>>>>>>>>>>> Test START <<<<<<<<<<<<<<<<<<<<<<\n");
  printf("-->LoopCnt:%d\n", u32LoopCnt);
  printf("-->LoopSeconds:%d\n", u32LoopSeconds);
  printf("-->BypassWidth:%d\n", u32Width);
  printf("-->BypassHeight:%d\n", u32Height);


  RK_MPI_SYS_Init();

  signal(SIGINT, sigterm_handler);
  while (!quit && (u32LoopValue < u32LoopCnt)) {
    g_frame_cnt = 0;
    PrintRss("Before", u32LoopValue);

    if (pIqfilesPath) {
#ifdef RKAIQ
      SAMPLE_COMM_ISP_Init(0, RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, pIqfilesPath);
      SAMPLE_COMM_ISP_Run(0);
      SAMPLE_COMM_ISP_SetFrameRate(0, 30);
#endif
    }

    if (StreamOn(0)) {
      printf("ERROR: Create MainStream failed!\n");
      break;
    }
    if (StreamOn(1)) {
      printf("ERROR: Create Sub0Stream failed!\n");
      break;
    }
    if (StreamOn(2)) {
      printf("ERROR: Create Sub1Stream failed!\n");
      break;
    }

    sleep(u32LoopSeconds);

    if (StreamOff(0)) {
      printf("ERROR: Destroy MainStream failed!\n");
      break;
    }
    if (StreamOff(1)) {
      printf("ERROR: Destroy Sub0Stream failed!\n");
      break;
    }
    if (StreamOff(2)) {
      printf("ERROR: Destroy Sub1Stream failed!\n");
      break;
    }

    usleep(100000); // 100ms

    if (pIqfilesPath) {
#ifdef RKAIQ
      SAMPLE_COMM_ISP_Stop(0);
      usleep(500000); // 500ms for aiq deinit
#endif
    }
    PrintRss("After", u32LoopValue);
    u32LoopValue++;
  }

  printf(">>>>>>>>>>>>>>> Test END <<<<<<<<<<<<<<<<<<<<<<\n");
  return 0;
}
