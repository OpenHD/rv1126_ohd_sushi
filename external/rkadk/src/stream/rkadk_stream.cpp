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

#include "rkadk_stream.h"
#include "rkadk_log.h"
#include "rkadk_param.h"
#include "rkmedia_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
  bool init;
  bool start;
  bool bVencChnMux;
  bool bRequestIDR;
  bool bWaitIDR;
  RKADK_U32 u32CamId;
  RKADK_U32 videoSeq;
  RKADK_CODEC_TYPE_E enCodecType;
} VIDEO_STREAM_INFO_S;

typedef struct {
  bool init;
  bool start;
  bool bGetBuffer;
  pthread_t tid;
  pthread_t pcmTid;
  RKADK_U32 audioSeq;
  RKADK_U32 pcmSeq;
  RKADK_CODEC_TYPE_E enCodecType;
  MPP_CHN_S stAiChn;
  MPP_CHN_S stAencChn;
} AUDIO_STREAM_INFO_S;

static VIDEO_STREAM_INFO_S g_videoStream[RKADK_MAX_SENSOR_CNT] = {0};
static AUDIO_STREAM_INFO_S g_audioStream = {0, 0, 0, 0,
                                            0, 0, 0, RKADK_CODEC_TYPE_BUTT};

static RKADK_VENC_DATA_PROC_FUNC g_pstVencCB[RKADK_MAX_SENSOR_CNT] = {NULL};
static RKADK_AENC_DATA_PROC_FUNC g_pstAencCB = NULL;
static RKADK_AENC_DATA_PROC_FUNC g_pstPcmCB = NULL;

static int RKADK_STREAM_CheckCodecType(bool is_video,
                                       RKADK_CODEC_TYPE_E enCodecType) {
  if (enCodecType == RKADK_CODEC_TYPE_BUTT) {
    RKADK_LOGE("invalid enCodecType = %d", enCodecType);
    return -1;
  }

  if (is_video) {
    if (enCodecType > RKADK_CODEC_TYPE_JPEG) {
      RKADK_LOGE("invalid video enCodecType = %d", enCodecType);
      return -1;
    }
  } else {
    if (enCodecType < RKADK_CODEC_TYPE_MP3) {
      RKADK_LOGE("invalid audio enCodecType = %d", enCodecType);
      return -1;
    }
  }

  return 0;
}

/**************************************************/
/*                     Video API                  */
/**************************************************/
static void RKADK_STREAM_GetNaluType(RKADK_CODEC_TYPE_E enPayload,
                                     RKADK_VENC_DATA_TYPE_S *stDataType,
                                     RKADK_S32 naluType) {
  // rkmedia only support VENC_NALU_PSLICE and VENC_NALU_IDRSLICE
  if (enPayload == RKADK_CODEC_TYPE_H264) {
    switch (naluType) {
    case VENC_NALU_BSLICE:
      stDataType->enH264EType = RKADK_H264E_NALU_BSLICE;
      break;
    case VENC_NALU_PSLICE:
      stDataType->enH264EType = RKADK_H264E_NALU_PSLICE;
      break;
    case VENC_NALU_ISLICE:
      stDataType->enH264EType = RKADK_H264E_NALU_ISLICE;
      break;
    case VENC_NALU_IDRSLICE:
      stDataType->enH264EType = RKADK_H264E_NALU_IDRSLICE;
      break;
    case VENC_NALU_SEI:
      stDataType->enH264EType = RKADK_H264E_NALU_SEI;
      break;
    case VENC_NALU_SPS:
      stDataType->enH264EType = RKADK_H264E_NALU_SPS;
      break;
    case VENC_NALU_PPS:
      stDataType->enH264EType = RKADK_H264E_NALU_PPS;
      break;
    default:
      stDataType->enH264EType = RKADK_H264E_NALU_BUTT;
      break;
    }
  } else if (enPayload == RKADK_CODEC_TYPE_H265) {
    switch (naluType) {
    case VENC_NALU_BSLICE:
      stDataType->enH265EType = RKADK_H265E_NALU_BSLICE;
      break;
    case VENC_NALU_PSLICE:
      stDataType->enH265EType = RKADK_H265E_NALU_PSLICE;
      break;
    case VENC_NALU_ISLICE:
      stDataType->enH265EType = RKADK_H265E_NALU_ISLICE;
      break;
    case VENC_NALU_IDRSLICE:
      stDataType->enH265EType = RKADK_H265E_NALU_IDRSLICE;
      break;
    case VENC_NALU_SEI:
      stDataType->enH265EType = RKADK_H265E_NALU_SEI;
      break;
    case VENC_NALU_VPS:
      stDataType->enH265EType = RKADK_H265E_NALU_VPS;
      break;
    case VENC_NALU_SPS:
      stDataType->enH265EType = RKADK_H265E_NALU_SPS;
      break;
    case VENC_NALU_PPS:
      stDataType->enH265EType = RKADK_H265E_NALU_PPS;
      break;
    default:
      stDataType->enH265EType = RKADK_H265E_NALU_BUTT;
      break;
    }
  } else {
    RKADK_LOGE("Unsupport now!");
  }
}

static RKADK_S32 RKADK_STREAM_RequestIDR(RKADK_U32 u32CamId,
                                         RKADK_U32 u32ChnId) {
  int ret = 0;
  RKADK_STREAM_TYPE_E enType = RKADK_PARAM_VencChnMux(u32CamId, u32ChnId);

  if (enType == RKADK_STREAM_TYPE_VIDEO_MAIN ||
      enType == RKADK_STREAM_TYPE_VIDEO_SUB) {
    int i;
    RKADK_PARAM_REC_CFG_S *pstRecCfg = RKADK_PARAM_GetRecCfg(u32CamId);
    if (!pstRecCfg) {
      RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
      return false;
    }

    for (i = 0; i < (int)pstRecCfg->file_num; i++) {
      RKADK_LOGI("chn mux, venc_chn[%d] request IDR",
                 pstRecCfg->attribute[i].venc_chn);
      ret |= RK_MPI_VENC_RequestIDR(pstRecCfg->attribute[i].venc_chn, RK_TRUE);
    }
  } else {
    ret = RK_MPI_VENC_RequestIDR(u32ChnId, RK_TRUE);
  }

  return ret;
}

static void RKADK_STREAM_VencOutCb(MEDIA_BUFFER mb, RKADK_VOID *pHandle) {
  RKADK_S32 s32NaluType;
  RKADK_VIDEO_STREAM_S vStreamData;

  VIDEO_STREAM_INFO_S *pstVideoStream = (VIDEO_STREAM_INFO_S *)pHandle;
  if (!pstVideoStream) {
    RKADK_LOGE("Can't find video stream handle");
    assert(pstVideoStream);
  }

  if (!g_pstVencCB[pstVideoStream->u32CamId] && !pstVideoStream->start) {
    if(pstVideoStream->start)
      RKADK_LOGE("Not register callback");

    if(!pstVideoStream->bVencChnMux)
      RK_MPI_MB_ReleaseBuffer(mb);

    return;
  }

  s32NaluType = RK_MPI_MB_GetFlag(mb);
  if (!pstVideoStream->bWaitIDR) {
    if (s32NaluType != VENC_NALU_IDRSLICE) {
      if (!pstVideoStream->bRequestIDR) {
        RKADK_LOGD("requst idr frame");
        RKADK_STREAM_RequestIDR(pstVideoStream->u32CamId, RK_MPI_MB_GetChannelID(mb));
        pstVideoStream->bRequestIDR = true;
      } else {
        RKADK_LOGD("wait first idr frame");
      }

      if(!pstVideoStream->bVencChnMux)
        RK_MPI_MB_ReleaseBuffer(mb);

      return;
    }

    pstVideoStream->bWaitIDR = true;
  }

  memset(&vStreamData, 0, sizeof(RKADK_VIDEO_STREAM_S));
  vStreamData.bEndOfStream = RKADK_FALSE;
  vStreamData.u32Seq = pstVideoStream->videoSeq;
  vStreamData.astPack.apu8Addr = (RKADK_U8 *)RK_MPI_MB_GetPtr(mb);
  vStreamData.astPack.au32Len = RK_MPI_MB_GetSize(mb);
  vStreamData.astPack.u64PTS = RK_MPI_MB_GetTimestamp(mb);
  vStreamData.astPack.stDataType.enPayloadType = pstVideoStream->enCodecType;
  RKADK_STREAM_GetNaluType(pstVideoStream->enCodecType,
                           &vStreamData.astPack.stDataType, s32NaluType);

  g_pstVencCB[pstVideoStream->u32CamId](&vStreamData);
  pstVideoStream->videoSeq++;

  if(!pstVideoStream->bVencChnMux)
    RK_MPI_MB_ReleaseBuffer(mb);
}

static void RKADK_STREAM_VideoSetChn(RKADK_PARAM_STREAM_CFG_S *pstStreamCfg,
                                     MPP_CHN_S *pstViChn,
                                     MPP_CHN_S *pstVencChn) {
  pstViChn->enModId = RK_ID_VI;
  pstViChn->s32DevId = 0;
  pstViChn->s32ChnId = pstStreamCfg->vi_attr.u32ViChn;

  pstVencChn->enModId = RK_ID_VENC;
  pstVencChn->s32DevId = 0;
  pstVencChn->s32ChnId = pstStreamCfg->attribute.venc_chn;
}

static int RKADK_STREAM_SetVencAttr(RKADK_U32 u32CamID,
                                    RKADK_PARAM_STREAM_CFG_S *pstStreamCfg,
                                    VENC_CHN_ATTR_S *pstVencAttr,
                                    RKADK_CODEC_TYPE_E enCodecType) {
  int ret;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  RKADK_CHECK_POINTER(pstVencAttr, RKADK_FAILURE);
  memset(pstVencAttr, 0, sizeof(VENC_CHN_ATTR_S));

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(u32CamID);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  pstVencAttr->stRcAttr.enRcMode =
      RKADK_PARAM_GetRcMode(pstStreamCfg->attribute.rc_mode, enCodecType);

  ret =
      RKADK_MEDIA_SetRcAttr(&pstVencAttr->stRcAttr, pstStreamCfg->attribute.gop,
                            pstStreamCfg->attribute.bitrate,
                            pstSensorCfg->framerate, pstSensorCfg->framerate);
  if (ret) {
    RKADK_LOGE("RKADK_MEDIA_SetRcAttr failed");
    return -1;
  }

  if (enCodecType == RKADK_CODEC_TYPE_H265)
    pstVencAttr->stVencAttr.stAttrH265e.bScaleList =
        (RK_BOOL)pstStreamCfg->attribute.venc_param.scaling_list;

  pstVencAttr->stVencAttr.enType = RKADK_MEDIA_GetRkCodecType(enCodecType);
  pstVencAttr->stVencAttr.imageType = pstStreamCfg->vi_attr.stChnAttr.enPixFmt;
  pstVencAttr->stVencAttr.u32PicWidth = pstStreamCfg->attribute.width;
  pstVencAttr->stVencAttr.u32PicHeight = pstStreamCfg->attribute.height;
  pstVencAttr->stVencAttr.u32VirWidth = pstStreamCfg->attribute.width;
  pstVencAttr->stVencAttr.u32VirHeight = pstStreamCfg->attribute.height;
  pstVencAttr->stVencAttr.u32Profile = pstStreamCfg->attribute.profile;
  pstVencAttr->stVencAttr.bFullRange =
      (RK_BOOL)pstStreamCfg->attribute.venc_param.full_range;

  return 0;
}

RKADK_S32
RKADK_STREAM_VencRegisterCallback(RKADK_U32 u32CamID,
                                  RKADK_VENC_DATA_PROC_FUNC pfnDataCB) {
  RKADK_CHECK_CAMERAID(u32CamID, RKADK_FAILURE);
  g_pstVencCB[u32CamID] = pfnDataCB;
  return 0;
}

RKADK_S32 RKADK_STREAM_VencUnRegisterCallback(RKADK_U32 u32CamID) {
  RKADK_CHECK_CAMERAID(u32CamID, RKADK_FAILURE);
  g_pstVencCB[u32CamID] = NULL;
  return 0;
}

static RKADK_S32 RKADK_STREAM_VencGetData(RKADK_U32 u32CamId,
                                          MPP_CHN_S *pstVencChn,
                                          VIDEO_STREAM_INFO_S *pstStreamInfo) {
  int ret = 0;

  if (pstStreamInfo->bVencChnMux) {
    ret = RKADK_MEDIA_GetMediaBuffer(pstVencChn, RKADK_STREAM_VencOutCb, pstStreamInfo);
  } else {
    ret = RK_MPI_SYS_RegisterOutCbEx(pstVencChn, RKADK_STREAM_VencOutCb, pstStreamInfo);
    if (ret) {
      RKADK_LOGE("Register output callback for VENC[%d] error %d",
                 pstVencChn->s32ChnId, ret);
      return ret;
    }

    VENC_RECV_PIC_PARAM_S stRecvParam;
    stRecvParam.s32RecvPicNum = 0;
    ret = RK_MPI_VENC_StartRecvFrame(pstVencChn->s32ChnId, &stRecvParam);
    if (ret) {
      RKADK_LOGE("RK_MPI_VENC_StartRecvFrame failed = %d", ret);
      return ret;
    }
  }

  return ret;
}

RKADK_S32 RKADK_STREAM_VideoInit(RKADK_U32 u32CamID,
                                 RKADK_CODEC_TYPE_E enCodecType) {
  int ret = 0;
  MPP_CHN_S stViChn;
  MPP_CHN_S stVencChn;
  RKADK_STREAM_TYPE_E enType;
  VENC_RC_PARAM_S stVencRcParam;

  RKADK_CHECK_CAMERAID(u32CamID, RKADK_FAILURE);

  RKADK_LOGI("Preview[%d, %d] Video Init...", u32CamID, enCodecType);

  if (RKADK_STREAM_CheckCodecType(true, enCodecType))
    return -1;

  VIDEO_STREAM_INFO_S *videoStream = &g_videoStream[u32CamID];
  if (videoStream->init) {
    RKADK_LOGI("stream: camera[%d] has been initialized", u32CamID);
    return 0;
  }
  memset(videoStream, 0, sizeof(VIDEO_STREAM_INFO_S));
  videoStream->enCodecType = enCodecType;
  videoStream->u32CamId = u32CamID;

  RK_MPI_SYS_Init();
  RKADK_PARAM_Init();

  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg =
      RKADK_PARAM_GetStreamCfg(u32CamID, RKADK_STREAM_TYPE_PREVIEW);
  if (!pstStreamCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  if (pstStreamCfg->attribute.codec_type != enCodecType)
    RKADK_LOGW("enCodecType(%d) != INI codec type(%d)", enCodecType,
               pstStreamCfg->attribute.codec_type);

  RKADK_STREAM_VideoSetChn(pstStreamCfg, &stViChn, &stVencChn);

  enType = RKADK_PARAM_VencChnMux(u32CamID, stVencChn.s32ChnId);
  if (enType != RKADK_STREAM_TYPE_BUTT && enType != RKADK_STREAM_TYPE_PREVIEW) {
    switch(enType) {
    case RKADK_STREAM_TYPE_VIDEO_MAIN:
      RKADK_LOGI("Preview and Record main venc[%d] mux", stVencChn.s32ChnId);
      break;
    case RKADK_STREAM_TYPE_VIDEO_SUB:
      RKADK_LOGI("Preview and Record sub venc[%d] mux", stVencChn.s32ChnId);
      break;
    case RKADK_STREAM_TYPE_PREVIEW:
      RKADK_LOGI("Preview and Preview venc[%d] mux", stVencChn.s32ChnId);
      break;
    default:
      RKADK_LOGW("Invaild venc[%d] mux, enType[%d]", stVencChn.s32ChnId, enType);
      break;
    }
    videoStream->bVencChnMux = true;
  }

  // Create VI
  ret = RKADK_MPI_VI_Init(u32CamID, stViChn.s32ChnId,
                          &(pstStreamCfg->vi_attr.stChnAttr));
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_Init faled %d", ret);
    return ret;
  }

  // Create VENC
  VENC_CHN_ATTR_S stVencChnAttr;
  ret = RKADK_STREAM_SetVencAttr(u32CamID, pstStreamCfg, &stVencChnAttr,
                                 enCodecType);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_SetVencAttr failed");
    goto failed;
  }

  ret = RKADK_MPI_VENC_Init(stVencChn.s32ChnId, &stVencChnAttr);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VENC_Init failed(%d)", ret);
    goto failed;
  }

  ret = RKADK_PARAM_GetRcParam(pstStreamCfg->attribute, &stVencRcParam);
  if (!ret) {
    ret = RK_MPI_VENC_SetRcParam(stVencChn.s32ChnId, &stVencRcParam);
    if (ret)
      RKADK_LOGW("RK_MPI_VENC_SetRcParam failed(%d), use default cfg", ret);
  }

  ret = RKADK_STREAM_VencGetData(u32CamID, &stVencChn, videoStream);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_VencGetData failed(%d)", ret);
    goto failed;
  }

  ret = RKADK_MPI_SYS_Bind(&stViChn, &stVencChn);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_SYS_Bind failed(%d)", ret);
    goto failed;
  }

  videoStream->init = true;
  RKADK_LOGI("Preview[%d, %d] Video Init End...", u32CamID, enCodecType);
  return 0;

failed:
  RKADK_LOGE("failed");
  RKADK_MPI_VENC_DeInit(stVencChn.s32ChnId);
  RKADK_MPI_VI_DeInit(u32CamID, stViChn.s32ChnId);
  return ret;
}

RKADK_S32 RKADK_STREAM_VideoDeInit(RKADK_U32 u32CamID) {
  int ret = 0;
  MPP_CHN_S stViChn;
  MPP_CHN_S stVencChn;

  RKADK_CHECK_CAMERAID(u32CamID, RKADK_FAILURE);

  RKADK_LOGI("Preview[%d] Video DeInit...", u32CamID);

  VIDEO_STREAM_INFO_S *videoStream = &g_videoStream[u32CamID];
  if (!videoStream->init) {
    RKADK_LOGI("u32CamID[%d] video has been deinit", u32CamID);
    return 0;
  }

  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg =
      RKADK_PARAM_GetStreamCfg(u32CamID, RKADK_STREAM_TYPE_PREVIEW);
  if (!pstStreamCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  RKADK_STREAM_VideoSetChn(pstStreamCfg, &stViChn, &stVencChn);

  if (videoStream->bVencChnMux)
    RKADK_MEDIA_StopGetMediaBuffer(&stVencChn, RKADK_STREAM_VencOutCb);

  // unbind first
  ret = RKADK_MPI_SYS_UnBind(&stViChn, &stVencChn);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_SYS_UnBind failed(%d)", ret);
    return ret;
  }

  // destroy venc before vi
  ret = RKADK_MPI_VENC_DeInit(stVencChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VENC_DeInit failed(%d)", ret);
    return ret;
  }

  // destroy vi
  ret = RKADK_MPI_VI_DeInit(u32CamID, stViChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_DeInit failed %d", ret);
    return ret;
  }

  videoStream->init = false;
  RKADK_LOGI("Preview[%d] Video DeInit End...", u32CamID);
  return 0;
}

RKADK_S32 RKADK_STREAM_VencStart(RKADK_U32 u32CamID,
                                 RKADK_CODEC_TYPE_E enCodecType,
                                 RKADK_S32 s32FrameCnt) {
  VIDEO_STREAM_INFO_S *videoStream;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamID, RKADK_FAILURE);

  if (RKADK_STREAM_CheckCodecType(true, enCodecType))
    return -1;

  videoStream = &g_videoStream[u32CamID];
  if (!videoStream->init) {
    RKADK_LOGI("u32CamID[%d] VENC uninitialized", u32CamID);
    return -1;
  }

  if (videoStream->start)
    return 0;

  pstStreamCfg = RKADK_PARAM_GetStreamCfg(u32CamID, RKADK_STREAM_TYPE_PREVIEW);
  if (!pstStreamCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  videoStream->start = true;
  videoStream->bRequestIDR = false;
  videoStream->bWaitIDR = false;

  // multiplex venc chn, thread get mediabuffer
  if (videoStream->bVencChnMux)
    return 0;

  VENC_RECV_PIC_PARAM_S stRecvParam;
  stRecvParam.s32RecvPicNum = s32FrameCnt;
  return RK_MPI_VENC_StartRecvFrame(pstStreamCfg->attribute.venc_chn,
                                    &stRecvParam);
}

RKADK_S32 RKADK_STREAM_VencStop(RKADK_U32 u32CamID) {
  VIDEO_STREAM_INFO_S *videoStream;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamID, RKADK_FAILURE);

  videoStream = &g_videoStream[u32CamID];
  if (!videoStream->init) {
    RKADK_LOGI("u32CamID[%d] VENC uninitialized", u32CamID);
    return -1;
  }

  if (!videoStream->start)
    return 0;

  pstStreamCfg = RKADK_PARAM_GetStreamCfg(u32CamID, RKADK_STREAM_TYPE_PREVIEW);
  if (!pstStreamCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  videoStream->start = false;

  // multiplex venc chn, thread get mediabuffer
  if (videoStream->bVencChnMux)
    return 0;

  VENC_RECV_PIC_PARAM_S stRecvParam;
  stRecvParam.s32RecvPicNum = 0;
  return RK_MPI_VENC_StartRecvFrame(pstStreamCfg->attribute.venc_chn,
                                    &stRecvParam);
}

RKADK_S32 RKADK_STREAM_GetVideoInfo(RKADK_U32 u32CamID,
                                    RKADK_VIDEO_INFO_S *pstVideoInfo) {
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg = NULL;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  RKADK_CHECK_POINTER(pstVideoInfo, RKADK_FAILURE);
  RKADK_CHECK_CAMERAID(u32CamID, RKADK_FAILURE);

  RKADK_PARAM_Init();

  pstStreamCfg = RKADK_PARAM_GetStreamCfg(u32CamID, RKADK_STREAM_TYPE_PREVIEW);
  if (!pstStreamCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  memset(pstVideoInfo, 0, sizeof(RKADK_VIDEO_INFO_S));
  if (!g_videoStream[u32CamID].init)
    pstVideoInfo->enCodecType = pstStreamCfg->attribute.codec_type;
  else
    pstVideoInfo->enCodecType = g_videoStream[u32CamID].enCodecType;

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(u32CamID);
  if (!pstStreamCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  pstVideoInfo->u32Width = pstStreamCfg->attribute.width;
  pstVideoInfo->u32Height = pstStreamCfg->attribute.height;
  pstVideoInfo->u32BitRate = pstStreamCfg->attribute.bitrate;
  pstVideoInfo->u32FrameRate = pstSensorCfg->framerate;
  pstVideoInfo->u32Gop = pstStreamCfg->attribute.gop;
  return 0;
}

/**************************************************/
/*                     Audio API                  */
/**************************************************/
static void *RKADK_STREAM_GetMB(void *params) {
  MEDIA_BUFFER mb = NULL;
  MPP_CHN_S *chn = &g_audioStream.stAencChn;
  RKADK_AUDIO_STREAM_S stStreamData;

  while (g_audioStream.bGetBuffer) {
    mb = RK_MPI_SYS_GetMediaBuffer(chn->enModId, chn->s32ChnId, -1);
    if (!mb) {
      RKADK_LOGE("RK_MPI_SYS_GetMediaBuffer get null buffer!");
      break;
    }

    stStreamData.pStream = (RKADK_U8 *)RK_MPI_MB_GetPtr(mb);
    stStreamData.u32Len = RK_MPI_MB_GetSize(mb);
    stStreamData.u32Seq = g_audioStream.audioSeq;
    stStreamData.u64TimeStamp = RK_MPI_MB_GetTimestamp(mb);

    if (g_pstAencCB && g_audioStream.start)
      g_pstAencCB(&stStreamData);

    g_audioStream.audioSeq++;
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  RK_MPI_SYS_StopGetMediaBuffer(chn->enModId, chn->s32ChnId);
  RKADK_LOGD("Exit aenc read thread");
  return NULL;
}

static void *RKADK_STREAM_GetPcmMB(void *params) {
  MEDIA_BUFFER mb = NULL;
  MPP_CHN_S *chn = &g_audioStream.stAiChn;
  RKADK_AUDIO_STREAM_S stStreamData;

  while (g_audioStream.bGetBuffer) {
    mb = RK_MPI_SYS_GetMediaBuffer(chn->enModId, chn->s32ChnId, -1);
    if (!mb) {
      RKADK_LOGE("RK_MPI_SYS_GetMediaBuffer get null buffer!");
      break;
    }

    stStreamData.pStream = (RKADK_U8 *)RK_MPI_MB_GetPtr(mb);
    stStreamData.u32Len = RK_MPI_MB_GetSize(mb);
    stStreamData.u32Seq = g_audioStream.pcmSeq;
    stStreamData.u64TimeStamp = RK_MPI_MB_GetTimestamp(mb);

    if (g_pstPcmCB && g_audioStream.start)
      g_pstPcmCB(&stStreamData);

    g_audioStream.pcmSeq++;
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  RK_MPI_SYS_StopGetMediaBuffer(chn->enModId, chn->s32ChnId);
  RKADK_LOGD("Exit pcm read thread");
  return NULL;
}

static RKADK_U16 RKADK_STREAM_GetAudioBitWidth(SAMPLE_FORMAT_E sample_format) {
  RKADK_U16 bitWidth = -1;
  switch (sample_format) {
  case RK_SAMPLE_FMT_U8:
  case RK_SAMPLE_FMT_U8P:
    bitWidth = 8;
    break;
  case RK_SAMPLE_FMT_S16:
  case RK_SAMPLE_FMT_S16P:
    bitWidth = 16;
    break;
  case RK_SAMPLE_FMT_S32:
  case RK_SAMPLE_FMT_S32P:
    bitWidth = 32;
    break;
  case RK_SAMPLE_FMT_FLT:
  case RK_SAMPLE_FMT_FLTP:
    bitWidth = sizeof(float);
    break;
  case RK_SAMPLE_FMT_G711A:
  case RK_SAMPLE_FMT_G711U:
    bitWidth = 8;
    break;
  default:
    break;
  }

  return bitWidth;
}

static int RKADK_STREAM_CreateDataThread() {
  int ret = 0;

  g_audioStream.bGetBuffer = true;

  if (g_audioStream.enCodecType == RKADK_CODEC_TYPE_PCM) {
    ret = pthread_create(&g_audioStream.pcmTid, NULL, RKADK_STREAM_GetPcmMB,
                         NULL);
  } else {
    ret = pthread_create(&g_audioStream.tid, NULL, RKADK_STREAM_GetMB, NULL);

    if (g_pstPcmCB)
      ret |= pthread_create(&g_audioStream.pcmTid, NULL, RKADK_STREAM_GetPcmMB,
                            NULL);
  }

  if (ret) {
    RKADK_LOGE("Create read audio thread for failed %d", ret);
    return ret;
  }

  return 0;
}

static int RKADK_STREAM_DestoryDataThread() {
  int ret = 0;

  g_audioStream.bGetBuffer = false;
  if (g_audioStream.tid) {
    ret = pthread_join(g_audioStream.tid, NULL);
    if (ret)
      RKADK_LOGE("read aenc thread exit failed!");
    else
      RKADK_LOGD("read aenc thread exit ok");
    g_audioStream.tid = 0;
  }

  if (g_audioStream.pcmTid) {
    ret = pthread_join(g_audioStream.pcmTid, NULL);
    if (ret)
      RKADK_LOGE("read pcm thread exit failed!");
    else
      RKADK_LOGD("read pcm thread exit ok");
    g_audioStream.pcmTid = 0;
  }

  return ret;
}

static RKADK_S32
RKADK_STREAM_SetAiConfig(MPP_CHN_S *pstAiChn, AI_CHN_ATTR_S *pstAiAttr,
                         RKADK_PARAM_AUDIO_CFG_S *pstAudioParam) {
  RKADK_CHECK_POINTER(pstAiChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstAiAttr, RKADK_FAILURE);

  pstAiAttr->pcAudioNode = pstAudioParam->audio_node;
  pstAiAttr->enSampleFormat = pstAudioParam->sample_format;
  pstAiAttr->u32NbSamples = pstAudioParam->samples_per_frame;
  pstAiAttr->u32SampleRate = pstAudioParam->samplerate;
  pstAiAttr->u32Channels = pstAudioParam->channels;
  pstAiAttr->enAiLayout = pstAudioParam->ai_layout;

  pstAiChn->enModId = RK_ID_AI;
  pstAiChn->s32DevId = 0;
  pstAiChn->s32ChnId = PREVIEW_AI_CHN;
  return 0;
}

static RKADK_S32 RKADK_STREAM_SetAencConfig(RKADK_CODEC_TYPE_E enCodecType,
                                            MPP_CHN_S *pstAencChn,
                                            AENC_CHN_ATTR_S *pstAencAttr) {
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  RKADK_CHECK_POINTER(pstAencChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstAencAttr, RKADK_FAILURE);

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  pstAencAttr->u32Bitrate = pstAudioParam->bitrate;

  if (enCodecType == LIVE_AUDIO_CODEC_TYPE)
    pstAencChn->s32ChnId = LIVE_AENC_CHN; // and record reuse aenc channel
  else
    pstAencChn->s32ChnId = PREVIEW_AENC_CHN;

  switch (enCodecType) {
  case RKADK_CODEC_TYPE_G711A:
    pstAencAttr->stAencG711A.u32Channels = pstAudioParam->channels;
    pstAencAttr->stAencG711A.u32NbSample = pstAudioParam->samples_per_frame;
    pstAencAttr->stAencG711A.u32SampleRate = pstAudioParam->samplerate;
    break;
  case RKADK_CODEC_TYPE_G711U:
    pstAencAttr->stAencG711U.u32Channels = pstAudioParam->channels;
    pstAencAttr->stAencG711U.u32NbSample = pstAudioParam->samples_per_frame;
    pstAencAttr->stAencG711U.u32SampleRate = pstAudioParam->samplerate;
    break;
  case RKADK_CODEC_TYPE_MP3:
    pstAencAttr->stAencMP3.u32Channels = pstAudioParam->channels;
    pstAencAttr->stAencMP3.u32SampleRate = pstAudioParam->samplerate;
    break;
  case RKADK_CODEC_TYPE_G726:
    pstAencAttr->stAencG726.u32Channels = pstAudioParam->channels;
    pstAencAttr->stAencG726.u32SampleRate = pstAudioParam->channels;
    break;
  case RKADK_CODEC_TYPE_MP2:
    pstAencAttr->stAencMP2.u32Channels = pstAudioParam->channels;
    pstAencAttr->stAencMP2.u32SampleRate = pstAudioParam->channels;
    break;

  default:
    RKADK_LOGE("Nonsupport codec type(%d)", enCodecType);
    return -1;
  }

  pstAencAttr->enCodecType = RKADK_MEDIA_GetRkCodecType(enCodecType);
  pstAencChn->enModId = RK_ID_AENC;
  pstAencChn->s32DevId = 0;
  pstAencAttr->u32Quality = 1;
  return 0;
}

RKADK_VOID
RKADK_STREAM_AencRegisterCallback(RKADK_CODEC_TYPE_E enCodecType,
                                  RKADK_AENC_DATA_PROC_FUNC pfnDataCB) {
  int ret = 0;

  if (enCodecType == RKADK_CODEC_TYPE_PCM) {
    g_pstPcmCB = pfnDataCB;
    if (g_audioStream.init && !g_audioStream.pcmTid) {
      g_audioStream.bGetBuffer = true;
      ret = pthread_create(&g_audioStream.pcmTid, NULL, RKADK_STREAM_GetPcmMB,
                           NULL);
    }
  } else {
    g_pstAencCB = pfnDataCB;
    if (g_audioStream.init && !g_audioStream.tid) {
      g_audioStream.bGetBuffer = true;
      ret = pthread_create(&g_audioStream.tid, NULL, RKADK_STREAM_GetMB, NULL);
    }
  }

  if (ret)
    RKADK_LOGE("Create read audio thread for failed %d", ret);
}

RKADK_VOID RKADK_STREAM_AencUnRegisterCallback(RKADK_CODEC_TYPE_E enCodecType) {
  if (enCodecType == RKADK_CODEC_TYPE_PCM)
    g_pstPcmCB = NULL;
  else
    g_pstAencCB = NULL;
}

RKADK_S32 RKADK_STREAM_AudioInit(RKADK_CODEC_TYPE_E enCodecType) {
  int ret;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  if (g_audioStream.init) {
    RKADK_LOGI("Audio has been initialized");
    return 0;
  }

  RKADK_LOGI("Preview[%d] Audio Init...", enCodecType);

  if (RKADK_STREAM_CheckCodecType(false, enCodecType))
    return -1;

  memset(&g_audioStream, 0, sizeof(AUDIO_STREAM_INFO_S));
  g_audioStream.enCodecType = enCodecType;
  g_audioStream.start = false;
  g_audioStream.pcmSeq = 0;
  g_audioStream.audioSeq = 0;
  g_audioStream.bGetBuffer = false;

  RKADK_PARAM_Init();
  RK_MPI_SYS_Init();

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  // Create AI
  AI_CHN_ATTR_S aiAttr;
  ret =
      RKADK_STREAM_SetAiConfig(&g_audioStream.stAiChn, &aiAttr, pstAudioParam);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_SetAiConfig failed");
    return ret;
  }

  ret = RKADK_MPI_AI_Init(g_audioStream.stAiChn.s32ChnId, &aiAttr,
                          pstAudioParam->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_Init faile(%d)", ret);
    return ret;
  }

  if (enCodecType != RKADK_CODEC_TYPE_PCM) {
    AENC_CHN_ATTR_S aencAttr;
    if (RKADK_STREAM_SetAencConfig(enCodecType, &g_audioStream.stAencChn,
                                   &aencAttr)) {
      RKADK_LOGE("StreamSetAencChnAttr error, enCodecType = %d", enCodecType);
      goto pcm_mode;
    }

    ret = RKADK_MPI_AENC_Init(g_audioStream.stAencChn.s32ChnId, &aencAttr);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_AENC_Init error %d", ret);
      goto failed;
    }

    ret = RKADK_STREAM_CreateDataThread();
    if (ret)
      goto failed;

    ret = RKADK_MPI_SYS_Bind(&g_audioStream.stAiChn, &g_audioStream.stAencChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_Bind error %d", ret);
      goto failed;
    }

    g_audioStream.init = true;
  RKADK_LOGI("Preview[%d] Audio Init End...", enCodecType);
    return 0;
  }

pcm_mode:
  RKADK_LOGI("PCM Mode");
  ret = RKADK_STREAM_CreateDataThread();
  if (ret)
    goto failed;

  ret = RK_MPI_AI_StartStream(g_audioStream.stAiChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("Start AI error %d", ret);
    goto failed;
  }

  g_audioStream.init = true;
  return 0;

failed:
  RKADK_MPI_AENC_DeInit(g_audioStream.stAencChn.s32ChnId);
  RKADK_MPI_AI_DeInit(g_audioStream.stAiChn.s32ChnId, pstAudioParam->vqe_mode);
  return ret;
}

RKADK_S32 RKADK_STREAM_AudioDeInit(RKADK_CODEC_TYPE_E enCodecType) {
  int ret = 0;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  RKADK_LOGI("Preview[%d] Audio DeInit...", enCodecType);

  if (!g_audioStream.init) {
    RKADK_LOGI("Audio has been deinit");
    return 0;
  }

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  if (RKADK_STREAM_CheckCodecType(false, enCodecType))
    return -1;

  RKADK_STREAM_DestoryDataThread();

  if (enCodecType != RKADK_CODEC_TYPE_PCM) {
    ret =
        RKADK_MPI_SYS_UnBind(&g_audioStream.stAiChn, &g_audioStream.stAencChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_UnBind error %d", ret);
      return ret;
    }

    ret = RKADK_MPI_AENC_DeInit(g_audioStream.stAencChn.s32ChnId);
    if (ret) {
      RKADK_LOGE("Destroy AENC[%d] error %d", g_audioStream.stAencChn.s32ChnId,
                 ret);
      return ret;
    }
  }

  ret = RKADK_MPI_AI_DeInit(g_audioStream.stAiChn.s32ChnId,
                            pstAudioParam->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_DeInit failed(%d)", ret);
    return ret;
  }

  g_audioStream.enCodecType = RKADK_CODEC_TYPE_BUTT;
  g_audioStream.init = false;
  g_audioStream.start = false;
  RKADK_LOGI("Preview[%d] Audio DeInit End...", enCodecType);
  return 0;
}

RKADK_S32 RKADK_STREAM_AencStart() {
  if (!g_audioStream.init) {
    RKADK_LOGE("Audio not initialized");
    return -1;
  }

  g_audioStream.start = true;
  return 0;
}

RKADK_S32 RKADK_STREAM_AencStop() {
  if (!g_audioStream.init) {
    RKADK_LOGE("Audio not initialized");
    return -1;
  }

  g_audioStream.start = false;
  return 0;
}

RKADK_S32 RKADK_STREAM_GetAudioInfo(RKADK_AUDIO_INFO_S *pstAudioInfo) {
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  RKADK_CHECK_POINTER(pstAudioInfo, RKADK_FAILURE);
  memset(pstAudioInfo, 0, sizeof(RKADK_AUDIO_INFO_S));

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  if (!g_audioStream.init)
    pstAudioInfo->enCodecType = RKADK_CODEC_TYPE_PCM;
  else
    pstAudioInfo->enCodecType = g_audioStream.enCodecType;

  pstAudioInfo->u16SampleBitWidth =
      RKADK_STREAM_GetAudioBitWidth(pstAudioParam->sample_format);
  pstAudioInfo->u32ChnCnt = pstAudioParam->channels;
  pstAudioInfo->u32SampleRate = pstAudioParam->samplerate;
  pstAudioInfo->u32AvgBytesPerSec = pstAudioParam->bitrate / 8;
  pstAudioInfo->u32SamplesPerFrame = pstAudioParam->samples_per_frame;

  return 0;
}
