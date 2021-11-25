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
#include "librtsp/rtsp_demo.h"
#include "rkmedia_api.h"
#include "rkmedia_venc.h"

// static bool g_enable_rtsp = true;
rtsp_demo_handle g_rtsplive = NULL;
rtsp_session_handle g_session;
static bool quit = false;
static bool g_connect = false;
static bool g_venc = false;
static bool g_fec = false;
static RK_S32 s32CamId = 0;
static RK_BOOL bMultictx = RK_FALSE;
static rk_aiq_working_mode_t g_wmode = RK_AIQ_WORKING_MODE_NORMAL;

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = false;
  g_connect = false;
}

void video_packet_cb(MEDIA_BUFFER mb) {
  if (RK_MPI_MB_GetModeID(mb) == RK_ID_VENC) {
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
    rtsp_tx_video(g_session, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb),
                  RK_MPI_MB_GetTimestamp(mb));
    RK_MPI_MB_ReleaseBuffer(mb);
    rtsp_do_event(g_rtsplive);
  } else if (RK_MPI_MB_GetModeID(mb) == RK_ID_VI) {
    printf("Get Video:ptr:%p, fd:%d, size:%zu, mode:%d\n", RK_MPI_MB_GetPtr(mb),
           RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb));
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  g_connect = false;
}

RK_U32 g_width = 1920;
RK_U32 g_height = 1080;
char *g_video_node = "rkispp_scale0";
IMAGE_TYPE_E g_enPixFmt = IMAGE_TYPE_NV12;
RK_S32 g_S32Rotation = 0;

static void StreamOnOff(RK_BOOL start) {
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 1;

  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32DevId = 0;
  stDestChn.s32ChnId = 0;
  if (start) {
    VENC_CHN_ATTR_S venc_chn_attr;
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
    venc_chn_attr.stVencAttr.imageType = g_enPixFmt;
    venc_chn_attr.stVencAttr.u32PicWidth = g_width;
    venc_chn_attr.stVencAttr.u32PicHeight = g_height;
    venc_chn_attr.stVencAttr.u32VirWidth = g_width;
    venc_chn_attr.stVencAttr.u32VirHeight = g_height;
    venc_chn_attr.stVencAttr.u32Profile = 77;
    venc_chn_attr.stVencAttr.enRotation = g_S32Rotation;

    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = g_width * g_height * 30 / 14;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 0;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 0;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;

    VI_CHN_ATTR_S vi_chn_attr;
    vi_chn_attr.pcVideoNode = g_video_node;
    vi_chn_attr.u32BufCnt = 3;
    vi_chn_attr.u32Width = g_width;
    vi_chn_attr.u32Height = g_height;
    vi_chn_attr.enPixFmt = g_enPixFmt;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;

    RK_MPI_VI_SetChnAttr(stSrcChn.s32DevId, stSrcChn.s32ChnId, &vi_chn_attr);
    RK_MPI_VI_EnableChn(stSrcChn.s32DevId, stSrcChn.s32ChnId);
    RK_MPI_VENC_CreateChn(stDestChn.s32ChnId, &venc_chn_attr);
    RK_MPI_SYS_RegisterOutCb(&stDestChn, video_packet_cb);
    RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);

    printf("%s exit!\n", __func__);
  } else {
    RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    RK_MPI_VENC_DestroyChn(stDestChn.s32ChnId);
    RK_MPI_VI_DisableChn(stSrcChn.s32DevId, stSrcChn.s32ChnId);
  }
}

static void StreamOnOffVI(RK_BOOL start) {
  MPP_CHN_S stSrcChn;
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32DevId = s32CamId;
  stSrcChn.s32ChnId = 1;
  if (start) {
    VI_CHN_ATTR_S vi_chn_attr;
    vi_chn_attr.pcVideoNode = g_video_node;
    vi_chn_attr.u32BufCnt = 3;
    vi_chn_attr.u32Width = g_width;
    vi_chn_attr.u32Height = g_height;
    vi_chn_attr.enPixFmt = g_enPixFmt;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;

    RK_MPI_VI_SetChnAttr(stSrcChn.s32DevId, stSrcChn.s32ChnId, &vi_chn_attr);
    RK_MPI_VI_EnableChn(stSrcChn.s32DevId, stSrcChn.s32ChnId);
    RK_MPI_SYS_RegisterOutCb(&stSrcChn, video_packet_cb);
    int ret = RK_MPI_VI_StartStream(stSrcChn.s32DevId, stSrcChn.s32ChnId);
    if (ret) {
      printf("Start Vi failed! ret=%d\n", ret);
    }
  } else {
    RK_MPI_VI_DisableChn(stSrcChn.s32DevId, stSrcChn.s32ChnId);
  }
}

static void streamOn() {
  if (g_venc) {
    StreamOnOff(RK_TRUE);
  } else {
    StreamOnOffVI(RK_TRUE);
  }
}
static void streamOff() {
  if (g_venc) {
    StreamOnOff(RK_FALSE);
  } else {
    StreamOnOffVI(RK_FALSE);
  }
}

static void startISP(rk_aiq_working_mode_t hdr_mode, RK_BOOL fec_enable,
                     char *iq_file_dir) {
  // hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  // fec_enable = RK_FALSE;
  int fps = 30;
  printf("hdr mode %d, fec mode %d, fps %d\n", hdr_mode, fec_enable, fps);
  SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, iq_file_dir);
  SAMPLE_COMM_ISP_SetFecEn(s32CamId, fec_enable);
  SAMPLE_COMM_ISP_Run(s32CamId);
  SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
}

static void testNormalToHdr2(char *iq_file_dir) {
  quit = true;
  RK_U32 count = 0;
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  startISP(hdr_mode, RK_FALSE, iq_file_dir); // isp aiq start before vi streamon
  streamOn();
  while (quit) {
    count++;
    if (hdr_mode == RK_AIQ_WORKING_MODE_NORMAL) {
      hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
      printf("set hdr mode to hdr2 count = %u.\n", count);
    } else {
      hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
      printf("set hdr mode to normal count = %u.\n", count);
    }

    SAMPLE_COMM_ISP_SET_HDR(s32CamId, hdr_mode);
    usleep(100 * 1000); // sleep 30 ms.
  }
  streamOff();
  SAMPLE_COMM_ISP_Stop(s32CamId);
}

static void testNormalToHdr2ToHdr3(char *iq_file_dir) {
  quit = true;
  RK_U32 count = 0;
  while (quit) {
    count++;
    // normal
    printf("######### normal : count = %u #############.\n", count);
    startISP(RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, iq_file_dir);
    streamOn();
    usleep(100 * 10000);

    // hdr2
    printf("############ hdr2 : count = %u ###############.\n", count);
    SAMPLE_COMM_ISP_SET_HDR(s32CamId, RK_AIQ_WORKING_MODE_ISP_HDR2);
    usleep(100 * 10000);

    // hdr3:  go or leave hdr3, must restart aiq
    printf("############ hdr3 : count = %u ##############.\n", count);
    streamOff();
    SAMPLE_COMM_ISP_Stop(s32CamId);
    startISP(RK_AIQ_WORKING_MODE_ISP_HDR3, RK_FALSE, iq_file_dir);
    streamOn();
    usleep(100 * 10000);
    streamOff();
    SAMPLE_COMM_ISP_Stop(s32CamId);
  }
}

static RK_VOID testFBCRotation(char *iq_file_dir) {
  quit = true;
  g_width = 3840;
  g_height = 2160;
  g_video_node = "rkispp_m_bypass";
  g_enPixFmt = IMAGE_TYPE_FBC0;
  RK_U32 count = 0;
  g_S32Rotation = 0;
  while (quit) {
    if (g_S32Rotation > 270) {
      g_S32Rotation = 0;
    } else {
      g_S32Rotation += 90;
    }
    printf("######### %d : count = %u #############.\n", g_S32Rotation,
           count++);
    startISP(RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, iq_file_dir);
    SAMPLE_COMM_ISP_SET_BypassStreamRotation(s32CamId, g_S32Rotation);
    streamOn();
    usleep(5 * 100 * 1000);
    streamOff();
    SAMPLE_COMM_ISP_Stop(s32CamId);
  }
}

static RK_VOID testDefog(char *iq_file_dir) {
  quit = true;
  // g_width = 3840;
  // g_height = 2160;
  // g_video_node = "rkispp_m_bypass";
  // g_enPixFmt = IMAGE_TYPE_FBC0;
  RK_U32 count = 0;
  RK_U32 u32Mode;
  startISP(RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, iq_file_dir);
  streamOn();
  while (quit) {
    printf("######## disable count = %u ############.\n", count++);
    u32Mode = 0;
    SAMPLE_COMM_ISP_SET_DefogEnable(s32CamId, u32Mode);

    printf("######## enable manaul count = %u ############.\n", count);
    u32Mode = 1; // manual
    SAMPLE_COMM_ISP_SET_DefogEnable(s32CamId, u32Mode);

    for (int i = 0; i < 255; i++) {
      SAMPLE_COMM_ISP_SET_DefogStrength(s32CamId, u32Mode, i);
    }

    printf("######## enable auto count = %u ############.\n", count);
    u32Mode = 2;
    SAMPLE_COMM_ISP_SET_DefogStrength(s32CamId, u32Mode, 0);
  }
  SAMPLE_COMM_ISP_Stop(s32CamId);
  streamOff();
}

static RK_VOID testImage(char *iq_file_dir) {
  quit = true;
  // g_width = 3840;
  // g_height = 2160;
  // g_video_node = "rkispp_m_bypass";
  // g_enPixFmt = IMAGE_TYPE_FBC0;
  RK_U32 u32Count = 0;
  startISP(RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, iq_file_dir);
  streamOn();
  while (quit) {
    printf("###########count = %u ###########.\n", u32Count++);
    for (int i = 0; i < 255; i++) {
      SAMPLE_COMM_ISP_SET_Brightness(s32CamId, i);
      SAMPLE_COMM_ISP_SET_Contrast(s32CamId, i);
      SAMPLE_COMM_ISP_SET_Saturation(s32CamId, i);
      SAMPLE_COMM_ISP_SET_Sharpness(s32CamId, i);
    }
  }
  SAMPLE_COMM_ISP_Stop(s32CamId);
  streamOff();
}

static RK_VOID testDayNight(char *iq_file_dir) {
  quit = true;
  RK_U32 u32Count = 0;
  startISP(RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, iq_file_dir);
  streamOn();
  while (quit) {
    // day
    printf("###########day count = %u ###########.\n", u32Count++);
    SAMPLE_COMM_ISP_SET_OpenColorCloseLed(s32CamId);
    SAMPLE_COMM_ISP_SET_HDR(s32CamId, RK_AIQ_WORKING_MODE_ISP_HDR2);
    usleep(2 * 1000 * 1000);
    // night
    printf("###########night count = %u ###########.\n", u32Count);
    SAMPLE_COMM_ISP_SET_HDR(s32CamId, RK_AIQ_WORKING_MODE_NORMAL);
    for (int i = 0; i < 100; i++)
      SAMPLE_COMM_ISP_SET_GrayOpenLed(s32CamId, i);
    usleep(2 * 1000 * 1000);
  }
  SAMPLE_COMM_ISP_Stop(s32CamId);
  streamOff();
}

static RK_VOID testExposure(char *iq_file_dir) {
  quit = true;
  // g_width = 3840;
  // g_height = 2160;
  // g_video_node = "rkispp_m_bypass";
  // g_enPixFmt = IMAGE_TYPE_FBC0;
  RK_U32 u32Count = 0;
  RK_BOOL bIsAutoExposure;
  RK_U32 bIsAGC;
  RK_U32 u32ElectronicShutter;
  RK_U32 u32Agc;
  startISP(RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, iq_file_dir);
  streamOn();
  while (quit) {
    printf("########### auto exposre auto gain: count = %u ###########.\n",
           u32Count++);
    bIsAutoExposure = RK_TRUE;
    u32ElectronicShutter = 0;
    bIsAGC = RK_TRUE;
    u32Agc = 0;
    SAMPLE_COMM_ISP_SET_Exposure(s32CamId, bIsAutoExposure, bIsAGC,
                                 u32ElectronicShutter, u32Agc);
    usleep(100 * 1000);

    printf(
        "########### manaul exposure, manual gain : count = %u ###########.\n",
        u32Count);
    bIsAutoExposure = RK_FALSE;
    u32ElectronicShutter = 0;
    bIsAGC = RK_FALSE;
    u32Agc = 0;
    for (u32ElectronicShutter = 0; u32ElectronicShutter < 17;
         u32ElectronicShutter++)
      for (u32Agc = 0; u32Agc < 255; u32Agc += 10)
        SAMPLE_COMM_ISP_SET_Exposure(s32CamId, bIsAutoExposure, bIsAGC,
                                     u32ElectronicShutter, u32Agc);
    usleep(100 * 1000);

    printf("########### manual exposure, auto gain : count = %u ###########.\n",
           u32Count);
    bIsAutoExposure = RK_FALSE;
    u32ElectronicShutter = 0;
    bIsAGC = RK_TRUE;
    u32Agc = 0;
    for (u32ElectronicShutter = 0; u32ElectronicShutter < 17;
         u32ElectronicShutter++)
      SAMPLE_COMM_ISP_SET_Exposure(s32CamId, bIsAutoExposure, bIsAGC,
                                   u32ElectronicShutter, u32Agc);
    usleep(100 * 1000);
  }
  SAMPLE_COMM_ISP_Stop(s32CamId);
  streamOff();
}

static RK_VOID testCrop(char *iq_file_dir) {
  quit = true;
  RK_U32 u32Count = 0;
  // g_width = 2560;
  // g_height = 1440;
  g_video_node = "rkispp_m_bypass";
  // g_enPixFmt = IMAGE_TYPE_FBC0;
  g_enPixFmt = IMAGE_TYPE_NV12;
  rk_aiq_rect_t rect;
  rect.left = 0;
  rect.top = 0;

  RK_BOOL bCrop = RK_TRUE;
  RK_BOOL bChange = RK_FALSE;
  if (strcmp(iq_file_dir, "400w") == 0) {
    bCrop = RK_FALSE;
    printf("%s: 400w.\n", __func__);
  } else if (strcmp(iq_file_dir, "500w") == 0) {
    bCrop = RK_TRUE;
    printf("%s: 500w.\n", __func__);
  } else {
    bChange = RK_TRUE;
    printf("%s: 500 -> 400w.\n", __func__);
  }

  while (quit) {
    if (bCrop) {
      iq_file_dir = "/userdata/iqfiles/500w/";
      g_width = rect.width = 2592;
      g_height = rect.height = 1944;
    } else {
      iq_file_dir = "/userdata/iqfiles/400w/";
      g_width = rect.width = 2560;
      g_height = rect.height = 1440;
    }
    printf("###########crop count = %u (%u x %u)###########.\n", u32Count++,
           g_width, g_height);
    int fps = 30;
    printf("hdr mode %d, fec mode %d, fps %d\n", g_wmode, g_fec, fps);
    SAMPLE_COMM_ISP_Init(s32CamId, g_wmode, g_fec, iq_file_dir);
    SAMPLE_COMM_ISP_SET_Crop(s32CamId, rect);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);

    streamOn();
    sleep(2);
    g_connect = true;
    while (g_connect) {
      usleep(10 * 1000);
    }
    streamOff();
    SAMPLE_COMM_ISP_Stop(s32CamId);
    if (bChange) {
      if (bCrop) {
        bCrop = RK_FALSE;
      } else {
        bCrop = RK_TRUE;
      }
    }
  }
}

static RK_VOID RKMEDIA_ISP_Usage() {
  printf("\n\n/Usage:./rkmdia_audio index --aiq iqfiles_dir /\n");
  printf("\tindex and its function list below\n");
  printf("\t0:  normal --> hdr2\n");
  printf("\t1:  normal --> hdr2 --> hdr3\n");
  printf("\t2:  FBC rotating 0-->90-->270.\n");
  printf("\t3:  Defog: close --> manual -> auto.\n");
  printf("\t4:  Image test.\n");
  printf("\t5:  DayNight test.\n");
  printf("\t6:  Exposure test.\n");
  printf("\t7:  Crop test.\n");
  printf("\t environment:\n");
  printf("\t VENC=ON: you can play rtsp://xxx.xxx.xxx.xxx/live/main_stream");
  printf("\t HDR=HDR2 or HDR=HDR3 or default normal.\n");
  printf("\t FEC=ON: open fec.\n");
}
static RK_VOID RKMEDIA_ISP_ENV() {
  char *env = NULL;
  env = getenv("FEC");
  if (env != NULL && strcmp(env, "ON") == 0) {
    printf("FEC ON.\n");
    g_fec = RK_TRUE;
  }

  env = getenv("HDR");
  if (env != NULL) {
    if (strcmp(env, "HDR3") == 0) {
      printf("HDR3 ON.\n");
      g_wmode = RK_AIQ_WORKING_MODE_ISP_HDR3;
    } else if (strcmp(env, "HDR2") == 0) {
      printf("HDR2 ON.\n");
      g_wmode = RK_AIQ_WORKING_MODE_ISP_HDR2;
    } else {
      printf("Normal ON.\n");
      g_wmode = RK_AIQ_WORKING_MODE_NORMAL;
    }
  }

  env = getenv("VENC");
  if (env != NULL && strcmp(env, "ON") == 0) {
    printf("VNEC ON.\n");
    g_venc = RK_TRUE;
  }
}
int main(int argc, char *argv[]) {
  signal(SIGINT, sigterm_handler);

  RK_U32 u32Index;

  if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0) {
    RKMEDIA_ISP_Usage();
    return -1;
  }
  char *iq_file_dir = NULL;
  if (argc == 4) {
    if (strcmp(argv[2], "--aiq") == 0) {
      iq_file_dir = argv[3];
    }
    u32Index = atoi(argv[1]);
  } else {
    RKMEDIA_ISP_Usage();
    return -1;
  }
  RKMEDIA_ISP_ENV();

  RK_MPI_SYS_Init();
  // init rtsp
  g_rtsplive = create_rtsp_demo(554);
  g_session = rtsp_new_session(g_rtsplive, "/live/main_stream");
  rtsp_set_video(g_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
  rtsp_sync_video_ts(g_session, rtsp_get_reltime(), rtsp_get_ntptime());

  switch (u32Index) {
  case 0:
    testNormalToHdr2(iq_file_dir);
    break;
  case 1:
    testNormalToHdr2ToHdr3(iq_file_dir);
    break;
  case 2:
    testFBCRotation(iq_file_dir);
    break;
  case 3:
    testDefog(iq_file_dir);
    break;
  case 4:
    testImage(iq_file_dir);
    break;
  case 5:
    testDayNight(iq_file_dir);
    break;
  case 6:
    testExposure(iq_file_dir);
    break;
  case 7:
    testCrop(iq_file_dir);
    break;
  default:
    break;
  }

  // release rtsp
  rtsp_del_demo(g_rtsplive);
  printf("exit program.\n");
  return 0;
}
