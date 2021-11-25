// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
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

typedef struct _streamInfo {
  // for common
  int width;
  int height;
  IMAGE_TYPE_E pix_fmt;

  // for vi
  int vi_chn;
  const char *video_node;

  // for venc
  int venc_chn;
  CODEC_TYPE_E codec_type;
  VENC_RC_MODE_E codec_mode;
  int bps;
  int fps_out;
  int fps_in;

  // for jpeg
  int jpeg_chn;
  // for luma
  int luma_chn;
  const char *luma_node;
} StreamInfo;

static bool quit = false;
static RK_S32 s32CamId = 0;
#ifdef RKAIQ
static RK_BOOL bMultictx = RK_FALSE;
#endif
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void video_packet_cb(MEDIA_BUFFER mb) {
#if 0
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
#endif
  RK_MPI_MB_ReleaseBuffer(mb);
}

static const char *GetPixFmtName(IMAGE_TYPE_E pix_fmt) {
  if (pix_fmt == IMAGE_TYPE_NV12)
    return "NV12";
  else if (pix_fmt == IMAGE_TYPE_NV16)
    return "NV16";
  else if (pix_fmt == IMAGE_TYPE_FBC0)
    return "FBC0";
  else
    return "UNKNOW";
}

static const char *GetCodecTypeName(CODEC_TYPE_E codec_type) {
  if (codec_type == RK_CODEC_TYPE_H264)
    return "H264";
  else if (codec_type == RK_CODEC_TYPE_H265)
    return "H265";
  else if (codec_type == RK_CODEC_TYPE_JPEG)
    return "JPEG";
  else if (codec_type == RK_CODEC_TYPE_MJPEG)
    return "MJPEG";
  else
    return "UNKNOW";
}

static void ResetStreamInfo(StreamInfo *info) {
  if (!info)
    return;
  memset(info, 0, sizeof(StreamInfo));
  info->luma_chn = -1;
  info->jpeg_chn = -1;
}

static void DumpStreamInfo(StreamInfo *info) {
  if (!info)
    return;

  printf("VI[%d]:%s -> Venc[%d]:%s%s\n", info->vi_chn, info->video_node,
         info->venc_chn, GetCodecTypeName(info->codec_type),
         (info->jpeg_chn >= 0) ? " + JPEG" : "");
  printf("\tResolution:%d x %d\n", info->width, info->height);
  printf("\tPixformat:%s\n", GetPixFmtName(info->pix_fmt));
  if (info->jpeg_chn >= 0)
    printf("\tJPEG:VENC[%d]\n", info->jpeg_chn);
  if (info->luma_chn >= 0) {
    printf("\tLUMA:VI[%d]:%s\n", info->luma_chn, info->luma_node);
  }
  printf("\n");
}

int StreamOn(StreamInfo *info) {
  int ret = 0;

  if (!info) {
    printf("ERROR: %s: invalid info!\n", __func__);
    return -1;
  }
  printf("+++++++++ StreamOn START ++++++++++++\n");
  DumpStreamInfo(info);

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = info->video_node;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = info->width;
  vi_chn_attr.u32Height = info->height;
  vi_chn_attr.enPixFmt = info->pix_fmt;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, info->vi_chn, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, info->vi_chn);
  if (ret) {
    printf("Create Vi[%d]:%s failed! ret=%d\n", info->vi_chn, info->video_node,
           ret);
    return -1;
  }

  if (info->luma_chn >= 0) {
    if (!info->luma_node) {
      printf("ERROR: loss luma node...\n");
      return -1;
    }
    memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
    vi_chn_attr.pcVideoNode = info->luma_node;
    vi_chn_attr.u32BufCnt = 3;
    vi_chn_attr.u32Width = 1280;
    vi_chn_attr.u32Height = 720;
    vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_LUMA_ONLY;
    ret = RK_MPI_VI_SetChnAttr(s32CamId, info->luma_chn, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(s32CamId, info->luma_chn);
    if (ret) {
      printf("ERROR: Create Vi[%d] for luma failed! ret=%d\n", info->luma_chn,
             ret);
      return -1;
    }
  }

  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = info->codec_type;
  venc_chn_attr.stVencAttr.imageType = info->pix_fmt;
  venc_chn_attr.stVencAttr.u32PicWidth = info->width;
  venc_chn_attr.stVencAttr.u32PicHeight = info->height;
  venc_chn_attr.stVencAttr.u32VirWidth = info->width;
  venc_chn_attr.stVencAttr.u32VirHeight = info->height;
  venc_chn_attr.stVencAttr.u32Profile = 77;
  venc_chn_attr.stRcAttr.enRcMode = info->codec_mode;
  if (info->codec_mode == VENC_RC_MODE_H264CBR) {
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 3 * info->fps_out;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = info->bps;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = info->fps_out;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = info->fps_in;
  } else if (info->codec_mode == VENC_RC_MODE_H265CBR) {
    venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = 3 * info->fps_out;
    venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = info->bps;
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = info->fps_out;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = info->fps_in;
  } else if (info->codec_mode == VENC_RC_MODE_H264VBR) {
    venc_chn_attr.stRcAttr.stH264Vbr.u32Gop = 3 * info->fps_out;
    venc_chn_attr.stRcAttr.stH264Vbr.u32MaxBitRate = info->bps;
    venc_chn_attr.stRcAttr.stH264Vbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Vbr.fr32DstFrameRateNum = info->fps_out;
    venc_chn_attr.stRcAttr.stH264Vbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Vbr.u32SrcFrameRateNum = info->fps_in;
  } else if (info->codec_mode == VENC_RC_MODE_H265VBR) {
    venc_chn_attr.stRcAttr.stH265Vbr.u32Gop = 3 * info->fps_out;
    venc_chn_attr.stRcAttr.stH265Vbr.u32MaxBitRate = info->bps;
    venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateNum = info->fps_out;
    venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateNum = info->fps_in;
  } else {
    printf("ERROR: invalid rc mode:%d\n", info->codec_mode);
    return -1;
  }

  ret = RK_MPI_VENC_CreateChn(info->venc_chn, &venc_chn_attr);
  if (ret) {
    printf("Create venc[%d]:%s failed! ret=%d\n", info->venc_chn,
           GetCodecTypeName(info->codec_type), ret);
    return -1;
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = info->venc_chn;
  RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);

  if (info->jpeg_chn >= 0) {
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
    venc_chn_attr.stVencAttr.imageType = info->pix_fmt;
    venc_chn_attr.stVencAttr.u32PicWidth = info->width;
    venc_chn_attr.stVencAttr.u32PicHeight = info->height;
    venc_chn_attr.stVencAttr.u32VirWidth = info->width;
    venc_chn_attr.stVencAttr.u32VirHeight = info->height;
    // venc_chn_attr.stVencAttr.enRotation = VENC_ROTATION_90;
    ret = RK_MPI_VENC_CreateChn(info->jpeg_chn, &venc_chn_attr);
    if (ret) {
      printf("ERROR: Create jpeg:VENC[%d] failed! ret=%d\n", info->jpeg_chn,
             ret);
      return -1;
    }

    // The encoder defaults to continuously receiving frames from the previous
    // stage. Before performing the bind operation, set s32RecvPicNum to 0 to
    // make the encoding enter the pause state.
    VENC_RECV_PIC_PARAM_S stRecvParam;
    stRecvParam.s32RecvPicNum = 0;
    RK_MPI_VENC_StartRecvFrame(info->jpeg_chn, &stRecvParam);
  }

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = info->vi_chn;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = info->venc_chn;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: Bind VI[%d]:%s --> Venc[%d]:%s failed! ret=%d\n",
           info->vi_chn, info->video_node, info->venc_chn,
           GetCodecTypeName(info->codec_type), ret);
    return -1;
  }

  if (info->jpeg_chn >= 0) {
    stDestChn.s32ChnId = info->jpeg_chn;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("ERROR: Bind VI[%d]:%s --> Venc[%d]:%s failed! ret=%d\n",
             info->vi_chn, info->video_node, info->jpeg_chn,
             GetCodecTypeName(RK_CODEC_TYPE_JPEG), ret);
      return -1;
    }
  }

  printf("+++++++++ StreamOn END ++++++++++++\n");
  return 0;
}

int StreamOff(StreamInfo *info) {
  int ret = 0;
  printf("+++++++++ StreamOff START ++++++++++++\n");

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = info->vi_chn;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = info->venc_chn;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: UnBind VI[%d]:%s --> Venc[%d]:%s failed! ret=%d\n",
           info->vi_chn, info->video_node, info->venc_chn,
           GetCodecTypeName(info->codec_type), ret);
    return -1;
  }

  if (info->jpeg_chn >= 0) {
    stDestChn.s32ChnId = info->jpeg_chn;
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("ERROR: UnBind VI[%d]:%s --> Venc[%d]:%s failed! ret=%d\n",
             info->vi_chn, info->video_node, info->jpeg_chn,
             GetCodecTypeName(RK_CODEC_TYPE_JPEG), ret);
      return -1;
    }
  }

  if (info->luma_chn >= 0) {
    ret = RK_MPI_VI_DisableChn(s32CamId, info->luma_chn);
    if (ret) {
      printf("ERROR: VI[%d]:%s Destroy failed! ret=%d\n", info->luma_chn,
             info->luma_node, ret);
      return -1;
    }
  }

  ret = RK_MPI_VI_DisableChn(s32CamId, info->vi_chn);
  if (ret) {
    printf("ERROR: VI[%d]:%s Destroy failed! ret=%d\n", info->vi_chn,
           info->video_node, ret);
    return -1;
  }
  ret = RK_MPI_VENC_DestroyChn(info->venc_chn);
  if (ret) {
    printf("ERROR: VENC[%d]:%s Destroy failed! ret=%d\n", info->venc_chn,
           GetCodecTypeName(info->codec_type), ret);
    return -1;
  }

  if (info->jpeg_chn >= 0) {
    ret = RK_MPI_VENC_DestroyChn(info->jpeg_chn);
    if (ret) {
      printf("ERROR: VENC[%d]:%s Destroy failed! ret=%d\n", info->jpeg_chn,
             GetCodecTypeName(RK_CODEC_TYPE_JPEG), ret);
      return -1;
    }
  }
  printf("+++++++++ StreamOff END ++++++++++++\n");

  return 0;
}

static char optstr[] = "?::s:w:h:a:I:M:";

static void print_usage(char *name) {
  printf("#Function description:\n");
  printf("There are three streams in total:MainStream/SubStream0/SubStream.\n"
         "All streams are switched periodically!\n");
  printf("  MainStream: VI[0]: FBC0 2K/4K --> VENC[0]:H265 CBR\n");
  printf("                                --> VENC[3]:JPEG\n");
  printf("              VI[3]: NV12 720P  --> Luma caculation.\n\n");
  printf("  SubStream0: VI[1]: NV12 480P  --> VENC[1]:H264 CBR\n\n");
  printf("  SubStream1: VI[2]: NV12 720P  --> VENC[2]:H265 VBR\n\n");
  printf("#Usage Example: \n");
  printf("  %s [-s 5] [-w 3840] [-h 2160] [-a /etc/iqfiles]\n", name);
  printf("  @[-s] The duration of the stream on. default:5s\n");
  printf("  @[-w] img width for rkispp_m_bypass. default: 2688\n");
  printf("  @[-h] img height for rkispp_m_bypass. default: 1520\n");
  printf("  @[-a] the path of iqfiles. default: /oem/etc/iqfiles/\n");
  printf("  @[-I] camera ctx id, Default 0\n");
  printf("  @[-M] switch of multictx in isp, set 0 to disable, set 1 to "
         "enable. Default: 0\n");
}

int main(int argc, char *argv[]) {
  int loop_seconds = 5; // 5s
  int main_width = 2688;
  int main_height = 1520;
  int ret = 0;
  const char *iq_file_dir = "/oem/etc/iqfiles/";
  int fps = 30;
  int c = 0;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 's':
      loop_seconds = atoi(optarg);
      printf("#IN ARGS: loop_seconds: %d\n", loop_seconds);
      break;
    case 'w':
      main_width = atoi(optarg);
      printf("#IN ARGS: bypass width: %d\n", main_width);
      break;
    case 'a':
      iq_file_dir = optarg;
      printf("#IN ARGS: path of iqfiles: %s\n", iq_file_dir);
      break;
    case 'h':
      main_height = atoi(optarg);
      printf("#IN ARGS: bypass height: %d\n", main_height);
      break;
    case 'I':
      s32CamId = atoi(optarg);
      printf("#IN ARGS: s32CamId: %d\n", s32CamId);
      break;
#ifdef RKAIQ
    case 'M':
      if (atoi(optarg)) {
        bMultictx = RK_TRUE;
      }
      printf("#IN ARGS: bMultictx: %d\n", bMultictx);
      break;
#endif
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  printf(">>>>>>>>>>>>>>> Test START <<<<<<<<<<<<<<<<<<<<<<\n");
  printf("-->LoopSeconds:%d\n", loop_seconds);
  printf("-->BypassWidth:%d\n", main_width);
  printf("-->BypassHeight:%d\n", main_height);

  signal(SIGINT, sigterm_handler);
  // init rkmedia sys
  RK_MPI_SYS_Init();

  int test_cnt = 0;

#ifdef RKAIQ
  int hdr_mode_value = 0;
  rk_aiq_working_mode_t hdr_mode;
  RK_BOOL fec_enable = RK_FALSE;
  const char *cur_hdr_mode_name = NULL;
  const char *last_hdr_mode_name = NULL;
#endif

  while (!quit) {
    srand((unsigned)time(NULL));

#ifdef RKAIQ
    hdr_mode_value = rand() % 3;
    switch (hdr_mode_value) {
    case 0:
      hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
      cur_hdr_mode_name = "AIQ_NORMAL";
      fps = 30;
      break;
    case 1:
      hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
      cur_hdr_mode_name = "AIQ_HDR2";
      fps = 25;
      break;
    case 2:
      hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR3;
      cur_hdr_mode_name = "AIQ_HDR3";
      fps = 20;
      break;
    default:
      break;
    } // end switch

#if 0
    if (test_cnt % 2)
      fec_enable = RK_TRUE;
    else
      fec_enable = RK_FALSE;
#endif

    printf("\n#Test Cnt: %d\n", test_cnt);
    if (last_hdr_mode_name) {
      printf("\n>>>>> HDR Mode: frome [%s] to [%s] fps:%d, fec:%s <<<<<\n",
             last_hdr_mode_name, cur_hdr_mode_name, fps,
             fec_enable ? "ON" : "OFF");
    } else {
      printf("\n>>>>> HDR Mode:[%s] fps:%d, fec:%s <<<<<\n", cur_hdr_mode_name,
             fps, fec_enable ? "ON" : "OFF");
    }

    printf("hdr mode %d, fec mode %d, fps %d\n", hdr_mode, fec_enable, fps);
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, iq_file_dir);
    SAMPLE_COMM_ISP_SetFecEn(s32CamId, fec_enable);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
#endif // RKAIQ

    // Main Stream
    StreamInfo stream_info0;
    ResetStreamInfo(&stream_info0);
    stream_info0.width = main_width;
    stream_info0.height = main_height;
    stream_info0.pix_fmt = IMAGE_TYPE_FBC0;
    stream_info0.vi_chn = 0;
    stream_info0.video_node = "rkispp_m_bypass";
    stream_info0.luma_chn = 3;
    stream_info0.luma_node = "rkispp_scale0";
    stream_info0.venc_chn = 0;
    stream_info0.codec_type = RK_CODEC_TYPE_H265;
    stream_info0.codec_mode = VENC_RC_MODE_H265CBR;
    stream_info0.fps_in = fps;
    stream_info0.fps_out = (fps < 25) ? fps : 25;
    stream_info0.bps = 4000000; // 4Mb
    stream_info0.jpeg_chn = 3;
    ret = StreamOn(&stream_info0);
    if (ret) {
      printf("ERROR: Main stream(%dx%d) error!\n", stream_info0.width,
             stream_info0.height);
      return -1;
    }

    // Sub stream1
    StreamInfo stream_info1;
    ResetStreamInfo(&stream_info1);
    stream_info1.width = 720;
    stream_info1.height = 480;
    stream_info1.pix_fmt = IMAGE_TYPE_NV12;
    stream_info1.vi_chn = 1;
    stream_info1.video_node = "rkispp_scale1";
    stream_info1.venc_chn = 1;
    stream_info1.codec_type = RK_CODEC_TYPE_H264;
    stream_info1.codec_mode = VENC_RC_MODE_H264CBR;
    stream_info1.fps_in = fps;
    stream_info1.fps_out = (fps < 25) ? fps : 25;
    stream_info1.bps = 300000; // 300kb
    ret = StreamOn(&stream_info1);
    if (ret) {
      printf("ERROR: Sub stream1(%dx%d) error!\n", stream_info1.width,
             stream_info1.height);
      return -1;
    }

    // Sub stream2
    StreamInfo stream_info2;
    ResetStreamInfo(&stream_info2);
    stream_info2.width = 1280;
    stream_info2.height = 720;
    stream_info2.pix_fmt = IMAGE_TYPE_NV12;
    stream_info2.vi_chn = 2;
    stream_info2.video_node = "rkispp_scale2";
    stream_info2.venc_chn = 2;
    stream_info2.codec_type = RK_CODEC_TYPE_H265;
    stream_info2.codec_mode = VENC_RC_MODE_H265VBR;
    stream_info2.fps_in = fps;
    stream_info2.fps_out = (fps < 25) ? fps : 25;
    stream_info2.bps = 1000000; // 1Mb
    ret = StreamOn(&stream_info2);
    if (ret) {
      printf("ERROR: Sub stream1(%dx%d) error!\n", stream_info2.width,
             stream_info2.height);
      return -1;
    }

    printf("--> Start Luma VI[%d]...\n", stream_info0.luma_chn);
    ret = RK_MPI_VI_StartStream(s32CamId, stream_info0.luma_chn);
    if (ret) {
      printf("ERROR: VI[%d]:RK_MPI_VI_StartStream ret = %d\n",
             stream_info0.luma_chn, ret);
      return -1;
    }

    printf("INFO: Sleep %ds...\n", loop_seconds);
    int loop_cnt = loop_seconds * 2; // 500ms ervery time.
    RECT_S stRects = {256, 256, 256, 256};
    VIDEO_REGION_INFO_S stVideoRgn;
    stVideoRgn.pstRegion = &stRects;
    stVideoRgn.u32RegionNum = 1;
    RK_U64 u64LumaData;
    while (!quit) {
      loop_cnt--;
      ret = RK_MPI_VI_GetChnRegionLuma(s32CamId, stream_info0.luma_chn,
                                       &stVideoRgn, &u64LumaData, 500);
      if (ret) {
        printf("ERROR: VI[%d]:RK_MPI_VI_GetChnRegionLuma ret = %d\n",
               stream_info0.luma_chn, ret);
      } else {
        printf("VI[%d]:Rect[0] {256, 256, 256, 256} -> luma:%llu\n",
               stream_info0.luma_chn, u64LumaData);
      }

      if (loop_cnt < 0)
        break;
    }

    ret = StreamOff(&stream_info0);
    if (ret) {
      printf("ERROR: Main strem off failed!\n");
      break;
    }
    ret = StreamOff(&stream_info1);
    if (ret) {
      printf("ERROR: Sub strem1 off failed!\n");
      break;
    }
    ret = StreamOff(&stream_info2);
    if (ret) {
      printf("ERROR: Sub strem2 off failed!\n");
      break;
    }

#ifdef RKAIQ
    printf("INFO: Stop ISP...\n");
    SAMPLE_COMM_ISP_Stop(s32CamId);
    last_hdr_mode_name = cur_hdr_mode_name;
#endif
    test_cnt++;
  }

  printf("%s exit!\n", __func__);
  return 0;
}
