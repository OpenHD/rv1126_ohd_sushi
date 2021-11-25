// Copyright 2021 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
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

typedef struct rkMuxerHandle {
  RK_U32 u32ModeIdx;
  RK_U32 u32ChnIdx;
} MuxerHandle;

void muxer_event_cb(RK_VOID *pHandle, RK_VOID *pstEvent) {
  RK_S32 s32ModeIdx = -1;
  RK_S32 s32ChnIdx = -1;

  printf("### %s: Handle:%p, event:%p\n", __func__, pHandle, pstEvent);
  if (pHandle) {
    MuxerHandle *pstMuxerHandle = (MuxerHandle *)pHandle;
    s32ModeIdx = (RK_S32)pstMuxerHandle->u32ModeIdx;
    s32ChnIdx = (RK_S32)pstMuxerHandle->u32ChnIdx;
  }
  if (pstEvent) {
    MUXER_EVENT_INFO_S *pstMuxerEvent = (MUXER_EVENT_INFO_S *)pstEvent;
    printf("@@@ %s: ModeID:%d, ChnID:%d, EventType:%d, filename:%s, value:%d\n",
           __func__, s32ModeIdx, s32ChnIdx, pstMuxerEvent->enEvent,
           pstMuxerEvent->unEventInfo.stFileInfo.asFileName,
           (int)pstMuxerEvent->unEventInfo.stFileInfo.u32Duration);
  }
}

static RK_CHAR optstr[] = "?::a::w:h:c:o:e:d:I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"device_name", required_argument, NULL, 'd'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"callback", required_argument, NULL, 'c'},
    {"output_path", required_argument, NULL, 'o'},
    {"encode", required_argument, NULL, 'e'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]] [-w 1920] "
         "[-h 1080]"
         "[-c 0] "
         "[-d rkispp_scale0] "
         "[-e h264] "
         "[-I 0] "
         "[-M 0]\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath emtpty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
#else
  printf("\t%s [-w 1920] "
         "[-h 1080]"
         "[-c 0] "
         "[-I 0] "
         "[-d rkispp_scale0] "
         "[-e h264]\n",
         name);
#endif
  printf("\t-w | --width: VI width, Default:1920\n");
  printf("\t-h | --heght: VI height, Default:1080\n");
  printf("\t-c | --callback: enable callback name, Default:0\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-d | --device_name set pcDeviceName, Default:rkispp_scale0, "
         "Option:[rkispp_scale0, rkispp_scale1, rkispp_scale2]\n");
  printf("\t-e | --encode: encode type, Default:h264, Value:h264, h265\n");
}

static RK_U32 gu32FileIdx;
int GetRecordFileName(RK_VOID *pHandle,
                      RK_CHAR *pcFileName, RK_U32 muxerId) {
  if(!pcFileName) {
    printf("Error: pcFileName is null\n");
    return -1;
  }

  printf("#%s: Handle:%p idx:%u, fileIndex:%d, ...\n", __func__, pHandle,
         *((RK_U32 *)pHandle), muxerId);
  sprintf(pcFileName, "/userdata/MuxerCbTest_%u.mp4",
          *((RK_U32 *)pHandle));

  printf("#%s: NewRecordFileName:[%s]\n", __func__, pcFileName);

  *((RK_U32 *)pHandle) = *((RK_U32 *)pHandle) + 1;

  return 0;
}

int main(int argc, char *argv[]) {
  RK_U32 u32Width = 1920;
  RK_U32 u32Height = 1080;
  RK_CHAR *pVideoNode = "rkispp_scale0";
  RK_CHAR *pIqfilesPath = NULL;
  CODEC_TYPE_E enCodecType = RK_CODEC_TYPE_H264;
  RK_CHAR *pCodecName = "H264";
  RK_S32 s32CamId = 0;
  RK_U32 u32BufCnt = 3;

  RK_U32 u32SampleRate = 16000;
  RK_U32 u32BitRate = 64000; // 64kbps
  RK_U32 u32ChnCnt = 2;
  RK_U32 u32FrameCnt = 1024; // always 1024 for mp3
  SAMPLE_FORMAT_E enSampleFmt = RK_SAMPLE_FMT_FLTP;
  // default:CARD=rockchiprk809co
  RK_CHAR *pAudioDevice = "default";
  RK_S32 s32EnableCallback = -1;

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
      s32EnableCallback = atoi(optarg);
      break;
    case 'd':
      pVideoNode = optarg;
      break;
    case 'e':
      if (!strcmp(optarg, "h264")) {
        enCodecType = RK_CODEC_TYPE_H264;
        pCodecName = "H264";
      } else if (!strcmp(optarg, "h265")) {
        enCodecType = RK_CODEC_TYPE_H265;
        pCodecName = "H265";
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
#endif
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#Device: %s\n", pVideoNode);
  printf("#CodecName: %s\n", pCodecName);
  printf("#Resolution: %dx%d\n", u32Width, u32Height);
  printf("#EnableCallback: %d\n", s32EnableCallback);
  printf("#CameraIdx: %d\n", s32CamId);
#ifdef RKAIQ
  printf("#Aiq MultiCtx: %d\n", bMultictx);
  printf("#Aiq XML Dir: %s\n", pIqfilesPath);
#endif

  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pIqfilesPath);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, u32Fps);
#endif
  }

  RK_MPI_SYS_Init();

  /************************************************
   * Create Video pipeline: VI[0] --> VENC[0]
   * **********************************************/
  // Create VI
  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = pVideoNode;
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

  // Create VENC
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
  venc_chn_attr.stVencAttr.u32Profile = 100;
  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("ERROR: create VENC[0] error! ret=%d\n", ret);
    return 0;
  }

  /************************************************
   * Create Audio pipeline: AI[0] --> AENC[0]
   * **********************************************/
  // Create AI
  AI_CHN_ATTR_S ai_attr;
  ai_attr.pcAudioNode = pAudioDevice;
  ai_attr.enSampleFormat = enSampleFmt;
  ai_attr.u32NbSamples = u32FrameCnt;
  ai_attr.u32SampleRate = u32SampleRate;
  ai_attr.u32Channels = u32ChnCnt;
  ai_attr.enAiLayout = AI_LAYOUT_NORMAL;
  ret = RK_MPI_AI_SetChnAttr(0, &ai_attr);
  ret |= RK_MPI_AI_EnableChn(0);
  if (ret) {
    printf("Create AI[0] failed! ret=%d\n", ret);
    return -1;
  }

  // Create AENC
  AENC_CHN_ATTR_S aenc_attr;
  aenc_attr.enCodecType = RK_CODEC_TYPE_MP3;
  aenc_attr.u32Bitrate = u32BitRate;
  aenc_attr.u32Quality = 1;
  aenc_attr.stAencMP3.u32Channels = u32ChnCnt;
  aenc_attr.stAencMP3.u32SampleRate = u32SampleRate;
  ret = RK_MPI_AENC_CreateChn(0, &aenc_attr);
  if (ret) {
    printf("Create AENC[0] failed! ret=%d\n", ret);
    return -1;
  }

  /************************************************
   * Create MediaMuxer:
   *   VENC[0]----->|----------|
   *                | MUXER[0] |
   *   AENC[0]----->|----------|
   * **********************************************/
  MUXER_CHN_ATTR_S stMuxerAttr;
  memset(&stMuxerAttr, 0, sizeof(stMuxerAttr));
  stMuxerAttr.u32MuxerId = 0;
  stMuxerAttr.enMode = MUXER_MODE_AUTOSPLIT;
  stMuxerAttr.enType = MUXER_TYPE_MP4;
  stMuxerAttr.stSplitAttr.enSplitType = MUXER_SPLIT_TYPE_TIME;
  stMuxerAttr.stSplitAttr.u32TimeLenSec = 30;
  if (!s32EnableCallback) {
    printf("#MuxerTest: use split name auto type...\n");
    stMuxerAttr.stSplitAttr.enSplitNameType = MUXER_SPLIT_NAME_TYPE_AUTO;
    stMuxerAttr.stSplitAttr.stNameAutoAttr.pcPrefix = "muxer_test";
    stMuxerAttr.stSplitAttr.stNameAutoAttr.pcBaseDir = "/userdata";
    // Split File name with timestamp.
    // stMuxerAttr.stSplitAttr.stNameAutoAttr.bTimeStampEnable = RK_TRUE;
    stMuxerAttr.stSplitAttr.stNameAutoAttr.u16StartIdx = 1;
  } else {
    printf("#MuxerTest: use split name callback type, cb:%p, handle:%p...\n",
           GetRecordFileName, &gu32FileIdx);
    stMuxerAttr.stSplitAttr.enSplitNameType = MUXER_SPLIT_NAME_TYPE_CALLBACK;
    stMuxerAttr.stSplitAttr.stNameCallBackAttr.pcbRequestFileNames =
        GetRecordFileName;
    stMuxerAttr.stSplitAttr.stNameCallBackAttr.pCallBackHandle =
        (RK_VOID *)&gu32FileIdx;
  }

  stMuxerAttr.stVideoStreamParam.enCodecType = enCodecType;
  stMuxerAttr.stVideoStreamParam.enImageType = IMAGE_TYPE_NV12;
  stMuxerAttr.stVideoStreamParam.u16Fps = 30;
  stMuxerAttr.stVideoStreamParam.u16Level = 41;    // for h264
  stMuxerAttr.stVideoStreamParam.u16Profile = 100; // for h264
  stMuxerAttr.stVideoStreamParam.u32BitRate = u32Width * u32Height;
  stMuxerAttr.stVideoStreamParam.u32Width = u32Width;
  stMuxerAttr.stVideoStreamParam.u32Height = u32Height;

  stMuxerAttr.stAudioStreamParam.enCodecType = RK_CODEC_TYPE_MP3;
  stMuxerAttr.stAudioStreamParam.enSampFmt = enSampleFmt;
  stMuxerAttr.stAudioStreamParam.u32Channels = u32ChnCnt;
  stMuxerAttr.stAudioStreamParam.u32NbSamples = u32FrameCnt;
  stMuxerAttr.stAudioStreamParam.u32SampleRate = u32SampleRate;
  ret = RK_MPI_MUXER_EnableChn(0, &stMuxerAttr);
  if (ret) {
    printf("Create MUXER[0] failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
  MUXER_CHN_S stMuxerChn;

  stSrcChn.enModId = RK_ID_MUXER;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;

  MuxerHandle stMuxerHandle;
  stMuxerHandle.u32ChnIdx = 0;
  stMuxerHandle.u32ModeIdx = RK_ID_MUXER;

  ret = RK_MPI_SYS_RegisterEventCb(&stSrcChn, &stMuxerHandle, muxer_event_cb);
  if (ret) {
    printf("Register event callback failed! ret=%d\n", ret);
    return -1;
  }

  printf("### Start muxer stream...\n");
  ret = RK_MPI_MUXER_StreamStart(0);
  if (ret) {
    printf("Muxer start stream failed! ret=%d\n", ret);
    return -1;
  }

  // Bind VENC[0] to MUXER[0]:VIDEO
  stSrcChn.enModId = RK_ID_VENC;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stMuxerChn.enModId = RK_ID_MUXER;
  stMuxerChn.enChnType = MUXER_CHN_TYPE_VIDEO;
  stMuxerChn.s32ChnId = 0;
  ret = RK_MPI_MUXER_Bind(&stSrcChn, &stMuxerChn);
  if (ret) {
    printf("ERROR: Bind VENC[0] and MUXER[0]:VIDEO error! ret=%d\n", ret);
    return 0;
  }

  // Bind VENC[0] to MUXER[0]:AUDIO
  stSrcChn.enModId = RK_ID_AENC;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stMuxerChn.enModId = RK_ID_MUXER;
  stMuxerChn.enChnType = MUXER_CHN_TYPE_AUDIO;
  stMuxerChn.s32ChnId = 0;
  ret = RK_MPI_MUXER_Bind(&stSrcChn, &stMuxerChn);
  if (ret) {
    printf("ERROR: Bind AENC[0] and MUXER[0]:AUDIO error! ret=%d\n", ret);
    return 0;
  }

  // Bind VI[0] to VENC[0]
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: Bind VI[0] and VENC[0] error! ret=%d\n", ret);
    return 0;
  }

  // Bind AI[0] to AENC[0]
  stSrcChn.enModId = RK_ID_AI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_AENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: Bind AI[0] and AENC[0] error! ret=%d\n", ret);
    return 0;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }
  printf("%s exit!\n", __func__);

  printf("### Stop muxer stream...\n");
  ret = RK_MPI_MUXER_StreamStop(0);
  if (ret) {
    printf("Muxer stop stream failed! ret=%d\n", ret);
    return -1;
  }

  // UnBind VENC[0] to MUXER[0]
  stSrcChn.enModId = RK_ID_VENC;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stMuxerChn.enModId = RK_ID_MUXER;
  stMuxerChn.enChnType = MUXER_CHN_TYPE_VIDEO;
  stMuxerChn.s32ChnId = 0;
  ret = RK_MPI_MUXER_UnBind(&stSrcChn, &stMuxerChn);
  if (ret) {
    printf("ERROR: UnBind VENC[0] and MUXER[0] error! ret=%d\n", ret);
    return 0;
  }

  // UnBind AENC[0] to MUXER[0]
  stSrcChn.enModId = RK_ID_AENC;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stMuxerChn.enModId = RK_ID_MUXER;
  stMuxerChn.enChnType = MUXER_CHN_TYPE_AUDIO;
  stMuxerChn.s32ChnId = 0;
  ret = RK_MPI_MUXER_UnBind(&stSrcChn, &stMuxerChn);
  if (ret) {
    printf("ERROR: UnBind AENC[0] and MUXER[0] error! ret=%d\n", ret);
    return 0;
  }

  // UnBind VI[0] to VENC[0]
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: UnBind VI[0] and VENC[0] error! ret=%d\n", ret);
    return 0;
  }

  // UnBind AI[0] to AENC[0]
  stSrcChn.enModId = RK_ID_AI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_AENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: UnBind AI[0] and AENC[0] error! ret=%d\n", ret);
    return 0;
  }

  /**********************************************
   * Destroy all chns
   * ********************************************/
  // Destroy MUXER[0]
  ret = RK_MPI_MUXER_DisableChn(0);
  if (ret) {
    printf("ERROR: Destroy MUXER[0] error! ret=%d\n", ret);
    return 0;
  }
  // Destroy VENC[0]
  ret = RK_MPI_VENC_DestroyChn(0);
  if (ret) {
    printf("ERROR: Destroy VENC[0] error! ret=%d\n", ret);
    return 0;
  }
  // Destroy VI[0]
  ret = RK_MPI_VI_DisableChn(s32CamId, 0);
  if (ret) {
    printf("ERROR: Destroy VI[0] error! ret=%d\n", ret);
    return 0;
  }

  // Destroy AENC[0]
  ret = RK_MPI_AENC_DestroyChn(0);
  if (ret) {
    printf("ERROR: Destroy AENC[0] error! ret=%d\n", ret);
    return 0;
  }

  // Destroy AI[0]
  ret = RK_MPI_AI_DisableChn(0);
  if (ret) {
    printf("ERROR: Destroy AI[0] error! ret=%d\n", ret);
    return 0;
  }

  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }
  return 0;
}
