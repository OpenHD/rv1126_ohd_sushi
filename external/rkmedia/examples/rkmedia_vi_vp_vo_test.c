// Copyright 2021 Rockchip Electronics Co., Ltd. All rights reserved.
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

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static RK_CHAR optstr[] = "?::a::I:M:i:m:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"input", required_argument, NULL, 'i'},
    {"media", required_argument, NULL, 'm'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]]"
         "[-I 0] "
         "[-M 0] "
         "[-i 45] "
         "[-m 5] "
         "\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
  printf("\t-i | --input: intput video of ISPP. Default: 45\n");
  printf("\t-m | --media: media of ISPP. Default: 5\n");
#else
  printf("\t%s [-I 0]\n", name);
#endif
  printf("\t-I | --camid: camera ctx id, Default 0\n");
}

static void *GetMediaBuffer(void *arg) {
  int chn = (int)arg;

  MEDIA_BUFFER mb = NULL;
  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VI, chn, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }

    MB_IMAGE_INFO_S stImageInfo = {0};
    int ret = RK_MPI_MB_GetImageInfo(mb, &stImageInfo);
    if (ret)
      printf("Warn: Get image info failed! ret = %d\n", ret);

    printf("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",
           RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb), stImageInfo.u32Width,
           stImageInfo.u32Height, stImageInfo.enImgType);

    RK_MPI_MB_ReleaseBuffer(mb);
  }

  pthread_detach(pthread_self());

  return NULL;
}

/*
 *                    -> VI2
 * VI0 -> VP0 -- ISPP -> VI3
 *                    -> VI4
 *                    -> VI5
 * How to use ISPP?
 * 1. add code to kernel dts:
 *    &rkispp_vir2 {
 *           status = "okay";
 *    };
 * 2. check new /dev/media*, as is my case: /dev/media5
 * 3. media-ctl -p -d /dev/media5
 *    check rkispp_input_image for VP0: as is my case: /dev/video45
 *    check rkispp_m_bypass for VI2: as is my case: /dev/video46
 *    check rkispp_scale0 for VI3: as is my case: /dev/video47
 *    check rkispp_scale1 for VI4: as is my case: /dev/video48
 *    check rkispp_scale2 for VI5: as is my case: /dev/video49
 * 4. config:
 *    media-ctl -d /dev/media5 -l '"rkispp_input_image":0->"rkispp-subdev":0[1]'
 *    media-ctl -d /dev/media5 --set-v4l2 '"rkispp-subdev":0[fmt:YUYV8_2X8/1920x1080]'
 *    media-ctl -d /dev/media5 --set-v4l2 '"rkispp-subdev":2[fmt:YUYV8_2X8/1920x1080]'
 * 5. enable VI2, VI3, VI4, VI5(one or more) before VP0
 */

int main(int argc, char *argv[]) {
  int ret = 0;
  int disp_width = 640;
  int disp_height = 360;
  int video_width = 1920;
  int video_height = 1080;
  int video1_width = 1920;
  int video1_height = 1080;
  int video2_width = disp_width;
  int video2_height = disp_height;
  int video3_width = 640;
  int video3_height = 360;
  int video4_width = 640;
  int video4_height = 480;
  RK_S32 s32CamId = 0;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
#endif
  int media_id = 5;
  int input_id = 45;
  char media[128];
  char video_input[128];
  char video_output1[128];
  char video_output2[128];
  char video_output3[128];
  char video_output4[128];
  int c;
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
        iq_file_dir = "/oem/etc/iqfiles";
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
    case 'i':
      input_id = atoi(optarg);
      break;
    case 'm':
      media_id = atoi(optarg);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  snprintf(media, sizeof(media), "/dev/media%d", media_id);
  snprintf(video_input, sizeof(video_input), "/dev/video%d", input_id);
  snprintf(video_output1, sizeof(video_output1), "/dev/video%d", input_id + 1);
  snprintf(video_output2, sizeof(video_output2), "/dev/video%d", input_id + 2);
  snprintf(video_output3, sizeof(video_output3), "/dev/video%d", input_id + 3);
  snprintf(video_output4, sizeof(video_output4), "/dev/video%d", input_id + 4);

  printf("media: %s, input: %s, output1: %s, output2: %s, output3: %s, "
         "output4: %s\n",
         media, video_input, video_output1, video_output2, video_output3,
         video_output4);

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

  char cmd[256];
  snprintf(
      cmd, sizeof(cmd),
      "media-ctl -d %s -l '\"rkispp_input_image\":0->\"rkispp-subdev\":0[1]'",
      media);
  system(cmd);
  snprintf(cmd, sizeof(cmd),
           "media-ctl -d %s --set-v4l2 "
           "'\"rkispp-subdev\":0[fmt:YUYV8_2X8/1920x1080]'",
           media);
  system(cmd);
  snprintf(cmd, sizeof(cmd),
           "media-ctl -d %s --set-v4l2 "
           "'\"rkispp-subdev\":2[fmt:YUYV8_2X8/1920x1080]'",
           media);
  system(cmd);

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
    printf("Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }

  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = video_output1;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video1_width;
  vi_chn_attr.u32Height = video1_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(2, 2, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(2, 2);
  if (ret) {
    printf("Create vi[2] failed! ret=%d\n", ret);
    return -1;
  }

  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = video_output2;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video2_width;
  vi_chn_attr.u32Height = video2_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(3, 3, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(3, 3);
  if (ret) {
    printf("Create vi[3] failed! ret=%d\n", ret);
    return -1;
  }

  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = video_output3;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video3_width;
  vi_chn_attr.u32Height = video3_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(4, 4, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(4, 4);
  if (ret) {
    printf("Create vi[4] failed! ret=%d\n", ret);
    return -1;
  }

  memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  vi_chn_attr.pcVideoNode = video_output4;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video4_width;
  vi_chn_attr.u32Height = video4_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(5, 5, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(5, 5);
  if (ret) {
    printf("Create vi[5] failed! ret=%d\n", ret);
    return -1;
  }

  VP_CHN_ATTR_S vp_chn_attr;
  vp_chn_attr.pcVideoNode = video_input;
  vp_chn_attr.u32BufCnt = 3;
  vp_chn_attr.u32Width = video_width;
  vp_chn_attr.u32Height = video_height;
  vp_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vp_chn_attr.enWorkMode = VP_WORK_MODE_NORMAL;
  ret = RK_MPI_VP_SetChnAttr(0, 0, &vp_chn_attr);
  ret |= RK_MPI_VP_EnableChn(0, 0);
  if (ret) {
    printf("Create vp[0] failed! ret=%d\n", ret);
    return -1;
  }

  VO_CHN_ATTR_S stVoAttr = {0};
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
  stVoAttr.enImgType = IMAGE_TYPE_NV12;
  stVoAttr.u16Zpos = 0;
  stVoAttr.stImgRect.s32X = 0;
  stVoAttr.stImgRect.s32Y = 0;
  stVoAttr.stImgRect.u32Width = disp_width;
  stVoAttr.stImgRect.u32Height = disp_height;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = disp_width;
  stVoAttr.stDispRect.u32Height = disp_height;
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn = {0};
  MPP_CHN_S stDestChn = {0};

  printf("#Bind VI[0] to VP[0]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VP;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to vp[0] failed! ret=%d\n", ret);
    return -1;
  }

  printf("#Bind VI[3] to VO[0]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 3;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[3] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  pthread_t th2, th4, th5;
  int chn = 2;
  pthread_create(&th2, NULL, GetMediaBuffer, (void *)chn);
  ret = RK_MPI_VI_StartStream(2, 2);
  if (ret) {
    printf("Start VI[2] failed! ret=%d\n", ret);
    return -1;
  }
  chn = 4;
  pthread_create(&th4, NULL, GetMediaBuffer, (void *)chn);
  ret = RK_MPI_VI_StartStream(4, 4);
  if (ret) {
    printf("Start VI[4] failed! ret=%d\n", ret);
    return -1;
  }
  chn = 5;
  pthread_create(&th5, NULL, GetMediaBuffer, (void *)chn);
  ret = RK_MPI_VI_StartStream(5, 5);
  if (ret) {
    printf("Start VI[5] failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 3;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[3] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VP;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[0] to vp[0] failed! ret=%d\n", ret);
    return -1;
  }

  RK_MPI_VO_DestroyChn(0);
  RK_MPI_VI_DisableChn(2, 2);
  RK_MPI_VI_DisableChn(3, 3);
  RK_MPI_VI_DisableChn(4, 4);
  RK_MPI_VI_DisableChn(5, 5);
  RK_MPI_VP_DisableChn(0, 0);
  RK_MPI_VI_DisableChn(s32CamId, 0);

  if (iq_file_dir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }
  return 0;
}
