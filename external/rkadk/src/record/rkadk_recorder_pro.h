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

#ifndef __RKADK_RECORDER_PRO_H__
#define __RKADK_RECORDER_PRO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rkadk_record.h"
#include "rkadk_track_source.h"
#include "rkmedia_api.h"
#include "rkmedia_event.h"

#define RKADK_REC_STREAM_MAX_CNT RECORD_FILE_NUM_MAX
#define RKADK_REC_TRACK_MAX_CNT 2 /* a video track and a audio track */

typedef struct {
  MPP_CHN_S stVencChn;
  MPP_CHN_S stAencChn;
  MUXER_CHN_S stMuxerChn;
} RKADK_RECORDER_STREAM_ATTR_S;

typedef struct {
  RKADK_S32 s32CamId;
  RKADK_REC_TYPE_E enRecType;
  RKADK_U32 u32StreamCnt;
  RKADK_RECORDER_STREAM_ATTR_S stStreamAttr[RKADK_REC_STREAM_MAX_CNT];
} RKADK_RECORDER_HANDLE_S;

/* record split attribute param */
typedef struct {
  MUXER_MODE_E enMode;
  union {
    RKADK_CHAR *pcOutputFile;
    MUXER_SPLIT_ATTR_S stSplitAttr;
  };
} RKADK_REC_SPLIT_ATTR_S;

/* normal record attribute param */
typedef struct { RKADK_U32 u32Rsv; /* reserve */ } RKADK_REC_NORMAL_ATTR_S;

/* lapse record attribute param */
typedef struct {
  RKADK_U32
  u32IntervalMs; /* lapse record time interval, unit in millisecord(ms) */
} RKADK_REC_LAPSE_ATTR_S;

/* record stream attribute */
typedef struct {
  RKADK_U32 u32TimeLenSec; /* record time */
  RKADK_U32 u32TrackCnt; /* track cnt*/
  RKADK_TRACK_SOURCE_S
  aHTrackSrcHandle[RKADK_REC_TRACK_MAX_CNT]; /* array of track source cnt */
  MUXER_TYPE_E enType;
} RKADK_REC_STREAM_ATTR_S;

typedef struct {
  RKADK_U32 u32PreRecTimeSec; /* pre record time, unit in second(s)*/
  RK_U32 u32PreRecCacheTime;
  MUXER_PRE_RECORD_MODE_E enPreRecordMode;
} RKADK_PRE_RECORD_ATTR_S;

/* record attribute param */
typedef struct {
  RKADK_REC_TYPE_E enRecType; /* record type */
  union {
    RKADK_REC_NORMAL_ATTR_S stNormalRecAttr; /* normal record attribute */
    RKADK_REC_LAPSE_ATTR_S stLapseRecAttr;   /* lapse record attribute */
  } unRecAttr;

  RKADK_REC_SPLIT_ATTR_S stRecSplitAttr; /* record split attribute */

  RKADK_U32 u32StreamCnt; /* stream cnt */
  RKADK_REC_STREAM_ATTR_S
  astStreamAttr[RKADK_REC_STREAM_MAX_CNT]; /* array of stream attr */
  RKADK_PRE_RECORD_ATTR_S stPreRecordAttr;
} RKADK_REC_ATTR_S;

/**
 * @brief create a new recorder
 * @param[in]pstRecAttr : the attribute of recorder
 * @param[out]ppRecorder : pointer of recorder
 * @return 0 success
 * @return others failure
 */
RKADK_S32 RKADK_REC_Create(RKADK_REC_ATTR_S *pstRecAttr,
                           RKADK_MW_PTR *ppRecorder, RKADK_S32 s32CamId);

/**
 * @brief destory a recorder.
 * @param[in]pRecorder : pointer of recorder
 * @return 0 success
 * @return others failure
 */
RKADK_S32 RKADK_REC_Destroy(RKADK_MW_PTR pRecorder);

/**
 * @brief start recorder
 * @param[in]pRecorder, pointer of recorder
 * @return 0 success
 * @return -1 failure
 */
RKADK_S32 RKADK_REC_Start(RKADK_MW_PTR pRecorder);

/**
 * @brief stop recorder
 * @param[in]pRecorder : pointer of recorder
 * @param[in]bQuickMode : quick stop mode flag.
 * @return 0 success
 * @return others failure
 */
RKADK_S32 RKADK_REC_Stop(RKADK_MW_PTR pRecorder);

/**
 * @brief manual splite file.
 * @param[in]pRecorder : pointer of recorder
 * @param[in]pstSplitAttr : manual split attr.
 * @return 0 success
 * @return others failure
 */
RKADK_S32 RKADK_REC_ManualSplit(RKADK_MW_PTR pRecorder,
                                RKADK_REC_MANUAL_SPLIT_ATTR_S *pstSplitAttr);

/**
 * @brief register recorder envent callback
 * @param[in]pRecorder : pointer of recorder
 * @param[in]pfnEventCallback : callback function
 * @return 0 success
 * @return others failure
 */
RKADK_S32
RKADK_REC_RegisterEventCallback(RKADK_MW_PTR pRecorder,
                                RKADK_REC_EVENT_CALLBACK_FN pfnEventCallback);

#ifdef __cplusplus
}
#endif
#endif
