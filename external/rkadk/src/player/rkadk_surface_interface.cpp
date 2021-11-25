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

#include "rkadk_surface_interface.h"
#include "RTMediaBuffer.h"
#include "RTMediaData.h"
#include "rk_common.h"
#include "rk_mpi_vo.h"
#include "rkadk_common.h"
#include "rt_mem.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

#define VOP_LAYER_CLUSTER_0 0
#define VOP_LAYER_CLUSTER_1 1

static RKADK_S32 RKADK_VO_GetPictureData(VO_FRAME_INFO_S *pstFrameInfo,
                                         RTMediaBuffer *buffer) {
  RKADK_CHECK_POINTER(pstFrameInfo, RKADK_FAILURE);
  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);

  memcpy(pstFrameInfo->pData, buffer->getData(), buffer->getLength());
  return RKADK_SUCCESS;
}

static RKADK_S32 RKADK_VO_CreateGFXData(RKADK_U32 u32Width, RKADK_U32 u32Height,
                                        RKADK_U32 foramt, RKADK_U32 value,
                                        RKADK_VOID **pMblk,
                                        RTMediaBuffer *buffer) {
  VO_FRAME_INFO_S stFrameInfo;
  RKADK_U32 u32BuffSize;

  RKADK_CHECK_POINTER(pMblk, RKADK_FAILURE);
  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);

  u32BuffSize =
      RK_MPI_VO_CreateGraphicsFrameBuffer(u32Width, u32Height, foramt, pMblk);
  if (u32BuffSize == 0) {
    RKADK_LOGD("can not create gfx buffer");
    return RKADK_FAILURE;
  }

  RK_MPI_VO_GetFrameInfo(*pMblk, &stFrameInfo);
  RKADK_VO_GetPictureData(&stFrameInfo, buffer);

  return RKADK_SUCCESS;
}

static RKADK_S32
RKADK_VO_StartLayer(VO_LAYER voLayer,
                    const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr) {
  RKADK_S32 s32Ret = RKADK_SUCCESS;

  RKADK_CHECK_POINTER(pstLayerAttr, RKADK_FAILURE);

  s32Ret = RK_MPI_VO_SetLayerAttr(voLayer, pstLayerAttr);
  if (s32Ret) {
    RKADK_LOGD("Set layer attribute failed");
    return s32Ret;
  }

  s32Ret = RK_MPI_VO_EnableLayer(voLayer);
  if (s32Ret) {
    RKADK_LOGD("Enable layer failed");
    return s32Ret;
  }

  return s32Ret;
}

static RKADK_S32 RKADK_VO_StartDev(VO_DEV voDev, VO_PUB_ATTR_S *pstPubAttr) {
  RKADK_S32 s32Ret = RKADK_SUCCESS;

  RKADK_CHECK_POINTER(pstPubAttr, RKADK_FAILURE);

  s32Ret = RK_MPI_VO_SetPubAttr(voDev, pstPubAttr);
  if (s32Ret) {
    RKADK_LOGD("Set public attribute failed");
    return s32Ret;
  }

  s32Ret = RK_MPI_VO_Enable(voDev);
  if (s32Ret) {
    RKADK_LOGD("VO enable failed");
    return s32Ret;
  }

  return s32Ret;
}

static RKADK_S32 RKADK_VO_StopLayer(VO_LAYER voLayer) {
  return RK_MPI_VO_DisableLayer(voLayer);
}

static RKADK_S32 RKADK_VO_StopDev(VO_DEV voDev) {
  return RK_MPI_VO_Disable(voDev);
}

static PIXEL_FORMAT_E RKADK_FmtToRtfmt(RKADK_PLAYER_VO_FORMAT_E format) {
  PIXEL_FORMAT_E rtfmt;

  switch (format) {
  case VO_FORMAT_ARGB8888:
    rtfmt = RK_FMT_BGRA8888;
    break;
  case VO_FORMAT_ABGR8888:
    rtfmt = RK_FMT_RGBA8888;
    break;
  case VO_FORMAT_RGB888:
    rtfmt = RK_FMT_BGR888;
    break;
  case VO_FORMAT_BGR888:
    rtfmt = RK_FMT_RGB888;
    break;
  case VO_FORMAT_ARGB1555:
    rtfmt = RK_FMT_BGRA5551;
    break;
  case VO_FORMAT_ABGR1555:
    rtfmt = RK_FMT_RGBA5551;
    break;
  case VO_FORMAT_NV12:
    rtfmt = RK_FMT_YUV420SP;
    break;
  case VO_FORMAT_NV21:
    rtfmt = RK_FMT_YUV420SP_VU;
    break;
  default:
    RKADK_LOGW("invalid format: %d", format);
    rtfmt = RK_FMT_BUTT;
  }

  return rtfmt;
}

static RKADK_S32 RKADK_VO_SetRtSyncInfo(VO_SYNC_INFO_S *pstSyncInfo,
                                        VIDEO_FRAMEINFO_S stFrmInfo) {
  RKADK_CHECK_POINTER(pstSyncInfo, RKADK_FAILURE);

  pstSyncInfo->bIdv = stFrmInfo.stSyncInfo.bIdv;
  pstSyncInfo->bIhs = stFrmInfo.stSyncInfo.bIhs;
  pstSyncInfo->bIvs = stFrmInfo.stSyncInfo.bIvs;
  pstSyncInfo->bSynm = stFrmInfo.stSyncInfo.bSynm;
  pstSyncInfo->bIop = stFrmInfo.stSyncInfo.bIop;
  pstSyncInfo->u16FrameRate = (stFrmInfo.stSyncInfo.u16FrameRate > 0)
                                  ? stFrmInfo.stSyncInfo.u16FrameRate
                                  : 60;
  pstSyncInfo->u16PixClock = (stFrmInfo.stSyncInfo.u16PixClock > 0)
                                 ? stFrmInfo.stSyncInfo.u16PixClock
                                 : 65000;
  pstSyncInfo->u16Hact =
      (stFrmInfo.stSyncInfo.u16Hact > 0) ? stFrmInfo.stSyncInfo.u16Hact : 1200;
  pstSyncInfo->u16Hbb =
      (stFrmInfo.stSyncInfo.u16Hbb > 0) ? stFrmInfo.stSyncInfo.u16Hbb : 24;
  pstSyncInfo->u16Hfb =
      (stFrmInfo.stSyncInfo.u16Hfb > 0) ? stFrmInfo.stSyncInfo.u16Hfb : 240;
  pstSyncInfo->u16Hpw =
      (stFrmInfo.stSyncInfo.u16Hpw > 0) ? stFrmInfo.stSyncInfo.u16Hpw : 136;
  pstSyncInfo->u16Hmid =
      (stFrmInfo.stSyncInfo.u16Hmid > 0) ? stFrmInfo.stSyncInfo.u16Hmid : 0;
  pstSyncInfo->u16Vact =
      (stFrmInfo.stSyncInfo.u16Vact > 0) ? stFrmInfo.stSyncInfo.u16Vact : 1200;
  pstSyncInfo->u16Vbb =
      (stFrmInfo.stSyncInfo.u16Vbb > 0) ? stFrmInfo.stSyncInfo.u16Vbb : 200;
  pstSyncInfo->u16Vfb =
      (stFrmInfo.stSyncInfo.u16Vfb > 0) ? stFrmInfo.stSyncInfo.u16Vfb : 194;
  pstSyncInfo->u16Vpw =
      (stFrmInfo.stSyncInfo.u16Vpw > 0) ? stFrmInfo.stSyncInfo.u16Vpw : 6;

  return 0;
}

static RKADK_S32 RKADK_VO_SetDispRect(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,
                                      VIDEO_FRAMEINFO_S stFrmInfo) {
  RKADK_CHECK_POINTER(pstLayerAttr, RKADK_FAILURE);

  if (0 < stFrmInfo.u32FrmInfoS32x)
    pstLayerAttr->stDispRect.s32X = stFrmInfo.u32FrmInfoS32x;
  else
    RKADK_LOGD("positive x less than zero, use default value");

  if (0 < stFrmInfo.u32FrmInfoS32y)
    pstLayerAttr->stDispRect.s32Y = stFrmInfo.u32FrmInfoS32y;
  else
    RKADK_LOGD("positive y less than zero, use default value");

  if (0 < stFrmInfo.u32DispWidth &&
      stFrmInfo.u32DispWidth < stFrmInfo.u32ImgWidth)
    pstLayerAttr->stDispRect.u32Width = stFrmInfo.u32DispWidth;
  else
    RKADK_LOGD("DispWidth use default value");

  if (0 < stFrmInfo.u32DispHeight &&
      stFrmInfo.u32DispHeight < stFrmInfo.u32ImgHeight)
    pstLayerAttr->stDispRect.u32Height = stFrmInfo.u32DispHeight;
  else
    RKADK_LOGD("DispHeight use default value");

  if (0 < stFrmInfo.u32ImgWidth)
    pstLayerAttr->stImageSize.u32Width = stFrmInfo.u32ImgWidth;
  else
    RKADK_LOGD("ImgWidth, use default value");

  if (0 < stFrmInfo.u32ImgHeight)
    pstLayerAttr->stImageSize.u32Height = stFrmInfo.u32ImgHeight;
  else
    RKADK_LOGD("ImgHeight use default value");

  return 0;
}

static RKADK_S32
RKADK_VO_SetDispRect_Default(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,
                             VO_DEV voDev) {
  int ret;
  VO_PUB_ATTR_S pstAttr;
  memset(&pstAttr, 0, sizeof(VO_SINK_CAPABILITY_S));

  RKADK_CHECK_POINTER(pstLayerAttr, RK_FAILURE);
  ret = RK_MPI_VO_GetPubAttr(voDev, &pstAttr);
  if (ret) {
    RKADK_LOGD("GetSinkCapability failed");
    return ret;
  }

  pstLayerAttr->stDispRect.u32Width = pstAttr.stSyncInfo.u16Hact;
  pstLayerAttr->stDispRect.u32Height = pstAttr.stSyncInfo.u16Vact;
  pstLayerAttr->stImageSize.u32Width = pstAttr.stSyncInfo.u16Hact;
  pstLayerAttr->stImageSize.u32Height = pstAttr.stSyncInfo.u16Vact;

  return ret;
}

static RKADK_S32 RKADK_VO_StartChnn(VO_LAYER voLayer,
                                    VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,
                                    VIDEO_FRAMEINFO_S stFrmInfo) {
  VO_CHN_ATTR_S stChnAttr;
  VO_CHN_PARAM_S stChnParam;
  VO_BORDER_S stBorder;
  int ret;

  stChnAttr.stRect.s32X = pstLayerAttr->stDispRect.s32X;
  stChnAttr.stRect.s32Y = pstLayerAttr->stDispRect.s32Y;
  stChnAttr.stRect.u32Width = pstLayerAttr->stDispRect.u32Width;
  stChnAttr.stRect.u32Height = pstLayerAttr->stDispRect.u32Height;
  // set priority
  stChnAttr.u32Priority = 1;
  ret = RK_MPI_VO_SetChnAttr(voLayer, stFrmInfo.u32ChnnNum, &stChnAttr);
  if (ret) {
    RKADK_LOGD("RK_MPI_VO_SetChnAttr failed(%d)", ret);
    return RK_FAILURE;
  }

  // set video aspect ratio
  if (stFrmInfo.u32EnMode == CHNN_ASPECT_RATIO_MANUAL) {
    stChnParam.stAspectRatio.enMode = ASPECT_RATIO_MANUAL;
    stChnParam.stAspectRatio.stVideoRect.s32X = pstLayerAttr->stDispRect.s32X;
    stChnParam.stAspectRatio.stVideoRect.s32Y = pstLayerAttr->stDispRect.s32Y;
    stChnParam.stAspectRatio.stVideoRect.u32Width =
        pstLayerAttr->stDispRect.u32Width / 2;
    stChnParam.stAspectRatio.stVideoRect.u32Height =
        pstLayerAttr->stDispRect.u32Height / 2;
    RK_MPI_VO_SetChnParam(voLayer, stFrmInfo.u32ChnnNum, &stChnParam);
  } else if (stFrmInfo.u32EnMode == CHNN_ASPECT_RATIO_AUTO) {
    stChnParam.stAspectRatio.enMode = ASPECT_RATIO_AUTO;
    stChnParam.stAspectRatio.stVideoRect.s32X = pstLayerAttr->stDispRect.s32X;
    stChnParam.stAspectRatio.stVideoRect.s32Y = pstLayerAttr->stDispRect.s32Y;
    stChnParam.stAspectRatio.stVideoRect.u32Width =
        pstLayerAttr->stDispRect.u32Width;
    stChnParam.stAspectRatio.stVideoRect.u32Height =
        pstLayerAttr->stDispRect.u32Height;
    RK_MPI_VO_SetChnParam(voLayer, stFrmInfo.u32ChnnNum, &stChnParam);
  }

  stBorder.stBorder.u32Color = stFrmInfo.u32BorderColor;
  stBorder.stBorder.u32TopWidth = stFrmInfo.u32BorderTopWidth;
  stBorder.stBorder.u32BottomWidth = stFrmInfo.u32BorderTopWidth;
  stBorder.stBorder.u32LeftWidth = stFrmInfo.u32BorderLeftWidth;
  stBorder.stBorder.u32RightWidth = stFrmInfo.u32BorderRightWidth;
  stBorder.bBorderEn = RK_TRUE;
  ret = RK_MPI_VO_SetChnBorder(voLayer, stFrmInfo.u32ChnnNum, &stBorder);
  if (ret) {
    RKADK_LOGD("RK_MPI_VO_SetChnBorder failed(%d)", ret);
    return RK_FAILURE;
  }

  ret = RK_MPI_VO_EnableChn(voLayer, stFrmInfo.u32ChnnNum);
  if (ret) {
    RKADK_LOGD("RK_MPI_VO_EnableChn failed(%d)", ret);
    return RK_FAILURE;
  }

  return ret;
}

RKADKSurfaceInterface::RKADKSurfaceInterface(VIDEO_FRAMEINFO_S *pstFrmInfo)
    : pCbMblk(nullptr), s32Flag(0) {
  memset(&stFrmInfo, 0, sizeof(VIDEO_FRAMEINFO_S));
  if (pstFrmInfo) {
    memcpy(&stFrmInfo, pstFrmInfo, sizeof(VIDEO_FRAMEINFO_S));
  } else {
    RKADK_LOGW("don't set video frame info");
    stFrmInfo.enIntfSync = VO_OUTPUT_DEFAULT;
  }
}

INT32 RKADKSurfaceInterface::queueBuffer(void *buf, INT32 fence) {
  VIDEO_FRAME_INFO_S stVFrame;
  VO_LAYER voLayer;
  int ret;

  RKADK_CHECK_POINTER(buf, RKADK_FAILURE);
  pCbMblk = buf;

  switch (stFrmInfo.u32VoDev) {
  case VO_DEV_HD0:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  case VO_DEV_HD1:
    voLayer = VOP_LAYER_CLUSTER_1;
    break;
  default:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  }

  if (!s32Flag) {
    RTFrame frame = {0};
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    RKADK_VOID *pMblk = RKADK_NULL;

    // start
    rt_memset(&stVoPubAttr, 0, sizeof(VO_PUB_ATTR_S));
    pMblk = buf;

    /* Bind Layer */
    VO_LAYER_MODE_E mode;
    switch (stFrmInfo.u32VoLayerMode) {
    case 0:
      mode = VO_LAYER_MODE_CURSOR;
      break;
    case 1:
      mode = VO_LAYER_MODE_GRAPHIC;
      break;
    case 2:
      mode = VO_LAYER_MODE_VIDEO;
      break;
    default:
      mode = VO_LAYER_MODE_VIDEO;
    }
    ret = RK_MPI_VO_BindLayer(voLayer, stFrmInfo.u32VoDev, mode);
    if (ret) {
      RKADK_LOGD("RK_MPI_VO_BindLayer failed(%d)", ret);
      return ret;
    }

    /* Enable VO Device */
    switch (stFrmInfo.u32EnIntfType) {
    case DISPLAY_TYPE_HDMI:
      stVoPubAttr.enIntfType = VO_INTF_HDMI;
      break;
    case DISPLAY_TYPE_EDP:
      stVoPubAttr.enIntfType = VO_INTF_EDP;
      break;
    case DISPLAY_TYPE_VGA:
      stVoPubAttr.enIntfType = VO_INTF_VGA;
      break;
    case DISPLAY_TYPE_MIPI:
      stVoPubAttr.enIntfType = VO_INTF_MIPI;
      break;
    default:
      stVoPubAttr.enIntfType = VO_INTF_HDMI;
      RKADK_LOGD("option not set ,use HDMI default");
    }

    stVoPubAttr.enIntfSync = stFrmInfo.enIntfSync;
    if (VO_OUTPUT_USER == stVoPubAttr.enIntfSync)
      RKADK_VO_SetRtSyncInfo(&stVoPubAttr.stSyncInfo, stFrmInfo);

    ret = RKADK_VO_StartDev(stFrmInfo.u32VoDev, &stVoPubAttr);
    if (ret) {
      RKADK_LOGD("RKADK_VO_StartDev failed(%d)", ret);
      return ret;
    }

    RKADK_LOGD("StartDev");

    /* Enable Layer */
    stLayerAttr.enPixFormat = RKADK_FmtToRtfmt(stFrmInfo.u32VoFormat);
    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.stDispRect.u32Width = frame.mFrameW / 2;
    stLayerAttr.stDispRect.u32Height = frame.mFrameH / 2;
    stLayerAttr.stImageSize.u32Width = frame.mFrameW;
    stLayerAttr.stImageSize.u32Height = frame.mFrameH;
    // RKADK_VO_SetDispRect(&stLayerAttr, stFrmInfo);

    if (VO_OUTPUT_DEFAULT == stVoPubAttr.enIntfSync) {
      ret = RKADK_VO_SetDispRect_Default(&stLayerAttr, stFrmInfo.u32VoDev);
      if (ret) {
        RKADK_LOGD("SetDispRect_Default failed(%d)", ret);
        return ret;
      }
    }

    RKADK_VO_SetDispRect(&stLayerAttr, stFrmInfo);

    stLayerAttr.u32DispFrmRt = stFrmInfo.u32DispFrmRt;
    ret = RKADK_VO_StartLayer(voLayer, &stLayerAttr);
    if (ret) {
      RKADK_LOGD("RKADK_VO_StartLayer failed(%d)", ret);
      return ret;
    }

    /* Enable channel */
    ret = RKADK_VO_StartChnn(voLayer, &stLayerAttr, stFrmInfo);
    if (ret) {
      RKADK_LOGD("RKADK_VO_StartChnn failed(%d)", ret);
      return ret;
    }

    /* Set Layer */
    stVFrame.stVFrame.pMbBlk = pMblk;
    ret = RK_MPI_VO_SendFrame(voLayer, stFrmInfo.u32ChnnNum, &stVFrame, 0);
    if (ret) {
      RKADK_LOGD("RK_MPI_VO_SendFrame failed(%d)", ret);
      return ret;
    }

    s32Flag = 1;
  } else {
    VO_FRAME_INFO_S stFrameInfo;
    void *pMblk = RKADK_NULL;
    memset(&stFrameInfo, 0, sizeof(VO_FRAME_INFO_S));

    pMblk = buf;
    stVFrame.stVFrame.pMbBlk = pMblk;
    ret = RK_MPI_VO_SendFrame(voLayer, stFrmInfo.u32ChnnNum, &stVFrame, 0);
    if (ret) {
      RKADK_LOGD("RK_MPI_VO_SendFrame failed(%d)", ret);
      return ret;
    }
  }

  return RKADK_SUCCESS;
}

void RKADKSurfaceInterface::replay() {
  ((RTMediaBuffer *)pCbMblk)->addRefs();

  VO_LAYER voLayer;
  switch (stFrmInfo.u32VoDev) {
  case VO_DEV_HD0:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  case VO_DEV_HD1:
    voLayer = VOP_LAYER_CLUSTER_1;
    break;
  default:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  }
  usleep(30 * 1000);
  RK_MPI_VO_ClearChnBuffer(voLayer, stFrmInfo.u32ChnnNum, RK_TRUE);
  ((RTMediaBuffer *)pCbMblk)->signalBufferRelease(RKADK_TRUE);
  s32Flag = 0;
}