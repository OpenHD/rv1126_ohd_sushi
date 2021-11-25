// Copyright 2021 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "json-c/json.h"
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

#include "dbus_signal.h"
#include "ispserver.h"
#include "libipcpro_log_control.h"
#include "rkmedia_api.h"

static VI_CHN_ATTR_S vi_chn_attr;
static RGA_ATTR_S stRgaAttr;
static MPP_CHN_S stSrcChn;
static MPP_CHN_S stDestChn;
static VO_CHN_ATTR_S stVoAttr;
static RK_S32 s32CamId = 0;
static pthread_mutex_t stream_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool stream_flag = false;
static bool quit = false;
static bool listener_flag = false;

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static int display_stream_on() {
  pthread_mutex_lock(&stream_mutex);
  if (stream_flag) {
    pthread_mutex_unlock(&stream_mutex);
    return 0;
  }
  stream_flag = true;
  int ret = 0;
  RK_MPI_SYS_Init();
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("Create vi[0] failed! ret=%d\n", ret);
    pthread_mutex_unlock(&stream_mutex);
    return -1;
  }

  // rga0 for primary plane
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
    printf("Create rga[0] falied! ret=%d\n", ret);
    pthread_mutex_unlock(&stream_mutex);
    return -1;
  }

  // VO[0] for overlay plane, Only overlay plane support scale.
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
    pthread_mutex_unlock(&stream_mutex);
    return -1;
  }

  printf("#Bind VI[0] to RGA[0]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stDestChn.enModId = RK_ID_RGA;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
    pthread_mutex_unlock(&stream_mutex);
    return -1;
  }

  printf("#Bind VI[0] to VO[0]....\n");
  stSrcChn.enModId = RK_ID_RGA;
  stDestChn.enModId = RK_ID_VO;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to vo[0] failed! ret=%d\n", ret);
    pthread_mutex_unlock(&stream_mutex);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  pthread_mutex_unlock(&stream_mutex);
  return ret;
}

static int display_stream_off() {
  pthread_mutex_lock(&stream_mutex);
  if (!stream_flag) {
    pthread_mutex_unlock(&stream_mutex);
    return 0;
  }
  stream_flag = false;
  int ret = 0;
  printf("%s exit!\n", __func__);
  stSrcChn.enModId = RK_ID_VI;
  stDestChn.enModId = RK_ID_RGA;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[0] to rga[0] failed! ret=%d\n", ret);
    pthread_mutex_unlock(&stream_mutex);
    return -1;
  }

  stSrcChn.enModId = RK_ID_RGA;
  stDestChn.enModId = RK_ID_VO;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[0] to vo[0] failed! ret=%d\n", ret);
    pthread_mutex_unlock(&stream_mutex);
    return -1;
  }

  RK_MPI_VO_DestroyChn(0);
  RK_MPI_RGA_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 0);
  pthread_mutex_unlock(&stream_mutex);
  return ret;
}

void ispserver_listener_thread(void *user_data) {
  char *json_str = (char *)user_data;
  printf("ispserver_listener_thread, json is %s\n", json_str);
  int ret = 0;
  json_object *j_cfg;
  json_object *j_data;
  if (!json_str)
    return;
  j_cfg = json_tokener_parse(json_str);
  if (j_cfg) {
    j_data = json_object_object_get(j_cfg, "status");
    if (j_data) {
      int status = (int)json_object_get_int(j_data);
      switch (status) {
      case 0:
        ret = display_stream_off();
        break;
      case 1:
        ret = display_stream_on();
        break;
      case 2:
        ret = display_stream_off();
        break;
      }
      if (ret) {
        printf("%s: get signal(%d), action fail, exit func\n", __FUNCTION__,
               status);
        quit = true;
      }
    }
  }
  json_object_put(j_cfg);
}

static void ispserver_register() {
  if (listener_flag)
    return;
  listener_flag = true;
  printf("[%s] attemp dbus of ispserver start\n", __FUNCTION__);
  bool log_status = true;
  while (!ispserver_expo_info_get()) {
    if (log_status) {
      log_status = false;
      IPCProtocol_log_en_set(false);
    }
    if (quit)
      return;
    usleep(100000);
  }
  IPCProtocol_log_en_set(true);
  printf("[%s] attemp dbus of ispserver success\n", __FUNCTION__);
  dbus_monitor_signal_registered(ISPSERVER_INTERFACE,
                                 NS_SIGNAL_ISPSTATUSCHANGED,
                                 &ispserver_listener_thread);
}

static void ispserver_unregister() {
  if (!listener_flag)
    return;
  listener_flag = false;
  dbus_monitor_signal_unregistered(ISPSERVER_INTERFACE,
                                   NS_SIGNAL_ISPSTATUSCHANGED,
                                   &ispserver_listener_thread);
}

static RK_CHAR optstr[] = "?::I:w:h:W:H:d:";
static const struct option long_options[] = {
    {"camid", required_argument, NULL, 'I'},
    {"vi_height", required_argument, NULL, 'H'},
    {"vi_width", required_argument, NULL, 'W'},
    {"display_height", required_argument, NULL, 'h'},
    {"display_width", required_argument, NULL, 'w'},
    {"device_name", required_argument, NULL, 'd'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-I 0]"
         "[-W 1280] "
         "[-H 720] "
         "[-w 720] "
         "[-h 1280] "
         "[-d rkispp_scale2]"
         "\n",
         name);
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-W | --vi_width: VI width, Default:1280\n");
  printf("\t-H | --vi_height: VI height, Default:720\n");
  printf("\t-w | --display_width: crop_width, Default:720\n");
  printf("\t-h | --display_height: crop_height, Default:1280\n");
  printf("\t-d  | --device_name set pcDeviceName, Default:rkispp_scale2\n");
}

int main(int argc, char *argv[]) {
  int c;
  int video_width = 1280;
  int video_height = 720;
  int disp_width = 720;
  int disp_height = 1280;
  char *video_node = "rkispp_scale2";
  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    switch (c) {
    case 'I':
      s32CamId = atoi(optarg);
      break;
    case 'w':
      disp_width = atoi(optarg);
      break;
    case 'h':
      disp_height = atoi(optarg);
      break;
    case 'W':
      video_width = atoi(optarg);
      break;
    case 'H':
      video_height = atoi(optarg);
      break;
    case 'd':
      video_node = optarg;
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#video_node: %s\n\n", video_node);
  printf("#CameraIdx: %d\n\n", s32CamId);
  printf("#video_width: %d\n\n", video_width);
  printf("#video_height: %d\n\n", video_height);
  printf("#disp_width: %d\n\n", disp_width);
  printf("#disp_height: %d\n\n", disp_height);

  // para init
  memset(&vi_chn_attr, 0, sizeof(VI_CHN_ATTR_S));
  vi_chn_attr.pcVideoNode = video_node;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;

  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 2;
  stRgaAttr.u16Rotaion = 90;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_height;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgOut.u32Width = disp_width;
  stRgaAttr.stImgOut.u32Height = disp_height;
  stRgaAttr.stImgOut.u32HorStride = disp_width;
  stRgaAttr.stImgOut.u32VirStride = disp_height;

  memset(&stSrcChn, 0, sizeof(MPP_CHN_S));
  memset(&stDestChn, 0, sizeof(MPP_CHN_S));

  memset(&stVoAttr, 0, sizeof(VO_CHN_ATTR_S));
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

  signal(SIGINT, sigterm_handler);
  int ret = display_stream_on();
  if (ret) {
    printf("%s: display_stream_on fail\n", __FUNCTION__);
    return -1;
  }
  ispserver_register();

  while (!quit) {
    usleep(500000);
  }

  if (stream_flag) {
    display_stream_off();
  }

  ispserver_unregister();
  return 0;
}
