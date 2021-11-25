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

#ifndef __RKADK_PARAM_H__
#define __RKADK_PARAM_H__

#include "rkadk_param_inner.h"

#ifdef __cplusplus
extern "C" {
#endif

/* version */
#define RKADK_PARAM_VERSION "1.1.0"

/* sensor default parameters */
#define SENSOR_MAX_WIDTH 2688
#define SENSOR_MAX_HEIGHT 1520

/* audio default parameters */
/* g711u must be 16K, g711a can be either 8K or 16K */
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_CHANNEL 1
#define AUDIO_BIT_REAT 64000
#define AUDIO_FRAME_COUNT 1024
#define AUDIO_SAMPLE_FORMAT RK_SAMPLE_FMT_S16
#define AI_DEVICE_NAME "default"

/* video default parameters */
#define VIDEO_GOP 30
#define VIDEO_FRAME_RATE 30
#define VIDEO_PROFILE 100
#define STREAM_VIDEO_WIDTH 1280
#define STREAM_VIDEO_HEIGHT 720
#define RECORD_VIDEO_WIDTH SENSOR_MAX_WIDTH
#define RECORD_VIDEO_HEIGHT SENSOR_MAX_HEIGHT
#define RECORD_VIDEO_WIDTH_S STREAM_VIDEO_WIDTH
#define RECORD_VIDEO_HEIGHT_S STREAM_VIDEO_HEIGHT
#define PHOTO_VIDEO_WIDTH SENSOR_MAX_WIDTH
#define PHOTO_VIDEO_HEIGHT SENSOR_MAX_HEIGHT
#define DISP_WIDTH 720
#define DISP_HEIGHT 1280

/* thumb default parameters */
#define THUMB_WIDTH 320
#define THUMB_HEIGHT 180
#define THUMB_VENC_CHN (VENC_MAX_CHN_NUM - 1)

/* setting file path */
#define RKADK_PARAM_PATH "/data/rkadk_setting.ini"
#define RKADK_DEFPARAM_PATH "/etc/rkadk_defsetting.ini"

/* Resolution */
#define RKADK_WIDTH_720P 1280
#define RKADK_HEIGHT_720P 720

#define RKADK_WIDTH_1080P 1920
#define RKADK_HEIGHT_1080P 1080

#define RKADK_WIDTH_1296P 2340
#define RKADK_HEIGHT_1296P 1296

#define RKADK_WIDTH_1440P 2560
#define RKADK_HEIGHT_1440P 1440

#define RKADK_WIDTH_1520P 2688
#define RKADK_HEIGHT_1520P 1520

#define RKADK_WIDTH_1600P 2560
#define RKADK_HEIGHT_1600P 1600

#define RKADK_WIDTH_1620P 2880
#define RKADK_HEIGHT_1620P 1620

#define RKADK_WIDTH_1944P 2592
#define RKADK_HEIGHT_1944P 1944

#define RKADK_WIDTH_3840P 3840
#define RKADK_HEIGHT_2160P 2160

/* Resolution type */
typedef enum {
  RKADK_RES_720P = 0, /* 1280*720 */
  RKADK_RES_1080P,    /* 1920*1080 */
  RKADK_RES_1296P,    /* 2340*1296 */
  RKADK_RES_1440P,    /* 2560*1440 */
  RKADK_RES_1520P,    /* 2688*1520 */
  RKADK_RES_1600P,    /* 2560*1600 */
  RKADK_RES_1620P,    /* 2880*1620 */
  RKADK_RES_1944P,    /* 2592*1944 */
  RKADK_RES_2160P,    /* 3840*2160 */
  RKADK_RES_BUTT,
} RKADK_PARAM_RES_E;

/* config param */
typedef enum {
  // CAM Dependent Param
  RKADK_PARAM_TYPE_FPS,             /* framerate */
  RKADK_PARAM_TYPE_RES,             /* specify RKADK_PARAM_RES_E(record) */
  RKADK_PARAM_TYPE_PHOTO_RES,       /* specify RKADK_PARAM_RES_E(photo) */
  RKADK_PARAM_TYPE_CODEC_TYPE,      /* specify RKADK_PARAM_CODEC_CFG_S */
  RKADK_PARAM_TYPE_BITRATE,         /* specify RKADK_PARAM_BITRATE_S */
  RKADK_PARAM_TYPE_FLIP,            /* bool */
  RKADK_PARAM_TYPE_MIRROR,          /* bool */
  RKADK_PARAM_TYPE_LDC,             /* ldc level [0,255] */
  RKADK_PARAM_TYPE_ANTIFOG,         /* antifog value, [0,10] */
  RKADK_PARAM_TYPE_WDR,             /* wdr level, [0,10] */
  RKADK_PARAM_TYPE_HDR,             /* 0: normal, 1: HDR2, 2: HDR3, [0,2] */
  RKADK_PARAM_TYPE_REC,             /* record  enable, bool*/
  RKADK_PARAM_TYPE_RECORD_TYPE,     /* specify RKADK_REC_TYPE_E */
  RKADK_PARAM_TYPE_RECORD_TIME,     /* specify RKADK_PARAM_REC_TIME_S, record time(s) */
  RKADK_PARAM_TYPE_PRE_RECORD_TIME, /* pre record time, unit in second(s) */
  RKADK_PARAM_TYPE_PRE_RECORD_MODE, /* pre record mode, specify MUXER_PRE_RECORD_MODE_E */
  RKADK_PARAM_TYPE_SPLITTIME, /* specify RKADK_PARAM_REC_TIME_S, manual splite time(s) */
  RKADK_PARAM_TYPE_FILE_CNT,  /* record file count, maximum RECORD_FILE_NUM_MAX */
  RKADK_PARAM_TYPE_LAPSE_INTERVAL, /* specify RKADK_PARAM_REC_TIME_S, lapse interval(s) */
  RKADK_PARAM_TYPE_LAPSE_MULTIPLE, /* lapse multiple */
  RKADK_PARAM_TYPE_PHOTO_ENABLE,   /* photo enable, bool*/
  RKADK_PARAM_TYPE_SNAP_NUM,       /* continue snap num */

  // COMM Dependent Param
  RKADK_PARAM_TYPE_REC_UNMUTE,      /* record audio mute, bool */
  RKADK_PARAM_TYPE_AUDIO,           /* speaker enable, bool */
  RKADK_PARAM_TYPE_VOLUME,          /* speaker volume, [0,100] */
  RKADK_PARAM_TYPE_MIC_UNMUTE,      /* mic(mute) enable, bool */
  RKADK_PARAM_TYPE_MIC_VOLUME,      /* mic volume, [0,100] */
  RKADK_PARAM_TYPE_OSD,             /* show osd or not, bool */
  RKADK_PARAM_TYPE_OSD_TIME_FORMAT, /* osd format for time */
  RKADK_PARAM_TYPE_BOOTSOUND,       /* boot sound enable, bool */
  RKADK_PARAM_TYPE_OSD_SPEED,       /* speed osd enable, bool */
  RKADK_PARAM_TYPE_BUTT
} RKADK_PARAM_TYPE_E;

typedef struct {
  RKADK_STREAM_TYPE_E enStreamType;
  RKADK_CODEC_TYPE_E enCodecType;
} RKADK_PARAM_CODEC_CFG_S;

typedef struct {
  RKADK_STREAM_TYPE_E enStreamType;
  RKADK_U32 u32Bitrate;
} RKADK_PARAM_BITRATE_S;

typedef struct {
  RKADK_STREAM_TYPE_E enStreamType;
  RKADK_U32 time;
} RKADK_PARAM_REC_TIME_S;

/**
 * @brief     Parameter Module Init
 * @return    0 success,non-zero error code.
 */
RKADK_S32 RKADK_PARAM_Init(RKADK_VOID);

/**
 * @brief     Parameter Module Deinit
 * @return    0 success,non-zero error code.
 */
RKADK_S32 RKADK_PARAM_Deinit(RKADK_VOID);

/**
* @brief         get cam related Item Values.
* @param[in]     s32CamID: the specify cam id,valid value range:
* [0,RKADK_PDT_MEDIA_VCAP_DEV_MAX_CNT]
* @param[in]     enType: param type
* @param[in]     pvParam: param value
* @return 0      success,non-zero error code.
*/
RKADK_S32 RKADK_PARAM_GetCamParam(RKADK_S32 s32CamID,
                                  RKADK_PARAM_TYPE_E enParamType,
                                  RKADK_VOID *pvParam);

/**
* @brief         set cam related Item Values.
* @param[in]     enWorkMode: workmode
* @param[in]     s32CamID: the specify cam id,valid value range:
* [0,RKADK_PDT_MEDIA_VCAP_DEV_MAX_CNT]
* @param[in]     enType: param type
* @param[in]     pvParam: param value
* @return        0 success,non-zero error code.
*/
RKADK_S32 RKADK_PARAM_SetCamParam(RKADK_S32 s32CamID,
                                  RKADK_PARAM_TYPE_E enParamType,
                                  const RKADK_VOID *pvParam);

/**
 * @brief        get common parameter configure
 * @param[in]    enParamType : param type
 * @param[out]   pvParam : param value
 * @return       0 success,non-zero error code.
 */
RKADK_S32 RKADK_PARAM_GetCommParam(RKADK_PARAM_TYPE_E enParamType,
                                   RKADK_VOID *pvParam);

/**
 * @brief        set common parameter configure
 * @param[in]    enParamType : param type
 * @param[out]   pvParam : param value
 * @return       0 success,non-zero error code.
 */
RKADK_S32 RKADK_PARAM_SetCommParam(RKADK_PARAM_TYPE_E enParamType,
                                   const RKADK_VOID *pvParam);

/**
 * @brief        defualt all parameters configure
 * @return       0 success,non-zero error code.
 */
RKADK_S32 RKADK_PARAM_SetDefault(RKADK_VOID);

/**
 * @brief        RKADK_PARAM_RES_E to width and height
 * @return       0 success,non-zero error code.
 */
RKADK_S32 RKADK_PARAM_GetResolution(RKADK_PARAM_RES_E type, RKADK_U32 *width,
                                    RKADK_U32 *height);

/**
 * @brief        width and height to RKADK_PARAM_RES_E
 * @return       0 success,non-zero error code.
 */
RKADK_PARAM_RES_E RKADK_PARAM_GetResType(RKADK_U32 width, RKADK_U32 height);

/**
 * @brief        get venc chn id
 * @return       venc chn id success,-1 error code.
 */
RKADK_S32 RKADK_PARAM_GetVencChnId(RKADK_U32 u32CamId,
                                   RKADK_STREAM_TYPE_E enStrmType);

#ifdef __cplusplus
}
#endif
#endif
