/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef __RKADK_PARAM_INNER_H__
#define __RKADK_PARAM_INNER_H__

#include "rkadk_common.h"
#include "rkadk_media_comm.h"
#include "rkadk_record.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RKADK_BUFFER_LEN 64
#define RKADK_PIX_FMT_LEN 10
#define RKADK_RC_MODE_LEN 5

#define RECORD_AUDIO_CODEC_TYPE RKADK_CODEC_TYPE_MP3
#define RECORD_AI_CHN 0
#define RECORD_AENC_CHN 0

#define PREVIEW_AI_CHN RECORD_AI_CHN
#define PREVIEW_AENC_CHN 1

#define LIVE_AI_CHN RECORD_AI_CHN
#define LIVE_AENC_CHN 2
#define LIVE_AUDIO_CODEC_TYPE RECORD_AUDIO_CODEC_TYPE

typedef enum {
  RKADK_STREAM_TYPE_VIDEO_MAIN,
  RKADK_STREAM_TYPE_VIDEO_SUB,
  RKADK_STREAM_TYPE_SNAP,
  RKADK_STREAM_TYPE_PREVIEW,
  RKADK_STREAM_TYPE_LIVE,
  RKADK_STREAM_TYPE_DISP,
  RKADK_STREAM_TYPE_BUTT
} RKADK_STREAM_TYPE_E;

typedef struct tagRKADK_PARAM_VERSION_S {
  char version[RKADK_BUFFER_LEN];
} RKADK_PARAM_VERSION_S;

typedef struct tagRKADK_PARAM_VI_CFG_S {
  RKADK_U32 chn_id;
  char device_name[RKADK_BUFFER_LEN];
  RKADK_U32 buf_cnt;
  RKADK_U32 width;
  RKADK_U32 height;
  char pix_fmt[RKADK_PIX_FMT_LEN]; /* options: NV12/NV16/YUYV/FBC0/FBC2 */
  char module
      [RKADK_BUFFER_LEN]; /* NONE/RECORD_MAIN/RECORD_SUB/PREVIEW/PHOTO/LIVE */
} RKADK_PARAM_VI_CFG_S;

typedef struct tagRKADK_PARAM_COMM_CFG_S {
  RKADK_U32 sensor_count;
  bool rec_unmute;          /* false:disable record audio, true:enable */
  bool enable_speaker;      /* speaker enable, default true */
  RKADK_U32 speaker_volume; /* speaker volume, [0,100] */
  bool mic_unmute;          /* 0:close mic(mute),  1:open mic(unmute) */
  RKADK_U32 mic_volume;     /* mic input volume, [0,100] */
  RKADK_U32 osd_time_format;
  bool osd;        /* Whether to display OSD */
  bool boot_sound; /* boot sound */
  bool osd_speed;  /* speed osd */
} RKADK_PARAM_COMM_CFG_S;

typedef struct tagRKADK_PARAM_SENSOR_CFG_S {
  RKADK_U32 max_width;
  RKADK_U32 max_height;
  RKADK_U32 framerate;
  bool enable_record; /* record  enable*/
  bool enable_photo;  /* photo enable, default true */
  bool flip;          /* FLIP */
  bool mirror;        /* MIRROR */
  RKADK_U32 ldc;      /* LDC level, [0,255]*/
  RKADK_U32 wdr;      /* WDR level, [0,10] */
  RKADK_U32 hdr;      /* hdr, [0: normal, 1: HDR2, 2: HDR3] */
  RKADK_U32 antifog;  /* antifog value, [0,10] */
} RKADK_PARAM_SENSOR_CFG_S;

typedef struct tagRKADK_PARAM_AUDIO_CFG_S {
  char audio_node[RKADK_BUFFER_LEN];
  SAMPLE_FORMAT_E sample_format;
  RKADK_U32 channels;
  RKADK_U32 samplerate;
  RKADK_U32 samples_per_frame;
  RKADK_U32 bitrate;
  AI_LAYOUT_E ai_layout;
  RKADK_VQE_MODE_E vqe_mode;
} RKADK_PARAM_AUDIO_CFG_S;

typedef struct tagRKADK_PARAM_VENC_PARAM_S {
  /* rc param */
  RKADK_S32 first_frame_qp; /* start QP value of the first frame, default: -1 */
  RKADK_U32 qp_step;
  RKADK_U32 max_qp; /* max QP: [8, 51], default: 48 */
  RKADK_U32 min_qp; /* min QP: [0, 48], can't be larger than max_qp, default: 8 */
  RKADK_U32 row_qp_delta_i; /* only CBR, [0, 10], default: 1 */
  RKADK_U32 row_qp_delta_p; /* only CBR, [0, 10], default: 2 */
  bool hier_qp_en;
  char hier_qp_delta[RKADK_BUFFER_LEN];
  char hier_frame_num[RKADK_BUFFER_LEN];

  bool full_range;
  bool scaling_list;
} RKADK_PARAM_VENC_PARAM_S;

typedef struct tagRKADK_PARAM_VENC_ATTR_S {
  RKADK_U32 width;
  RKADK_U32 height;
  RKADK_U32 bitrate;
  RKADK_U32 gop;
  RKADK_U32 profile;
  RKADK_CODEC_TYPE_E codec_type;
  RKADK_U32 venc_chn;
  char rc_mode[RKADK_RC_MODE_LEN]; /* options: CBR/VBR/AVBR */
  RKADK_PARAM_VENC_PARAM_S venc_param;
} RKADK_PARAM_VENC_ATTR_S;

typedef struct {
  RKADK_U32 u32ViChn;
  VI_CHN_ATTR_S stChnAttr;
} RKADK_PRAAM_VI_ATTR_S;

typedef struct tagRKADK_PARAM_REC_TIME_CFG_S {
  RKADK_U32 record_time;
  RKADK_U32 splite_time;
  RKADK_U32 lapse_interval;
} RKADK_PARAM_REC_TIME_CFG_S;

typedef struct tagRKADK_PARAM_REC_CFG_S {
  RKADK_REC_TYPE_E record_type;
  RKADK_U32 pre_record_time;
  MUXER_PRE_RECORD_MODE_E pre_record_mode;
  RKADK_U32 lapse_multiple;
  RKADK_U32 file_num;
  RKADK_PARAM_REC_TIME_CFG_S record_time_cfg[RECORD_FILE_NUM_MAX];
  RKADK_PARAM_VENC_ATTR_S attribute[RECORD_FILE_NUM_MAX];
  RKADK_PRAAM_VI_ATTR_S vi_attr[RECORD_FILE_NUM_MAX];
} RKADK_PARAM_REC_CFG_S;

typedef struct tagRKADK_PARAM_STREAM_CFG_S {
  RKADK_PARAM_VENC_ATTR_S attribute;
  RKADK_PRAAM_VI_ATTR_S vi_attr;
} RKADK_PARAM_STREAM_CFG_S;

typedef struct tagRKADK_PARAM_PHOTO_CFG_S {
  RKADK_U32 image_width;
  RKADK_U32 image_height;
  RKADK_U32 snap_num;
  RKADK_U32 venc_chn;
  RKADK_PRAAM_VI_ATTR_S vi_attr;
} RKADK_PARAM_PHOTO_CFG_S;

typedef struct tagRKADK_PARAM_DISP_CFG_S {
  RKADK_U32 width;
  RKADK_U32 height;
  // rga
  bool enable_buf_pool;
  RKADK_U32 buf_pool_cnt;
  RKADK_U32 rotaion;
  RKADK_U32 rga_chn;
  // vo
  char device_node[RKADK_BUFFER_LEN];
  VO_PLANE_TYPE_E plane_type;
  char img_type[RKADK_PIX_FMT_LEN]; /* specify IMAGE_TYPE_E: NV12/RGB888... */
  RKADK_U32 z_pos;
  RKADK_U32 vo_chn;
  RKADK_PRAAM_VI_ATTR_S vi_attr;
} RKADK_PARAM_DISP_CFG_S;

typedef struct tagRKADK_PARAM_MEDIA_CFG_S {
  RKADK_PARAM_VI_CFG_S stViCfg[RKADK_ISPP_VI_NODE_CNT];
  RKADK_PARAM_REC_CFG_S stRecCfg;
  RKADK_PARAM_STREAM_CFG_S stStreamCfg;
  RKADK_PARAM_STREAM_CFG_S stLiveCfg;
  RKADK_PARAM_PHOTO_CFG_S stPhotoCfg;
  RKADK_PARAM_DISP_CFG_S stDispCfg;
} RKADK_PARAM_MEDIA_CFG_S;

typedef struct tagRKADK_PARAM_THUMB_CFG_S {
  RKADK_U32 thumb_width;
  RKADK_U32 thumb_height;
  RKADK_U32 venc_chn;
} RKADK_PARAM_THUMB_CFG_S;

typedef struct tagPARAM_CFG_S {
  RKADK_PARAM_VERSION_S stVersion;
  RKADK_PARAM_COMM_CFG_S stCommCfg;
  RKADK_PARAM_AUDIO_CFG_S stAudioCfg;
  RKADK_PARAM_THUMB_CFG_S stThumbCfg;
  RKADK_PARAM_SENSOR_CFG_S stSensorCfg[RKADK_MAX_SENSOR_CNT];
  RKADK_PARAM_MEDIA_CFG_S stMediaCfg[RKADK_MAX_SENSOR_CNT];
} RKADK_PARAM_CFG_S;

/* Param Context */
typedef struct {
  bool bInit;                /* module init status */
  pthread_mutex_t mutexLock; /* param lock, protect pstCfg */
  RKADK_PARAM_CFG_S stCfg;   /* param config */
} RKADK_PARAM_CONTEXT_S;

RKADK_PARAM_CONTEXT_S *RKADK_PARAM_GetCtx(RKADK_VOID);

RKADK_PARAM_COMM_CFG_S *RKADK_PARAM_GetCommCfg();

RKADK_PARAM_REC_CFG_S *RKADK_PARAM_GetRecCfg(RKADK_U32 u32CamId);

RKADK_PARAM_STREAM_CFG_S *
RKADK_PARAM_GetStreamCfg(RKADK_U32 u32CamId, RKADK_STREAM_TYPE_E enStrmType);

RKADK_PARAM_PHOTO_CFG_S *RKADK_PARAM_GetPhotoCfg(RKADK_U32 u32CamId);

RKADK_PARAM_SENSOR_CFG_S *RKADK_PARAM_GetSensorCfg(RKADK_U32 u32CamId);

RKADK_PARAM_DISP_CFG_S *RKADK_PARAM_GetDispCfg(RKADK_U32 u32CamId);

RKADK_PARAM_AUDIO_CFG_S *RKADK_PARAM_GetAudioCfg(RKADK_VOID);

RKADK_PARAM_THUMB_CFG_S *RKADK_PARAM_GetThumbCfg(RKADK_VOID);

VENC_RC_MODE_E RKADK_PARAM_GetRcMode(char *rcMode,
                                     RKADK_CODEC_TYPE_E enCodecType);

RKADK_S32 RKADK_PARAM_GetRcParam(RKADK_PARAM_VENC_ATTR_S stVencAttr,
                                 VENC_RC_PARAM_S *pstRcParam);

RKADK_STREAM_TYPE_E RKADK_PARAM_VencChnMux(RKADK_U32 u32CamId,
                                           RKADK_U32 u32ChnId);

IMAGE_TYPE_E RKADK_PARAM_GetPixFmt(char *pixFmt);

#ifdef __cplusplus
}
#endif
#endif
