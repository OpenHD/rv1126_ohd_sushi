// Copyright 2021 Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <rga/im2d.h>
#include <rga/rga.h>

#ifdef RKAIQ
#include "common/sample_common.h"
#endif

#include "librtsp/rtsp_demo.h"
#include "rkmedia_api.h"
#include "rkmedia_venc.h"
#include "rockx.h"

#include "utilList/rkmedia_list_method.h"

#define RSS_TEST 0
#define MAX_SESSION_NUM 2
#define DRAW_INDEX 0
#define RK_NN_INDEX 1
#define MAX_RKNN_LIST_NUM 10
#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#define UPALIGNTO16(value) UPALIGNTO(value, 16)
#define MAX_DRAW_NUM 50
#define DETECT_RECT_DIFF 25
#define DETECT_ACTIVE_CNT 3

/* flow init struc */
struct Session {
  char path[64];
  CODEC_TYPE_E video_type;
  RK_U32 u32Width;
  RK_U32 u32Height;
  IMAGE_TYPE_E enImageType;
  char videopath[120];

  rtsp_session_handle session;
  MPP_CHN_S stViChn;
  MPP_CHN_S stVenChn;
};

struct demo_cfg {
  int session_count;
  struct Session session_cfg[MAX_SESSION_NUM];
};

/* detect info list for draw osd */
typedef struct person_info_s {
  long timeval;
  rockx_object_array_t person_array;
} person_info_t;

/* detect info list for keep osd */
typedef struct detect_record_s {
  int activate;
  int id;
  int x;
  int y;
  int height;
  int width;
  int top;
  int bottom;
  int left;
  int right;
} detect_record_t;

typedef struct main_arg_s {
  int draw_en;
  int time_log_en;
} main_arg_t;

typedef struct sub_arg_s {
  rockx_module_t data_version;
  int time_log_en;
  char runtime_path[128];
  int ir_en;
  int filter_en;
  int filter_en_raw;
} sub_arg_t;

struct demo_cfg cfg;
static rkmedia_link_list *g_rkmedia_link_list = NULL;
static rockx_config_t *g_rockx_config = NULL;
static rockx_handle_t g_object_det_handle;
static rockx_handle_t g_object_track_handle;
static rtsp_demo_handle g_rtsplive = NULL;
static detect_record_t g_detect_list[MAX_DRAW_NUM];

static int g_flag_run = 1;
static char *g_photo_dirpath = NULL;

/* common method start */
static void sig_proc(int signo) {
  fprintf(stderr, "signal %d\n", signo);
  g_flag_run = 0;
}

static long getCurrentTimeMsec() {
  long msec = 0;
  char str[20] = {0};
  struct timeval stuCurrentTime;

  gettimeofday(&stuCurrentTime, NULL);
  sprintf(str, "%ld%03ld", stuCurrentTime.tv_sec,
          (stuCurrentTime.tv_usec) / 1000);
  for (size_t i = 0; i < strlen(str); i++) {
    msec = msec * 10 + (str[i] - '0');
  }

  return msec;
}

#if RSS_TEST
static void GetRss(char *tag) {
  pid_t id = getpid();
  char cmd[64] = {0};
  sprintf(cmd, "cat /proc/%d/status | grep RSS", id);
  long now_time = getCurrentTimeMsec();
  printf(">>>>[%s] time:%ld\n", tag, now_time);
  system(cmd);
}
#endif
/* common method end */

/* init flow method start */
static void dump_cfg() {
  for (int i = 0; i < cfg.session_count; i++) {
    printf("rtsp path = %s.\n", cfg.session_cfg[i].path);
    printf("video_type = %d.\n", cfg.session_cfg[i].video_type);
    printf("width = %d.\n", cfg.session_cfg[i].u32Width);
    printf("height = %d.\n", cfg.session_cfg[i].u32Height);
    printf("video path =%s.\n", cfg.session_cfg[i].videopath);
    printf("image type = %u.\n", cfg.session_cfg[i].enImageType);
  }
}

static int load_cfg(const char *cfg_file) {
  // cfgline:
  // path=%s video_type=%d width=%u height=%u
  FILE *fp = fopen(cfg_file, "r");
  char line[1024];
  int count = 0;

  if (!fp) {
    fprintf(stderr, "open %s failed\n", cfg_file);
    return -1;
  }

  memset(&cfg, 0, sizeof(cfg));
  while (fgets(line, sizeof(line) - 1, fp)) {
    const char *p;
    // char codec_type[20];
    memset(&cfg.session_cfg[count], 0, sizeof(cfg.session_cfg[count]));

    if (line[0] == '#')
      continue;
    p = strstr(line, "path=");
    if (!p)
      continue;
    if (sscanf(p, "path=%s", cfg.session_cfg[count].path) != 1)
      continue;

    if ((p = strstr(line, "video_type="))) {
      if (sscanf(p,
                 "video_type=%d width=%u height=%u image_type=%u video_path=%s",
                 &cfg.session_cfg[count].video_type,
                 &cfg.session_cfg[count].u32Width,
                 &cfg.session_cfg[count].u32Height,
                 &cfg.session_cfg[count].enImageType,
                 cfg.session_cfg[count].videopath) == 0) {
        printf("parse video file failed %s.\n", p);
      }
    }
    if (cfg.session_cfg[count].video_type != RK_CODEC_TYPE_NONE) {
      count++;
    } else {
      printf("parse line %s failed\n", line);
    }
  }
  cfg.session_count = count;
  fclose(fp);
  dump_cfg();
  return count;
}

static RK_S32 SAMPLE_COMMON_VI_Start(struct Session *session,
                                     VI_CHN_WORK_MODE mode, RK_S32 vi_pipe) {
  RK_S32 ret = 0;
  VI_CHN_ATTR_S vi_chn_attr;

  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = session->u32Width;
  vi_chn_attr.u32Height = session->u32Height;
  vi_chn_attr.enWorkMode = mode;
  vi_chn_attr.pcVideoNode = session->videopath;
  vi_chn_attr.enPixFmt = session->enImageType;

  ret |= RK_MPI_VI_SetChnAttr(vi_pipe, session->stViChn.s32ChnId, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(vi_pipe, session->stViChn.s32ChnId);

  return ret;
}

static RK_S32 SAMPLE_COMMON_VENC_Start(struct Session *session) {
  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = session->video_type;

  venc_chn_attr.stVencAttr.imageType = session->enImageType;

  venc_chn_attr.stVencAttr.u32PicWidth = session->u32Width;
  venc_chn_attr.stVencAttr.u32PicHeight = session->u32Height;
  venc_chn_attr.stVencAttr.u32VirWidth = session->u32Width;
  venc_chn_attr.stVencAttr.u32VirHeight = session->u32Height;
  venc_chn_attr.stVencAttr.u32Profile = 77;

  int env_fps = 30;
  char *fps_c = getenv("FPS");
  if (fps_c) {
    printf("FPS: %s\n", fps_c);
    env_fps = atoi(fps_c);
  }
  printf("env_fps: %d\n", env_fps);
  switch (venc_chn_attr.stVencAttr.enType) {
  case RK_CODEC_TYPE_H264:
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate =
        session->u32Width * session->u32Height * 30 / 14;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;
    break;
  case RK_CODEC_TYPE_H265:
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
    venc_chn_attr.stRcAttr.stH265Vbr.u32Gop = 30;
    venc_chn_attr.stRcAttr.stH265Vbr.u32MaxBitRate =
        session->u32Width * session->u32Height * 30 / 14;
    venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateNum = 30;
    venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateNum = 30;
    break;
  default:
    printf("error: video codec not support.\n");
    break;
  }

  RK_S32 ret =
      RK_MPI_VENC_CreateChn(session->stVenChn.s32ChnId, &venc_chn_attr);
  return ret;
}
/* init flow method end */

/* rockx handle and method start */
static void DataVersionNameGet(char *buf, rockx_module_t data_version) {
  switch (data_version) {
  case ROCKX_MODULE_PERSON_DETECTION_V2:
    sprintf(buf, "ROCKX_MODULE_PERSON_DETECTION_V2");
    break;
  case ROCKX_MODULE_PERSON_DETECTION_V3:
    sprintf(buf, "ROCKX_MODULE_PERSON_DETECTION_V3");
    break;
  default:
    sprintf(buf, "UNKNOWN_DATA");
    break;
  }
}

static RK_S32 ROCKX_HANDLE_INIT(sub_arg_t *init_cfg) {
  if (!init_cfg)
    return -1;

  rockx_ret_t rockx_ret;
  char data_name[64];
  memset(data_name, 0, sizeof(data_name));
  DataVersionNameGet(data_name, init_cfg->data_version);

  // init config
  g_rockx_config = rockx_create_config();
  rockx_add_config(g_rockx_config, ROCKX_CONFIG_RKNN_RUNTIME_PATH,
                   init_cfg->runtime_path);

  // create a object detection handle
  rockx_ret = rockx_create(&g_object_det_handle, init_cfg->data_version,
                           g_rockx_config, sizeof(rockx_config_t));

  if (rockx_ret != ROCKX_RET_SUCCESS) {
    printf("init %s error %d\n", data_name, rockx_ret);
    g_flag_run = 0;
    return -1;
  }
  printf("%s rockx_create success\n", data_name);

  // create a object track handle
  rockx_ret =
      rockx_create(&g_object_track_handle, ROCKX_MODULE_OBJECT_TRACK, NULL, 0);
  if (rockx_ret != ROCKX_RET_SUCCESS) {
    printf("init rockx module ROCKX_MODULE_OBJECT_DETECTION error %d\n",
           rockx_ret);
    g_flag_run = 0;
    return -1;
  }
  printf("init rockx module ROCKX_MODULE_OBJECT_DETECTION successful!\n");

  return 0;
}

static RK_S32 ROCKX_HANDLE_UNINIT() {
  if (g_object_track_handle)
    rockx_destroy(g_object_track_handle);
  if (g_object_det_handle)
    rockx_destroy(g_object_det_handle);
  rockx_release_config(g_rockx_config);
  printf("uninit rockx handle!\n");
  return 0;
}

static RK_S32 ROCKX_DETECT(sub_arg_t *init_cfg, rockx_image_t *img,
                           rockx_object_array_t *out_array) {
  if (!g_flag_run)
    return -1;

  rockx_ret_t rockx_ret;
  rockx_object_array_t detect_array;
  memset(&detect_array, 0, sizeof(detect_array));
  // different method for person_detect
  rockx_ret = rockx_person_detect2(g_object_det_handle, img,
                                  init_cfg->ir_en, &detect_array, NULL);

  if (rockx_ret != ROCKX_RET_SUCCESS) {
    printf("rockx_detect error %d\n", rockx_ret);
    g_flag_run = 0;
    return -1;
  }

  // track
  rockx_ret = rockx_object_track(g_object_track_handle, img->width, img->height,
                                 8, &detect_array, out_array);
  if (rockx_ret != ROCKX_RET_SUCCESS) {
    printf("rockx_object_track error %d\n", rockx_ret);
    g_flag_run = 0;
    return -1;
  }
  // printf("get: %d,track: %d\n", detect_array.count, out_array->count);
  return 0;
}
/* rockx handle and method end */

/* method to keep osd start*/
static void detect_rcd_list_init() {
  memset(&g_detect_list, 0, sizeof(g_detect_list));
  for (int i = 0; i < MAX_DRAW_NUM; i++) {
    g_detect_list[i].id = -1;
  }
}

static void set_detect_rcd_unactivate() {
  for (int i = 0; i < MAX_DRAW_NUM; i++) {
    g_detect_list[i].activate = 0;
  }
}

static int update_board_rcd(int id, int top, int bottom, int left, int right,
                            float x_rate, float y_rate, int w_max, int h_max,
                            RK_BOOL bAllowNew) {
  int empty_order = -1;
  int match_order = -1;
  int ret = 0;
  for (int i = 0; i < MAX_DRAW_NUM; i++) {
    if (g_detect_list[i].id == id) {
      if (g_detect_list[i].x == 0 && g_detect_list[i].y == 0 &&
          g_detect_list[i].width == 0 && g_detect_list[i].height == 0) {
      }
      match_order = i;
      break;
    } else if (bAllowNew && g_detect_list[i].id < 0 && empty_order < 0) {
      empty_order = i;
    }
  }
  if (match_order < 0 && empty_order < 0) {
    if (bAllowNew)
      printf("over max rcd num:%d\n", MAX_DRAW_NUM);
    return -1;
  } else if (match_order < 0) {
    match_order = empty_order;
  }
  g_detect_list[match_order].activate = DETECT_ACTIVE_CNT;
  g_detect_list[match_order].id = id;
  if (abs(top - g_detect_list[match_order].top) > DETECT_RECT_DIFF ||
      abs(bottom - g_detect_list[match_order].bottom) > DETECT_RECT_DIFF ||
      abs(left - g_detect_list[match_order].left) > DETECT_RECT_DIFF ||
      abs(right - g_detect_list[match_order].right) > DETECT_RECT_DIFF) {
    g_detect_list[match_order].top = top;
    g_detect_list[match_order].bottom = bottom;
    g_detect_list[match_order].left = left;
    g_detect_list[match_order].right = right;
    int x = left * x_rate;
    int y = top * y_rate;
    int w = (right - left) * x_rate;
    int h = (bottom - top) * y_rate;
    if (x < 0)
      x = 0;
    if (y < 0)
      y = 0;
    while ((x + w) >= w_max) {
      w -= 16;
    }
    while ((y + h) >= h_max) {
      h -= 16;
    }
    g_detect_list[match_order].width = UPALIGNTO(w, 2);
    g_detect_list[match_order].height = UPALIGNTO(h, 2);
    g_detect_list[match_order].x = UPALIGNTO(x, 2);
    g_detect_list[match_order].y = UPALIGNTO(y, 2);
    ret = 1;
  }
  return ret;
}

static RK_BOOL clear_detect_rcd() {
  RK_BOOL ret = RK_FALSE;
  for (int i = 0; i < MAX_DRAW_NUM; i++) {
    if (g_detect_list[i].id > 0 && g_detect_list[i].activate <= 0) {
      memset(&g_detect_list[i], 0, sizeof(detect_record_t));
      g_detect_list[i].id = -1;
    } else if (g_detect_list[i].id > 0 && g_detect_list[i].activate > 0) {
      ret = RK_TRUE;
    }
  }
  return ret;
}

/* method to keep osd end */
static IM_STATUS rga_nv12_border(rga_buffer_t buf, RK_S32 x, RK_S32 y,
                                 RK_S32 width, RK_S32 height, RK_S32 line_pixel,
                                 RK_S32 color) {
  im_rect rect_up = {x, y, width, line_pixel};
  im_rect rect_buttom = {x, y + height - line_pixel, width, line_pixel};
  im_rect rect_left = {x, y, line_pixel, height};
  im_rect rect_right = {x + width - line_pixel, y, line_pixel, height};
  IM_STATUS STATUS = imfill(buf, rect_up, color);
  STATUS |= imfill(buf, rect_buttom, color);
  STATUS |= imfill(buf, rect_left, color);
  STATUS |= imfill(buf, rect_right, color);
  return STATUS;
}

static void *GetMediaBuffer(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);
  sub_arg_t *in_arg = (sub_arg_t *)arg;
  RK_S32 ret;

  // init handle
  ret = ROCKX_HANDLE_INIT(in_arg);
  if (ret)
    return NULL;

  // create rockx_object_array_t for store result
  rockx_object_array_t out_track_objects;
  memset(&out_track_objects, 0, sizeof(out_track_objects));
  rockx_image_t input_image;
  input_image.height = cfg.session_cfg[RK_NN_INDEX].u32Height;
  input_image.width = cfg.session_cfg[RK_NN_INDEX].u32Width;
  input_image.pixel_format = ROCKX_PIXEL_FORMAT_YUV420SP_NV12;
  input_image.is_prealloc_buf = 1;

  MEDIA_BUFFER buffer = NULL;
  person_info_t person_info = {0};
  long nn_before_time;
  long nn_after_time;
  while (g_flag_run) {
    memset(&out_track_objects, 0, sizeof(out_track_objects));
    buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_VI, RK_NN_INDEX, -1);
    if (!buffer) {
      continue;
    }
    input_image.size = RK_MPI_MB_GetSize(buffer);
    input_image.data = RK_MPI_MB_GetPtr(buffer);
    // detect object
    if (in_arg->time_log_en)
      nn_before_time = getCurrentTimeMsec();

    ret = ROCKX_DETECT(in_arg, &input_image, &out_track_objects);
    if (ret)
      break;

    if (in_arg->time_log_en) {
      nn_after_time = getCurrentTimeMsec();
      printf("Algorithm time-consuming is %ld\n",
             (nn_after_time - nn_before_time));
    }

    // process result
    if (out_track_objects.count > 0) {
      person_info.person_array = out_track_objects;
      person_info.timeval = getCurrentTimeMsec();
      rkmedia_link_list_push(g_rkmedia_link_list, (void *)(&person_info));
      int size = rkmedia_link_list_size(g_rkmedia_link_list);
      if (size >= MAX_RKNN_LIST_NUM)
        rkmedia_link_list_drop(g_rkmedia_link_list);
    }
    RK_MPI_MB_ReleaseBuffer(buffer);
  }
  // release
  ROCKX_HANDLE_UNINIT();
  return NULL;
}

static void *MainStream(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);
  main_arg_t *in_arg = (main_arg_t *)arg;
  MEDIA_BUFFER mb;
  float x_rate = (float)cfg.session_cfg[DRAW_INDEX].u32Width /
                 (float)cfg.session_cfg[RK_NN_INDEX].u32Width;
  float y_rate = (float)cfg.session_cfg[DRAW_INDEX].u32Height /
                 (float)cfg.session_cfg[RK_NN_INDEX].u32Height;
  printf("x_rate is %f, y_rate is %f\n", x_rate, y_rate);
  RK_BOOL need_draw = RK_FALSE;
  person_info_t person_info = {0};
  while (g_flag_run) {
    mb = RK_MPI_SYS_GetMediaBuffer(
        RK_ID_VI, cfg.session_cfg[DRAW_INDEX].stViChn.s32ChnId, -1);
    if (!mb)
      break;
    // draw
    if (in_arg->draw_en && rkmedia_link_list_size(g_rkmedia_link_list)) {
      long draw_before_time_first;
      if (in_arg->time_log_en)
        draw_before_time_first = getCurrentTimeMsec();
      memset(&person_info, 0, sizeof(person_info));
      rkmedia_link_list_pop(g_rkmedia_link_list, (void *)(&person_info));
      if (in_arg->time_log_en)
        printf("time interval is %ld\n", getCurrentTimeMsec() - person_info.timeval);

      set_detect_rcd_unactivate();
      for (int j = 0; j < person_info.person_array.count; j++) {
        float score = person_info.person_array.object[j].score;
        int update_rcd_ret = 0;
        if (score <= 0.0f) {
          update_rcd_ret = update_board_rcd(
              person_info.person_array.object[j].id, person_info.person_array.object[j].box.top,
              person_info.person_array.object[j].box.bottom,
              person_info.person_array.object[j].box.left, person_info.person_array.object[j].box.right,
              x_rate, y_rate, (int)cfg.session_cfg[DRAW_INDEX].u32Width,
              (int)cfg.session_cfg[DRAW_INDEX].u32Height, RK_FALSE);
        } else {
          update_rcd_ret = update_board_rcd(
              person_info.person_array.object[j].id, person_info.person_array.object[j].box.top,
              person_info.person_array.object[j].box.bottom,
              person_info.person_array.object[j].box.left, person_info.person_array.object[j].box.right,
              x_rate, y_rate, (int)cfg.session_cfg[DRAW_INDEX].u32Width,
              (int)cfg.session_cfg[DRAW_INDEX].u32Height, RK_TRUE);
        }
        if (0)
          printf("update_board_rcd: %d\n", update_rcd_ret);
      }
      need_draw = clear_detect_rcd();
      if (in_arg->time_log_en) {
        long draw_after_time_first = getCurrentTimeMsec();
        printf("get first draw rect time-consuming is %ld\n",
               (draw_after_time_first - draw_before_time_first));
      }
    }
    // draw
    if (need_draw) {
      long draw_before_time;
      if (in_arg->time_log_en)
        draw_before_time = getCurrentTimeMsec();
      // RK_MPI_MB_BeginCPUAccess(mb, RK_FALSE);
      rga_buffer_t src = wrapbuffer_fd(
          RK_MPI_MB_GetFD(mb), cfg.session_cfg[DRAW_INDEX].u32Width,
          cfg.session_cfg[DRAW_INDEX].u32Height, RK_FORMAT_YCbCr_420_SP);
      for (int k = 0; k < MAX_DRAW_NUM; k++) {
        if (g_detect_list[k].id >= 0 && g_detect_list[k].activate) {
          rga_nv12_border(src, g_detect_list[k].x, g_detect_list[k].y,
                          g_detect_list[k].width, g_detect_list[k].height, 4,
                          255);
          g_detect_list[k].activate--;
        }
      }
      // RK_MPI_MB_EndCPUAccess(mb, RK_FALSE);
      if (in_arg->time_log_en) {
        long draw_after_time = getCurrentTimeMsec();
        printf("draw rect time-consuming is %ld\n",
               (draw_after_time - draw_before_time));
      }
    }
    // send from VI to VENC
    RK_MPI_SYS_SendMediaBuffer(
        RK_ID_VENC, cfg.session_cfg[DRAW_INDEX].stVenChn.s32ChnId, mb);
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  return NULL;
}

static RK_CHAR optstr[] = "?a::c:v:r:p:t:f:m:I:M:b:g:D:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"data_version", required_argument, NULL, 'v'},
    {"cfg_path", required_argument, NULL, 'c'},
    {"runtime_path", required_argument, NULL, 'r'},
    {"photo_dirpath", required_argument, NULL, 'p'},
    {"time_log", required_argument, NULL, 't'},
    {"fps", required_argument, NULL, 'f'},
    {"hdr_mode", required_argument, NULL, 'm'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"help", no_argument, NULL, '?'},
    {"black_mode", required_argument, NULL, 'b'},
    {"filter", required_argument, NULL, 'g'},
    {"draw_en", required_argument, NULL, 'D'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]] "
         "[-I 0] "
         "[-M 0] "
         "[-v 0] "
         "[-c rtsp-nn.cfg] "
         "[-r librknn_runtime.so] "
         "[-t 0] "
         "[-f 30] "
         "[-m 0] "
         "[-b 0] "
         "[-D 1]"
         "[-s] \n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-M | --multictx: switch of multictx in isp, set 0 to disable, set "
         "1 to enable. Default: 0\n");
#else
  printf("\t%s "
         "[-I 0] "
        //  "[-v 0] "
         "[-c rtsp-nn.cfg] "
         "[-r librknn_runtime.so] "
         "[-t 0] "
         "[-f 30] "
         "[-m 0] "
         "[-b 0] "
         "[-D 1]"
         "\n",
         name);
#endif
//   printf("\t-v | --data_version [0~1]; Default: 0\n"
//          "\t\t-->0: ROCKX_MODULE_PERSON_DETECTION_V2\n"
//          "\t\t-->1: ROCKX_MODULE_PERSON_DETECTION_V3\n"
//          "\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-c | --cfg_path: rtsp cfg path, Default: "
         "\"/oem/usr/share/rtsp-nn.cfg\"\n");
  printf("\t-r | --runtime_path, rknn runtime lib, Default: "
         "\"/usr/lib/librknn_runtime.so\"\n");
  printf(
      "\t-t | --time_log consuming time log. option[0~3]"
      "Default: 0, 0: no log, 1 detect consuming, 2 draw consuming, 3 all\n");
  printf("\t-f | --fps Default:30\n");
  printf("\t-m | --hdr_mode Default:0, set 1 to open hdr, set 0 to close\n");
  printf("\t-b | --black_mode Default:0, set 0 to display rgb, set 1 to "
         "display ir, set -1 to disabled\n");
  printf(
      "\t-D | --draw_en set 1 to enabled draw rect in mainstream, Default 1\n");
  printf(
      "\tonly when ftp_ip, ftp_name, ftp_passwd, ftp_port set, ftp enable\n");
  printf("\trstp: rtsp://<ip>/live/main_stream\n");
}

int main(int argc, char **argv) {
  RK_S32 ret = 0;
  RK_CHAR *pIqfilesPath = NULL;
  RK_CHAR *pCfgPath = "/oem/usr/share/rtsp-nn.cfg";
  main_arg_t main_arg;
  main_arg.time_log_en = 0;
  main_arg.draw_en = 1;
  sub_arg_t sub_arg;
  memset(&sub_arg, 0, sizeof(sub_arg_t));
  sprintf(sub_arg.runtime_path, "/usr/lib/librknn_runtime.so");
  sub_arg.data_version = ROCKX_MODULE_PERSON_DETECTION_V2;
  RK_S32 s32CamId = 0;
  RK_BOOL bMultictx = RK_FALSE;
#ifdef RKAIQ
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  int fps = 30;
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
        pIqfilesPath = (char *)tmp_optarg;
      } else {
        pIqfilesPath = "/oem/etc/iqfiles";
      }
      break;
    case 'b':
      sub_arg.ir_en = atoi(optarg);
      break;
    case 'v':
      switch (atoi(optarg)) {
      case 0:
        sub_arg.data_version = ROCKX_MODULE_PERSON_DETECTION_V2;
        break;
      case 1:
        sub_arg.data_version = ROCKX_MODULE_PERSON_DETECTION_V3;
        break;
      }
      break;
    case 'c':
      pCfgPath = optarg ? optarg : pCfgPath;
      break;
    case 'r':
      memset(&sub_arg.runtime_path, 0, sizeof(sub_arg.runtime_path));
      sprintf(sub_arg.runtime_path, optarg);
      break;
    case 'p':
      g_photo_dirpath = optarg;
      break;
    case 't':
      switch (atoi(optarg)) {
      case 1:
        sub_arg.time_log_en = 1;
        break;
      case 2:
        main_arg.time_log_en = 1;
        break;
      case 3:
        sub_arg.time_log_en = 1;
        main_arg.time_log_en = 1;
        break;
      default:
        break;
      }
      break;
    case 'D':
      main_arg.draw_en = atoi(optarg);
      break;
    case 'I':
      s32CamId = atoi(optarg);
      break;
#ifdef RKAIQ
    case 'f':
      fps = atoi(optarg);
      break;
    case 'm':
      if (atoi(optarg)) {
        hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
      }
      break;

    case 'M':
      if (atoi(optarg)) {
        bMultictx = RK_TRUE;
      }
      break;
#endif
    case 'g':
      sub_arg.filter_en = atoi(optarg);
      sub_arg.filter_en_raw = sub_arg.filter_en;
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf(">>>>>>>>>>>>>>> Test START <<<<<<<<<<<<<<<<<<<<<<\n");
#ifdef RKAIQ
  printf("####Aiq xml dirpath: %s\n\n", pIqfilesPath);
  printf("####hdr mode: %d\n\n", hdr_mode);
  printf("####fps: %d\n\n", fps);
  printf("#####bMultictx: %d\n\n", bMultictx);
  printf("####ir_en: %d\n\n", sub_arg.ir_en);
#endif
  printf("#CameraIdx: %d\n\n", s32CamId);
  printf("####data version: %d\n\n", sub_arg.data_version);
  printf("####config dirpath: %s\n\n", pCfgPath);
  printf("####runtime path: %s\n\n", sub_arg.runtime_path);
  printf("####detect time log en: %d\n\n", sub_arg.time_log_en);
  printf("####draw time log en: %d\n\n", main_arg.time_log_en);
  printf("####draw_en: %d\n\n", main_arg.draw_en);

  signal(SIGINT, sig_proc);
  detect_rcd_list_init();
  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pIqfilesPath);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
    if (sub_arg.ir_en == s32CamId) {
      SAMPLE_COMM_ISP_SET_OpenColorCloseLed(s32CamId);
    } else if (sub_arg.ir_en == 1) {
      SAMPLE_COMM_ISP_SET_GrayOpenLed(s32CamId, 100);
    }
#endif
  }
  load_cfg(pCfgPath);

  // init rtsp
  printf("init rtsp\n");
  g_rtsplive = create_rtsp_demo(554);

  // init mpi
  printf("init mpi\n");
  RK_MPI_SYS_Init();

  // create session
  for (int i = 0; i < cfg.session_count; i++) {
    cfg.session_cfg[i].session =
        rtsp_new_session(g_rtsplive, cfg.session_cfg[i].path);

    // VI create
    printf("VI create\n");
    cfg.session_cfg[i].stViChn.enModId = RK_ID_VI;
    cfg.session_cfg[i].stViChn.s32ChnId = i;
    ret = SAMPLE_COMMON_VI_Start(&cfg.session_cfg[i], VI_WORK_MODE_NORMAL,
                                 s32CamId);
    if (ret) {
      printf("vi start fail, ret is %d, order is %d\n", ret, i);
      return -1;
    }
    // VENC create
    printf("VENC create\n");
    cfg.session_cfg[i].stVenChn.enModId = RK_ID_VENC;
    cfg.session_cfg[i].stVenChn.s32ChnId = i;
    ret = SAMPLE_COMMON_VENC_Start(&cfg.session_cfg[i]);
    if (ret) {
      printf("venc start fail, ret is %d, order is %d\n", ret, i);
      return -1;
    }
    if (i == DRAW_INDEX)
      ret =
          RK_MPI_VI_StartStream(s32CamId, cfg.session_cfg[i].stViChn.s32ChnId);
    else
      ret = RK_MPI_SYS_Bind(&cfg.session_cfg[i].stViChn,
                            &cfg.session_cfg[i].stVenChn);
    if (ret) {
      printf("stream start fail, ret is %d, order is %d\n", ret, i);
      return -1;
    }

    // rtsp video
    printf("rtsp video\n");
    switch (cfg.session_cfg[i].video_type) {
    case RK_CODEC_TYPE_H264:
      ret = rtsp_set_video(cfg.session_cfg[i].session, RTSP_CODEC_ID_VIDEO_H264,
                           NULL, 0);
      break;
    case RK_CODEC_TYPE_H265:
      ret = rtsp_set_video(cfg.session_cfg[i].session, RTSP_CODEC_ID_VIDEO_H265,
                           NULL, 0);
      break;
    default:
      ret = -1;
      printf("video codec not support.\n");
      break;
    }
    if (ret) {
      printf("rtsp_set_video fail, ret is %d, order is %d\n", ret, i);
      return -1;
    }

    ret = rtsp_sync_video_ts(cfg.session_cfg[i].session, rtsp_get_reltime(),
                             rtsp_get_ntptime());
    if (ret) {
      printf("rtsp_sync_video_ts, ret is %d, order is %d\n", ret, i);
      return -1;
    }
  }

  ret = create_rkmedia_link_list(&g_rkmedia_link_list, sizeof(person_info_t));
  if (ret) {
    printf("%s: create_rkmedia_link_list: %d\n", __FUNCTION__, ret);
    g_flag_run = 0;
  }
  // Get the sub-stream buffer for humanoid recognition
  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, &sub_arg);

  // The mainstream draws a box asynchronously based on the recognition result
  pthread_t main_stream_thread;
  pthread_create(&main_stream_thread, NULL, MainStream, &main_arg);

  signal(SIGINT, sig_proc);
#if RSS_TEST
  int64_t loop_cnt = 0;
#endif
  while (g_flag_run) {
    for (int i = 0; i < cfg.session_count; i++) {
      MEDIA_BUFFER buffer;
      // send video buffer
      buffer = RK_MPI_SYS_GetMediaBuffer(
          RK_ID_VENC, cfg.session_cfg[i].stVenChn.s32ChnId, 0);
      if (buffer) {
        rtsp_tx_video(cfg.session_cfg[i].session, RK_MPI_MB_GetPtr(buffer),
                      RK_MPI_MB_GetSize(buffer),
                      RK_MPI_MB_GetTimestamp(buffer));
        RK_MPI_MB_ReleaseBuffer(buffer);
      }
    }
    rtsp_do_event(g_rtsplive);
#if RSS_TEST
    loop_cnt++;
    if (loop_cnt == 10) {
      GetRss(">>>>>>>>>>>>>>mem_test:");
      loop_cnt = 0;
    }
#endif
  }

  rtsp_del_demo(g_rtsplive);
  pthread_join(read_thread, NULL);
  pthread_join(main_stream_thread, NULL);

  for (int i = 0; i < cfg.session_count; i++) {
    if (i != DRAW_INDEX)
      RK_MPI_SYS_UnBind(&cfg.session_cfg[i].stViChn,
                        &cfg.session_cfg[i].stVenChn);
    RK_MPI_VENC_DestroyChn(cfg.session_cfg[i].stVenChn.s32ChnId);
    RK_MPI_VI_DisableChn(s32CamId, cfg.session_cfg[i].stViChn.s32ChnId);
  }

  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }

  destory_rkmedia_link_list(&g_rkmedia_link_list);
  return 0;
}
