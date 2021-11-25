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

#include "rkadk_record.h"
#include "rkadk_log.h"
#include "rkadk_media_comm.h"
#include "rkadk_param.h"
#include "rkadk_recorder_pro.h"
#include "rkadk_track_source.h"
#include "rkmedia_api.h"
#include <deque>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static RKADK_REC_REQUEST_FILE_NAMES_FN
    g_pfnRequestFileNames[RKADK_MAX_SENSOR_CNT] = {NULL};
static std::deque<char *> g_fileNameDeque[RKADK_REC_STREAM_MAX_CNT];
static pthread_mutex_t g_fileNameMutexLock = PTHREAD_MUTEX_INITIALIZER;

static int GetRecordFileName(RKADK_VOID *pHandle, RKADK_CHAR *pcFileName,
                             RKADK_U32 muxerId) {
  int i, ret;
  RKADK_RECORDER_HANDLE_S *pstRecorder;

  RKADK_MUTEX_LOCK(g_fileNameMutexLock);

  if (muxerId >= RKADK_REC_STREAM_MAX_CNT) {
    RKADK_LOGE("Incorrect file index: %d", muxerId);
    RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
    return -1;
  }

  pstRecorder = (RKADK_RECORDER_HANDLE_S *)pHandle;
  if (!pstRecorder) {
    RKADK_LOGE("pstRecorder is null");
    RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
    return -1;
  }

  if (!g_pfnRequestFileNames[pstRecorder->s32CamId]) {
    RKADK_LOGE("Not Registered request name callback");
    RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
    return -1;
  }

  if (g_fileNameDeque[muxerId].empty()) {
    ARRAY_FILE_NAME fileName;
    fileName = (ARRAY_FILE_NAME)malloc(pstRecorder->u32StreamCnt *
                                       RKADK_MAX_FILE_PATH_LEN);
    ret = g_pfnRequestFileNames[pstRecorder->s32CamId](
        pHandle, pstRecorder->u32StreamCnt, fileName);
    if (ret) {
      RKADK_LOGE("get file name failed(%d)", ret);
      RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
      return -1;
    }

    char *newName[pstRecorder->u32StreamCnt];
    for (i = 0; i < (int)pstRecorder->u32StreamCnt; i++) {
      newName[i] = strdup(fileName[i]);
      g_fileNameDeque[i].push_back(newName[i]);
    }
    free(fileName);
  }

  auto front = g_fileNameDeque[muxerId].front();
  memcpy(pcFileName, front, strlen(front));
  g_fileNameDeque[muxerId].pop_front();
  free(front);

  RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
  return 0;
}

static void RKADK_RECORD_SetVideoChn(int index,
                                     RKADK_PARAM_REC_CFG_S *pstRecCfg,
                                     MPP_CHN_S *pstViChn,
                                     MPP_CHN_S *pstVencChn) {
  pstViChn->enModId = RK_ID_VI;
  pstViChn->s32DevId = 0;
  pstViChn->s32ChnId = pstRecCfg->vi_attr[index].u32ViChn;

  pstVencChn->enModId = RK_ID_VENC;
  pstVencChn->s32DevId = 0;
  pstVencChn->s32ChnId = pstRecCfg->attribute[index].venc_chn;
}

static void RKADK_RECORD_SetAudioChn(MPP_CHN_S *pstAiChn,
                                     MPP_CHN_S *pstAencChn) {
  pstAiChn->enModId = RK_ID_AI;
  pstAiChn->s32DevId = 0;
  pstAiChn->s32ChnId = RECORD_AI_CHN;

  pstAencChn->enModId = RK_ID_AENC;
  pstAencChn->s32DevId = 0;
  pstAencChn->s32ChnId = RECORD_AENC_CHN;
}

static int RKADK_RECORD_SetVideoAttr(int index, RKADK_S32 s32CamId,
                                     RKADK_PARAM_REC_CFG_S *pstRecCfg,
                                     VENC_CHN_ATTR_S *pstVencAttr) {
  int ret;
  RKADK_U32 u32Gop;
  RKADK_U32 u32DstFrameRateNum = 0;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;
  RKADK_U32 bitrate;

  RKADK_CHECK_POINTER(pstVencAttr, RKADK_FAILURE);

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(s32CamId);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  memset(pstVencAttr, 0, sizeof(VENC_CHN_ATTR_S));

  if (pstRecCfg->record_type == RKADK_REC_TYPE_LAPSE) {
    bitrate = pstRecCfg->attribute[index].bitrate / pstRecCfg->lapse_multiple;
    u32DstFrameRateNum = pstSensorCfg->framerate / pstRecCfg->lapse_multiple;
    if (u32DstFrameRateNum < 1)
      u32DstFrameRateNum = 1;
    else if (u32DstFrameRateNum > pstSensorCfg->framerate)
      u32DstFrameRateNum = pstSensorCfg->framerate;

    u32Gop = pstRecCfg->attribute[index].gop / pstRecCfg->lapse_multiple;
  } else {
    bitrate = pstRecCfg->attribute[index].bitrate;
    u32DstFrameRateNum = pstSensorCfg->framerate;
    u32Gop = pstRecCfg->attribute[index].gop;
  }

  pstVencAttr->stRcAttr.enRcMode =
      RKADK_PARAM_GetRcMode(pstRecCfg->attribute[index].rc_mode,
                            pstRecCfg->attribute[index].codec_type);

  ret = RKADK_MEDIA_SetRcAttr(&pstVencAttr->stRcAttr, u32Gop, bitrate,
                              pstSensorCfg->framerate, u32DstFrameRateNum);
  if (ret) {
    RKADK_LOGE("RKADK_MEDIA_SetRcAttr failed");
    return -1;
  }

  if (pstRecCfg->attribute[index].codec_type == RKADK_CODEC_TYPE_H265)
    pstVencAttr->stVencAttr.stAttrH265e.bScaleList =
        (RK_BOOL)pstRecCfg->attribute[index].venc_param.scaling_list;

  pstVencAttr->stVencAttr.enType =
      RKADK_MEDIA_GetRkCodecType(pstRecCfg->attribute[index].codec_type);
  pstVencAttr->stVencAttr.imageType =
      pstRecCfg->vi_attr[index].stChnAttr.enPixFmt;
  pstVencAttr->stVencAttr.u32PicWidth = pstRecCfg->attribute[index].width;
  pstVencAttr->stVencAttr.u32PicHeight = pstRecCfg->attribute[index].height;
  pstVencAttr->stVencAttr.u32VirWidth = pstRecCfg->attribute[index].width;
  pstVencAttr->stVencAttr.u32VirHeight = pstRecCfg->attribute[index].height;
  pstVencAttr->stVencAttr.u32Profile = pstRecCfg->attribute[index].profile;
  pstVencAttr->stVencAttr.bFullRange =
      (RK_BOOL)pstRecCfg->attribute[index].venc_param.full_range;
  return 0;
}

static int RKADK_RECORD_CreateVideoChn(RKADK_S32 s32CamID) {
  int ret;
  VENC_CHN_ATTR_S stVencChnAttr;
  VENC_RC_PARAM_S stVencRcParam;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    ret = RKADK_RECORD_SetVideoAttr(i, s32CamID, pstRecCfg, &stVencChnAttr);
    if (ret) {
      RKADK_LOGE("RKADK_RECORD_SetVideoAttr(%d) failed", i);
      return ret;
    }

    // Create VI
    ret = RKADK_MPI_VI_Init(s32CamID, pstRecCfg->vi_attr[i].u32ViChn,
                            &(pstRecCfg->vi_attr[i].stChnAttr));
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VI_Init faile, ret = %d", ret);
      return ret;
    }

    // Create VENC
    ret = RKADK_MPI_VENC_Init(pstRecCfg->attribute[i].venc_chn, &stVencChnAttr);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VENC_Init failed(%d)", ret);
      RKADK_MPI_VI_DeInit(s32CamID, pstRecCfg->vi_attr[i].u32ViChn);
      return ret;
    }

    ret = RKADK_PARAM_GetRcParam(pstRecCfg->attribute[i], &stVencRcParam);
    if (!ret) {
      ret = RK_MPI_VENC_SetRcParam(pstRecCfg->attribute[i].venc_chn,
                                   &stVencRcParam);
      if (ret)
        RKADK_LOGW("RK_MPI_VENC_SetRcParam failed(%d), use default cfg", ret);
    }
  }

  return 0;
}

static int RKADK_RECORD_DestoryVideoChn(RKADK_S32 s32CamID) {
  int ret;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    // Destroy VENC
    ret = RKADK_MPI_VENC_DeInit(pstRecCfg->attribute[i].venc_chn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VENC_DeInit failed(%d)", ret);
      return ret;
    }

    // Destroy VI
    ret = RKADK_MPI_VI_DeInit(s32CamID, pstRecCfg->vi_attr[i].u32ViChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VI_DeInit failed, ret = %d", ret);
      return ret;
    }
  }

  return 0;
}

static int RKADK_RECORD_CreateAudioChn() {
  int ret;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  // Create AI
  AI_CHN_ATTR_S stAiAttr;
  stAiAttr.pcAudioNode = pstAudioParam->audio_node;
  stAiAttr.enSampleFormat = pstAudioParam->sample_format;
  stAiAttr.u32NbSamples = pstAudioParam->samples_per_frame;
  stAiAttr.u32SampleRate = pstAudioParam->samplerate;
  stAiAttr.u32Channels = pstAudioParam->channels;
  stAiAttr.enAiLayout = pstAudioParam->ai_layout;
  ret = RKADK_MPI_AI_Init(RECORD_AI_CHN, &stAiAttr, pstAudioParam->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_Init faile(%d)", ret);
    return ret;
  }

  // Create AENC
  AENC_CHN_ATTR_S stAencAttr;
  stAencAttr.enCodecType = RKADK_MEDIA_GetRkCodecType(RECORD_AUDIO_CODEC_TYPE);
  stAencAttr.u32Bitrate = pstAudioParam->bitrate;
  stAencAttr.u32Quality = 1;
  stAencAttr.stAencMP3.u32Channels = pstAudioParam->channels;
  stAencAttr.stAencMP3.u32SampleRate = pstAudioParam->samplerate;
  ret = RKADK_MPI_AENC_Init(RECORD_AENC_CHN, &stAencAttr);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AENC_Init failed(%d)", ret);
    RKADK_MPI_AI_DeInit(RECORD_AI_CHN, pstAudioParam->vqe_mode);
    return -1;
  }

  return 0;
}

static int RKADK_RECORD_DestoryAudioChn() {
  int ret;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  ret = RKADK_MPI_AENC_DeInit(RECORD_AENC_CHN);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AENC_DeInit failed(%d)", ret);
    return ret;
  }

  ret = RKADK_MPI_AI_DeInit(RECORD_AI_CHN, pstAudioParam->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_DeInit failed(%d)", ret);
    return ret;
  }

  return 0;
}

static int RKADK_RECORD_BindChn(RKADK_S32 s32CamID,
                                RKADK_REC_TYPE_E enRecType) {
  int ret;
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  if (enRecType != RKADK_REC_TYPE_LAPSE) {
    // Bind AI to AENC
    RKADK_RECORD_SetAudioChn(&stSrcChn, &stDestChn);
    ret = RKADK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_Bind failed(%d)", ret);
      return -1;
    }
  }

  // Bind VI to VENC
  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    RKADK_RECORD_SetVideoChn(i, pstRecCfg, &stSrcChn, &stDestChn);
    ret = RKADK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_Bind failed(%d)", ret);
      return -1;
    }
  }

  return 0;
}

static int RKADK_RECORD_UnBindChn(RKADK_S32 s32CamID,
                                  RKADK_REC_TYPE_E enRecType) {
  int ret;
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  if (enRecType != RKADK_REC_TYPE_LAPSE) {
    // UnBind AI to AENC
    RKADK_RECORD_SetAudioChn(&stSrcChn, &stDestChn);
    ret = RKADK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_UnBind failed(%d)", ret);
      return -1;
    }
  }

  // UnBind VI to VENC
  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    RKADK_RECORD_SetVideoChn(i, pstRecCfg, &stSrcChn, &stDestChn);
    ret = RKADK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_UnBind failed(%d)", ret);
      return -1;
    }
  }

  return 0;
}

static RKADK_S32 RKADK_RECORD_SetRecordAttr(RKADK_S32 s32CamID,
                                            RKADK_REC_TYPE_E enRecType,
                                            RKADK_REC_ATTR_S *pstRecAttr) {
  RKADK_U32 u32Integer = 0, u32Remainder = 0;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(s32CamID);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  memset(pstRecAttr, 0, sizeof(RKADK_REC_ATTR_S));

  if (pstRecCfg->record_type != enRecType)
    RKADK_LOGW("enRecType(%d) != param record_type(%d)", enRecType,
               pstRecCfg->record_type);

  u32Integer = pstRecCfg->attribute[0].gop / pstSensorCfg->framerate;
  u32Remainder = pstRecCfg->attribute[0].gop % pstSensorCfg->framerate;
  pstRecAttr->stPreRecordAttr.u32PreRecCacheTime =
      pstRecCfg->pre_record_time + u32Integer;
  if (u32Remainder)
    pstRecAttr->stPreRecordAttr.u32PreRecCacheTime += 1;

  pstRecAttr->enRecType = enRecType;
  pstRecAttr->stRecSplitAttr.enMode = MUXER_MODE_AUTOSPLIT;
  pstRecAttr->u32StreamCnt = pstRecCfg->file_num;
  pstRecAttr->stPreRecordAttr.u32PreRecTimeSec = pstRecCfg->pre_record_time;
  pstRecAttr->stPreRecordAttr.enPreRecordMode = pstRecCfg->pre_record_mode;

  MUXER_SPLIT_ATTR_S *stSplitAttr = &(pstRecAttr->stRecSplitAttr.stSplitAttr);
  stSplitAttr->enSplitType = MUXER_SPLIT_TYPE_TIME;
  stSplitAttr->enSplitNameType = MUXER_SPLIT_NAME_TYPE_CALLBACK;
  stSplitAttr->stNameCallBackAttr.pcbRequestFileNames = GetRecordFileName;

  for (int i = 0; i < (int)pstRecAttr->u32StreamCnt; i++) {
    pstRecAttr->astStreamAttr[i].enType = MUXER_TYPE_MP4;
    if (pstRecAttr->enRecType == RKADK_REC_TYPE_LAPSE) {
      pstRecAttr->astStreamAttr[i].u32TimeLenSec =
          pstRecCfg->record_time_cfg[i].lapse_interval;
      pstRecAttr->astStreamAttr[i].u32TrackCnt = 1; // only video track
    } else {
      pstRecAttr->astStreamAttr[i].u32TimeLenSec =
          pstRecCfg->record_time_cfg[i].record_time;
      pstRecAttr->astStreamAttr[i].u32TrackCnt = RKADK_REC_TRACK_MAX_CNT;
    }

    // video track
    RKADK_TRACK_SOURCE_S *aHTrackSrcHandle =
        &(pstRecAttr->astStreamAttr[i].aHTrackSrcHandle[0]);
    aHTrackSrcHandle->enTrackType = RKADK_TRACK_SOURCE_TYPE_VIDEO;
    aHTrackSrcHandle->s32PrivateHandle = pstRecCfg->attribute[i].venc_chn;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.enCodecType =
        RKADK_MEDIA_GetRkCodecType(pstRecCfg->attribute[i].codec_type);
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.enImageType =
        pstRecCfg->vi_attr[i].stChnAttr.enPixFmt;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32FrameRate =
        pstSensorCfg->framerate;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u16Level = 41;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u16Profile =
        pstRecCfg->attribute[i].profile;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32BitRate =
        pstRecCfg->attribute[i].bitrate;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32Width =
        pstRecCfg->attribute[i].width;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32Height =
        pstRecCfg->attribute[i].height;

    if (pstRecAttr->enRecType == RKADK_REC_TYPE_LAPSE)
      continue;

    // audio track
    aHTrackSrcHandle = &(pstRecAttr->astStreamAttr[i].aHTrackSrcHandle[1]);
    aHTrackSrcHandle->enTrackType = RKADK_TRACK_SOURCE_TYPE_AUDIO;
    aHTrackSrcHandle->s32PrivateHandle = RECORD_AENC_CHN;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.enCodecType =
        RKADK_MEDIA_GetRkCodecType(RECORD_AUDIO_CODEC_TYPE);
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.enSampFmt =
        pstAudioParam->sample_format;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.u32ChnCnt =
        pstAudioParam->channels;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.u32SamplesPerFrame =
        pstAudioParam->samples_per_frame;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.u32SampleRate =
        pstAudioParam->samplerate;
  }

  return 0;
}

RKADK_S32 RKADK_RECORD_Create(RKADK_RECORD_ATTR_S *pstRecAttr,
                              RKADK_MW_PTR *ppRecorder) {
  int ret;

  RKADK_CHECK_POINTER(pstRecAttr, RKADK_FAILURE);
  RKADK_CHECK_CAMERAID(pstRecAttr->s32CamID, RKADK_FAILURE);

  RKADK_LOGI("Create Record[%d, %d] Start...", pstRecAttr->s32CamID,
             pstRecAttr->enRecType);

  RK_MPI_SYS_Init();
  RKADK_PARAM_Init();

  if (RKADK_RECORD_CreateVideoChn(pstRecAttr->s32CamID))
    return -1;

  if (pstRecAttr->enRecType != RKADK_REC_TYPE_LAPSE) {
    if (RKADK_RECORD_CreateAudioChn()) {
      RKADK_RECORD_DestoryVideoChn(pstRecAttr->s32CamID);
      return -1;
    }
  }

  g_pfnRequestFileNames[pstRecAttr->s32CamID] = pstRecAttr->pfnRequestFileNames;

  RKADK_REC_ATTR_S stRecAttr;
  ret = RKADK_RECORD_SetRecordAttr(pstRecAttr->s32CamID, pstRecAttr->enRecType,
                                   &stRecAttr);
  if (ret) {
    RKADK_LOGE("RKADK_RECORD_SetRecordAttr failed");
    goto failed;
  }

  if (RKADK_REC_Create(&stRecAttr, ppRecorder, pstRecAttr->s32CamID))
    goto failed;

  if (RKADK_RECORD_BindChn(pstRecAttr->s32CamID, pstRecAttr->enRecType))
    goto failed;

  RKADK_LOGI("Create Record[%d, %d] End...", pstRecAttr->s32CamID,
             pstRecAttr->enRecType);
  return 0;

failed:
  RKADK_LOGE("Create Record[%d, %d] failed", pstRecAttr->s32CamID,
             pstRecAttr->enRecType);
  RKADK_RECORD_DestoryVideoChn(pstRecAttr->s32CamID);

  if (pstRecAttr->enRecType != RKADK_REC_TYPE_LAPSE)
    RKADK_RECORD_DestoryAudioChn();

  return -1;
}

RKADK_S32 RKADK_RECORD_Destroy(RKADK_MW_PTR pRecorder) {
  RKADK_S32 ret;
  RKADK_S32 s32CamId;
  RKADK_REC_TYPE_E enRecType;
  RKADK_RECORDER_HANDLE_S *stRecorder = NULL;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);
  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  if (!stRecorder) {
    RKADK_LOGE("stRecorder is null");
    return -1;
  }

  RKADK_LOGI("Destory Record[%d, %d] Start...", stRecorder->s32CamId,
             stRecorder->enRecType);

  s32CamId = stRecorder->s32CamId;
  enRecType = stRecorder->enRecType;
  RKADK_CHECK_CAMERAID(s32CamId, RKADK_FAILURE);

  for (int i = 0; i < (int)stRecorder->u32StreamCnt; i++) {
    while (!g_fileNameDeque[i].empty()) {
      RKADK_LOGD("clear file name deque(%d)", i);
      auto front = g_fileNameDeque[i].front();
      g_fileNameDeque[i].pop_front();
      free(front);
    }
    g_fileNameDeque[i].clear();
  }

  ret = RKADK_REC_Destroy(pRecorder);
  if (ret) {
    RKADK_LOGE("RK_REC_Destroy failed, ret = %d", ret);
    return ret;
  }

  ret = RKADK_RECORD_UnBindChn(s32CamId, enRecType);
  if (ret) {
    RKADK_LOGE("RKADK_RECORD_UnBindChn failed, ret = %d", ret);
    return ret;
  }

  ret = RKADK_RECORD_DestoryVideoChn(s32CamId);
  if (ret) {
    RKADK_LOGE("RKADK_RECORD_DestoryVideoChn failed, ret = %d", ret);
    return ret;
  }

  if (enRecType != RKADK_REC_TYPE_LAPSE) {
    ret = RKADK_RECORD_DestoryAudioChn();
    if (ret) {
      RKADK_LOGE("RKADK_RECORD_DestoryAudioChn failed, ret = %d", ret);
      return ret;
    }
  }

  g_pfnRequestFileNames[s32CamId] = NULL;
  RKADK_LOGI("Destory Record[%d, %d] End...", s32CamId, enRecType);
  return 0;
}

RKADK_S32 RKADK_RECORD_Start(RKADK_MW_PTR pRecorder) {
  return RKADK_REC_Start(pRecorder);
}

RKADK_S32 RKADK_RECORD_Stop(RKADK_MW_PTR pRecorder) {
  return RKADK_REC_Stop(pRecorder);
}

RKADK_S32
RKADK_RECORD_ManualSplit(RKADK_MW_PTR pRecorder,
                         RKADK_REC_MANUAL_SPLIT_ATTR_S *pstSplitAttr) {
  return RKADK_REC_ManualSplit(pRecorder, pstSplitAttr);
}

RKADK_S32 RKADK_RECORD_RegisterEventCallback(
    RKADK_MW_PTR pRecorder, RKADK_REC_EVENT_CALLBACK_FN pfnEventCallback) {
  return RKADK_REC_RegisterEventCallback(pRecorder, pfnEventCallback);
}

RKADK_S32 RKADK_RECORD_GetAencChn() { return RECORD_AENC_CHN; }
