// Copyright 2021 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_VIDEO_MUXER_
#define __RKMEDIA_VIDEO_MUXER_

#ifdef __cplusplus
extern "C" {
#endif

#include "rkmedia_common.h"

#define MUXER_MAX_CHN_NUM 16
#define MUXER_FILE_NAME_LEN 256

typedef enum rkMUXER_CHN_TYPE_E {
  MUXER_CHN_TYPE_VIDEO = 0,
  MUXER_CHN_TYPE_AUDIO,
  MUXER_CHN_TYPE_BUTT
} MUXER_CHN_TYPE_E;

typedef struct rkMUXER_CHN_S {
  MOD_ID_E enModId;
  MUXER_CHN_TYPE_E enChnType;
  RK_S32 s32ChnId;
} MUXER_CHN_S;

typedef enum rkMUXER_EVENT_E {
  MUXER_EVENT_STREAM_START = 0,
  MUXER_EVENT_STREAM_STOP,
  MUXER_EVENT_FILE_BEGIN,
  MUXER_EVENT_FILE_END,
  MUXER_EVENT_MANUAL_SPLIT_END,
  MUXER_EVENT_ERR_CREATE_FILE_FAIL,
  MUXER_EVENT_ERR_WRITE_FILE_FAIL,
  MUXER_EVENT_BUTT
} MUXER_EVENT_E;

typedef struct rkMUXER_FILE_EVENT_INFO_S {
  RK_CHAR asFileName[MUXER_FILE_NAME_LEN];
  RK_U32 u32Duration; // ms
} MUXER_FILE_EVENT_INFO_S;

typedef struct rkMUXER_ERROR_EVENT_INFO_S {
  RK_CHAR asFileName[MUXER_FILE_NAME_LEN];
  RK_S32 s32ErrorCode;
} MUXER_ERROR_EVENT_INFO_S;

typedef struct rkMUXER_EVENT_INFO_S {
  MUXER_EVENT_E enEvent;
  union {
    MUXER_FILE_EVENT_INFO_S stFileInfo;
    MUXER_ERROR_EVENT_INFO_S stErrorInfo;
  } unEventInfo;
} MUXER_EVENT_INFO_S;

/* Muxer event callback function */
typedef RK_S32 (*MUXER_REQUEST_EVENT_CB)(
    RK_VOID *pHandle, const MUXER_EVENT_INFO_S *pstEventInfo);
/* Muxer request file name callback function */
typedef int (*MUXER_REQUEST_FILE_NAME_CB)(RK_VOID *pHandle, RK_CHAR *file_name,
                                          RK_U32 muxer_id);

typedef enum rkMUXER_SPLIT_TYPE_E {
  MUXER_SPLIT_TYPE_TIME = 0, /* split when time reaches */
  MUXER_SPLIT_TYPE_SIZE,     /* TODO: split when size reaches */
  MUXER_SPLIT_TYPE_BUTT
} MUXER_SPLIT_TYPE_E;

typedef enum rkMUXER_SPLIT_NAME_TYPE_E {
  MUXER_SPLIT_NAME_TYPE_AUTO = 0,
  MUXER_SPLIT_NAME_TYPE_CALLBACK,
  MUXER_SPLIT_NAME_TYPE_BUTT
} MUXER_SPLIT_NAME_TYPE_E;

typedef struct rkMUXER_NAME_AUTO_S {
  RK_BOOL bTimeStampEnable; /* Auto name with time stamp */
  RK_U16 u16StartIdx;
  RK_CHAR *pcPrefix;
  RK_CHAR *pcBaseDir;
} MUXER_NAME_AUTO_S;

typedef struct rkMUXER_NAME_CALLBCAK_S {
  MUXER_REQUEST_FILE_NAME_CB pcbRequestFileNames;
  RK_VOID *pCallBackHandle;
} MUXER_NAME_CALLBCAK_S;

/* muxer split attribute param */
typedef struct rkMUXER_SPLIT_ATTR_S {
  MUXER_SPLIT_TYPE_E enSplitType; /* split type */
  RK_U32 u32SizeLenByte;          /* split size, unit in byte(s) */
  RK_U32 u32TimeLenSec;           /* split time, unit in second(s) */

  MUXER_SPLIT_NAME_TYPE_E enSplitNameType;
  union {
    MUXER_NAME_AUTO_S stNameAutoAttr;
    MUXER_NAME_CALLBCAK_S stNameCallBackAttr;
  };
} MUXER_SPLIT_ATTR_S;

typedef enum rkMUXER_TYPE_E {
  MUXER_TYPE_MP4 = 0,
  MUXER_TYPE_MPEGTS,
  MUXER_TYPE_FLV,
  MUXER_TYPE_BUTT
} MUXER_TYPE_E;

typedef enum rkMUXER_MODE_E {
  MUXER_MODE_SINGLE = 0,
  MUXER_MODE_AUTOSPLIT,
  MUXER_MODE_BUTT
} MUXER_MODE_E;

typedef struct rkMUXER_VIDEO_STREAM_PARAM_S {
  CODEC_TYPE_E enCodecType; // the type of encodec
  IMAGE_TYPE_E enImageType; // the type of input image
  RK_U32 u32Width;
  RK_U32 u32Height;
  RK_U32 u32BitRate;
  RK_U16 u16Fps;
  RK_U16 u16Profile;
  RK_U16 u16Level;
} MUXER_VIDEO_STREAM_PARAM_S;

typedef struct rkMUXER_AUDIO_STREAM_PARAM_S {
  CODEC_TYPE_E enCodecType;  // the type of encodec
  SAMPLE_FORMAT_E enSampFmt; // audio sample formate
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
  RK_U32 u32NbSamples;
} MUXER_AUDIO_STREAM_PARAM_S;

typedef enum rkMUXER_PRE_RECORD_TYPE_E {
  MUXER_PRE_RECORD_NONE = 0,
  MUXER_PRE_RECORD_MANUAL_SPLIT,
  MUXER_PRE_RECORD_SINGLE,
  MUXER_PRE_RECORD_NORMAL
} MUXER_PRE_RECORD_MODE_E;

typedef struct rkMUXER_PRE_RECORD_PARAM_S {
  // pre record time
  RK_U32 u32PreRecTimeSec;
  // pre record time(s)
  RK_U32 u32PreRecCacheTime;
  MUXER_PRE_RECORD_MODE_E enPreRecordMode;
} MUXER_PRE_RECORD_PARAM_S;

typedef struct rkMUXER_CHN_ATTR_S {
  MUXER_MODE_E enMode;
  MUXER_TYPE_E enType;
  union {
    RK_CHAR *pcOutputFile;
    MUXER_SPLIT_ATTR_S stSplitAttr;
  };

  // video stream params
  MUXER_VIDEO_STREAM_PARAM_S stVideoStreamParam;
  // audio stream params
  MUXER_AUDIO_STREAM_PARAM_S stAudioStreamParam;

  // pre record params
  MUXER_PRE_RECORD_PARAM_S stPreRecordParam;

  RK_U32 u32MuxerId;
  RK_BOOL bLapseRecord;
} MUXER_CHN_ATTR_S;

/* record manual split type enum */
typedef enum {
  MUXER_POST_MANUAL_SPLIT = 0, /* post maunal split type */
  MUXER_PRE_MANUAL_SPLIT,      /* pre manual split type */
  MUXER_NORMAL_MANUAL_SPLIT,   /* normal manual split type */
  MUXER_MANUAL_SPLIT_BUTT
} MUXER_MANUAL_SPLIT_TYPE_E;

/* post manual split attribute */
typedef struct {
  RK_U32 u32AfterSec; /* manual split file will end after u32AfterSec */
} MUXER_POST_MANUAL_SPLIT_ATTR_S;

/* pre manual split attribute */
typedef struct {
  RK_U32 u32DurationSec; /* file duration of manual split file */
} MUXER_PRE_MANUAL_SPLIT_ATTR_S;

/* normal manual split attribute */
typedef struct {
  RK_U32 u32Rsv; /* reserve */
} MUXER_NORMAL_MANUAL_SPLIT_ATTR_S;

/* pre manual split attribute */
typedef struct {
  MUXER_MANUAL_SPLIT_TYPE_E enManualType; /* maual split type */
  union {
    MUXER_POST_MANUAL_SPLIT_ATTR_S stPostSplitAttr; /* post manual split attr */
    MUXER_PRE_MANUAL_SPLIT_ATTR_S stPreSplitAttr;   /* pre manual split attr */
    MUXER_NORMAL_MANUAL_SPLIT_ATTR_S
    stNormalSplitAttr; /* normal manual split attr */
  };
} MUXER_MANUAL_SPLIT_ATTR_S;

#ifdef __cplusplus
}
#endif

#endif // #ifndef __RKMEDIA_VIDEO_MUXER_
