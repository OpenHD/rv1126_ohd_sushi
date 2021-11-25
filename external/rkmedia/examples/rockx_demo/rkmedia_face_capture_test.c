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
#include "utilFtp/ftp.h"

#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#define DOWNALIGNTO(value, align) (value & (~(align - 1)))
#define UPALIGNTO16(value) UPALIGNTO(value, 16)
#define MAX_SESSION_NUM 2
#define DRAW_INDEX 0
#define RK_NN_INDEX DRAW_INDEX
#define MEDIA_LIST_SIZE 3
#define RGB_INDEX 0
#define NV12_INDEX 1
#define MAX_FACE_NUM 30
#define QUEUE_SIZE MAX_FACE_NUM
#define MAX_DRAW_NUM 50
#define DETECT_RECT_DIFF 25
#define DETECT_ACTIVE_CNT 3
#define COUNT_FILTER 2
#define SNAP_NAME_LEN 64
#define SNAP_WIDTH 320
#define SNAP_HEIGHT 448
#define RGA_ALIGN 8
#define SNAP_EXPAND_RATE 1

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

/* snap pic name list */
typedef struct rk_list_s {
  int qty;
  int tail;
  int max_num;
  int cell_size;
  pthread_mutex_t list_mutex;
  void **buf;
} rk_list_t;

typedef struct rk_rockx_filter_info_s {
  MEDIA_BUFFER rgb_buf;
  MEDIA_BUFFER nv12_buf;
  rockx_object_array_t track_array;
} rk_rockx_filter_info_t;

typedef struct stream_arg_s {
  rockx_module_t data_version;
  int time_log_en;
  int draw_time_log_en;
  char runtime_path[128];
  int ir_en;
  int filter_en;
  int filter_en_raw;
  int draw_en;
} stream_arg_t;

struct image_face {
  int id;
  MEDIA_BUFFER wrapper;
  rockx_face_quality_result_t quality;
  int left;
  int top;
  int right;
  int bottom;
  int detected; // to mark a face detected or leaved
  pthread_mutex_t face_mutex;
  unsigned int count;
};

struct demo_cfg cfg;
static rockx_config_t *g_rockx_config = NULL;
static rockx_handle_t g_object_det_handle;
static rockx_handle_t g_filter_handle;
static rockx_handle_t g_object_track_handle;
static rtsp_demo_handle g_rtsplive = NULL;
struct image_face g_faces[MAX_FACE_NUM] = {0};
static rk_list_t g_snap_name_list;
static rk_list_t g_media_buffer_list;

static int g_flag_run = 1;
static int g_ftp_flag = 0;
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

static void GetRss(char *tag) {
  pid_t id = getpid();
  char cmd[64] = {0};
  sprintf(cmd, "cat /proc/%d/status | grep RSS", id);
  long now_time = getCurrentTimeMsec();
  printf(">>>>[%s] time:%ld\n", tag, now_time);
  system(cmd);
}
/* common method end */

/* list method start */
static void rk_create_list(rk_list_t *p_queue, int max_num, int cell_size) {
  pthread_mutex_init(&p_queue->list_mutex, NULL);
  pthread_mutex_lock(&p_queue->list_mutex);
  memset(p_queue, 0, sizeof(rk_list_t));
  p_queue->buf = (void **)malloc(sizeof(void *) * max_num);
  memset(p_queue->buf, 0, sizeof(void *) * max_num);
  for (int i = 0; i < max_num; i++) {
    p_queue->buf[i] = (void *)malloc(cell_size);
    memset(p_queue->buf[i], 0, cell_size);
  }
  p_queue->max_num = max_num;
  p_queue->cell_size = cell_size;
  pthread_mutex_unlock(&p_queue->list_mutex);
}

static int get_head(rk_list_t *p_queue) {
  if (p_queue->tail >= p_queue->qty) {
    return p_queue->tail - p_queue->qty;
  }
  return p_queue->max_num - (p_queue->qty - p_queue->tail);
}

static void rk_destroy_list(rk_list_t *p_queue) {
  if (!p_queue)
    return;
  pthread_mutex_lock(&p_queue->list_mutex);
  for (int i = 0; i < p_queue->max_num; i++) {
    if (p_queue->buf[i]) {
      free(p_queue->buf[i]);
      p_queue->buf[i] = NULL;
    }
  }
  free(p_queue->buf);
  memset(p_queue, 0, sizeof(rk_list_t));
  pthread_mutex_unlock(&p_queue->list_mutex);
}

static int rk_list_empty_cell_get(rk_list_t *p_queue, void **p_val,
                                  int *order) {
  while (p_queue->qty == p_queue->max_num) {
    if (g_flag_run)
      usleep(100);
    else
      return -1;
  }
  if (p_queue->tail == p_queue->max_num) {
    p_queue->tail = 0;
  }
  pthread_mutex_lock(&p_queue->list_mutex);
  *p_val = p_queue->buf[p_queue->tail];
  *order = p_queue->tail;
  pthread_mutex_unlock(&p_queue->list_mutex);
  return 0;
}

static int rk_list_filled_cell_get(rk_list_t *p_queue, void **p_val,
                                   int *order) {
  while (!p_queue->qty) {
    if (g_flag_run)
      usleep(100);
    else
      return -1;
  }
  pthread_mutex_lock(&p_queue->list_mutex);
  *order = get_head(p_queue);
  *p_val = p_queue->buf[*order];
  pthread_mutex_unlock(&p_queue->list_mutex);
  return 0;
}

static int rk_list_ref(rk_list_t *p_queue, int order) {
  if (order == p_queue->tail) {
    pthread_mutex_lock(&p_queue->list_mutex);
    p_queue->tail++;
    p_queue->qty++;
    pthread_mutex_unlock(&p_queue->list_mutex);
    return 0;
  }
  return -1;
}

static int rk_list_unref(rk_list_t *p_queue, int order) {
  if (order == get_head(p_queue)) {
    pthread_mutex_lock(&p_queue->list_mutex);
    p_queue->qty--;
    pthread_mutex_unlock(&p_queue->list_mutex);
    return 0;
  }
  return -1;
}

static int rk_list_push(rk_list_t *p_queue, void *p_val) {
  while (p_queue->qty == p_queue->max_num) {
    usleep(100);
  }
  if (p_queue->tail == p_queue->max_num) {
    p_queue->tail = 0;
  }
  pthread_mutex_lock(&p_queue->list_mutex);
  memcpy(p_queue->buf[p_queue->tail], p_val, p_queue->cell_size);
  p_queue->tail++;
  p_queue->qty++;
  pthread_mutex_unlock(&p_queue->list_mutex);
  return 0;
}

static int rk_list_pop(rk_list_t *p_queue, void *p_val) {
  while (!p_queue->qty) {
    usleep(100);
  }
  pthread_mutex_lock(&p_queue->list_mutex);
  memcpy(p_val, p_queue->buf[get_head(p_queue)], p_queue->cell_size);
  p_queue->qty--;
  pthread_mutex_unlock(&p_queue->list_mutex);
  return 0;
}
/* list method end */

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

static RK_S32 SAMPLE_COMMON_VI_Start(struct Session *session,
                                     VI_CHN_WORK_MODE mode, RK_S32 vi_pipe) {
  RK_S32 ret = 0;
  VI_CHN_ATTR_S vi_chn_attr;

  if (session->stViChn.s32ChnId == RK_NN_INDEX) {
    vi_chn_attr.u32BufCnt = MAX_FACE_NUM;
  } else {
    vi_chn_attr.u32BufCnt = 3;
  }
  vi_chn_attr.u32Width = session->u32Width;
  vi_chn_attr.u32Height = session->u32Height;
  vi_chn_attr.enWorkMode = mode;
  vi_chn_attr.pcVideoNode = session->videopath;
  vi_chn_attr.enPixFmt = session->enImageType;

  ret |= RK_MPI_VI_SetChnAttr(vi_pipe, session->stViChn.s32ChnId, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(vi_pipe, session->stViChn.s32ChnId);

  return ret;
}

static RK_S32 SAMPLE_COMMON_VENC_Start(struct Session *session, int fps_arg) {
  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = session->video_type;

  venc_chn_attr.stVencAttr.imageType = session->enImageType;

  venc_chn_attr.stVencAttr.u32PicWidth = session->u32Width;
  venc_chn_attr.stVencAttr.u32PicHeight = session->u32Height;
  venc_chn_attr.stVencAttr.u32VirWidth = session->u32Width;
  venc_chn_attr.stVencAttr.u32VirHeight = session->u32Height;
  venc_chn_attr.stVencAttr.u32Profile = 77;

  int env_fps = fps_arg;
#if 0
  char *fps_c = getenv("FPS");
  if (fps_c) {
    printf("FPS: %s\n", fps_c);
    env_fps = atoi(fps_c);
  }
#endif
  printf("env_fps: %d\n", env_fps);
  switch (venc_chn_attr.stVencAttr.enType) {
  case RK_CODEC_TYPE_H264:
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate =
        session->u32Width * session->u32Height * env_fps / 14;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = env_fps;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = env_fps;
    break;
  case RK_CODEC_TYPE_H265:
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
    venc_chn_attr.stRcAttr.stH265Vbr.u32Gop = 30;
    venc_chn_attr.stRcAttr.stH265Vbr.u32MaxBitRate =
        session->u32Width * session->u32Height * env_fps / 14;
    venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateNum = env_fps;
    venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateNum = env_fps;
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

/* convert rgb start */
static int media_buffer_list_init(uint32_t width, uint32_t height,
                                  int buf_len) {
  rk_create_list(&g_media_buffer_list, buf_len, sizeof(rk_rockx_filter_info_t));
  uint32_t buf_size = width * height * 3;
  int ret;
  MB_IMAGE_INFO_S stImageInfo = {width, height, width, height,
                                 IMAGE_TYPE_RGB888};
  for (int i = 0; i < buf_len; i++) {
    int order;
    rk_rockx_filter_info_t *f_info;
    ret =
        rk_list_empty_cell_get(&g_media_buffer_list, (void **)&f_info, &order);
    if (ret) {
      return -1;
    }
    f_info->rgb_buf = RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, 0);
    RK_MPI_MB_SetSize(f_info->rgb_buf, buf_size);
    rk_list_ref(&g_media_buffer_list, order);
  }
  for (int i = 0; i < buf_len; i++) {
    rk_list_unref(&g_media_buffer_list, i);
  }
  return 0;
}

static int RGBImgInit(rockx_image_t *rgb_image, uint32_t width,
                      uint32_t height) {
  memset(rgb_image, 0, sizeof(rockx_image_t));
  rgb_image->height = height;
  rgb_image->width = width;
  rgb_image->pixel_format = ROCKX_PIXEL_FORMAT_RGB888;
  rgb_image->is_prealloc_buf = 1;
  return 0;
}

static void RKMediaBufferConvertRGB(MEDIA_BUFFER nv12_buf, MEDIA_BUFFER rgb_buf,
                                    RK_U32 width, RK_U32 height) {
  if (0)
    GetRss("");
  rga_buffer_t src;
  rga_buffer_t dst_buf;
  src = wrapbuffer_fd(RK_MPI_MB_GetFD(nv12_buf), width, height,
                      RK_FORMAT_YCbCr_420_SP);
  dst_buf =
      wrapbuffer_fd(RK_MPI_MB_GetFD(rgb_buf), width, height, RK_FORMAT_RGB_888);
  IM_STATUS STATUS = imcopy(src, dst_buf);
  if (STATUS != IM_STATUS_SUCCESS)
    printf("imcopy failed: %s\n", imStrError(STATUS));
}

static int RGBBufferGet(rockx_image_t *rgb_image, int *index,
                        MEDIA_BUFFER nv12_buf, rockx_object_array_t **out_ary) {
  int ret;
  rk_rockx_filter_info_t *empty_buf;
  ret =
      rk_list_empty_cell_get(&g_media_buffer_list, (void **)&empty_buf, index);
  if (ret)
    return -1;
  rgb_image->data = RK_MPI_MB_GetPtr(empty_buf->rgb_buf);
  rgb_image->size = RK_MPI_MB_GetSize(empty_buf->rgb_buf);
  empty_buf->nv12_buf = RK_MPI_MB_Copy(nv12_buf, RK_TRUE);
  RKMediaBufferConvertRGB(nv12_buf, empty_buf->rgb_buf, rgb_image->width,
                          rgb_image->height);
  memset(&empty_buf->track_array, 0, sizeof(rockx_object_array_t));
  *out_ary = &empty_buf->track_array;
  return 0;
}

static void RGBBufferRef(int index) {
  rk_list_ref(&g_media_buffer_list, index);
}

static int RockxDetectedInfoGet(int *index, rockx_image_t *rgb_image,
                                MEDIA_BUFFER *buf,
                                rockx_object_array_t *out_ary) {
  rk_rockx_filter_info_t *filled_buf;
  int ret = rk_list_filled_cell_get(&g_media_buffer_list, (void **)&filled_buf,
                                    index);
  if (ret)
    return ret;
  *buf = filled_buf->nv12_buf;
  rgb_image->data = RK_MPI_MB_GetPtr(filled_buf->rgb_buf);
  rgb_image->size = RK_MPI_MB_GetSize(filled_buf->rgb_buf);
  *out_ary = filled_buf->track_array;
  return 0;
}

static void RockxDetectedInfoUnref(int idx) {
  int index;
  rk_rockx_filter_info_t *filled_buf;
  int ret = rk_list_filled_cell_get(&g_media_buffer_list, (void **)&filled_buf,
                                    &index);
  if (ret) {
    printf("%s check fail\n", __FUNCTION__);
  } else {
    if (index == idx) {
      if (filled_buf->nv12_buf) {
        RK_MPI_MB_ReleaseBuffer(filled_buf->nv12_buf);
        filled_buf->nv12_buf = NULL;
      }
    } else {
      printf("%s release nv12 buffer fail\n", __FUNCTION__);
    }
  }
  rk_list_unref(&g_media_buffer_list, index);
}

static void media_buffer_list_uninit() {
  // while (g_media_buffer_list.qty) {
  //   RockxDetectedInfoUnref(get_head(&g_media_buffer_list));
  // }
  for (int i = 0; i < g_media_buffer_list.max_num; i++) {
    rk_rockx_filter_info_t *cell =
        (rk_rockx_filter_info_t *)g_media_buffer_list.buf[i];
    if (cell->rgb_buf)
      RK_MPI_MB_ReleaseBuffer(cell->rgb_buf);
    if (cell->nv12_buf)
      RK_MPI_MB_ReleaseBuffer(cell->nv12_buf);
  }
  rk_destroy_list(&g_media_buffer_list);
}
/* convert rgb end */

/* method for snap start */
void face_init() {
  memset(g_faces, 0, MAX_FACE_NUM * sizeof(struct image_face));
  for (int i = 0; i < MAX_FACE_NUM; i++) {
    g_faces[i].id = -1;
    pthread_mutex_init(&g_faces[i].face_mutex, NULL);
  }
}

// 1 good, 0 bad
int face_filter(rockx_face_quality_result_t quality) {
  // filter quality
  if (quality.result != ROCKX_FACE_QUALITY_PASS)
    return 1;
  // filter size, TODO...
  // filter blur, TODO...
  return 0;
}

int bright_filter(rockx_face_quality_result_t quality) {
  if (quality.brightness > 230)
    return 1;

  return 0;
}

// 1, new > old
int face_compare(rockx_face_quality_result_t new_quality,
                 rockx_face_quality_result_t old_quality) {
  if (new_quality.face_score * new_quality.det_score >
      old_quality.face_score * old_quality.det_score)
    return 1;
  else
    return 0;
}

int face_update(int id, MEDIA_BUFFER wrapper, int left, int top, int right,
                int bottom, rockx_face_quality_result_t quality, int ir_en) {
  int i;
  int need_updated = 0;

  // filter bad quality face
  if (face_filter(quality))
    return -1;

  if (ir_en && bright_filter(quality))
    return -1;

  // find face id
  for (i = 0; i < MAX_FACE_NUM; i++) {
    if (g_faces[i].id == id)
      break;
  }
  // if new, find a slot
  if (i == MAX_FACE_NUM) {
    for (i = 0; i < MAX_FACE_NUM; i++) {
      if (g_faces[i].id < 0)
        break;
    }
    // if full,to do ...
    if (i == MAX_FACE_NUM)
      return 0;
    pthread_mutex_lock(&g_faces[i].face_mutex);
    g_faces[i].count = 1;
    g_faces[i].detected = 0;
    need_updated = 1;
  } else {
    pthread_mutex_lock(&g_faces[i].face_mutex);
    if (g_faces[i].id != id) {
      pthread_mutex_unlock(&g_faces[i].face_mutex);
      return -1;
    }
    g_faces[i].count++;
    // id find, compare image quality
    if (face_compare(quality, g_faces[i].quality) > 0) {
      need_updated = 1;
    } else {
      need_updated = 0;
      pthread_mutex_unlock(&g_faces[i].face_mutex);
    }
  }
  if (need_updated) {
    g_faces[i].id = id;
    g_faces[i].left = left;
    g_faces[i].top = top;
    g_faces[i].right = right;
    g_faces[i].bottom = bottom;
    g_faces[i].quality = quality;
    if (g_faces[i].wrapper) {
      RK_MPI_MB_ReleaseBuffer(g_faces[i].wrapper);
    }
    g_faces[i].wrapper = RK_MPI_MB_Copy(wrapper, RK_TRUE);
    pthread_mutex_unlock(&g_faces[i].face_mutex);
  }
  return 0;
}

// for face leaved checking
void face_set_detected(int id) {
  // find face id
  for (int i = 0; i < MAX_FACE_NUM; i++) {
    if (g_faces[i].id == id) {
      pthread_mutex_lock(&g_faces[i].face_mutex);
      g_faces[i].detected += 1;
      pthread_mutex_unlock(&g_faces[i].face_mutex);
    }
  }
}

void face_snap_name_set(struct image_face snap_info) {
  char path_str[SNAP_NAME_LEN] = {0};
  if (g_photo_dirpath) {
    snprintf(path_str, SNAP_NAME_LEN, "%s/out-%d-%d-%.3f-%.3f.jpg",
           g_photo_dirpath, snap_info.id, snap_info.count,
           snap_info.quality.face_score, snap_info.quality.det_score);
  } else {
    snprintf(path_str, SNAP_NAME_LEN, "/tmp/out-%d-%d-%.3f-%.3f.jpg",
           snap_info.id, snap_info.count,
           snap_info.quality.face_score, snap_info.quality.det_score);
  }
  rk_list_push(&g_snap_name_list, (void *)path_str);
}

void face_snap(struct image_face snap_info) {
  int rga_x = UPALIGNTO(snap_info.left, 2);
  int rga_y = UPALIGNTO(snap_info.top, 2);
  int rga_width = snap_info.right - snap_info.left;
  int rga_height = snap_info.bottom - snap_info.top;
  rga_width = rga_width * (1 + (SNAP_EXPAND_RATE * 2));
  rga_height = rga_height * (1 + (SNAP_EXPAND_RATE * 2));

  if (rga_width > (int)cfg.session_cfg[RK_NN_INDEX].u32Width) {
    rga_width = (int)cfg.session_cfg[RK_NN_INDEX].u32Width;
  }
  if (rga_height > (int)cfg.session_cfg[RK_NN_INDEX].u32Height) {
    rga_height = (int)cfg.session_cfg[RK_NN_INDEX].u32Height;
  }
  if ((rga_width * SNAP_HEIGHT) > (rga_height * SNAP_WIDTH)) {
    rga_height = rga_width * SNAP_HEIGHT / SNAP_WIDTH;
  } else if ((rga_width * SNAP_HEIGHT) < (rga_height * SNAP_WIDTH)) {
    rga_width = rga_height * SNAP_WIDTH / SNAP_HEIGHT;
  }
  if (rga_width > (int)cfg.session_cfg[RK_NN_INDEX].u32Width) {
    rga_height = rga_height * (int)cfg.session_cfg[RK_NN_INDEX].u32Width / rga_width;
    rga_width = (int)cfg.session_cfg[RK_NN_INDEX].u32Width;
  }
  if (rga_height > (int)cfg.session_cfg[RK_NN_INDEX].u32Height) {
    rga_width =
        rga_width * (int)cfg.session_cfg[RK_NN_INDEX].u32Height / rga_height;
    rga_height = (int)cfg.session_cfg[RK_NN_INDEX].u32Height;
  }
  rga_width = DOWNALIGNTO(rga_width, 2);
  rga_height = DOWNALIGNTO(rga_height, 2);
  if ((rga_x + rga_width) > (int)cfg.session_cfg[RK_NN_INDEX].u32Width) {
    if (rga_width > (int)cfg.session_cfg[RK_NN_INDEX].u32Width) {
      rga_height =
          rga_height / rga_width * (int)cfg.session_cfg[RK_NN_INDEX].u32Width;
      rga_width = (int)cfg.session_cfg[RK_NN_INDEX].u32Width;
      rga_x = 0;
    } else {
      rga_x = DOWNALIGNTO(
          ((int)cfg.session_cfg[RK_NN_INDEX].u32Width - rga_width), 2);
    }
  }
  if ((rga_y + rga_height) > (int)cfg.session_cfg[RK_NN_INDEX].u32Height) {
    if (rga_height > (int)cfg.session_cfg[RK_NN_INDEX].u32Height) {
      rga_width =
          rga_width / rga_height * (int)cfg.session_cfg[RK_NN_INDEX].u32Height;
      rga_height = (int)cfg.session_cfg[RK_NN_INDEX].u32Height;
      rga_y = 0;
    } else {
      rga_y = DOWNALIGNTO(
          ((int)cfg.session_cfg[RK_NN_INDEX].u32Height - rga_height), 2);
    }
  }
  while ((snap_info.left - rga_x) > (rga_x + rga_width - snap_info.right)) {
    if ((rga_x + RGA_ALIGN) <= (int)cfg.session_cfg[RK_NN_INDEX].u32Width) {
      rga_x += RGA_ALIGN;
    } else {
      break;
    }
  }
  while ((snap_info.left - rga_x) < (rga_x + rga_width - snap_info.right)) {
    if (rga_x >= RGA_ALIGN) {
      rga_x -= RGA_ALIGN;
    } else {
      break;
    }
  }
  while ((snap_info.top - rga_y) < (rga_y + rga_height - snap_info.bottom)) {
    if (rga_y >= RGA_ALIGN) {
      rga_y -= RGA_ALIGN;
    } else {
      break;
    }
  }
  while ((snap_info.top - rga_y) > (rga_y + rga_height - snap_info.bottom)) {
    if ((rga_y + RGA_ALIGN) <= (int)cfg.session_cfg[RK_NN_INDEX].u32Height) {
      rga_y += RGA_ALIGN;
    } else {
      break;
    }
  }

  rga_buffer_t src;
  rga_buffer_t dst;
  rga_buffer_t empty_buf = {0};
  MEDIA_BUFFER dst_mb = NULL;
  MB_IMAGE_INFO_S stImageInfo = {SNAP_WIDTH, SNAP_HEIGHT, SNAP_WIDTH,
                                 SNAP_HEIGHT, IMAGE_TYPE_NV12};
  dst_mb = RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, 0);
  if (!dst_mb) {
    printf("ERROR: RK_MPI_MB_CreateImageBuffer get null buffer!\n");
    return;
  }
  src = wrapbuffer_fd(
      RK_MPI_MB_GetFD(snap_info.wrapper), cfg.session_cfg[RK_NN_INDEX].u32Width,
      cfg.session_cfg[RK_NN_INDEX].u32Height, RK_FORMAT_YCbCr_420_SP);
  dst = wrapbuffer_fd(RK_MPI_MB_GetFD(dst_mb), SNAP_WIDTH, SNAP_HEIGHT,
                      RK_FORMAT_YCbCr_420_SP);
  im_rect src_rect = {rga_x, rga_y, rga_width, rga_height};
  im_rect dst_rect = {0, 0, SNAP_WIDTH, SNAP_HEIGHT};
  im_rect empty_rect = {0};
  // IM_STATUS STATUS = imcrop(src, dst, src_rect);
  printf("improcess %d, %d, %d, %d\n", rga_x, rga_y, rga_width, rga_height);
  IM_STATUS STATUS =
      improcess(src, dst, empty_buf, src_rect, dst_rect, empty_rect, IM_CROP);
  if (STATUS != IM_STATUS_SUCCESS) {
    printf("imcrop failed: %s\n", imStrError(STATUS));
    RK_MPI_MB_ReleaseBuffer(dst_mb);
    return;
  }

  printf("dst_mb: size: %d, ptr: %p\n", RK_MPI_MB_GetSize(dst_mb),
         RK_MPI_MB_GetPtr(dst_mb));
  // RK_S32 ret;
  face_snap_name_set(snap_info);
  RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, cfg.session_count, dst_mb);
  RK_MPI_MB_ReleaseBuffer(dst_mb);
}

void face_update_end(int ir_en) {
  for (int i = 0; i < MAX_FACE_NUM; i++) {
    pthread_mutex_lock(&g_faces[i].face_mutex);
    if (g_faces[i].detected == 0 && g_faces[i].id >= 0 && g_faces[i].wrapper) {
      // capture if face exists for a while
      if (g_faces[i].count > COUNT_FILTER || ir_en) {
        printf("snap \n");
        if (g_photo_dirpath || g_ftp_flag) {
          face_snap(g_faces[i]);
        }
        g_faces[i].count = 0;
      } else {
        printf("no snap \n");
      }
      RK_MPI_MB_ReleaseBuffer(g_faces[i].wrapper);
      g_faces[i].wrapper = NULL;
      g_faces[i].id = -1;
    } else if (g_faces[i].detected == 0 && g_faces[i].id >= 0) {
      printf("%s: %d wrapper loss\n", __FUNCTION__, i);
    } else if (g_faces[i].detected > 0 && g_faces[i].id >= 0) {
      g_faces[i].detected -= 1;
    }
    pthread_mutex_unlock(&g_faces[i].face_mutex);
  }
}

void face_release_all() {
  for (int i = 0; i < MAX_FACE_NUM; i++) {
    pthread_mutex_lock(&g_faces[i].face_mutex);
    if (g_faces[i].id >= 0 && g_faces[i].wrapper) {
      RK_MPI_MB_ReleaseBuffer(g_faces[i].wrapper);
      g_faces[i].wrapper = NULL;
      g_faces[i].id = -1;
    }
    pthread_mutex_unlock(&g_faces[i].face_mutex);
  }
}
/* method for snap end */

/* rockx handle and method start */
static void DataVersionNameGet(char *buf, rockx_module_t data_version) {
  switch (data_version) {
  case ROCKX_MODULE_PERSON_DETECTION_V2:
    sprintf(buf, "ROCKX_MODULE_PERSON_DETECTION_V2");
    break;
  case ROCKX_MODULE_PERSON_DETECTION_V3:
    sprintf(buf, "ROCKX_MODULE_PERSON_DETECTION_V3");
    break;
  case ROCKX_MODULE_FACE_DETECTION_V2_HORIZONTAL:
    sprintf(buf, "ROCKX_MODULE_FACE_DETECTION_V2_HORIZONTAL");
    break;
  case ROCKX_MODULE_FACE_DETECTION_V3_LARGE:
    sprintf(buf, "ROCKX_MODULE_FACE_DETECTION_V3_LARGE");
    break;
  default:
    sprintf(buf, "ROCKX_MODULE_UNKNOWN");
    break;
  }
}

static RK_S32 ROCKX_HANDLE_INIT(stream_arg_t *init_cfg) {
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

  // init filter handle
  if (init_cfg->filter_en) {
    rockx_ret =
        rockx_create(&g_filter_handle, ROCKX_MODULE_FACE_LANDMARK_5, NULL, 0);
    if (rockx_ret != ROCKX_RET_SUCCESS) {
      printf("init rockx module ROCKX_MODULE_FACE_LANDMARK_5 error %d\n",
             rockx_ret);
      g_flag_run = 0;
      return -1;
    } else {
      printf("init rockx module ROCKX_MODULE_FACE_LANDMARK_5 successful!\n");
    }
  }

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

static RK_S32 ROCKX_HANDLE_UNINIT(stream_arg_t *init_cfg) {
  if (init_cfg->filter_en)
    rockx_destroy(g_filter_handle);
  rockx_destroy(g_object_track_handle);
  rockx_destroy(g_object_det_handle);
  rockx_release_config(g_rockx_config);
  printf("uninit rockx handle!\n");
  return 0;
}

static RK_S32 ROCKX_DETECT(rockx_image_t *img,
                           rockx_object_array_t *out_array) {
  if (!g_flag_run)
    return -1;

  rockx_ret_t rockx_ret;
  rockx_object_array_t detect_array;
  memset(&detect_array, 0, sizeof(detect_array));
  rockx_ret = rockx_face_detect(g_object_det_handle, img, &detect_array, NULL);
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

static void *MainStream(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);
  stream_arg_t *in_arg = (stream_arg_t *)arg;

  RK_S32 ret;
  // init face
  face_init();
  // init handle
  ret = ROCKX_HANDLE_INIT(in_arg);
  if (ret)
    return NULL;
  // create rockx_object_array_t for store result
  rockx_object_array_t *out_track_objects;
  rockx_image_t input_image_rgb;
  ret = RGBImgInit(&input_image_rgb, cfg.session_cfg[RK_NN_INDEX].u32Width,
                   cfg.session_cfg[RK_NN_INDEX].u32Height);
  if (ret) {
    g_flag_run = 0;
  }

  MEDIA_BUFFER mb;
  rga_buffer_t src;
  long time_mark[4];
  int left;
  int top;
  int right;
  int bottom;
  int buf_index;
  while (g_flag_run) {
    mb = RK_MPI_SYS_GetMediaBuffer(
        RK_ID_VI, cfg.session_cfg[DRAW_INDEX].stViChn.s32ChnId, -1);
    if (!mb)
      continue;
    // detect
    ret = RGBBufferGet(&input_image_rgb, &buf_index, mb, &out_track_objects);
    if (ret) {
      RK_MPI_MB_ReleaseBuffer(mb);
      break;
    }
    if (in_arg->time_log_en) {
      memset(time_mark, 0, sizeof(time_mark));
      time_mark[0] = getCurrentTimeMsec();
    }
    ret = ROCKX_DETECT(&input_image_rgb, out_track_objects);
    if (in_arg->time_log_en) {
      time_mark[1] = getCurrentTimeMsec();
      printf("ROCKX_DETECT time-consuming is %ld\n",
             (time_mark[1] - time_mark[0]));
    }
    if (ret) {
      printf("ROCKX_DETECT fail, exit\n");
      g_flag_run = 0;
      break;
    }
    if (in_arg->draw_en) {
      if (in_arg->time_log_en)
        time_mark[2] = getCurrentTimeMsec();
      for (int i = 0; i < out_track_objects->count; i++) {
        if (i == 0) {
          src = wrapbuffer_fd(
              RK_MPI_MB_GetFD(mb), cfg.session_cfg[DRAW_INDEX].u32Width,
              cfg.session_cfg[DRAW_INDEX].u32Height, RK_FORMAT_YCbCr_420_SP);
        }
        // draw
        left = out_track_objects->object[i].box.left;
        top = out_track_objects->object[i].box.top;
        right = out_track_objects->object[i].box.right;
        bottom = out_track_objects->object[i].box.bottom;
        if (left < 0 || top < 0 ||
            right >= (int)cfg.session_cfg[RK_NN_INDEX].u32Width ||
            bottom >= (int)cfg.session_cfg[RK_NN_INDEX].u32Height) {
          continue;
        }
        int x = UPALIGNTO(left, 2);
        int y = UPALIGNTO(top, 2);
        int w = UPALIGNTO((right - left), 2);
        int h = UPALIGNTO((bottom - top), 2);
        rga_nv12_border(src, x, y, w, h, 4, 255);
      }
      if (in_arg->time_log_en) {
        time_mark[3] = getCurrentTimeMsec();
        printf("draw rect time-consuming is %ld\n",
               (time_mark[3] - time_mark[2]));
      }
    }
    // send from VI to VENC
    RK_MPI_SYS_SendMediaBuffer(
        RK_ID_VENC, cfg.session_cfg[DRAW_INDEX].stVenChn.s32ChnId, mb);
    RK_MPI_MB_ReleaseBuffer(mb);
    RGBBufferRef(buf_index);
  }
  ROCKX_HANDLE_UNINIT(in_arg);
  // RGBImgUninit();
  printf("#exit %s thread\n", __func__);
  return NULL;
}

static void *RockxFilterThread(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);
  stream_arg_t *in_arg = (stream_arg_t *)arg;
  int idx;
  int ret;
  MEDIA_BUFFER mb;
  rockx_object_array_t out_track_objects;
  rockx_face_quality_config_t face_quality_config;
  rockx_face_quality_config_init(&face_quality_config);
  rockx_image_t rgb_image;
  memset(&rgb_image, 0, sizeof(rockx_image_t));
  rgb_image.height = cfg.session_cfg[RK_NN_INDEX].u32Height;
  rgb_image.width = cfg.session_cfg[RK_NN_INDEX].u32Width;
  rgb_image.pixel_format = ROCKX_PIXEL_FORMAT_RGB888;
  rgb_image.is_prealloc_buf = 1;
  int id_l;
  int left;
  int top;
  int right;
  int bottom;
  float score;
  long time_mark[2];
  while (g_flag_run) {
    ret = RockxDetectedInfoGet(&idx, &rgb_image, (void **)&mb,
                               &out_track_objects);
    if (ret) {
      break;
    }
    if (in_arg->time_log_en) {
      memset(time_mark, 0, sizeof(time_mark));
      time_mark[0] = getCurrentTimeMsec();
    }
    if (in_arg->filter_en) {
      for (int i = 0; i < out_track_objects.count; i++) {
        id_l = out_track_objects.object[i].id;
        left = out_track_objects.object[i].box.left;
        top = out_track_objects.object[i].box.top;
        right = out_track_objects.object[i].box.right;
        bottom = out_track_objects.object[i].box.bottom;
        score = out_track_objects.object[i].score;
        if (left < 0 || top < 0 ||
            right >= (int)cfg.session_cfg[RK_NN_INDEX].u32Width ||
            bottom >= (int)cfg.session_cfg[RK_NN_INDEX].u32Height) {
          printf("skip out of image, id_l=%d box=(%d %d %d %d) score=%f\n",
                 id_l, left, top, right, bottom, score);
          face_set_detected(id_l);
          continue;
        }
        rockx_face_quality_result_t quality_result;
        ret = rockx_face_quality(g_filter_handle, &rgb_image,
                                 &out_track_objects.object[i],
                                 &face_quality_config, &quality_result);
        if (ret != ROCKX_RET_SUCCESS) {
          printf("rockx_face_filter error %d\n", ret);
        }
        // update face cache buffer
        if (face_update(id_l, mb, left, top, right, bottom, quality_result,
                        in_arg->ir_en)) {
          out_track_objects.object[i].score = -1.0f;
          printf("id_l %d: quality %d, score %.3f,face score %.3f\n", id_l,
                 quality_result.result, quality_result.face_score, score);
          face_set_detected(id_l); // for face leaved checking
          continue;
        }
        printf("id_l %d: quality %d, score %.3f,face score %.3f\n", id_l,
               quality_result.result, quality_result.face_score, score);
        face_set_detected(id_l); // for face leaved checking
      }
      if (in_arg->time_log_en) {
        time_mark[1] = getCurrentTimeMsec();
        printf("ROCKX_FILTER time-consuming is %ld\n",
               (time_mark[1] - time_mark[0]));
      }
      face_update_end(in_arg->ir_en);
    }
    if (mb) {
      mb = NULL;
    }
    RockxDetectedInfoUnref(idx);
  }
  printf("#exit %s thread\n", __func__);
  return NULL;
}

void video_packet_cb(MEDIA_BUFFER snap_mb) {
  static int jpeg_id = 0;
  char snap_path[SNAP_NAME_LEN] = {0};
  int ret = rk_list_pop(&g_snap_name_list, (void *)snap_path);
  if (ret && g_photo_dirpath)
    sprintf(snap_path, "%s/unknown_snap_%d.jpg", g_photo_dirpath, jpeg_id);
  else if (ret && !g_photo_dirpath)
    sprintf(snap_path, "/tmp/unknown_snap_%d.jpg", jpeg_id);
  if (g_ftp_flag) {
    printf("%s: saveftp %s, size: %d\n", __FUNCTION__, snap_path,
           RK_MPI_MB_GetSize(snap_mb));
    ftp_file_put_data(RK_MPI_MB_GetPtr(snap_mb), RK_MPI_MB_GetSize(snap_mb),
                      snap_path);
    printf("save completed\n");
  } else if (g_photo_dirpath) {
    printf("%s: save %s, size: %d\n", __FUNCTION__, snap_path,
           RK_MPI_MB_GetSize(snap_mb));
    FILE *snap_file = fopen(snap_path, "w");
    if (snap_file) {
      if (RK_MPI_MB_GetPtr(snap_mb) && RK_MPI_MB_GetSize(snap_mb)) {
        fwrite(RK_MPI_MB_GetPtr(snap_mb), 1, RK_MPI_MB_GetSize(snap_mb),
               snap_file);
      } else {
        printf("write invalid data: file:%s, data: %p, size: %d\n", snap_path,
               RK_MPI_MB_GetPtr(snap_mb), RK_MPI_MB_GetSize(snap_mb));
      }
    } else {
      printf(">>>open file: %s fail\n", snap_path);
    }
    if (snap_file)
      fclose(snap_file);
  }
  jpeg_id++;
  RK_MPI_MB_ReleaseBuffer(snap_mb);
}

static RK_S32 SAMPLE_COMMON_VENC_SNAP_Start(RK_S32 chnId) {
  VENC_CHN_ATTR_S venc_chn_attr;

  // Create JPEG for take pictures.
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  venc_chn_attr.stVencAttr.u32PicWidth = SNAP_WIDTH;
  venc_chn_attr.stVencAttr.u32PicHeight = SNAP_HEIGHT;
  venc_chn_attr.stVencAttr.u32VirWidth = SNAP_WIDTH;
  venc_chn_attr.stVencAttr.u32VirHeight = SNAP_HEIGHT;
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

static RK_CHAR optstr[] = "?a::v:s:r:p:t:f:m:I:M:b:g:A:N:W:P:D:w:h:d:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"data_version", required_argument, NULL, 'v'},
    {"runtime_path", required_argument, NULL, 'r'},
    {"photo_dirpath", required_argument, NULL, 'p'},
    {"time_log", required_argument, NULL, 't'},
    {"fps", required_argument, NULL, 'f'},
    {"hdr_mode", required_argument, NULL, 'm'},
    {"stress", required_argument, NULL, 's'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"help", no_argument, NULL, '?'},
    {"black_mode", required_argument, NULL, 'b'},
    {"filter", required_argument, NULL, 'g'},
    {"ftp_ip", required_argument, NULL, 'A'},
    {"ftp_name", required_argument, NULL, 'N'},
    {"ftp_passwd", required_argument, NULL, 'W'},
    {"ftp_port", required_argument, NULL, 'P'},
    {"draw_en", required_argument, NULL, 'D'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"device_name", required_argument, NULL, 'd'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]] "
         "[-I 0] "
         "[-M 0] "
         "[-w 1920] "
         "[-h 1080]"
         "[-d rkispp_scale0] "
         "[-v 0] "
         "[-r librknn_runtime.so] "
         "[-p pic_dir] "
         "[-f 10] "
         "[-m 0] "
         "[-b 0] "
         "[-g 0] "
         "[-A ftp_ip]"
         "[-N ftp_name]"
         "[-W ftp_passwd]"
         "[-P ftp_port]"
         "[-D 1]"
         "[-t 0]"
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
         "[-w 1920] "
         "[-h 1080]"
         "[-d rkispp_scale0] "
         "[-v 0] "
         "[-r librknn_runtime.so] "
         "[-p pic_dir] "
         "[-f 10] "
         "[-m 0] "
         "[-b 0] "
         "[-g 0] "
         "[-t 0]"
         "[-A ftp_ip]"
         "[-N ftp_name]"
         "[-W ftp_passwd]"
         "[-P ftp_port]"
         "[-D 1]"
         "\n",
         name);
#endif
  printf("\t-v | --data_version [0~3]; Default: 0\n"
         "\t\t-->0: ROCKX_MODULE_FACE_DETECTION_V2_HORIZONTAL\n"
         "\t\t-->1: ROCKX_MODULE_FACE_DETECTION_V3_LARGE"
         "\t\t-->2: ROCKX_MODULE_FACE_DETECTION_V2"
         "\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-w | --width: VI width, Default:1920\n");
  printf("\t-h | --heght: VI height, Default:1080\n");
  printf(
      "\t-d | --device_name: set device node(v4l2), Default:rkispp_scale0\n");
  printf("\t-r | --runtime_path, rknn runtime lib, Default: "
         "\"/usr/lib/librknn_runtime.so\"\n");
  printf("\t-p | --photo_dirpath, detect snap photo dirpath, Default: NULL, "
         "only when this path set, snapping is enabled.\n");
  printf("\t-f | --fps Default:10\n");
  printf("\t-m | --hdr_mode Default:0, set 1 to open hdr, set 0 to close\n");
  printf("\t-b | --black_mode ir mode Default:0, set 0 to display rgb, set 1 to "
         "display ir, set -1 to disabled\n");
  printf("\t-g | --filter set 1 to enabled landmark filter\n");
  printf("\t-t | --time_log set 0 to enabled time log\n");
  printf(
      "\t-D | --draw_en set 1 to enabled draw rect in mainstream, Default 1\n");
  printf("\t-A | --ftp_ip ftp ip\n");
  printf("\t-N | --ftp_name ftp name\n");
  printf("\t-W | --ftp_passwd ftp passwd\n");
  printf("\t-P | --ftp_port ftp port\n");
  printf(
      "\tonly when ftp_ip, ftp_name, ftp_passwd, ftp_port set, ftp enable\n");
  printf("\trstp: rtsp://<ip>/live/test\n");
}

#ifndef RKMEDIA_ROCKX_STRESS_TEST
int main(int argc, char **argv) {
  // init video cfg
  memset(&cfg, 0, sizeof(cfg));
  cfg.session_count = 1;
  cfg.session_cfg[RK_NN_INDEX].video_type = RK_CODEC_TYPE_H265;
  cfg.session_cfg[RK_NN_INDEX].u32Width = 1920;
  cfg.session_cfg[RK_NN_INDEX].u32Height = 1080;
  cfg.session_cfg[RK_NN_INDEX].enImageType = 4;
  sprintf(cfg.session_cfg[RK_NN_INDEX].videopath, "rkispp_scale0");
  sprintf(cfg.session_cfg[RK_NN_INDEX].path, "/live/test");
  RK_S32 ret = 0;
  RK_CHAR *pIqfilesPath = NULL;
  RK_CHAR *ftp_ip = NULL;
  RK_CHAR *ftp_name = "anonymous";
  RK_CHAR *ftp_pw = "anonymous@163.com";
  int ftp_port = -1;
  stream_arg_t stream_arg;
  memset(&stream_arg, 0, sizeof(stream_arg_t));
  sprintf(stream_arg.runtime_path, "/usr/lib/librknn_runtime.so");
  stream_arg.data_version = ROCKX_MODULE_FACE_DETECTION_V2_HORIZONTAL;
  stream_arg.draw_en = 1;
  stream_arg.filter_en = 1;
  RK_S32 s32CamId = 0;
  RK_BOOL bMultictx = RK_FALSE;
#ifdef RKAIQ
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
#endif
  int fps = 10;
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
    case 'w':
      cfg.session_cfg[RK_NN_INDEX].u32Width = atoi(optarg);
      break;
    case 'h':
      cfg.session_cfg[RK_NN_INDEX].u32Height = atoi(optarg);
      break;
    case 'd':
      memset(cfg.session_cfg[RK_NN_INDEX].videopath, 0, sizeof(cfg.session_cfg[RK_NN_INDEX].videopath));
      sprintf(cfg.session_cfg[RK_NN_INDEX].videopath, "%s", optarg);
      break;
    case 'b':
      stream_arg.ir_en = atoi(optarg);
      break;
    case 'v':
      switch (atoi(optarg)) {
      case 0:
        stream_arg.data_version = ROCKX_MODULE_FACE_DETECTION_V2_HORIZONTAL;
        break;
      case 1:
        stream_arg.data_version = ROCKX_MODULE_FACE_DETECTION_V3_LARGE;
        break;
      case 2:
        stream_arg.data_version = ROCKX_MODULE_FACE_DETECTION_V2;
        break;
      default:
        stream_arg.data_version = ROCKX_MODULE_FACE_DETECTION_V2_HORIZONTAL;
      }
      break;
    case 'r':
      memset(&stream_arg.runtime_path, 0, sizeof(stream_arg.runtime_path));
      sprintf(stream_arg.runtime_path, optarg);
      break;
    case 'p':
      g_photo_dirpath = optarg;
      break;
    case 't':
      stream_arg.time_log_en = atoi(optarg);
      break;
    case 'A':
      ftp_ip = optarg;
      break;
    case 'N':
      ftp_name = optarg;
      break;
    case 'W':
      ftp_pw = optarg;
      break;
    case 'P':
      ftp_port = atoi(optarg);
      break;
    case 'D':
      stream_arg.draw_en = atoi(optarg);
      break;
    case 'I':
      s32CamId = atoi(optarg);
      break;
    case 'f':
      fps = atoi(optarg);
      break;
#ifdef RKAIQ
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
      stream_arg.filter_en = atoi(optarg);
      stream_arg.filter_en_raw = stream_arg.filter_en;
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf(">>>>>>>>>>>>>>> Test START <<<<<<<<<<<<<<<<<<<<<<\n");
  dump_cfg();
#ifdef RKAIQ
  printf("####Aiq xml dirpath: %s\n\n", pIqfilesPath);
  printf("####hdr mode: %d\n\n", hdr_mode);
  printf("#####bMultictx: %d\n\n", bMultictx);
  printf("####ir_en: %d\n\n", stream_arg.ir_en);
#endif
  printf("####fps: %d\n\n", fps);
  printf("#####cam id: %d\n\n", s32CamId);
  printf("####data version: %d\n\n", stream_arg.data_version);
  printf("####runtime path: %s\n\n", stream_arg.runtime_path);
  printf("####photo dirpath: %s\n\n", g_photo_dirpath);
  printf("####detect time log en: %d\n\n", stream_arg.time_log_en);
  printf("####filter_en: %d\n\n", stream_arg.filter_en);
  printf("####ftp_ip: %s\n\n", ftp_ip);
  printf("####ftp_name: %s\n\n", ftp_name);
  printf("####ftp_passwd: %s\n\n", ftp_pw);
  printf("####ftp_port: %d\n\n", ftp_port);
  printf("####draw_en: %d\n\n", stream_arg.draw_en);

  signal(SIGINT, sig_proc);
  if (ftp_ip && (ftp_port != -1)) {
    g_ftp_flag = 1;
    ftp_init(ftp_ip, ftp_port, ftp_name, ftp_pw, "test");
  }
  if (pIqfilesPath) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pIqfilesPath);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
    if (stream_arg.ir_en == s32CamId) {
      SAMPLE_COMM_ISP_SET_OpenColorCloseLed(s32CamId);
    } else if (stream_arg.ir_en == 1) {
      SAMPLE_COMM_ISP_SET_GrayOpenLed(s32CamId, 100);
    }
#endif
  }
  rk_create_list(&g_snap_name_list, QUEUE_SIZE, sizeof(char) * SNAP_NAME_LEN);

  media_buffer_list_init(cfg.session_cfg[RK_NN_INDEX].u32Width,
                         cfg.session_cfg[RK_NN_INDEX].u32Height,
                         MEDIA_LIST_SIZE);
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
    ret = SAMPLE_COMMON_VENC_Start(&cfg.session_cfg[i], fps);
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
  ret = SAMPLE_COMMON_VENC_SNAP_Start(cfg.session_count);
  if (ret) {
    printf("snap venc start fail, ret is %d\n", ret);
    return -1;
  }

  pthread_t main_stream_thread;
  pthread_t filter_thread;
  pthread_create(&main_stream_thread, NULL, MainStream, &stream_arg);
  pthread_create(&filter_thread, NULL, RockxFilterThread, &stream_arg);

  signal(SIGINT, sig_proc);
#if 0
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
#if 0
    loop_cnt++;
    if (loop_cnt == 100) {
      GetRss(">>>>>>>>>>>>>>mem_test:");
      loop_cnt = 0;
    }
#endif
  }

  rtsp_del_demo(g_rtsplive);
  pthread_join(main_stream_thread, NULL);
  pthread_join(filter_thread, NULL);
  face_release_all();
  media_buffer_list_uninit();

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

  rk_destroy_list(&g_snap_name_list);
  return 0;
}
#endif