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

#include "rkadk_recorder_pro.h"
#include "rkadk_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RKADK_REC_MAX_VIDEO_TRACK 1
#define RKADK_REC_MAX_AUDIO_TRACK 1

static RKADK_REC_EVENT_CALLBACK_FN g_pfnEventCallback = NULL;

/** Stream Count Check */
#define RKADK_CHECK_STREAM_CNT(cnt)                                            \
  do {                                                                         \
    if ((cnt > RKADK_REC_STREAM_MAX_CNT) || (cnt == 0)) {                      \
      RKADK_LOGE("stream count[%d] > RKADK_REC_STREAM_MAX_CNT or == 0", cnt);  \
      return -1;                                                               \
    }                                                                          \
  } while (0)

static void MuxerEventCb(RKADK_VOID *pHandle, RKADK_VOID *pstEvent) {
  if (!g_pfnEventCallback) {
    RKADK_LOGW("Not Registered event callback");
    return;
  }

  g_pfnEventCallback((RKADK_MW_PTR)pHandle, (RKADK_REC_EVENT_INFO_S *)pstEvent);
}

static void DumpMuxerChanAttr(MUXER_CHN_ATTR_S stMuxerAttr) {
  RKADK_LOGD("enMode = %s(%d)",
             stMuxerAttr.enMode == MUXER_MODE_SINGLE ? "MUXER_MODE_SINGLE"
                                                     : "MUXER_MODE_AUTOSPLIT",
             stMuxerAttr.enMode);
  RKADK_LOGD("enType = %s(%d)",
             stMuxerAttr.enType == MUXER_TYPE_MP4 ? "MUXER_TYPE_MP4"
                                                  : "MUXER_TYPE_MPEGTS",
             stMuxerAttr.enType);

  RKADK_LOGD("u32PreRecTimeSec = %d(s)",
             stMuxerAttr.stPreRecordParam.u32PreRecTimeSec);
  RKADK_LOGD("u32PreRecCacheTime = %d(s)",
             stMuxerAttr.stPreRecordParam.u32PreRecCacheTime);
  RKADK_LOGD("enPreRecordMode = %d(s)",
             stMuxerAttr.stPreRecordParam.enPreRecordMode);
  RKADK_LOGD("u32MuxerId = %d", stMuxerAttr.u32MuxerId);

  if (stMuxerAttr.enMode == MUXER_MODE_SINGLE) {
    RKADK_LOGD("pcOutputFile = %s", stMuxerAttr.pcOutputFile);
  } else if (stMuxerAttr.enMode == MUXER_MODE_AUTOSPLIT) {
    RKADK_LOGD("stSplitAttr.enSplitType = %s(%d)",
               stMuxerAttr.stSplitAttr.enSplitType == MUXER_SPLIT_TYPE_TIME
                   ? "MUXER_SPLIT_TYPE_TIME"
                   : "MUXER_SPLIT_TYPE_SIZE",
               stMuxerAttr.stSplitAttr.enSplitType);

    if (stMuxerAttr.stSplitAttr.enSplitType == MUXER_SPLIT_TYPE_TIME)
      RKADK_LOGD("stSplitAttr.u32TimeLenSec = %d",
                 stMuxerAttr.stSplitAttr.u32TimeLenSec);
    else
      RKADK_LOGD("stSplitAttr.u32SizeLenByte = %d",
                 stMuxerAttr.stSplitAttr.u32SizeLenByte);

    RKADK_LOGD("stSplitAttr.enSplitNameType = %s(%d)",
               stMuxerAttr.stSplitAttr.enSplitNameType ==
                       MUXER_SPLIT_NAME_TYPE_AUTO
                   ? "MUXER_SPLIT_NAME_TYPE_AUTO"
                   : "MUXER_SPLIT_NAME_TYPE_CALLBACK",
               stMuxerAttr.stSplitAttr.enSplitNameType);

    if (stMuxerAttr.stSplitAttr.enSplitNameType == MUXER_SPLIT_NAME_TYPE_AUTO) {
      RKADK_LOGD("stSplitAttr.stNameAutoAttr.bTimeStampEnable = %d",
                 stMuxerAttr.stSplitAttr.stNameAutoAttr.bTimeStampEnable);
      RKADK_LOGD("stSplitAttr.stNameAutoAttr.u16StartIdx = %d",
                 stMuxerAttr.stSplitAttr.stNameAutoAttr.u16StartIdx);
      RKADK_LOGD("stSplitAttr.stNameAutoAttr.pcPrefix = %s",
                 stMuxerAttr.stSplitAttr.stNameAutoAttr.pcPrefix);
      RKADK_LOGD("stSplitAttr.stNameAutoAttr.pcBaseDir = %s",
                 stMuxerAttr.stSplitAttr.stNameAutoAttr.pcBaseDir);
    }
  }

  RKADK_LOGD("stAudioStreamParam.enCodecType = %d",
             stMuxerAttr.stAudioStreamParam.enCodecType);
  RKADK_LOGD("stAudioStreamParam.enSampFmt = %d",
             stMuxerAttr.stAudioStreamParam.enSampFmt);
  RKADK_LOGD("stAudioStreamParam.u32Channels = %d",
             stMuxerAttr.stAudioStreamParam.u32Channels);
  RKADK_LOGD("stAudioStreamParam.u32NbSamples = %d",
             stMuxerAttr.stAudioStreamParam.u32NbSamples);
  RKADK_LOGD("stAudioStreamParam.u32SampleRate = %d",
             stMuxerAttr.stAudioStreamParam.u32SampleRate);

  RKADK_LOGD("stVideoStreamParam.enCodecType = %d",
             stMuxerAttr.stVideoStreamParam.enCodecType);
  RKADK_LOGD("stVideoStreamParam.enImageType = %d",
             stMuxerAttr.stVideoStreamParam.enImageType);
  RKADK_LOGD("stVideoStreamParam.u16Fps = %d",
             stMuxerAttr.stVideoStreamParam.u16Fps);
  RKADK_LOGD("stVideoStreamParam.u16Level = %d",
             stMuxerAttr.stVideoStreamParam.u16Level);
  RKADK_LOGD("stVideoStreamParam.u16Profile = %d",
             stMuxerAttr.stVideoStreamParam.u16Profile);
  RKADK_LOGD("stVideoStreamParam.u32BitRate = %d",
             stMuxerAttr.stVideoStreamParam.u32BitRate);
  RKADK_LOGD("stVideoStreamParam.u32Height = %d",
             stMuxerAttr.stVideoStreamParam.u32Height);
  RKADK_LOGD("stVideoStreamParam.u32Width = %d",
             stMuxerAttr.stVideoStreamParam.u32Width);
  RKADK_LOGD("stVideoStreamParam.enCodecType = %d",
             stMuxerAttr.stVideoStreamParam.enCodecType);
}

static RKADK_S32 EnableMuxerChn(RKADK_REC_TYPE_E enRecType,
                                RKADK_RECORDER_STREAM_ATTR_S *stDstStreamAttr,
                                RKADK_REC_ATTR_S *pstRecAttr,
                                RKADK_U32 u32StreamIndex) {
  int videoTrackCnt = 0, audioTrackCnt = 0;
  MUXER_CHN_ATTR_S stMuxerAttr;
  RKADK_REC_STREAM_ATTR_S *pstSrcStreamAttr =
      &(pstRecAttr->astStreamAttr[u32StreamIndex]);

  memset(&stMuxerAttr, 0, sizeof(stMuxerAttr));

  stDstStreamAttr->stMuxerChn.enModId = RK_ID_MUXER;
  stDstStreamAttr->stMuxerChn.s32ChnId = u32StreamIndex;

  if (enRecType == RKADK_REC_TYPE_LAPSE)
    stMuxerAttr.bLapseRecord = RK_TRUE;
  else
    stMuxerAttr.bLapseRecord = RK_FALSE;

  stMuxerAttr.u32MuxerId = u32StreamIndex;
  stMuxerAttr.stPreRecordParam.u32PreRecTimeSec =
      pstRecAttr->stPreRecordAttr.u32PreRecTimeSec;
  stMuxerAttr.stPreRecordParam.u32PreRecCacheTime =
      pstRecAttr->stPreRecordAttr.u32PreRecCacheTime;
  stMuxerAttr.stPreRecordParam.enPreRecordMode =
      pstRecAttr->stPreRecordAttr.enPreRecordMode;

  stMuxerAttr.enMode = pstRecAttr->stRecSplitAttr.enMode;
  pstRecAttr->stRecSplitAttr.stSplitAttr.u32TimeLenSec =
      pstSrcStreamAttr->u32TimeLenSec;
  if (stMuxerAttr.enMode == MUXER_MODE_AUTOSPLIT) {
    memcpy(&(stMuxerAttr.stSplitAttr),
           &(pstRecAttr->stRecSplitAttr.stSplitAttr),
           sizeof(MUXER_SPLIT_ATTR_S));
  } else if (stMuxerAttr.enMode == MUXER_MODE_SINGLE) {
    stMuxerAttr.pcOutputFile = pstRecAttr->stRecSplitAttr.pcOutputFile;
  } else {
    RKADK_LOGE("invalid enMode = %d", stMuxerAttr.enMode);
    return -1;
  }

  stMuxerAttr.enType = pstSrcStreamAttr->enType;
  if (pstSrcStreamAttr->u32TrackCnt > RKADK_REC_TRACK_MAX_CNT) {
    RKADK_LOGE(
        "u32StreamIndex(%d), u32TrackCnt(%d) > RKADK_REC_TRACK_MAX_CNT(%d)",
        u32StreamIndex, pstSrcStreamAttr->u32TrackCnt, RKADK_REC_TRACK_MAX_CNT);
    return -1;
  }

  for (RKADK_U32 i = 0; i < pstSrcStreamAttr->u32TrackCnt; i++) {
    RKADK_TRACK_SOURCE_S *trackSrcHandle =
        &(pstSrcStreamAttr->aHTrackSrcHandle[i]);
    if (trackSrcHandle->enTrackType == RKADK_TRACK_SOURCE_TYPE_VIDEO) {
      videoTrackCnt++;
      if (videoTrackCnt > RKADK_REC_MAX_VIDEO_TRACK) {
        RKADK_LOGE("videoTrackCnt(%d) > RKADK_REC_MAX_VIDEO_TRACK(%d)",
                   videoTrackCnt, RKADK_REC_MAX_VIDEO_TRACK);
        return -1;
      }

      stDstStreamAttr->stVencChn.enModId = RK_ID_VENC;
      stDstStreamAttr->stVencChn.s32ChnId = trackSrcHandle->s32PrivateHandle;
      stDstStreamAttr->stVencChn.s32DevId = 0;

      RKADK_TRACK_VIDEO_SOURCE_INFO_S *videoInfo =
          &(trackSrcHandle->unTrackSourceAttr.stVideoInfo);
      stMuxerAttr.stVideoStreamParam.enCodecType = videoInfo->enCodecType;
      stMuxerAttr.stVideoStreamParam.enImageType = videoInfo->enImageType;
      stMuxerAttr.stVideoStreamParam.u32Width = videoInfo->u32Width;
      stMuxerAttr.stVideoStreamParam.u32Height = videoInfo->u32Height;
      stMuxerAttr.stVideoStreamParam.u32BitRate = videoInfo->u32BitRate;
      stMuxerAttr.stVideoStreamParam.u16Fps = videoInfo->u32FrameRate;
      stMuxerAttr.stVideoStreamParam.u16Profile = videoInfo->u16Profile;
      stMuxerAttr.stVideoStreamParam.u16Level = videoInfo->u16Level;
    } else if (trackSrcHandle->enTrackType == RKADK_TRACK_SOURCE_TYPE_AUDIO) {
      audioTrackCnt++;
      if (audioTrackCnt > RKADK_REC_MAX_VIDEO_TRACK) {
        RKADK_LOGE("ERROR: audioTrackCnt(%d) > RKADK_REC_MAX_AUDIO_TRACK(%d)",
                   audioTrackCnt, RKADK_REC_MAX_AUDIO_TRACK);
        return -1;
      }

      stDstStreamAttr->stAencChn.enModId = RK_ID_AENC;
      stDstStreamAttr->stAencChn.s32ChnId = trackSrcHandle->s32PrivateHandle;
      stDstStreamAttr->stAencChn.s32DevId = 0;

      RKADK_TRACK_AUDIO_SOURCE_INFO_S *audioInfo =
          &(trackSrcHandle->unTrackSourceAttr.stAudioInfo);
      stMuxerAttr.stAudioStreamParam.enCodecType = audioInfo->enCodecType;
      stMuxerAttr.stAudioStreamParam.enSampFmt = audioInfo->enSampFmt;
      stMuxerAttr.stAudioStreamParam.u32Channels = audioInfo->u32ChnCnt;
      stMuxerAttr.stAudioStreamParam.u32NbSamples =
          audioInfo->u32SamplesPerFrame;
      stMuxerAttr.stAudioStreamParam.u32SampleRate = audioInfo->u32SampleRate;
    } else {
      RKADK_LOGE("invalid enTrackType = %d", trackSrcHandle->enTrackType);
      return -1;
    }
  }

#ifdef RKADK_DUMP_CONFIG
  DumpMuxerChanAttr(stMuxerAttr);
#endif

  return RK_MPI_MUXER_EnableChn(stMuxerAttr.u32MuxerId, &stMuxerAttr);
}

RKADK_S32 RKADK_REC_Create(RKADK_REC_ATTR_S *pstRecAttr,
                           RKADK_MW_PTR *ppRecorder, RKADK_S32 s32CamId) {
  RKADK_RECORDER_HANDLE_S *pstRecorder = NULL;
  int ret = 0;

  RKADK_CHECK_POINTER(pstRecAttr, RKADK_FAILURE);

  if (*ppRecorder) {
    RKADK_LOGE("Recorder has been created");
    return -1;
  }

  pstRecorder =
      (RKADK_RECORDER_HANDLE_S *)malloc(sizeof(RKADK_RECORDER_HANDLE_S));
  if (!pstRecorder) {
    RKADK_LOGE("malloc stRecorder failed");
    return -1;
  }
  memset(pstRecorder, 0, sizeof(RKADK_RECORDER_HANDLE_S));

  RKADK_CHECK_STREAM_CNT(pstRecAttr->u32StreamCnt);
  pstRecorder->u32StreamCnt = pstRecAttr->u32StreamCnt;
  pstRecorder->s32CamId = s32CamId;
  pstRecorder->enRecType = pstRecAttr->enRecType;

  if (pstRecAttr->stRecSplitAttr.stSplitAttr.enSplitNameType ==
      MUXER_SPLIT_NAME_TYPE_CALLBACK)
    pstRecAttr->stRecSplitAttr.stSplitAttr.stNameCallBackAttr.pCallBackHandle =
        pstRecorder;

  for (RKADK_U32 i = 0; i < pstRecAttr->u32StreamCnt; i++) {
    ret = EnableMuxerChn(pstRecorder->enRecType,
                         &(pstRecorder->stStreamAttr[i]), pstRecAttr, i);
    if (ret) {
      RKADK_LOGE("Create Vm(%d) failed", i);
      goto failed;
    }

    // Bind VENC to MUXER:VIDEO
    pstRecorder->stStreamAttr[i].stMuxerChn.enChnType = MUXER_CHN_TYPE_VIDEO;
    ret = RK_MPI_MUXER_Bind(&(pstRecorder->stStreamAttr[i].stVencChn),
                            &(pstRecorder->stStreamAttr[i].stMuxerChn));
    if (ret) {
      RKADK_LOGE("Bind VENC[%d] and MUXER[%d]:VIDEO failed(%d)",
                 pstRecorder->stStreamAttr[i].stVencChn.s32ChnId,
                 pstRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
      RK_MPI_MUXER_DisableChn(pstRecorder->stStreamAttr[i].stMuxerChn.s32ChnId);
      goto failed;
    }

    if (pstRecAttr->enRecType == RKADK_REC_TYPE_LAPSE)
      continue;

    // Bind AENC to MUXER:AIDEO
    pstRecorder->stStreamAttr[i].stMuxerChn.enChnType = MUXER_CHN_TYPE_AUDIO;
    ret = RK_MPI_MUXER_Bind(&(pstRecorder->stStreamAttr[i].stAencChn),
                            &(pstRecorder->stStreamAttr[i].stMuxerChn));
    if (ret) {
      RKADK_LOGE("Bind AENC[%d] and MUXER[%d]:AUDIO failed(%d)",
                 pstRecorder->stStreamAttr[i].stAencChn.s32ChnId,
                 pstRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
      RK_MPI_MUXER_UnBind(&(pstRecorder->stStreamAttr[i].stVencChn),
                          &(pstRecorder->stStreamAttr[i].stMuxerChn));
      RK_MPI_MUXER_DisableChn(pstRecorder->stStreamAttr[i].stMuxerChn.s32ChnId);
      goto failed;
    }
  }

  *ppRecorder = (RKADK_MW_PTR)pstRecorder;
  return 0;

failed:
  if (pstRecorder) {
    free(pstRecorder);
    pstRecorder = NULL;
  }

  return ret;
}

RKADK_S32 RKADK_REC_Destroy(RKADK_MW_PTR pRecorder) {
  RKADK_RECORDER_HANDLE_S *stRecorder;
  int ret = 0;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  RKADK_CHECK_STREAM_CNT(stRecorder->u32StreamCnt);

  for (RKADK_U32 i = 0; i < stRecorder->u32StreamCnt; i++) {
    // UnBind VENC to MUXER:VIDEO
    stRecorder->stStreamAttr[i].stMuxerChn.enChnType = MUXER_CHN_TYPE_VIDEO;
    ret = RK_MPI_MUXER_UnBind(&(stRecorder->stStreamAttr[i].stVencChn),
                              &(stRecorder->stStreamAttr[i].stMuxerChn));
    if (ret) {
      RKADK_LOGE("UnBind VENC[%d] and MUXER[%d]:VIDEO failed(%d)",
                 stRecorder->stStreamAttr[i].stVencChn.s32ChnId,
                 stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
      goto end;
    }

    if (stRecorder->enRecType != RKADK_REC_TYPE_LAPSE) {
      // UnBind AENC to MUXER:AIDEO
      stRecorder->stStreamAttr[i].stMuxerChn.enChnType = MUXER_CHN_TYPE_AUDIO;
      ret = RK_MPI_MUXER_UnBind(&(stRecorder->stStreamAttr[i].stAencChn),
                                &(stRecorder->stStreamAttr[i].stMuxerChn));
      if (ret) {
        RKADK_LOGE("UnBind AENC[%d] and MUXER[%d]:AIDEO failed(%d)",
                   stRecorder->stStreamAttr[i].stAencChn.s32ChnId,
                   stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
        goto end;
      }
    }

    ret = RK_MPI_MUXER_DisableChn(
        stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId);
    if (ret) {
      RKADK_LOGE("Destroy MUXER[%d] failed(%d)",
                 stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
      goto end;
    }
  }

end:
  if (pRecorder) {
    free(pRecorder);
    pRecorder = NULL;
  }

  g_pfnEventCallback = NULL;
  return ret;
}

RKADK_S32 RKADK_REC_Start(RKADK_MW_PTR pRecorder) {
  RKADK_RECORDER_HANDLE_S *stRecorder;
  int ret;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  RKADK_CHECK_STREAM_CNT(stRecorder->u32StreamCnt);

  RKADK_LOGI("Record[%d, %d] Start...", stRecorder->s32CamId,
             stRecorder->enRecType);

  for (RKADK_U32 i = 0; i < stRecorder->u32StreamCnt; i++) {
    ret = RK_MPI_MUXER_StreamStart(
        stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId);
    if (ret) {
      RKADK_LOGE("Start MuxerChnId(%d) Stream(%d) failed(%d)", i,
                 stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
      return ret;
    }
  }

  RKADK_LOGI("Record[%d, %d] Start End...", stRecorder->s32CamId,
             stRecorder->enRecType);
  return 0;
}

RKADK_S32 RKADK_REC_Stop(RKADK_MW_PTR pRecorder) {
  RKADK_RECORDER_HANDLE_S *stRecorder;
  int ret;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  RKADK_CHECK_STREAM_CNT(stRecorder->u32StreamCnt);

  RKADK_LOGI("Record[%d, %d] Stop...", stRecorder->s32CamId,
             stRecorder->enRecType);

  for (RKADK_U32 i = 0; i < stRecorder->u32StreamCnt; i++) {
    ret = RK_MPI_MUXER_StreamStop(
        stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId);
    if (ret) {
      RKADK_LOGE("Stop MuxerChnId(%d) Stream(%d) failed(%d)", i,
                 stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
      return ret;
    }
  }

  RKADK_LOGI("Record[%d, %d] Stop End...", stRecorder->s32CamId,
             stRecorder->enRecType);
  return 0;
}

RKADK_S32 RKADK_REC_ManualSplit(RKADK_MW_PTR pRecorder,
                                RKADK_REC_MANUAL_SPLIT_ATTR_S *pstSplitAttr) {
  RKADK_RECORDER_HANDLE_S *stRecorder;
  int ret;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstSplitAttr, RKADK_FAILURE);

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  RKADK_CHECK_STREAM_CNT(stRecorder->u32StreamCnt);

  RKADK_LOGI("Record[%d, %d] Manual Split Start...", stRecorder->s32CamId,
             stRecorder->enRecType);

  for (RKADK_U32 i = 0; i < stRecorder->u32StreamCnt; i++) {
    ret = RK_MPI_MUXER_ManualSplit(
        stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId,
        (MUXER_MANUAL_SPLIT_ATTR_S *)pstSplitAttr);
    if (ret) {
      RKADK_LOGE("Manual Split MuxerChnId(%d) Stream(%d) failed(%d)", i,
                 stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId, ret);
      return ret;
    }
  }

  RKADK_LOGI("Record[%d, %d] Manual Split End...", stRecorder->s32CamId,
             stRecorder->enRecType);
  return 0;
}

RKADK_S32
RKADK_REC_RegisterEventCallback(RKADK_MW_PTR pRecorder,
                                RKADK_REC_EVENT_CALLBACK_FN pfnEventCallback) {
  RKADK_RECORDER_HANDLE_S *stRecorder;
  MPP_CHN_S stMuxerChn;
  int ret;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  RKADK_CHECK_STREAM_CNT(stRecorder->u32StreamCnt);

  g_pfnEventCallback = pfnEventCallback;

  for (RKADK_U32 i = 0; i < stRecorder->u32StreamCnt; i++) {
    stMuxerChn.enModId = stRecorder->stStreamAttr[i].stMuxerChn.enModId;
    stMuxerChn.s32ChnId = stRecorder->stStreamAttr[i].stMuxerChn.s32ChnId;
    stMuxerChn.s32DevId = 0;

    ret = RK_MPI_SYS_RegisterEventCb(&stMuxerChn, pRecorder, MuxerEventCb);
    if (ret) {
      RKADK_LOGE("Register MuxerChnId(%d) event callback failed(%d)",
                 stMuxerChn.s32ChnId, ret);
      return ret;
    }
  }

  return 0;
}
