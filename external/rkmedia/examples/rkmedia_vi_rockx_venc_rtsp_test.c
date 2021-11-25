// Copyright 2020 Rockchip Electronics Co., Ltd. All rights reserved.
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

#define MAX_SESSION_NUM 2
#define DRAW_INDEX 0
#define RK_NN_INDEX 1
#define MAX_RKNN_LIST_NUM 10
#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#define UPALIGNTO16(value) UPALIGNTO(value, 16)
#define DETECT_RCD_NUM 64
#define DETECT_RECT_DIFF 25
#define DETECT_ACTIVE_CNT 5

typedef struct node {
  long timeval;
  rockx_object_array_t person_array;
  struct node *next;
} Node;

typedef struct my_stack {
  int size;
  Node *top;
} rknn_list;

typedef struct detect_record_s {
  int used;
  int activate;
  int id;
  int x;
  int y;
  int height;
  int width;
} detect_record_t;

static detect_record_t g_detect_list[DETECT_RCD_NUM];

void create_rknn_list(rknn_list **s) {
  if (*s != NULL)
    return;
  *s = (rknn_list *)malloc(sizeof(rknn_list));
  (*s)->top = NULL;
  (*s)->size = 0;
  printf("create rknn_list success\n");
}

void destory_rknn_list(rknn_list **s) {
  Node *t = NULL;
  if (*s == NULL)
    return;
  while ((*s)->top) {
    t = (*s)->top;
    (*s)->top = t->next;
    free(t);
  }
  free(*s);
  *s = NULL;
}

void rknn_list_push(rknn_list *s, long timeval,
                    rockx_object_array_t person_array) {
  Node *t = NULL;
  t = (Node *)malloc(sizeof(Node));
  t->timeval = timeval;
  t->person_array = person_array;
  if (s->top == NULL) {
    s->top = t;
    t->next = NULL;
  } else {
    t->next = s->top;
    s->top = t;
  }
  s->size++;
}

void rknn_list_pop(rknn_list *s, long *timeval,
                   rockx_object_array_t *person_array) {
  Node *t = NULL;
  if (s == NULL || s->top == NULL)
    return;
  t = s->top;
  *timeval = t->timeval;
  *person_array = t->person_array;
  s->top = t->next;
  free(t);
  s->size--;
}

void rknn_list_drop(rknn_list *s) {
  Node *t = NULL;
  if (s == NULL || s->top == NULL)
    return;
  t = s->top;
  s->top = t->next;
  free(t);
  s->size--;
}

int rknn_list_size(rknn_list *s) {
  if (s == NULL)
    return -1;
  return s->size;
}

rtsp_demo_handle g_rtsplive = NULL;

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

typedef struct main_arg_s {
  int time_log_en;
  float face_score;
} main_arg_t;

typedef struct sub_arg_s {
  float face_score;
  rockx_module_t data_version;
  int time_log_en;
  char runtime_path[128];
} sub_arg_t;

struct demo_cfg cfg;
rknn_list *rknn_list_;

static int g_flag_run = 1;
static char *g_photo_dirpath = "/tmp/";

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

int nv12_border(char *pic, int pic_w, int pic_h, int rect_x, int rect_y,
                int rect_w, int rect_h, int R, int G, int B) {
  /* Set up the rectangle border size */
  const int border = 5;
  /* RGB convert YUV */
  int Y, U, V;
  Y = 0.299 * R + 0.587 * G + 0.114 * B;
  U = -0.1687 * R + 0.3313 * G + 0.5 * B + 128;
  V = 0.5 * R - 0.4187 * G - 0.0813 * B + 128;
  /* Locking the scope of rectangle border range */
  int j, k;
  for (j = rect_y; j < rect_y + rect_h; j++) {
    for (k = rect_x; k < rect_x + rect_w; k++) {
      if (k < (rect_x + border) || k > (rect_x + rect_w - border) ||
          j < (rect_y + border) || j > (rect_y + rect_h - border)) {
        /* Components of YUV's storage address index */
        int y_index = j * pic_w + k;
        int u_index =
            (y_index / 2 - pic_w / 2 * ((j + 1) / 2)) * 2 + pic_w * pic_h;
        int v_index = u_index + 1;
        /* set up YUV's conponents value of rectangle border */
        pic[y_index] = Y;
        pic[u_index] = U;
        pic[v_index] = V;
      }
    }
  }

  return 0;
}

static void DataVersionNameGet(char *buf, rockx_module_t data_version) {
  switch (data_version) {
  case ROCKX_MODULE_PERSON_DETECTION_V2:
    sprintf(buf, "ROCKX_MODULE_PERSON_DETECTION_V2");
    break;
  case ROCKX_MODULE_PERSON_DETECTION_V3:
    sprintf(buf, "ROCKX_MODULE_PERSON_DETECTION_V3");
    break;
  case ROCKX_MODULE_FACE_DETECTION_V2:
    sprintf(buf, "ROCKX_MODULE_FACE_DETECTION_V2");
    break;
  case ROCKX_MODULE_FACE_DETECTION_V3_LARGE:
    sprintf(buf, "ROCKX_MODULE_FACE_DETECTION_V3_LARGE");
    break;
  default:
    sprintf(buf, "ROCKX_MODULE_PERSON_DETECTION_V2");
    break;
  }
}

static void *GetMediaBuffer(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);
  sub_arg_t *in_arg = (sub_arg_t *)arg;
  char data_name[64];
  memset(data_name, 0, sizeof(data_name));
  DataVersionNameGet(data_name, in_arg->data_version);
  // rockx
  rockx_ret_t rockx_ret;
  rockx_handle_t object_det_handle;
  rockx_image_t input_image;
  input_image.height = cfg.session_cfg[RK_NN_INDEX].u32Height;
  input_image.width = cfg.session_cfg[RK_NN_INDEX].u32Width;
  input_image.pixel_format = ROCKX_PIXEL_FORMAT_YUV420SP_NV12;
  input_image.is_prealloc_buf = 1;
  // create a object detection handle
  rockx_config_t *config = rockx_create_config();
  rockx_add_config(config, ROCKX_CONFIG_RKNN_RUNTIME_PATH,
                   in_arg->runtime_path);
  rockx_ret = rockx_create(&object_det_handle, in_arg->data_version, config,
                           sizeof(rockx_config_t));
  if (rockx_ret != ROCKX_RET_SUCCESS) {
    printf("init %s error %d\n", data_name, rockx_ret);
    g_flag_run = 0;
    return NULL;
  }
  printf("%s rockx_create success\n", data_name);
  // create rockx_object_array_t for store result
  rockx_object_array_t person_array;
  memset(&person_array, 0, sizeof(person_array));

  // create a object track handle
  rockx_handle_t object_track_handle;
  rockx_ret =
      rockx_create(&object_track_handle, ROCKX_MODULE_OBJECT_TRACK, NULL, 0);
  if (rockx_ret != ROCKX_RET_SUCCESS) {
    printf("init rockx module ROCKX_MODULE_OBJECT_DETECTION error %d\n",
           rockx_ret);
    g_flag_run = 0;
    return NULL;
  }

  MEDIA_BUFFER buffer = NULL;
  while (g_flag_run) {
    buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_VI, RK_NN_INDEX, -1);
    if (!buffer) {
      continue;
    }
    // printf("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
    //        "timestamp:%lld\n",
    //        RK_MPI_MB_GetPtr(buffer), RK_MPI_MB_GetFD(buffer),
    //        RK_MPI_MB_GetSize(buffer),
    //        RK_MPI_MB_GetModeID(buffer), RK_MPI_MB_GetChannelID(buffer),
    //        RK_MPI_MB_GetTimestamp(buffer));
    input_image.size = RK_MPI_MB_GetSize(buffer);
    input_image.data = RK_MPI_MB_GetPtr(buffer);
    // detect object
    long nn_before_time;
    long nn_after_time;
    if (in_arg->time_log_en)
      nn_before_time = getCurrentTimeMsec();
    // different method for person_detect and face_detect
    if (in_arg->data_version == ROCKX_MODULE_PERSON_DETECTION_V2 ||
        in_arg->data_version == ROCKX_MODULE_PERSON_DETECTION_V3) {
      rockx_ret = rockx_person_detect(object_det_handle, &input_image,
                                      &person_array, NULL);
    } else {
      rockx_ret = rockx_face_detect(object_det_handle, &input_image,
                                    &person_array, NULL);
    }
    if (rockx_ret != ROCKX_RET_SUCCESS) {
      printf("rockx_person_detect error %d\n", rockx_ret);
      g_flag_run = 0;
    }
    if (in_arg->time_log_en) {
      nn_after_time = getCurrentTimeMsec();
      printf("Algorithm time-consuming is %ld\n",
             (nn_after_time - nn_before_time));
    }
    // printf("person_array.count is %d\n", person_array.count);
    rockx_object_array_t out_track_objects;
    rockx_ret = rockx_object_track(object_track_handle, input_image.width,
                                   input_image.height, 10, &person_array,
                                   &out_track_objects);
    if (rockx_ret != ROCKX_RET_SUCCESS) {
      printf("rockx_object_track error %d\n", rockx_ret);
      g_flag_run = 0;
    }

    // process result
    if (out_track_objects.count > 0) {
      // for (int k = 0; k < person_array.count; k++) {
      //   printf("## order: %d, p: %d, o: %d\n", k, person_array.object[k].id,
      //   out_track_objects.object[k].id);
      // }
      rknn_list_push(rknn_list_, getCurrentTimeMsec(), out_track_objects);
      int size = rknn_list_size(rknn_list_);
      if (size >= MAX_RKNN_LIST_NUM)
        rknn_list_drop(rknn_list_);
      // printf("size is %d\n", size);
    }
    RK_MPI_MB_ReleaseBuffer(buffer);
  }
  // release
  rockx_image_release(&input_image);
  rockx_destroy(object_track_handle);
  rockx_destroy(object_det_handle);
  rockx_release_config(config);
  return NULL;
}

static void *GetMediaBufferStress(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);
  sub_arg_t *in_arg = (sub_arg_t *)arg;
  char data_name[64];
  memset(data_name, 0, sizeof(data_name));
  DataVersionNameGet(data_name, in_arg->data_version);
  while (g_flag_run) {
    // rockx
    rockx_ret_t rockx_ret;
    rockx_handle_t object_det_handle;
    rockx_image_t input_image;
    input_image.height = cfg.session_cfg[RK_NN_INDEX].u32Height;
    input_image.width = cfg.session_cfg[RK_NN_INDEX].u32Width;
    input_image.pixel_format = ROCKX_PIXEL_FORMAT_YUV420SP_NV12;
    input_image.is_prealloc_buf = 1;
    // create a object detection handle
    rockx_config_t *config = rockx_create_config();
    rockx_add_config(config, ROCKX_CONFIG_RKNN_RUNTIME_PATH,
                     in_arg->runtime_path);
    rockx_ret = rockx_create(&object_det_handle, in_arg->data_version, config,
                             sizeof(rockx_config_t));
    if (rockx_ret != ROCKX_RET_SUCCESS) {
      printf("init rockx module %s error %d\n", data_name, rockx_ret);
      return NULL;
    }
    printf("%s rockx_create success\n", data_name);
    // create rockx_object_array_t for store result
    rockx_object_array_t person_array;
    memset(&person_array, 0, sizeof(person_array));

    // create a object track handle
    rockx_handle_t object_track_handle;
    rockx_ret =
        rockx_create(&object_track_handle, ROCKX_MODULE_OBJECT_TRACK, NULL, 0);
    if (rockx_ret != ROCKX_RET_SUCCESS) {
      printf("init rockx module ROCKX_MODULE_OBJECT_DETECTION error %d\n",
             rockx_ret);
    }

    MEDIA_BUFFER buffer = NULL;
    buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_VI, RK_NN_INDEX, -1);
    if (!buffer) {
      continue;
    }

    // printf("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
    //        "timestamp:%lld\n",
    //        RK_MPI_MB_GetPtr(buffer), RK_MPI_MB_GetFD(buffer),
    //        RK_MPI_MB_GetSize(buffer),
    //        RK_MPI_MB_GetModeID(buffer), RK_MPI_MB_GetChannelID(buffer),
    //        RK_MPI_MB_GetTimestamp(buffer));
    input_image.size = RK_MPI_MB_GetSize(buffer);
    input_image.data = RK_MPI_MB_GetPtr(buffer);
    // detect object
    long nn_before_time;
    long nn_after_time;
    if (in_arg->time_log_en)
      nn_before_time = getCurrentTimeMsec();
    // different method for person_detect and face_detect
    if (in_arg->data_version == ROCKX_MODULE_PERSON_DETECTION_V2 ||
        in_arg->data_version == ROCKX_MODULE_PERSON_DETECTION_V3) {
      rockx_ret = rockx_person_detect(object_det_handle, &input_image,
                                      &person_array, NULL);
    } else {
      rockx_ret = rockx_face_detect(object_det_handle, &input_image,
                                    &person_array, NULL);
    }
    if (rockx_ret != ROCKX_RET_SUCCESS) {
      printf("rockx_detect error %d\n", rockx_ret);
      g_flag_run = 0;
    }
    if (in_arg->time_log_en) {
      nn_after_time = getCurrentTimeMsec();
      printf("Algorithm time-consuming is %ld\n",
             (nn_after_time - nn_before_time));
    }
    // printf("person_array.count is %d\n", person_array.count);

    rockx_object_array_t out_track_objects;
    rockx_ret = rockx_object_track(object_track_handle, input_image.width,
                                   input_image.height, 10, &person_array,
                                   &out_track_objects);
    if (rockx_ret != ROCKX_RET_SUCCESS) {
      printf("rockx_object_track error %d\n", rockx_ret);
      g_flag_run = 0;
    }
    // process result
    if (out_track_objects.count > 0) {
      rknn_list_push(rknn_list_, getCurrentTimeMsec(), out_track_objects);
      int size = rknn_list_size(rknn_list_);
      if (size >= MAX_RKNN_LIST_NUM)
        rknn_list_drop(rknn_list_);
      // printf("size is %d\n", size);
    }
    RK_MPI_MB_ReleaseBuffer(buffer);
    // release
    rockx_image_release(&input_image);
    rockx_destroy(object_track_handle);
    rockx_destroy(object_det_handle);
    rockx_release_config(config);
    printf("destroy %s success\n", data_name);
  }

  return NULL;
}

void video_packet_cb(MEDIA_BUFFER snap_mb) {
  static int jpeg_id = 0;
  char snap_path[256];
  if (g_photo_dirpath[strlen(g_photo_dirpath) - 1] == '/') {
    sprintf(snap_path, "%sdetect_snap_%d.jpeg", g_photo_dirpath, jpeg_id);
  } else {
    sprintf(snap_path, "%s/detect_snap_%d.jpeg", g_photo_dirpath, jpeg_id);
  }
  FILE *snap_file = fopen(snap_path, "w");
  printf("save jpeg file: %s\n", snap_path);
  fwrite(RK_MPI_MB_GetPtr(snap_mb), 1, RK_MPI_MB_GetSize(snap_mb), snap_file);
  fclose(snap_file);
  jpeg_id++;
  RK_MPI_MB_ReleaseBuffer(snap_mb);
}

static int update_detect_rcd(int id, int *x, int *y, int *width, int *height) {
  int empty_order = -1;
  int match_order = -1;
  int ret = 0;
  for (int i = 0; i < DETECT_RCD_NUM; i++) {
    if (!g_detect_list[i].used && empty_order < 0) {
      empty_order = i;
      continue;
    }
    if (g_detect_list[i].id == id) {
      if (g_detect_list[i].x == 0 && g_detect_list[i].y == 0 &&
          g_detect_list[i].width == 0 && g_detect_list[i].height == 0) {
        ret = 1;
      }
      match_order = i;
      break;
    }
  }
  if (match_order < 0 && empty_order < 0) {
    printf("over max rcd num:%d\n", DETECT_RCD_NUM);
    return -1;
  } else if (match_order < 0) {
    match_order = empty_order;
    ret = 1;
  }
  g_detect_list[match_order].used = 1;
  g_detect_list[match_order].activate = DETECT_ACTIVE_CNT;
  g_detect_list[match_order].id = id;
  *x = abs(*x - g_detect_list[match_order].x) > DETECT_RECT_DIFF
           ? *x
           : g_detect_list[match_order].x;
  *y = abs(*y - g_detect_list[match_order].y) > DETECT_RECT_DIFF
           ? *y
           : g_detect_list[match_order].y;
  if (g_detect_list[match_order].x == *x &&
      g_detect_list[match_order].y == *y) {
    *width =
        abs(*width - g_detect_list[match_order].width) > (DETECT_RECT_DIFF * 2)
            ? *width
            : g_detect_list[match_order].width;
    *height = abs(*height - g_detect_list[match_order].height) >
                      (DETECT_RECT_DIFF * 2)
                  ? *height
                  : g_detect_list[match_order].height;
  }
  if ((*x + *width) > (int)cfg.session_cfg[DRAW_INDEX].u32Width)
    *width = (int)cfg.session_cfg[DRAW_INDEX].u32Width - *x - 1;
  if ((*y + *height) > (int)cfg.session_cfg[DRAW_INDEX].u32Height)
    *height = (int)cfg.session_cfg[DRAW_INDEX].u32Height - *y - 1;
  g_detect_list[match_order].width = *width;
  g_detect_list[match_order].height = *height;
  g_detect_list[match_order].x = *x;
  g_detect_list[match_order].y = *y;
  return ret;
}

static void clear_detect_rcd() {
  for (int i = 0; i < DETECT_RCD_NUM; i++) {
    if (g_detect_list[i].used && g_detect_list[i].activate <= 0) {
      memset(&g_detect_list[i], 0, sizeof(detect_record_t));
    }
  }
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

  while (g_flag_run) {
    mb = RK_MPI_SYS_GetMediaBuffer(
        RK_ID_VI, cfg.session_cfg[DRAW_INDEX].stViChn.s32ChnId, -1);
    if (!mb)
      continue;
    // draw
    if (rknn_list_size(rknn_list_)) {
      long draw_before_time_first;
      if (in_arg->time_log_en)
        draw_before_time_first = getCurrentTimeMsec();
      long time_before;
      rockx_object_array_t person_array;
      memset(&person_array, 0, sizeof(person_array));
      rknn_list_pop(rknn_list_, &time_before, &person_array);
      if (in_arg->time_log_en)
        printf("time interval is %ld\n", getCurrentTimeMsec() - time_before);

      for (int j = 0; j < person_array.count; j++) {
        // printf("ttl: %d, now: %d, id: %d\n", person_array.count, j,
        // person_array.object[j].id);
        // printf("person_array.count is %d\n", person_array.count);
        // const char *cls_name =
        // OBJECT_DETECTION_LABELS_91[person_array.object[j].cls_idx];
        // int left = person_array.object[j].box.left;
        // int top = person_array.object[j].box.top;
        // int right = person_array.object[j].box.right;
        // int bottom = person_array.object[j].box.bottom;
        // float score = person_array.object[j].score;
        // printf("box=(%d %d %d %d) cls_name=%s, score=%f\n", left, top, right,
        // bottom, cls_name, score);
        float score = person_array.object[j].score;
        if (score < in_arg->face_score) {
          printf("face score: %f, smaller than %f, be filtered\n", score,
                 in_arg->face_score);
          continue;
        }
        printf("face score: %f\n", score);
        int x = person_array.object[j].box.left * x_rate;
        int y = person_array.object[j].box.top * y_rate;
        int w = (person_array.object[j].box.right -
                 person_array.object[j].box.left) *
                x_rate;
        int h = (person_array.object[j].box.bottom -
                 person_array.object[j].box.top) *
                y_rate;
        if (x < 0)
          x = 0;
        if (y < 0)
          y = 0;
        while ((uint32_t)(x + w) >= cfg.session_cfg[DRAW_INDEX].u32Width) {
          w -= 16;
        }
        while ((uint32_t)(y + h) >= cfg.session_cfg[DRAW_INDEX].u32Height) {
          h -= 16;
        }
        // printf("get id:%d border=(%d %d %d %d)\n", person_array.object[j].id,
        // x, y, w, h);
        int snap_en =
            update_detect_rcd(person_array.object[j].id, &x, &y, &w, &h);
        // snap shot
        while (snap_en == 1) {
          // printf("in rga snap\n");
          int rga_x = UPALIGNTO(x, 2);
          int rga_y = UPALIGNTO(y, 2);
          int rga_width = UPALIGNTO(w, 8);
          int rga_height = UPALIGNTO(h, 8);

          if ((rga_x + rga_width) > (int)cfg.session_cfg[DRAW_INDEX].u32Width) {
            rga_x = (int)cfg.session_cfg[DRAW_INDEX].u32Width - rga_width;
          }
          if ((rga_y + rga_height) >
              (int)cfg.session_cfg[DRAW_INDEX].u32Height) {
            rga_y = (int)cfg.session_cfg[DRAW_INDEX].u32Height - rga_height;
          }
          // printf("rga para, %d, %d, %d, %d, %d, %d\n", rga_x, rga_y,
          // rga_width, rga_height, cfg.session_cfg[DRAW_INDEX].u32Width,
          // cfg.session_cfg[DRAW_INDEX].u32Height);
          rga_buffer_t src;
          rga_buffer_t dst;
          MEDIA_BUFFER dst_mb = NULL;
          MB_IMAGE_INFO_S stImageInfo = {rga_width, rga_height, rga_width,
                                         rga_height, IMAGE_TYPE_NV12};
          dst_mb = RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, 0);
          if (!dst_mb) {
            printf("ERROR: RK_MPI_MB_CreateImageBuffer get null buffer!\n");
            break;
          }
          src = wrapbuffer_fd(
              RK_MPI_MB_GetFD(mb), cfg.session_cfg[DRAW_INDEX].u32Width,
              cfg.session_cfg[DRAW_INDEX].u32Height, RK_FORMAT_YCbCr_420_SP);
          dst = wrapbuffer_fd(RK_MPI_MB_GetFD(dst_mb), rga_width, rga_height,
                              RK_FORMAT_YCbCr_420_SP);
          im_rect src_rect = {rga_x, rga_y, rga_width, rga_height};
          im_rect dst_rect = {0};
          int ret = imcheck(src, dst, src_rect, dst_rect, IM_CROP);
          if (IM_STATUS_NOERROR != ret) {
            printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
            RK_MPI_MB_ReleaseBuffer(dst_mb);
            break;
          }
          IM_STATUS STATUS = imcrop(src, dst, src_rect);
          if (STATUS != IM_STATUS_SUCCESS) {
            printf("imcrop failed: %s\n", imStrError(STATUS));
            RK_MPI_MB_ReleaseBuffer(dst_mb);
            break;
          }

          VENC_RESOLUTION_PARAM_S stResolution;
          stResolution.u32Width = rga_width;
          stResolution.u32Height = rga_height;
          stResolution.u32VirWidth = rga_width;
          stResolution.u32VirHeight = rga_height;

          ret = RK_MPI_VENC_SetResolution(cfg.session_count, stResolution);
          // printf("RK_MPI_VENC_SetResolution, ret: %d\n", ret);
          ret =
              RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, cfg.session_count, dst_mb);
          // printf("RK_MPI_SYS_SendMediaBuffer, ret: %d\n", ret);
          RK_MPI_MB_ReleaseBuffer(dst_mb);
          // printf("out rga snap\n");
          break;
        }
      }
      if (in_arg->time_log_en) {
        long draw_after_time_first = getCurrentTimeMsec();
        printf("get first draw rect time-consuming is %ld\n",
               (draw_after_time_first - draw_before_time_first));
      }
    }
    // draw
    long draw_before_time;
    if (in_arg->time_log_en)
      draw_before_time = getCurrentTimeMsec();
    for (int k = 0; k < DETECT_RCD_NUM; k++) {
      if (g_detect_list[k].used && g_detect_list[k].activate) {
        // printf("rcd:%d border=(%d %d %d %d)\n", g_detect_list[k].id,
        //        g_detect_list[k].x, g_detect_list[k].y,
        //        g_detect_list[k].width,
        //        g_detect_list[k].height);
        nv12_border((char *)RK_MPI_MB_GetPtr(mb),
                    cfg.session_cfg[DRAW_INDEX].u32Width,
                    cfg.session_cfg[DRAW_INDEX].u32Height, g_detect_list[k].x,
                    g_detect_list[k].y, g_detect_list[k].width,
                    g_detect_list[k].height, 0, 0, 255);
        g_detect_list[k].activate--;
      }
    }
    clear_detect_rcd();
    if (in_arg->time_log_en) {
      long draw_after_time = getCurrentTimeMsec();
      printf("draw rect time-consuming is %ld\n",
             (draw_after_time - draw_before_time));
    }
    // send from VI to VENC
    usleep(30000);
    RK_MPI_SYS_SendMediaBuffer(
        RK_ID_VENC, cfg.session_cfg[DRAW_INDEX].stVenChn.s32ChnId, mb);
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  return NULL;
}

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

static RK_S32 SAMPLE_COMMON_VENC_SNAP_Start(RK_S32 chnId) {
  VENC_CHN_ATTR_S venc_chn_attr;

  // Create JPEG for take pictures.
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  venc_chn_attr.stVencAttr.u32PicWidth = cfg.session_cfg[DRAW_INDEX].u32Width;
  venc_chn_attr.stVencAttr.u32PicHeight = cfg.session_cfg[DRAW_INDEX].u32Height;
  venc_chn_attr.stVencAttr.u32VirWidth = cfg.session_cfg[DRAW_INDEX].u32Width;
  venc_chn_attr.stVencAttr.u32VirHeight = cfg.session_cfg[DRAW_INDEX].u32Height;
  RK_S32 ret = RK_MPI_VENC_CreateChn(chnId, &venc_chn_attr);
  if (ret) {
    printf("Create Venc(JPEG) failed! ret=%d\n", ret);
    g_flag_run = 0;
    return ret;
  }

  MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32DevId = 0;
  stEncChn.s32ChnId = chnId;

  ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
  if (ret) {
    printf("ERROR: register output callback for VENC error! ret=%d\n", ret);
    g_flag_run = 0;
    return ret;
  }

  return ret;
}

static RK_CHAR optstr[] = "?a::c:v:l:sr:p:t:f:m:I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"data_version", required_argument, NULL, 'v'},
    {"cfg_path", required_argument, NULL, 'c'},
    {"limit_score", required_argument, NULL, 'l'},
    {"runtime_path", required_argument, NULL, 'r'},
    {"photo_dirpath", required_argument, NULL, 'p'},
    {"time_log", required_argument, NULL, 't'},
    {"fps", required_argument, NULL, 'f'},
    {"hdr_mode", required_argument, NULL, 'm'},
    {"stress", no_argument, NULL, 's'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"help", no_argument, NULL, '?'},
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
         "[-p pic_dir] "
         "[-l 0.45] "
         "[-t 0] "
         "[-f 30] "
         "[-m 0] "
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
         "[-v 0] "
         "[-c rtsp-nn.cfg] "
         "[-r librknn_runtime.so] "
         "[-p pic_dir] "
         "[-l 0.45] "
         "[-t 0] "
         "[-f 30] "
         "[-m 0] "
         "[-s] \n",
         name);
#endif
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-v | --data_version [0~3]; Default: 0\n"
         "\t\t-->0: ROCKX_MODULE_PERSON_DETECTION_V2\n"
         "\t\t-->1: ROCKX_MODULE_PERSON_DETECTION_V3\n"
         "\t\t-->2: ROCKX_MODULE_FACE_DETECTION_V2\n"
         "\t\t-->3: ROCKX_MODULE_FACE_DETECTION_V3_LARGE\n");
  printf("\t-c | --cfg_path: rtsp cfg path, Default: "
         "\"/oem/usr/share/rtsp-nn.cfg\"\n");
  printf("\t-r | --runtime_path, rknn runtime lib, Default: "
         "\"/usr/lib/librknn_runtime.so\"\n");
  printf("\t-p | --photo_dirpath, detect snap photo dirpath, Default: "
         "\"/tmp/\"\n");
  printf("\t-l | --limit_score score below set, would not be detected. "
         "Default: 0.45 in person_detect, 0.80 in face_detect_v3, 0.75 in "
         "face_detect_v2\n");
  printf(
      "\t-t | --time_log consuming time log. option[0~3]"
      "Default: 0, 0: no log, 1 detect consuming, 2 draw consuming, 3 all\n");
  printf("\t-f | --fps Default:30\n");
  printf("\t-m | --hdr_mode Default:0, set 1 to open hdr, set 0 to close\n");
  printf("\t-s | --stress provide to set stress mode\n");
  printf("\trstp: rtsp://<ip>/live/main_stream\n");
}

int main(int argc, char **argv) {
  memset(&g_detect_list, 0, sizeof(g_detect_list));
  RK_S32 ret = 0;
  RK_CHAR *pIqfilesPath = NULL;
  RK_CHAR *pCfgPath = "/oem/usr/share/rtsp-nn.cfg";
  main_arg_t main_arg;
  main_arg.time_log_en = 0;
  main_arg.face_score = -1.0f;
  sub_arg_t sub_arg;
  memset(&sub_arg, 0, sizeof(sub_arg_t));
  sprintf(sub_arg.runtime_path, "/usr/lib/librknn_runtime.so");
  sub_arg.face_score = -1.0f;
  sub_arg.data_version = ROCKX_MODULE_PERSON_DETECTION_V2;
  RK_S32 s32CamId = 0;
#ifdef RKAIQ
  RK_BOOL bMultictx = RK_FALSE;
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  int fps = 30;
#endif
  int c;
  int stress_mode = 0;
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
    case 'v':
      switch (atoi(optarg)) {
      case 0:
        sub_arg.data_version = ROCKX_MODULE_PERSON_DETECTION_V2;
        break;
      case 1:
        sub_arg.data_version = ROCKX_MODULE_PERSON_DETECTION_V3;
        break;
      case 2:
        sub_arg.data_version = ROCKX_MODULE_FACE_DETECTION_V2;
        break;
      case 3:
        sub_arg.data_version = ROCKX_MODULE_FACE_DETECTION_V3_LARGE;
        break;
      }
      break;
    case 's':
      stress_mode = 1;
      break;
    case 'l':
      sub_arg.face_score = atof(optarg);
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
    case 'f':
#ifdef RKAIQ
      fps = atoi(optarg);
#endif
      break;
    case 'm':
#ifdef RKAIQ
      if (atoi(optarg)) {
        hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
      }
#endif
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

  if (sub_arg.face_score < 0) {
    switch (sub_arg.data_version) {
    case ROCKX_MODULE_PERSON_DETECTION_V2:
      sub_arg.face_score = 0.45f;
      break;
    case ROCKX_MODULE_PERSON_DETECTION_V3:
      sub_arg.face_score = 0.45f;
      break;
    case ROCKX_MODULE_FACE_DETECTION_V2:
      sub_arg.face_score = 0.75f;
      break;
    case ROCKX_MODULE_FACE_DETECTION_V3_LARGE:
      sub_arg.face_score = 0.80f;
      break;
    default:
      break;
    }
  }
  main_arg.face_score = sub_arg.face_score;

  printf(">>>>>>>>>>>>>>> Test START <<<<<<<<<<<<<<<<<<<<<<\n");
#ifdef RKAIQ
  printf("####Aiq xml dirpath: %s\n\n", pIqfilesPath);
  printf("####hdr mode: %d\n\n", hdr_mode);
  printf("####fps: %d\n\n", fps);
  printf("#bMultictx: %d\n\n", bMultictx);
#endif
  printf("#CameraIdx: %d\n\n", s32CamId);
  printf("####data version: %d\n\n", sub_arg.data_version);
  printf("####config dirpath: %s\n\n", pCfgPath);
  printf("####runtime path: %s\n\n", sub_arg.runtime_path);
  printf("####photo dirpath: %s\n\n", g_photo_dirpath);
  printf("####limit score: %f\n\n", sub_arg.face_score);
  printf("####detect time log en: %d\n\n", sub_arg.time_log_en);
  printf("####draw time log en: %d\n\n", main_arg.time_log_en);
  printf("####stress_mode: %d\n\n", stress_mode);

  signal(SIGINT, sig_proc);

  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pIqfilesPath);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
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

  // add extre venc for snap
  if (!stress_mode) {
    ret = SAMPLE_COMMON_VENC_SNAP_Start(cfg.session_count);
    if (ret) {
      printf("snap venc start fail, ret is %d\n", ret);
      return -1;
    }
  } else {
    printf("snap function is closed in stress mode\n");
  }

  create_rknn_list(&rknn_list_);
  // Get the sub-stream buffer for humanoid recognition
  pthread_t read_thread;
  if (stress_mode) {
    pthread_create(&read_thread, NULL, GetMediaBufferStress, &sub_arg);
  } else {
    pthread_create(&read_thread, NULL, GetMediaBuffer, &sub_arg);
  }

  // The mainstream draws a box asynchronously based on the recognition result
  pthread_t main_stream_thread;
  pthread_create(&main_stream_thread, NULL, MainStream, &main_arg);

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
  }

  rtsp_del_demo(g_rtsplive);

  if (!stress_mode)
    RK_MPI_VENC_DestroyChn(cfg.session_count);

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

  destory_rknn_list(&rknn_list_);
  return 0;
}
