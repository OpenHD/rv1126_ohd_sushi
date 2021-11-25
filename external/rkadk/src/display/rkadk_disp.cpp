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

#include "rkadk_disp.h"
#include "rkadk_log.h"
#include "rkadk_param.h"
#include "rkmedia_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void RKADK_DISP_SetChn(RKADK_PARAM_DISP_CFG_S *pstDispCfg,
                              MPP_CHN_S *pstViChn, MPP_CHN_S *pstVoChn,
                              MPP_CHN_S *pstRgaChn) {
  pstViChn->enModId = RK_ID_VI;
  pstViChn->s32DevId = 0;
  pstViChn->s32ChnId = pstDispCfg->vi_attr.u32ViChn;

  pstRgaChn->enModId = RK_ID_RGA;
  pstRgaChn->s32DevId = 0;
  pstRgaChn->s32ChnId = pstDispCfg->rga_chn;

  pstVoChn->enModId = RK_ID_VO;
  pstVoChn->s32DevId = 0;
  pstVoChn->s32ChnId = pstDispCfg->vo_chn;
}

static bool RKADK_DISP_IsUseRga(RKADK_PARAM_DISP_CFG_S *pstDispCfg) {
  RKADK_U32 u32InWidth, u32InHeight;
  RKADK_U32 u32OutWidth, u32OutHeight;
  IMAGE_TYPE_E enInImgType, enOutImgType;

  enInImgType = pstDispCfg->vi_attr.stChnAttr.enPixFmt;
  u32InWidth = pstDispCfg->vi_attr.stChnAttr.u32Width;
  u32InHeight = pstDispCfg->vi_attr.stChnAttr.u32Height;
  u32OutWidth = pstDispCfg->width;
  u32OutHeight = pstDispCfg->height;
  enOutImgType = RKADK_PARAM_GetPixFmt(pstDispCfg->img_type);

  if (pstDispCfg->rotaion != 0 || enOutImgType != enInImgType ||
      (u32InWidth != u32OutWidth || u32InHeight != u32OutHeight)) {
    RKADK_LOGD("rotaion: %d", pstDispCfg->rotaion);
    RKADK_LOGD("enInImgType: %d, enOutImgType: %d", enInImgType, enOutImgType);
    RKADK_LOGD("u32InWidth: %d, u32InHeight: %d", u32InWidth, u32InHeight);
    RKADK_LOGD("u32OutWidth: %d, u32OutHeight: %d", u32OutWidth, u32OutHeight);
    return true;
  }

  return false;
}

static RKADK_S32 RKADK_DISP_Enable(RKADK_U32 u32CamId,
                                   RKADK_PARAM_DISP_CFG_S *pstDispCfg) {
  int ret = 0;
  RGA_ATTR_S stRgaAttr;
  VO_CHN_ATTR_S stVoAttr;

  // Create VI
  ret = RKADK_MPI_VI_Init(u32CamId, pstDispCfg->vi_attr.u32ViChn,
                          &(pstDispCfg->vi_attr.stChnAttr));
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_Init faled %d", ret);
    return ret;
  }

  // Create RGA
  if (RKADK_DISP_IsUseRga(pstDispCfg)) {
    memset(&stRgaAttr, 0, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = (RK_BOOL)pstDispCfg->enable_buf_pool;
    stRgaAttr.u16BufPoolCnt = pstDispCfg->buf_pool_cnt;
    stRgaAttr.u16Rotaion = pstDispCfg->rotaion;
    stRgaAttr.stImgIn.u32X = 0;
    stRgaAttr.stImgIn.u32Y = 0;
    stRgaAttr.stImgIn.imgType = pstDispCfg->vi_attr.stChnAttr.enPixFmt;
    stRgaAttr.stImgIn.u32Width = pstDispCfg->vi_attr.stChnAttr.u32Width;
    stRgaAttr.stImgIn.u32Height = pstDispCfg->vi_attr.stChnAttr.u32Height;
    stRgaAttr.stImgIn.u32HorStride = pstDispCfg->vi_attr.stChnAttr.u32Width;
    stRgaAttr.stImgIn.u32VirStride = pstDispCfg->vi_attr.stChnAttr.u32Height;
    stRgaAttr.stImgOut.u32X = 0;
    stRgaAttr.stImgOut.u32Y = 0;
    stRgaAttr.stImgOut.imgType = RKADK_PARAM_GetPixFmt(pstDispCfg->img_type);
    stRgaAttr.stImgOut.u32Width = pstDispCfg->width;
    stRgaAttr.stImgOut.u32Height = pstDispCfg->height;
    stRgaAttr.stImgOut.u32HorStride = pstDispCfg->width;
    stRgaAttr.stImgOut.u32VirStride = pstDispCfg->height;
    ret = RK_MPI_RGA_CreateChn(pstDispCfg->rga_chn, &stRgaAttr);
    if (ret) {
      RKADK_LOGE("Create rga[%d] falied(%d)", pstDispCfg->rga_chn, ret);
      RKADK_MPI_VI_DeInit(u32CamId, pstDispCfg->vi_attr.u32ViChn);
      return ret;
    }
  }

  // Create VO
  memset(&stVoAttr, 0, sizeof(VO_CHN_ATTR_S));
  stVoAttr.pcDevNode = pstDispCfg->device_node;
  stVoAttr.emPlaneType = pstDispCfg->plane_type;
  stVoAttr.enImgType = RKADK_PARAM_GetPixFmt(pstDispCfg->img_type);
  stVoAttr.u16Zpos = pstDispCfg->z_pos;
  stVoAttr.stImgRect.s32X = 0;
  stVoAttr.stImgRect.s32Y = 0;
  stVoAttr.stImgRect.u32Width = pstDispCfg->width;
  stVoAttr.stImgRect.u32Height = pstDispCfg->height;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = pstDispCfg->width;
  stVoAttr.stDispRect.u32Height = pstDispCfg->height;
  ret = RK_MPI_VO_CreateChn(pstDispCfg->vo_chn, &stVoAttr);
  if (ret) {
    RKADK_LOGE("Create VO[%d] failed(%d)", pstDispCfg->vo_chn, ret);
    RK_MPI_RGA_DestroyChn(pstDispCfg->rga_chn);
    RKADK_MPI_VI_DeInit(u32CamId, pstDispCfg->vi_attr.u32ViChn);
    return ret;
  }

  return 0;
}

RKADK_S32 RKADK_DISP_Init(RKADK_U32 u32CamId) {
  int ret = 0;
  MPP_CHN_S stViChn, stVoChn, stRgaChn;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);
  RKADK_LOGI("Disp u32CamId[%d] Init Start...", u32CamId);

  RK_MPI_SYS_Init();
  RKADK_PARAM_Init();

  RKADK_PARAM_DISP_CFG_S *pstDispCfg = RKADK_PARAM_GetDispCfg(u32CamId);
  if (!pstDispCfg) {
    RKADK_LOGE("Live RKADK_PARAM_GetDispCfg failed");
    return -1;
  }

  RKADK_DISP_SetChn(pstDispCfg, &stViChn, &stVoChn, &stRgaChn);
  ret = RKADK_DISP_Enable(u32CamId, pstDispCfg);
  if (ret) {
    RKADK_LOGE("RKADK_DISP_Enable failed(%d)", ret);
    return ret;
  }

  if (RKADK_DISP_IsUseRga(pstDispCfg)) {
    // Bind RGA to VO
    ret = RK_MPI_SYS_Bind(&stRgaChn, &stVoChn);
    if (ret) {
      RKADK_LOGE("Bind RGA[%d] to VO[%d] failed(%d)", stRgaChn.s32ChnId,
                 stVoChn.s32ChnId, ret);
      goto failed;
    }

    // Bind VI to RGA
    ret = RK_MPI_SYS_Bind(&stViChn, &stRgaChn);
    if (ret) {
      RKADK_LOGE("Bind VI[%d] to RGA[%d] failed(%d)", stViChn.s32ChnId,
                 stRgaChn.s32ChnId, ret);
      goto failed;
    }
  } else {
    // Bind VI to VO
    ret = RK_MPI_SYS_Bind(&stViChn, &stVoChn);
    if (ret) {
      RKADK_LOGE("Bind VI[%d] to VO[%d] failed(%d)", stViChn.s32ChnId,
                 stVoChn.s32ChnId, ret);
      goto failed;
    }
  }

  RKADK_LOGI("Disp u32CamId[%d] Init End...", u32CamId);
  return 0;

failed:
  RKADK_LOGI("Disp u32CamId[%d] Init failed...", u32CamId);
  RK_MPI_VO_DestroyChn(stVoChn.s32ChnId);

  if (RKADK_DISP_IsUseRga(pstDispCfg))
    RK_MPI_RGA_DestroyChn(stRgaChn.s32ChnId);

  RKADK_MPI_VI_DeInit(u32CamId, stViChn.s32ChnId);
  return ret;
}

RKADK_S32 RKADK_DISP_DeInit(RKADK_U32 u32CamId) {
  int ret = 0;
  MPP_CHN_S stViChn, stVoChn, stRgaChn;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);
  RKADK_LOGI("Disp u32CamId[%d] DeInit Start...", u32CamId);

  RKADK_PARAM_DISP_CFG_S *pstDispCfg = RKADK_PARAM_GetDispCfg(u32CamId);
  if (!pstDispCfg) {
    RKADK_LOGE("Live RKADK_PARAM_GetDispCfg failed");
    return -1;
  }

  RKADK_DISP_SetChn(pstDispCfg, &stViChn, &stVoChn, &stRgaChn);

  if (RKADK_DISP_IsUseRga(pstDispCfg)) {
    ret = RK_MPI_SYS_UnBind(&stRgaChn, &stVoChn);
    if (ret) {
      RKADK_LOGE("UnBind RGA[%d] to VO[%d] failed(%d)", stRgaChn.s32ChnId,
                 stVoChn.s32ChnId, ret);
      return ret;
    }

    ret = RK_MPI_SYS_UnBind(&stViChn, &stRgaChn);
    if (ret) {
      RKADK_LOGE("UnBind VI[%d] to RGA[%d] failed(%d)", stViChn.s32ChnId,
                 stRgaChn.s32ChnId, ret);
      return ret;
    }
  } else {
    ret = RK_MPI_SYS_UnBind(&stViChn, &stVoChn);
    if (ret) {
      RKADK_LOGE("UnBind VI[%d] to VO[%d] failed(%d)", stViChn.s32ChnId,
                 stVoChn.s32ChnId, ret);
      return ret;
    }
  }

  ret = RK_MPI_VO_DestroyChn(stVoChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("Destory VO[%d] failed(%d)", stVoChn.s32ChnId, ret);
    return ret;
  }

  if (RKADK_DISP_IsUseRga(pstDispCfg)) {
    ret = RK_MPI_RGA_DestroyChn(stRgaChn.s32ChnId);
    if (ret) {
      RKADK_LOGE("Destory RGA[%d] failed(%d)", stRgaChn.s32ChnId, ret);
      return ret;
    }
  }

  ret = RKADK_MPI_VI_DeInit(u32CamId, stViChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_DeInit failed(%d)", ret);
    return ret;
  }

  RKADK_LOGI("Disp u32CamId[%d] DeInit End...", u32CamId);
  return 0;
}
