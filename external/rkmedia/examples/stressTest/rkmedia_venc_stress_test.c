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
  if (g_frame_cnt++ < 8) {
    const char *nalu_type = "JPEG";
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

    printf("Get Video Encoded packet(%s):size:%zu, Chn:%d\n",
          nalu_type, RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetChannelID(mb));
  }

  RK_MPI_MB_ReleaseBuffer(mb);
}

int StreamOn(int stream_idx) {
  int ret = 0;
  int width = 0;
  int height = 0;
  int vi_chn = 0;
  int venc_chn = 0;
  const char *streamName = NULL;
  const char *videoNode = NULL;
  IMAGE_TYPE_E img_type = IMAGE_TYPE_NV12;
  CODEC_TYPE_E codec_type = RK_CODEC_TYPE_H264;
  static int codec_chg_idx = 0;

  switch (stream_idx) {
    case 0:
      streamName = "MainStream";
      videoNode = "rkispp_m_bypass";
      width = 2688;
      height = 1520;
      vi_chn = MAIN_STREAM_VI_CHN;
      venc_chn = MAIN_STREAM_VENC_CHN;
      img_type = IMAGE_TYPE_FBC0;
      break;
    case 1:
      streamName = "Sub0Stream";
      videoNode = "rkispp_scale0";
      width = 1280;
      height = 720;
      vi_chn = SUB0_STREAM_VI_CHN;
      venc_chn = SUB0_STREAM_VENC_CHN;
      break;
    case 2:
      streamName = "Sub1Stream";
      videoNode = "rkispp_scale1";
      width = 720;
      height = 480;
      vi_chn = SUB1_STREAM_VI_CHN;
      venc_chn = SUB1_STREAM_VENC_CHN;
      break;
    default:
      printf("#Error: %s invalid stream idx:%d\n", __func__, stream_idx);
      return -1;
  }

  if (codec_chg_idx++ % 2)
    codec_type = RK_CODEC_TYPE_H265;
  else
    codec_type = RK_CODEC_TYPE_H264;

  printf("#%s, wxh: %dx%d, CodeType: %s, ImgType: %s Start........\n\n",
         streamName, width, height,
         (codec_type == RK_CODEC_TYPE_H264) ? "H264" : "H265",
         (img_type == IMAGE_TYPE_FBC0) ? "FBC0" : "NV12");

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = videoNode;
  vi_chn_attr.u32BufCnt = 4;
  vi_chn_attr.u32Width = width;
  vi_chn_attr.u32Height = height;
  vi_chn_attr.enPixFmt = img_type;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(0, vi_chn, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(0, vi_chn);
  if (ret) {
    printf("Create Vi failed! ret=%d\n", ret);
    return -1;
  }

#if 0
  if (img_type == IMAGE_TYPE_FBC0) {
    printf("TEST: INFO: FBC0 use rkispp_scale0 for luma caculation!\n");
    vi_chn_attr.pcVideoNode = "rkispp_scale0";
    vi_chn_attr.u32BufCnt = 4;
    vi_chn_attr.u32Width = 1280;
    vi_chn_attr.u32Height = 720;
    vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_LUMA_ONLY;
    ret = RK_MPI_VI_SetChnAttr(0, 3, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(0, 3);
    if (ret) {
      printf("Create Vi[3] for luma failed! ret=%d\n", ret);
      return -1;
    }
  }
#endif

  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = codec_type;
  venc_chn_attr.stVencAttr.imageType = img_type;
  venc_chn_attr.stVencAttr.u32PicWidth = width;
  venc_chn_attr.stVencAttr.u32PicHeight = height;
  venc_chn_attr.stVencAttr.u32VirWidth = width;
  venc_chn_attr.stVencAttr.u32VirHeight = height;

  if (codec_type == RK_CODEC_TYPE_H264) {
    venc_chn_attr.stVencAttr.u32Profile = 77;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 75;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = width * height;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;
  } else {
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
    venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = 75;
    venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = width * height;
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = 30;
  }
  ret = RK_MPI_VENC_CreateChn(venc_chn, &venc_chn_attr);
  if (ret) {
    printf("Create avc failed! ret=%d\n", ret);
    return -1;
  }

  if (stream_idx == 0) {
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
    venc_chn_attr.stVencAttr.imageType = img_type;
    venc_chn_attr.stVencAttr.u32PicWidth = width;
    venc_chn_attr.stVencAttr.u32PicHeight = height;
    venc_chn_attr.stVencAttr.u32VirWidth = width;
    venc_chn_attr.stVencAttr.u32VirHeight = height;
    // venc_chn_attr.stVencAttr.enRotation = VENC_ROTATION_90;
    ret = RK_MPI_VENC_CreateChn(MAIN_STREAM_JPEG_CHN, &venc_chn_attr);
    if (ret) {
      printf("Create jpeg failed! ret=%d\n", ret);
      return -1;
    }

#if 0
    // The encoder defaults to continuously receiving frames from the previous
    // stage. Before performing the bind operation, set s32RecvPicNum to 0 to
    // make the encoding enter the pause state.
    VENC_RECV_PIC_PARAM_S stRecvParam;
    stRecvParam.s32RecvPicNum = 0;
    RK_MPI_VENC_StartRecvFrame(3, &stRecvParam);
#else
    MPP_CHN_S stEncChn;
    stEncChn.enModId = RK_ID_VENC;
    stEncChn.s32DevId = 0;
    stEncChn.s32ChnId = MAIN_STREAM_JPEG_CHN;
    RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
#endif
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = venc_chn;
  RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = vi_chn;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = venc_chn;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Create RK_MPI_SYS_Bind0 failed! ret=%d\n", ret);
    return -1;
  }

  if (stream_idx == 0) {
    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32ChnId = MAIN_STREAM_JPEG_CHN;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("ERROR: JPEG: Bind VO[%d] to VENC[%d] failed! ret=%d\n", vi_chn, MAIN_STREAM_JPEG_CHN, ret);
      return -1;
    }
  }

  return 0;
}

#if 0
  RECT_S stRects[2] = {{0, 0, 256, 256}, {256, 256, 256, 256}};

  VIDEO_REGION_INFO_S stVideoRgn;
  stVideoRgn.pstRegion = stRects;
  stVideoRgn.u32RegionNum = 2;
  RK_U64 u64LumaData[2];
  while (!quit) {
    usleep(500000);
    loop_cnt--;

    ret = RK_MPI_VI_GetChnRegionLuma(0, luma_chn, &stVideoRgn, u64LumaData, 100);
    if (ret) {
      printf("ERROR: VI[%d]:RK_MPI_VI_GetChnRegionLuma ret = %d\n", luma_chn, ret);
    } else {
      printf("VI[%d]:Rect[0] {0, 0, 256, 256} -> luma:%llu\n", luma_chn, u64LumaData[0]);
      printf("VI[%d]:Rect[1] {256, 256, 256, 256} -> luma:%llu\n", luma_chn, u64LumaData[1]);
    }
    if (loop_cnt < 0)
      break;
  }

#endif

int StreamOff(int stream_idx) {
  int ret = 0;
  int vi_chn = 0;
  int venc_chn = 0;
  const char *streamName = NULL;

  switch (stream_idx) {
    case 0:
      streamName = "MainStream";
      vi_chn = MAIN_STREAM_VI_CHN;
      venc_chn = MAIN_STREAM_VENC_CHN;
      break;
    case 1:
      streamName = "Sub0Stream";
      vi_chn = SUB0_STREAM_VI_CHN;
      venc_chn = SUB0_STREAM_VENC_CHN;
      break;
    case 2:
      streamName = "Sub1Stream";
      vi_chn = SUB1_STREAM_VI_CHN;
      venc_chn = SUB1_STREAM_VENC_CHN;
      break;
    default:
      printf("#Error: %s invalid stream idx:%d\n", __func__, stream_idx);
      return -1;
  }

  printf("#%s end ......................................\n", streamName);

  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = 0;
  stSrcChn.s32ChnId = vi_chn;
  MPP_CHN_S stDestChn;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = venc_chn;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("ERROR: unbind vi[%d] -> venc[%d] failed!\n", vi_chn, venc_chn);
    return -1;
  }

  if (stream_idx == 0) {
    stDestChn.s32ChnId = MAIN_STREAM_JPEG_CHN;
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("ERROR: JPEG: unbind vi[%d] -> venc[%d] failed!\n", vi_chn, MAIN_STREAM_JPEG_CHN);
      return -1;
    }
    RK_MPI_VENC_DestroyChn(MAIN_STREAM_JPEG_CHN);
  }

  RK_MPI_VENC_DestroyChn(venc_chn);
  RK_MPI_VI_DisableChn(0, vi_chn);

  return 0;
}

static char optstr[] = "?:c:s:w:h:a:";

static void print_usage(char *name) {
  printf("#Function description:\n");
  printf("There are three streams in total:MainStream/SubStream0/SubStream.\n"
         "The sub-stream remains unchanged, and the resolution of the main \n"
         "stream is constantly switched.\n");
  printf("  SubStream0: rkispp_scale1: 720x480 NV12 -> /userdata/sub0.h264(150 "
         "frames)\n");
  printf("  SubStream1: rkispp_scale2: 1280x720 NV12 -> "
         "/userdata/sub1.h264(150 frames)\n");
  printf("  MainStream: case1: rkispp_m_bypass: widthxheight FBC0\n");
  printf("                     rkispp_scale0: 1280x720 for luma caculation.\n");
  printf("  MainStream: case2: rkispp_scale0: 1280x720 NV12\n");
  printf("#Usage Example: \n");
  printf("  %s [-c 20] [-s 5] [-w 3840] [-h 2160] [-a the path of iqfiles]\n",
         name);
  printf("  @[-c] Main stream switching times. defalut:20\n");
  printf("  @[-s] The duration of the main stream. default:5\n");
  printf("  @[-w] img width for rkispp_m_bypass. default: 3840\n");
  printf("  @[-h] img height for rkispp_m_bypass. default: 2160\n");
  printf("  @[-a] the path of iqfiles. default: NULL\n");
}

int main(int argc, char *argv[]) {
  int loop_cnt = 20;
  int loop_seconds = 5; // 5s
  int width = 3840;
  int height = 2160;
  char *iq_file_dir = NULL;

  int c = 0;
  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'c':
      loop_cnt = atoi(optarg);
      printf("#IN ARGS: loop_cnt: %d\n", loop_cnt);
      break;
    case 's':
      loop_seconds = atoi(optarg);
      printf("#IN ARGS: loop_seconds: %d\n", loop_seconds);
      break;
    case 'w':
      width = atoi(optarg);
      ;
      printf("#IN ARGS: bypass width: %d\n", width);
      break;
    case 'h':
      height = atoi(optarg);
      ;
      printf("#IN ARGS: bypass height: %d\n", height);
      break;
    case 'a':
      iq_file_dir = optarg;
      printf("#IN ARGS: the path of iqfiles: %s\n", iq_file_dir);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  printf(">>>>>>>>>>>>>>> Test START <<<<<<<<<<<<<<<<<<<<<<\n");
  printf("-->LoopCnt:%d\n", loop_cnt);
  printf("-->LoopSeconds:%d\n", loop_seconds);
  printf("-->BypassWidth:%d\n", width);
  printf("-->BypassHeight:%d\n", height);

  signal(SIGINT, sigterm_handler);
  RK_MPI_SYS_Init();

  int loop = 0;
  while (!quit && (loop < loop_cnt)) {
    g_frame_cnt = 0;
    PrintRss("Before", loop);
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

    sleep(loop_seconds);

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

    PrintRss("After", loop);
    loop++;
    sleep(1);
  }

  printf(">>>>>>>>>>>>>>> Test END <<<<<<<<<<<<<<<<<<<<<<\n");
  return 0;
}
