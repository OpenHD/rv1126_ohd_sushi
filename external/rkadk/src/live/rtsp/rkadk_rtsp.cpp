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

#include "rkadk_rtsp.h"
#include "rkadk_log.h"
#include "rkadk_param.h"
#include "rkmedia_api.h"
#include "rtsp_demo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
  bool start;
  bool bRequestIDR;
  bool bWaitIDR;
  bool bVencChnMux;
  RKADK_U32 u32CamId;
  rtsp_demo_handle stRtspHandle;
  rtsp_session_handle stRtspSession;
} RKADK_RTSP_HANDLE_S;

static void RKADK_RTSP_VideoSetChn(RKADK_PARAM_STREAM_CFG_S *pstLiveCfg,
                                   MPP_CHN_S *pstViChn, MPP_CHN_S *pstVencChn) {
  pstViChn->enModId = RK_ID_VI;
  pstViChn->s32DevId = 0;
  pstViChn->s32ChnId = pstLiveCfg->vi_attr.u32ViChn;

  pstVencChn->enModId = RK_ID_VENC;
  pstVencChn->s32DevId = 0;
  pstVencChn->s32ChnId = pstLiveCfg->attribute.venc_chn;
}

static int RKADK_RTSP_SetVencAttr(RKADK_U32 u32CamId,
                                  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg,
                                  VENC_CHN_ATTR_S *pstVencAttr) {
  int ret;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  RKADK_CHECK_POINTER(pstVencAttr, RKADK_FAILURE);
  memset(pstVencAttr, 0, sizeof(VENC_CHN_ATTR_S));

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(u32CamId);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  pstVencAttr->stRcAttr.enRcMode = RKADK_PARAM_GetRcMode(
      pstLiveCfg->attribute.rc_mode, pstLiveCfg->attribute.codec_type);

  ret = RKADK_MEDIA_SetRcAttr(&pstVencAttr->stRcAttr, pstLiveCfg->attribute.gop,
                              pstLiveCfg->attribute.bitrate,
                              pstSensorCfg->framerate, pstSensorCfg->framerate);
  if (ret) {
    RKADK_LOGE("RKADK_MEDIA_SetRcAttr failed");
    return -1;
  }

  if (pstLiveCfg->attribute.codec_type == RKADK_CODEC_TYPE_H265)
    pstVencAttr->stVencAttr.stAttrH265e.bScaleList =
        (RK_BOOL)pstLiveCfg->attribute.venc_param.scaling_list;

  pstVencAttr->stVencAttr.enType =
      RKADK_MEDIA_GetRkCodecType(pstLiveCfg->attribute.codec_type);
  pstVencAttr->stVencAttr.imageType = pstLiveCfg->vi_attr.stChnAttr.enPixFmt;
  pstVencAttr->stVencAttr.u32PicWidth = pstLiveCfg->attribute.width;
  pstVencAttr->stVencAttr.u32PicHeight = pstLiveCfg->attribute.height;
  pstVencAttr->stVencAttr.u32VirWidth = pstLiveCfg->attribute.width;
  pstVencAttr->stVencAttr.u32VirHeight = pstLiveCfg->attribute.height;
  pstVencAttr->stVencAttr.u32Profile = pstLiveCfg->attribute.profile;
  pstVencAttr->stVencAttr.bFullRange =
      (RK_BOOL)pstLiveCfg->attribute.venc_param.full_range;

  return 0;
}

static RKADK_S32 RKADK_RTSP_InitService(RKADK_CODEC_TYPE_E enCodecType,
                                        RKADK_U32 port, const char *path,
                                        RKADK_RTSP_HANDLE_S *pHandle) {
  int ret = 0;

  pHandle->stRtspHandle = create_rtsp_demo(port);
  pHandle->stRtspSession = rtsp_new_session(pHandle->stRtspHandle, path);
  if (enCodecType == RKADK_CODEC_TYPE_H264) {
    ret = rtsp_set_video(pHandle->stRtspSession, RTSP_CODEC_ID_VIDEO_H264, NULL,
                         0);
    if (ret) {
      RKADK_LOGE("rtsp_set_video failed(%d)", ret);
      return -1;
    }
  } else if (enCodecType == RKADK_CODEC_TYPE_H265) {
    ret = rtsp_set_video(pHandle->stRtspSession, RTSP_CODEC_ID_VIDEO_H265, NULL,
                         0);
    if (ret) {
      RKADK_LOGE("rtsp_set_video failed(%d)", ret);
      return -1;
    }
  } else {
    RKADK_LOGE("Unsupport enCodecType: %d", enCodecType);
    return -1;
  }

  return rtsp_sync_video_ts(pHandle->stRtspSession, rtsp_get_reltime(),
                            rtsp_get_ntptime());
}

static void RKADK_RTSP_DeInitService(RKADK_RTSP_HANDLE_S *pHandle) {
  if (pHandle->stRtspHandle)
    rtsp_del_demo(pHandle->stRtspHandle);
}

static RKADK_S32 RKADK_RTSP_RequestIDR(RKADK_U32 u32CamId, RKADK_U32 u32ChnId) {
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

static void RKADK_RTSP_VencOutCb(MEDIA_BUFFER mb, RKADK_VOID *handle) {
  RKADK_S32 s32NaluType;

  RKADK_RTSP_HANDLE_S *pHandle = (RKADK_RTSP_HANDLE_S *)handle;
  if (!pHandle) {
    RKADK_LOGE("Can't find rtsp handle");
    assert(pHandle);
  }

  if(!pHandle->start) {
    if(!pHandle->bVencChnMux)
      RK_MPI_MB_ReleaseBuffer(mb);

    return;
  }

  s32NaluType = RK_MPI_MB_GetFlag(mb);
  if (!pHandle->bWaitIDR) {
    if (s32NaluType != VENC_NALU_IDRSLICE) {
      if (!pHandle->bRequestIDR) {
        RKADK_LOGD("requst idr frame");
        RKADK_RTSP_RequestIDR(pHandle->u32CamId, RK_MPI_MB_GetChannelID(mb));
        pHandle->bRequestIDR = true;
      } else {
        RKADK_LOGD("wait first idr frame");
      }

      if(!pHandle->bVencChnMux)
        RK_MPI_MB_ReleaseBuffer(mb);
      return;
    }

    pHandle->bWaitIDR = true;
  }

  if (pHandle->stRtspHandle && pHandle->stRtspSession) {
    rtsp_tx_video(pHandle->stRtspSession, (uint8_t *)RK_MPI_MB_GetPtr(mb),
                  RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetTimestamp(mb));
    rtsp_do_event(pHandle->stRtspHandle);
  }

  if(!pHandle->bVencChnMux)
    RK_MPI_MB_ReleaseBuffer(mb);
}

static RKADK_S32 RKADK_RTSP_VencGetData(RKADK_U32 u32CamId,
                                        MPP_CHN_S *pstVencChn,
                                        RKADK_RTSP_HANDLE_S *pHandle) {
  int ret = 0;

  if (pHandle->bVencChnMux) {
    ret = RKADK_MEDIA_GetMediaBuffer(pstVencChn, RKADK_RTSP_VencOutCb,
                                     (RKADK_VOID *)pHandle);
  } else {
    ret = RK_MPI_SYS_RegisterOutCbEx(pstVencChn, RKADK_RTSP_VencOutCb,
                                     (RKADK_VOID *)pHandle);
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

RKADK_S32 RKADK_RTSP_Init(RKADK_U32 u32CamId, RKADK_U32 port, const char *path,
                          RKADK_MW_PTR *ppHandle) {
  int ret = 0;
  MPP_CHN_S stViChn;
  MPP_CHN_S stVencChn;
  RKADK_STREAM_TYPE_E enType;
  VENC_RC_PARAM_S stVencRcParam;
  RKADK_RTSP_HANDLE_S *pHandle;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);
  RKADK_CHECK_POINTER(path, RKADK_FAILURE);

  RKADK_LOGI("Rtsp[%d, %d, %s] Init...", u32CamId, port, path);

  if (*ppHandle) {
    RKADK_LOGE("rtsp handle has been created");
    return -1;
  }

  RK_MPI_SYS_Init();
  RKADK_PARAM_Init();

  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg =
      RKADK_PARAM_GetStreamCfg(u32CamId, RKADK_STREAM_TYPE_LIVE);
  if (!pstLiveCfg) {
    RKADK_LOGE("Live RKADK_PARAM_GetStreamCfg Live failed");
    return -1;
  }

  pHandle = (RKADK_RTSP_HANDLE_S *)malloc(sizeof(RKADK_RTSP_HANDLE_S));
  if (!pHandle) {
    RKADK_LOGE("malloc pHandle failed");
    return -1;
  }
  memset(pHandle, 0, sizeof(RKADK_RTSP_HANDLE_S));
  pHandle->u32CamId = u32CamId;

  ret = RKADK_RTSP_InitService(pstLiveCfg->attribute.codec_type, port, path,
                               pHandle);
  if (ret) {
    RKADK_LOGE("RKADK_RTSP_InitService failed");
    free(pHandle);
    return -1;
  }

  RKADK_RTSP_VideoSetChn(pstLiveCfg, &stViChn, &stVencChn);

  enType = RKADK_PARAM_VencChnMux(u32CamId, stVencChn.s32ChnId);
  if (enType != RKADK_STREAM_TYPE_BUTT && enType != RKADK_STREAM_TYPE_LIVE) {
    switch(enType) {
    case RKADK_STREAM_TYPE_VIDEO_MAIN:
      RKADK_LOGI("Live and Record main venc[%d] mux", stVencChn.s32ChnId);
      break;
    case RKADK_STREAM_TYPE_VIDEO_SUB:
      RKADK_LOGI("Live and Record sub venc[%d] mux", stVencChn.s32ChnId);
      break;
    case RKADK_STREAM_TYPE_PREVIEW:
      RKADK_LOGI("Live and Preview venc[%d] mux", stVencChn.s32ChnId);
      break;
    default:
      RKADK_LOGW("Invaild venc[%d] mux, enType[%d]", stVencChn.s32ChnId, enType);
      break;
    }
    pHandle->bVencChnMux = true;
  }

  // Create VI
  ret = RKADK_MPI_VI_Init(u32CamId, stViChn.s32ChnId,
                          &(pstLiveCfg->vi_attr.stChnAttr));
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_Init faled %d", ret);
    goto failed;
  }

  // Create VENC
  VENC_CHN_ATTR_S stVencChnAttr;
  ret = RKADK_RTSP_SetVencAttr(u32CamId, pstLiveCfg, &stVencChnAttr);
  if (ret) {
    RKADK_LOGE("RKADK_RTSP_SetVencAttr failed");
    goto failed;
  }

  ret = RKADK_MPI_VENC_Init(stVencChn.s32ChnId, &stVencChnAttr);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VENC_Init failed(%d)", ret);
    goto failed;
  }

  ret = RKADK_PARAM_GetRcParam(pstLiveCfg->attribute, &stVencRcParam);
  if (!ret) {
    ret = RK_MPI_VENC_SetRcParam(stVencChn.s32ChnId, &stVencRcParam);
    if (ret)
      RKADK_LOGW("RK_MPI_VENC_SetRcParam failed(%d), use default cfg", ret);
  }

  ret = RKADK_RTSP_VencGetData(u32CamId, &stVencChn, pHandle);
  if (ret) {
    RKADK_LOGE("RKADK_RTSP_VencGetData failed(%d)", ret);
    goto failed;
  }

  ret = RKADK_MPI_SYS_Bind(&stViChn, &stVencChn);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_SYS_Bind failed(%d)", ret);
    goto failed;
  }

  *ppHandle = (RKADK_MW_PTR)pHandle;
  RKADK_LOGI("Rtsp[%d, %d, %s] Init End...", u32CamId, port, path);
  return 0;

failed:
  RKADK_LOGE("failed");
  RKADK_MPI_VENC_DeInit(stVencChn.s32ChnId);
  RKADK_MPI_VI_DeInit(u32CamId, stViChn.s32ChnId);

  if (pHandle) {
    RKADK_RTSP_DeInitService((RKADK_RTSP_HANDLE_S *)pHandle);
    free(pHandle);
  }

  return ret;
}

RKADK_S32 RKADK_RTSP_DeInit(RKADK_MW_PTR pHandle) {
  int ret = 0;
  MPP_CHN_S stViChn;
  MPP_CHN_S stVencChn;

  RKADK_CHECK_POINTER(pHandle, RKADK_FAILURE);
  RKADK_RTSP_HANDLE_S *pstHandle = (RKADK_RTSP_HANDLE_S *)pHandle;

  RKADK_LOGI("Rtsp[%d] DeInit...", pstHandle->u32CamId);

  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg =
      RKADK_PARAM_GetStreamCfg(pstHandle->u32CamId, RKADK_STREAM_TYPE_LIVE);
  if (!pstLiveCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  RKADK_RTSP_DeInitService((RKADK_RTSP_HANDLE_S *)pHandle);

  RKADK_RTSP_VideoSetChn(pstLiveCfg, &stViChn, &stVencChn);

  // exit get media buffer
  if (pstHandle->bVencChnMux)
    RKADK_MEDIA_StopGetMediaBuffer(&stVencChn, RKADK_RTSP_VencOutCb);

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
  ret = RKADK_MPI_VI_DeInit(pstHandle->u32CamId, stViChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_DeInit failed %d", ret);
    return ret;
  }

  RKADK_LOGI("Rtsp[%d] DeInit End...", pstHandle->u32CamId);
  free(pHandle);
  return 0;
}

RKADK_S32 RKADK_RTSP_Start(RKADK_MW_PTR pHandle) {
  RKADK_CHECK_POINTER(pHandle, RKADK_FAILURE);

  RKADK_RTSP_HANDLE_S *pstHandle = (RKADK_RTSP_HANDLE_S *)pHandle;
  if (pstHandle->start)
    return 0;

  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg =
      RKADK_PARAM_GetStreamCfg(pstHandle->u32CamId, RKADK_STREAM_TYPE_LIVE);
  if (!pstLiveCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  pstHandle->start = true;
  pstHandle->bRequestIDR = false;
  pstHandle->bWaitIDR = false;

  // multiplex venc chn, thread get mediabuffer
  if (pstHandle->bVencChnMux)
    return 0;

  VENC_RECV_PIC_PARAM_S stRecvParam;
  stRecvParam.s32RecvPicNum = -1;
  return RK_MPI_VENC_StartRecvFrame(pstLiveCfg->attribute.venc_chn,
                                    &stRecvParam);
}

RKADK_S32 RKADK_RTSP_Stop(RKADK_MW_PTR pHandle) {
  RKADK_CHECK_POINTER(pHandle, RKADK_FAILURE);

  RKADK_RTSP_HANDLE_S *pstHandle = (RKADK_RTSP_HANDLE_S *)pHandle;
  if (!pstHandle->start)
    return 0;

  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg =
      RKADK_PARAM_GetStreamCfg(pstHandle->u32CamId, RKADK_STREAM_TYPE_LIVE);
  if (!pstLiveCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  pstHandle->start = false;

  // multiplex venc chn, thread get mediabuffer
  if (pstHandle->bVencChnMux)
    return 0;

  VENC_RECV_PIC_PARAM_S stRecvParam;
  stRecvParam.s32RecvPicNum = 0;
  return RK_MPI_VENC_StartRecvFrame(pstLiveCfg->attribute.venc_chn,
                                    &stRecvParam);
}
