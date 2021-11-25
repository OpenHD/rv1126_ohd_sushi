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

#include "rkadk_param.h"
#include "rkadk_param_map.h"

#define RKISPP_SCALE0_NV12_WIDTH_MAX 2080
#define RKISPP_SCALE1_WIDTH_MAX 1280

/** parameter context */
static RKADK_PARAM_CONTEXT_S g_stPARAMCtx = {
    .bInit = false, .mutexLock = PTHREAD_MUTEX_INITIALIZER, .stCfg = {0}};

static RKADK_S32 RKADK_PARAM_SaveViCfg(const char *path, RKADK_U32 viIndex,
                                       RKADK_U32 u32CamId) {
  int ret = 0;
  RKADK_PARAM_MEDIA_CFG_S *pstMediaCfg = NULL;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);
  pstMediaCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId];

  switch (viIndex) {
  case 0:
    pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_VI0_MAP);
    break;

  case 1:
    pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_VI1_MAP);
    break;

  case 2:
    pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_VI2_MAP);
    break;

  case 3:
    pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_VI3_MAP);
    break;

  default:
    RKADK_LOGE("Invaild vi index = %d", viIndex);
    return -1;
  }
  RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

  ret = RKADK_Struct2Ini(path, &pstMediaCfg->stViCfg[viIndex],
                         pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret)
    RKADK_LOGE("save u32CamId[%d] VI[%d] param failed", u32CamId, viIndex);

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveVersion(const char *path) {
  int ret = 0;
  RKADK_PARAM_VERSION_S *pstVersion = &g_stPARAMCtx.stCfg.stVersion;

  ret = RKADK_Struct2Ini(path, pstVersion, g_stVersionMapTable,
                         sizeof(g_stVersionMapTable) /
                             sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret)
    RKADK_LOGE("save version failed");

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveCommCfg(const char *path) {
  int ret = 0;
  RKADK_PARAM_COMM_CFG_S *pstCommCfg = &g_stPARAMCtx.stCfg.stCommCfg;

  ret = RKADK_Struct2Ini(path, pstCommCfg, g_stCommCfgMapTable,
                         sizeof(g_stCommCfgMapTable) /
                             sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret)
    RKADK_LOGE("save common param failed");

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveAudioCfg(const char *path) {
  int ret = 0;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg = &g_stPARAMCtx.stCfg.stAudioCfg;

  ret = RKADK_Struct2Ini(path, pstAudioCfg, g_stAudioCfgMapTable,
                         sizeof(g_stAudioCfgMapTable) /
                             sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret)
    RKADK_LOGE("save audio param failed");

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveThumbCfg(const char *path) {
  int ret = 0;
  RKADK_PARAM_THUMB_CFG_S *pstThumbCfg = &g_stPARAMCtx.stCfg.stThumbCfg;

  ret = RKADK_Struct2Ini(path, pstThumbCfg, g_stThumbCfgMapTable,
                         sizeof(g_stThumbCfgMapTable) /
                             sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret)
    RKADK_LOGE("save thumb param failed");

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveSensorCfg(const char *path,
                                           RKADK_U32 u32CamId) {
  int ret;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  pstSensorCfg = &g_stPARAMCtx.stCfg.stSensorCfg[u32CamId];

  pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_SENSOR_MAP);
  RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

  ret = RKADK_Struct2Ini(path, pstSensorCfg, pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret)
    RKADK_LOGE("save sensor[%d] param failed", u32CamId);

  return ret;
}

static RKADK_S32 RKADK_PARAM_SavePhotoCfg(const char *path,
                                          RKADK_U32 u32CamId) {
  int ret;
  RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  pstPhotoCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stPhotoCfg;

  pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_PHOTO_MAP);
  RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

  ret = RKADK_Struct2Ini(path, pstPhotoCfg, pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret)
    RKADK_LOGE("save sensor[%d] photo param failed", u32CamId);

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveVencParamCfg(const char *path,
                                              RKADK_U32 u32CamId,
                                              RKADK_STREAM_TYPE_E enStrmType) {
  int ret = 0;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;
  RKADK_PARAM_VENC_PARAM_S *pstVencParam = NULL;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  pstRecCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;
  switch (enStrmType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    pstVencParam = &(pstRecCfg->attribute[0].venc_param);
    pstMapTableCfg =
        RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_REC_MAIN_PARAM_MAP);
    break;
  case RKADK_STREAM_TYPE_VIDEO_SUB:
    pstVencParam = &(pstRecCfg->attribute[1].venc_param);
    pstMapTableCfg =
        RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_REC_SUB_PARAM_MAP);
    break;
  case RKADK_STREAM_TYPE_PREVIEW:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stStreamCfg;
    pstVencParam = &(pstStreamCfg->attribute.venc_param);
    pstMapTableCfg =
        RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_PREVIEW_PARAM_MAP);
    break;
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stLiveCfg;
    pstVencParam = &(pstStreamCfg->attribute.venc_param);
    pstMapTableCfg =
        RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_LIVE_PARAM_MAP);
    break;
  default:
    return -1;
  }
  RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

  ret = RKADK_Struct2Ini(path, pstVencParam, pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret)
    RKADK_LOGE("save sensor[%d] stream param failed", u32CamId);

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveStreamCfg(const char *path, RKADK_U32 u32CamId,
                                           RKADK_STREAM_TYPE_E enStrmType) {
  int ret = 0;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;
  RKADK_PARAM_MAP_TYPE_E enMapType;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  switch (enStrmType) {
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stLiveCfg;
    enMapType = RKADK_PARAM_LIVE_MAP;
    break;
  case RKADK_STREAM_TYPE_PREVIEW:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stStreamCfg;
    enMapType = RKADK_PARAM_PREVIEW_MAP;
    break;
  default:
    RKADK_LOGW("unsupport enStrmType = %d", enStrmType);
    return -1;
  }

  pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, enMapType);
  RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

  ret = RKADK_Struct2Ini(path, &pstStreamCfg->attribute,
                         pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret)
    RKADK_LOGE("save sensor[%d] stream(enStrmType = %d) param failed",
               enStrmType, u32CamId);

  return ret;
}

static RKADK_S32 RKADK_PARAM_SaveRecCfg(const char *path, RKADK_U32 u32CamId) {
  int ret = 0;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  pstRecCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;

  pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_REC_MAP);
  RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

  ret = RKADK_Struct2Ini(path, pstRecCfg, pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret) {
    RKADK_LOGE("save sensor[%d] record param failed", u32CamId);
    return ret;
  }

  return 0;
}

static RKADK_S32 RKADK_PARAM_SaveRecTime(const char *path, RKADK_U32 u32CamId,
                                         RKADK_STREAM_TYPE_E enStrmType) {
  int ret;
  RKADK_U32 index;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  pstRecCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;
  switch (enStrmType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    index = 0;
    pstMapTableCfg =
        RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_REC_MAIN_TIME_MAP);
    break;
  case RKADK_STREAM_TYPE_VIDEO_SUB:
    index = 1;
    pstMapTableCfg =
        RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_REC_SUB_TIME_MAP);
    break;
  default:
    return -1;
  }

  if (!pstMapTableCfg)
    return -1;

  ret = RKADK_Struct2Ini(path, &pstRecCfg->record_time_cfg[index],
                         pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret) {
    RKADK_LOGE("save sensor[%d] record time[%d] param failed", u32CamId, index);
    return ret;
  }

  return 0;
}

static RKADK_S32 RKADK_PARAM_SaveRecAttr(const char *path, RKADK_U32 u32CamId,
                                         RKADK_STREAM_TYPE_E enStrmType) {
  int ret;
  RKADK_U32 index;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  pstRecCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;
  switch (enStrmType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    index = 0;
    pstMapTableCfg =
        RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_REC_MAIN_MAP);
    break;
  case RKADK_STREAM_TYPE_VIDEO_SUB:
    index = 1;
    pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_REC_SUB_MAP);
    break;
  default:
    return -1;
  }

  if (!pstMapTableCfg)
    return -1;

  ret = RKADK_Struct2Ini(path, &pstRecCfg->attribute[index],
                         pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret) {
    RKADK_LOGE("save sensor[%d] record attribute[%d] param failed", u32CamId,
               index);
    return ret;
  }

  return 0;
}

static RKADK_S32 RKADK_PARAM_SaveDispCfg(const char *path, RKADK_U32 u32CamId) {
  int ret;
  RKADK_PARAM_DISP_CFG_S *pstDispCfg;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  pstDispCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stDispCfg;
  pstMapTableCfg = RKADK_PARAM_GetMapTable(u32CamId, RKADK_PARAM_DISP_MAP);
  RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

  ret = RKADK_Struct2Ini(path, pstDispCfg, pstMapTableCfg->pstMapTable,
                         pstMapTableCfg->u32TableLen);
  if (ret)
    RKADK_LOGE("save sensor[%d] display param failed", u32CamId);

  return ret;
}

static bool RKADK_PARAM_CheckCfg(RKADK_U32 *u32Value, RKADK_U32 u32DefValue,
                                 const char *tag) {
  if (*u32Value == 0) {
    RKADK_LOGW("%s: invalid value(%d), use default(%d)", tag, *u32Value,
               u32DefValue);
    *u32Value = u32DefValue;
    return true;
  } else {
    return false;
  }
}

static bool RKADK_PARAM_CheckCfgU32(RKADK_U32 *u32Value, RKADK_U32 u32Min,
                                    RKADK_U32 u32Max, RKADK_U32 u32DefValue,
                                    const char *tag) {
  if ((*u32Value > u32Max) || (*u32Value < u32Min)) {
    RKADK_LOGW("%s: invalid value(%d), use default(%d)", tag, *u32Value,
               u32DefValue);
    *u32Value = u32DefValue;
    return true;
  } else {
    return false;
  }
}

static bool RKADK_PARAM_CheckCfgStr(char *value, const char *defValue,
                                    RKADK_U32 S32ValueLen, const char *tag) {
  RKADK_U32 len;

  len = S32ValueLen > strlen(defValue) ? strlen(defValue) : S32ValueLen;
  if (!strlen(value)) {
    RKADK_LOGW("%s: invalid value(%s), use default(%s)", tag, value, defValue);
    strncpy(value, defValue, len);
    return true;
  } else {
    return false;
  }
}

static void RKADK_PARAM_CheckCommCfg(const char *path) {
  bool change = false;
  RKADK_PARAM_COMM_CFG_S *pstCommCfg = &g_stPARAMCtx.stCfg.stCommCfg;

  change = RKADK_PARAM_CheckCfgU32(&pstCommCfg->sensor_count, 1,
                                   RKADK_MAX_SENSOR_CNT, 1, "sensor_count");
  change |= RKADK_PARAM_CheckCfgU32(&pstCommCfg->speaker_volume, 0, 100, 70,
                                    "speaker_volume");
  change |= RKADK_PARAM_CheckCfgU32(&pstCommCfg->mic_volume, 0, 100, 70,
                                    "mic_volume");

  if (change)
    RKADK_PARAM_SaveCommCfg(path);
}

static void RKADK_PARAM_CheckAudioCfg(const char *path) {
  bool change = false;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg = &g_stPARAMCtx.stCfg.stAudioCfg;

  change = RKADK_PARAM_CheckCfgStr(pstAudioCfg->audio_node, AI_DEVICE_NAME,
                                   RKADK_BUFFER_LEN, "audio_node");
  change |= RKADK_PARAM_CheckCfgU32((RKADK_U32 *)&pstAudioCfg->sample_format,
                                    RK_SAMPLE_FMT_U8, RK_SAMPLE_FMT_NB,
                                    AUDIO_SAMPLE_FORMAT, "sample_format");
  change |=
      RKADK_PARAM_CheckCfg(&pstAudioCfg->channels, AUDIO_CHANNEL, "channels");
  change |= RKADK_PARAM_CheckCfg(&pstAudioCfg->samplerate, AUDIO_SAMPLE_RATE,
                                 "samplerate");
  change |= RKADK_PARAM_CheckCfg(&pstAudioCfg->samples_per_frame,
                                 AUDIO_FRAME_COUNT, "samples_per_frame");
  change |=
      RKADK_PARAM_CheckCfg(&pstAudioCfg->bitrate, AUDIO_BIT_REAT, "bitrate");
  change |= RKADK_PARAM_CheckCfgU32((RKADK_U32 *)&pstAudioCfg->ai_layout,
                                    AI_LAYOUT_NORMAL, AI_LAYOUT_REF_MIC,
                                    AI_LAYOUT_NORMAL, "ai_layout");
  change |= RKADK_PARAM_CheckCfgU32((RKADK_U32 *)&pstAudioCfg->vqe_mode,
                                    RKADK_VQE_MODE_AI_TALK, RKADK_VQE_MODE_BUTT,
                                    RKADK_VQE_MODE_AI_RECORD, "vqe_mode");

  if (change)
    RKADK_PARAM_SaveAudioCfg(path);
}

static void RKADK_PARAM_CheckThumbCfg(const char *path) {
  bool change = false;
  RKADK_PARAM_THUMB_CFG_S *pstThumbCfg = &g_stPARAMCtx.stCfg.stThumbCfg;

  change = RKADK_PARAM_CheckCfgU32(&pstThumbCfg->thumb_width, 240, 1280,
                                   THUMB_WIDTH, "thumb_width");
  change |= RKADK_PARAM_CheckCfgU32(&pstThumbCfg->thumb_height, 180, 720,
                                    THUMB_HEIGHT, "thumb_height");
  change |= RKADK_PARAM_CheckCfgU32(&pstThumbCfg->venc_chn, 0, VENC_MAX_CHN_NUM,
                                    THUMB_VENC_CHN, "thumb venc_chn");

  if (change)
    RKADK_PARAM_SaveThumbCfg(path);
}

static void RKADK_PARAM_CheckSensorCfg(const char *path, RKADK_U32 u32CamId) {
  bool change = false;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg =
      &g_stPARAMCtx.stCfg.stSensorCfg[u32CamId];

  change = RKADK_PARAM_CheckCfg(&pstSensorCfg->framerate, VIDEO_FRAME_RATE,
                                "framerate");
  change |= RKADK_PARAM_CheckCfgU32(&pstSensorCfg->ldc, 0, 255, 0, "ldc");
  change |= RKADK_PARAM_CheckCfgU32(&pstSensorCfg->wdr, 0, 10, 0, "wdr");
  change |= RKADK_PARAM_CheckCfgU32(&pstSensorCfg->hdr, 0, 2, 0, "hdr");
  change |=
      RKADK_PARAM_CheckCfgU32(&pstSensorCfg->antifog, 0, 10, 0, "antifog");

  if (change)
    RKADK_PARAM_SaveSensorCfg(path, u32CamId);
}

static void RKADK_PARAM_CheckStreamCfg(const char *path, RKADK_U32 u32CamId,
                                       RKADK_STREAM_TYPE_E enStrmType) {
  bool change = false;
  RKADK_PARAM_VENC_ATTR_S *pstAttribute = NULL;

  switch (enStrmType) {
  case RKADK_STREAM_TYPE_LIVE:
    pstAttribute = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stLiveCfg.attribute;
    break;
  case RKADK_STREAM_TYPE_PREVIEW:
    pstAttribute =
        &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stStreamCfg.attribute;
    break;
  default:
    RKADK_LOGW("unsupport enStrmType = %d", enStrmType);
    return;
  }

  if (strcmp(pstAttribute->rc_mode, "CBR") &&
      strcmp(pstAttribute->rc_mode, "VBR") &&
      strcmp(pstAttribute->rc_mode, "AVBR")) {
    memcpy(pstAttribute->rc_mode, "CBR", strlen("CBR"));
    change = true;
  }

  change |= RKADK_PARAM_CheckCfg(&pstAttribute->width, STREAM_VIDEO_WIDTH,
                                 "stream width");
  change |= RKADK_PARAM_CheckCfg(&pstAttribute->height, STREAM_VIDEO_HEIGHT,
                                 "stream height");
  change |= RKADK_PARAM_CheckCfgU32(&pstAttribute->venc_chn, 0,
                                    VENC_MAX_CHN_NUM, 1, "stream venc_chn");
  change |= RKADK_PARAM_CheckCfg(&pstAttribute->bitrate, 4 * 1024 * 1024,
                                 "stream bitrate");
  change |= RKADK_PARAM_CheckCfg(&pstAttribute->gop, VIDEO_GOP, "stream gop");
  change |= RKADK_PARAM_CheckCfg(&pstAttribute->profile, VIDEO_PROFILE,
                                 "stream profile");
  change |= RKADK_PARAM_CheckCfgU32(
      (RKADK_U32 *)&pstAttribute->codec_type, RKADK_CODEC_TYPE_H264,
      RKADK_CODEC_TYPE_MJPEG, RKADK_CODEC_TYPE_H264, "stream codec_type");

  if (change)
    RKADK_PARAM_SaveStreamCfg(path, u32CamId, enStrmType);

  // check venc param
  change = RKADK_PARAM_CheckCfgU32(&pstAttribute->venc_param.max_qp, 8, 51, 49,
                                   "stream max_qp");
  change |= RKADK_PARAM_CheckCfgU32(&pstAttribute->venc_param.min_qp, 0, 48, 25,
                                    "stream min_qp");

  if (pstAttribute->venc_param.hier_qp_en) {
    change |= RKADK_PARAM_CheckCfgStr(pstAttribute->venc_param.hier_qp_delta,
                                      "-3,0,0,0", RKADK_BUFFER_LEN,
                                      "stream hier_qp_delta");
    change |= RKADK_PARAM_CheckCfgStr(pstAttribute->venc_param.hier_frame_num,
                                      "3,0,0,0", RKADK_BUFFER_LEN,
                                      "stream hier_frame_num");
  }

  if (change)
    RKADK_PARAM_SaveVencParamCfg(path, u32CamId, enStrmType);
}

static void RKADK_PARAM_CheckPhotoCfg(const char *path, RKADK_U32 u32CamId) {
  bool change = false;
  RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stPhotoCfg;

  change = RKADK_PARAM_CheckCfg(&pstPhotoCfg->image_width, PHOTO_VIDEO_WIDTH,
                                "photo width");
  change |= RKADK_PARAM_CheckCfg(&pstPhotoCfg->image_height, PHOTO_VIDEO_HEIGHT,
                                 "photo height");
  change |= RKADK_PARAM_CheckCfg(&pstPhotoCfg->snap_num, 1, "photo snap_num");
  change |= RKADK_PARAM_CheckCfgU32(&pstPhotoCfg->venc_chn, 0, VENC_MAX_CHN_NUM,
                                    2, "photo venc_chn");

  if (change)
    RKADK_PARAM_SavePhotoCfg(path, u32CamId);
}

static void RKADK_PARAM_CheckRecCfg(const char *path, RKADK_U32 u32CamId) {
  bool change = false;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg =
      &g_stPARAMCtx.stCfg.stSensorCfg[u32CamId];
  RKADK_PARAM_REC_CFG_S *pstRecCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;

  change = RKADK_PARAM_CheckCfgU32((RKADK_U32 *)&pstRecCfg->record_type,
                                   RKADK_REC_TYPE_NORMAL, RKADK_REC_TYPE_LAPSE,
                                   RKADK_REC_TYPE_NORMAL, "record_type");
  change |= RKADK_PARAM_CheckCfgU32(
      (RKADK_U32 *)&pstRecCfg->pre_record_mode, MUXER_PRE_RECORD_NONE,
      MUXER_PRE_RECORD_NORMAL, MUXER_PRE_RECORD_NONE, "pre_record_mode");
  change |= RKADK_PARAM_CheckCfgU32(&pstRecCfg->lapse_multiple, 1,
                                    pstSensorCfg->framerate,
                                    pstSensorCfg->framerate, "lapse_multiple");
  change |=
      RKADK_PARAM_CheckCfgU32(&pstRecCfg->file_num, 1, RECORD_FILE_NUM_MAX,
                              RECORD_FILE_NUM_MAX, "file_num");

  if (change)
    RKADK_PARAM_SaveRecCfg(path, u32CamId);

  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    RKADK_STREAM_TYPE_E enStrmType;
    RKADK_PARAM_VENC_ATTR_S *pstAttribute = &pstRecCfg->attribute[i];
    RKADK_PARAM_REC_TIME_CFG_S *pstRecTimeCfg = &pstRecCfg->record_time_cfg[i];

    // check record time
    change =
        RKADK_PARAM_CheckCfg(&pstRecTimeCfg->record_time, 60, "record_time");
    change |=
        RKADK_PARAM_CheckCfg(&pstRecTimeCfg->splite_time, 60, "splite_time");
    change |= RKADK_PARAM_CheckCfg(&pstRecTimeCfg->lapse_interval, 60,
                                   "lapse_interval");

    if (change)
      RKADK_PARAM_SaveRecTime(path, u32CamId, enStrmType);

    // check venc attribute
    change = false;
    if (strcmp(pstAttribute->rc_mode, "CBR") &&
        strcmp(pstAttribute->rc_mode, "VBR") &&
        strcmp(pstAttribute->rc_mode, "AVBR")) {
      memcpy(pstAttribute->rc_mode, "CBR", strlen("CBR"));
      change = true;
    }

    RKADK_U32 u32DefWidth, u32DefHeight, u32DefVencChn, u32DefBitrate;
    if (i == 0) {
      enStrmType = RKADK_STREAM_TYPE_VIDEO_MAIN;
      u32DefWidth = RECORD_VIDEO_WIDTH;
      u32DefHeight = RECORD_VIDEO_HEIGHT;
      u32DefVencChn = 0;
      u32DefBitrate = 30 * 1024 * 1024;
    } else {
      enStrmType = RKADK_STREAM_TYPE_VIDEO_SUB;
      u32DefWidth = RECORD_VIDEO_WIDTH_S;
      u32DefHeight = RECORD_VIDEO_HEIGHT_S;
      u32DefVencChn = 1;
      u32DefBitrate = 4 * 1024 * 1024;
    }

    change |=
        RKADK_PARAM_CheckCfg(&pstAttribute->width, u32DefWidth, "rec width");
    change |=
        RKADK_PARAM_CheckCfg(&pstAttribute->height, u32DefHeight, "rec height");
    change |=
        RKADK_PARAM_CheckCfgU32(&pstAttribute->venc_chn, 0, VENC_MAX_CHN_NUM,
                                u32DefVencChn, "rec venc_chn");
    change |= RKADK_PARAM_CheckCfg(&pstAttribute->bitrate, u32DefBitrate,
                                   "rec bitrate");
    change |= RKADK_PARAM_CheckCfg(&pstAttribute->gop, VIDEO_GOP, "rec gop");
    change |= RKADK_PARAM_CheckCfg(&pstAttribute->profile, VIDEO_PROFILE,
                                   "rec profile");
    change |= RKADK_PARAM_CheckCfgU32(
        (RKADK_U32 *)&pstAttribute->codec_type, RKADK_CODEC_TYPE_H264,
        RKADK_CODEC_TYPE_MJPEG, RKADK_CODEC_TYPE_H264, "rec codec_type");

    if (change)
      RKADK_PARAM_SaveRecAttr(path, u32CamId, enStrmType);

    // check venc param
    change = RKADK_PARAM_CheckCfgU32(&pstAttribute->venc_param.max_qp, 8, 51,
                                     49, "rec max_qp");
    change |= RKADK_PARAM_CheckCfgU32(&pstAttribute->venc_param.min_qp, 0, 48,
                                      25, "rec min_qp");
    if (pstAttribute->venc_param.hier_qp_en) {
      change |= RKADK_PARAM_CheckCfgStr(pstAttribute->venc_param.hier_qp_delta,
                                        "-3,0,0,0", RKADK_BUFFER_LEN,
                                        "rec hier_qp_delta");
      change |= RKADK_PARAM_CheckCfgStr(pstAttribute->venc_param.hier_frame_num,
                                        "3,0,0,0", RKADK_BUFFER_LEN,
                                        "rec hier_frame_num");
    }

    if (change)
      RKADK_PARAM_SaveVencParamCfg(path, u32CamId, enStrmType);
  }
}

static void RKADK_PARAM_CheckViCfg(const char *path, RKADK_U32 u32CamId,
                                   RKADK_U32 index) {
  bool change = false;
  RKADK_U32 u32ChnId;
  const char *pixFmt = "NV12";
  const char *deviceName = "rkispp_m_bypass";
  const char *module = "RECORD_MAIN|PHOTO";
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg =
      &g_stPARAMCtx.stCfg.stSensorCfg[u32CamId];
  RKADK_PARAM_VI_CFG_S *pstViCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stViCfg[index];

  u32ChnId = index + (u32CamId * RKADK_ISPP_VI_NODE_CNT);

  switch (u32ChnId) {
  case 0:
    pixFmt = "FBC0";
    change =
        RKADK_PARAM_CheckCfg(&pstViCfg->width, RECORD_VIDEO_WIDTH, "vi width");
    change |= RKADK_PARAM_CheckCfg(&pstViCfg->height, RECORD_VIDEO_HEIGHT,
                                   "vi height");
    break;
  case 1:
    deviceName = "rkispp_scale0";
    break;
  case 2:
    deviceName = "rkispp_scale1";
    module = "NONE";
    break;
  case 3:
    deviceName = "rkispp_scale2";
    module = "RECORD_SUB|PREVIEW|LIVE|DISP";
    change =
        RKADK_PARAM_CheckCfg(&pstViCfg->width, STREAM_VIDEO_WIDTH, "vi width");
    change |= RKADK_PARAM_CheckCfg(&pstViCfg->height, STREAM_VIDEO_HEIGHT,
                                   "vi height");
    break;
  default:
    break;
  }

  if (pstViCfg->chn_id != u32ChnId) {
    pstViCfg->chn_id = u32ChnId;
    change = true;
  }

  change |= RKADK_PARAM_CheckCfgStr(pstViCfg->device_name, deviceName,
                                    RKADK_BUFFER_LEN, "vi device_name");
  change |= RKADK_PARAM_CheckCfgStr(pstViCfg->module, module, RKADK_BUFFER_LEN,
                                    "vi module");
  change |= RKADK_PARAM_CheckCfg(&pstViCfg->buf_cnt, 4, "vi buf_cnt");
  change |= RKADK_PARAM_CheckCfgStr(pstViCfg->pix_fmt, pixFmt,
                                    RKADK_PIX_FMT_LEN, "vi pix_fmt");

  if (!strcmp(pstViCfg->device_name, "rkispp_scale0")) {
    if ((pstViCfg->width != pstSensorCfg->max_width) &&
        (pstViCfg->width > RKISPP_SCALE0_NV12_WIDTH_MAX)) {
      if (strcmp(pstViCfg->pix_fmt, "NV16")) {
        RKADK_LOGW("rkispp_scale0 resolution[%dx%d] > 2K, default NV16",
                   pstViCfg->width, pstViCfg->height);
        memcpy(pstViCfg->pix_fmt, "NV16", strlen("NV16"));
        change = true;
      }
    }
  }

  if (change)
    RKADK_PARAM_SaveViCfg(path, index, u32CamId);
}

static void RKADK_PARAM_CheckDispCfg(const char *path, RKADK_U32 u32CamId) {
  bool change = false;
  RKADK_PARAM_DISP_CFG_S *pstDispCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stDispCfg;

  change =
      RKADK_PARAM_CheckCfg(&pstDispCfg->width, DISP_WIDTH, "display width");
  change |=
      RKADK_PARAM_CheckCfg(&pstDispCfg->height, DISP_HEIGHT, "display height");

  if (pstDispCfg->rotaion != 0 && pstDispCfg->rotaion != 90 &&
      pstDispCfg->rotaion != 180 && pstDispCfg->rotaion != 270) {
    RKADK_LOGW("invalid rga rotaion(%d), use default(0)", pstDispCfg->rotaion);
    pstDispCfg->rotaion = 0;
    change = true;
  }

  if (pstDispCfg->enable_buf_pool)
    change |= RKADK_PARAM_CheckCfg(&pstDispCfg->buf_pool_cnt, 3,
                                   "display buf_pool_cnt");

  change |= RKADK_PARAM_CheckCfgU32(&pstDispCfg->rga_chn, 0, RGA_MAX_CHN_NUM, 0,
                                    "display rga_chn");
  change |= RKADK_PARAM_CheckCfgStr(pstDispCfg->device_node, "/dev/dri/card0",
                                    RKADK_BUFFER_LEN, "display vo device_node");
  change |= RKADK_PARAM_CheckCfgStr(pstDispCfg->img_type, "RGB888",
                                    RKADK_PIX_FMT_LEN, "display pix_fmt");
  change |= RKADK_PARAM_CheckCfgU32((RKADK_U32 *)&pstDispCfg->plane_type,
                                    VO_PLANE_PRIMARY, VO_PLANE_CURSOR,
                                    VO_PLANE_PRIMARY, "display plane_type");
  change |= RKADK_PARAM_CheckCfgU32(&pstDispCfg->vo_chn, 0, VO_MAX_CHN_NUM, 0,
                                    "display vo_chn");

  if (change)
    RKADK_PARAM_SavePhotoCfg(path, u32CamId);
}

static bool RKADK_PARAM_CheckVersion() {
  int ret;
  char version[RKADK_BUFFER_LEN];
  RKADK_PARAM_CFG_S *pstCfg = &g_stPARAMCtx.stCfg;

  memset(version, 0, RKADK_BUFFER_LEN);
  memset(&pstCfg->stVersion, 0, sizeof(RKADK_PARAM_VERSION_S));

  ret = RKADK_Ini2Struct(
      RKADK_DEFPARAM_PATH, &pstCfg->stVersion, g_stVersionMapTable,
      sizeof(g_stVersionMapTable) / sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret) {
    RKADK_LOGE("load default ini version failed");
    return false;
  }
  strncpy(version, pstCfg->stVersion.version,
          strlen(pstCfg->stVersion.version));

  memset(&pstCfg->stVersion, 0, sizeof(RKADK_PARAM_VERSION_S));
  ret = RKADK_Ini2Struct(
      RKADK_PARAM_PATH, &pstCfg->stVersion, g_stVersionMapTable,
      sizeof(g_stVersionMapTable) / sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret) {
    RKADK_LOGE("load setting ini version failed");
    return false;
  }

  if (strcmp(version, pstCfg->stVersion.version)) {
    RKADK_LOGW("default ini version[%s] != setting ini version[%s]", version,
               pstCfg->stVersion.version);
    return false;
  }

  return true;
}

static void RKADK_PARAM_DefCommCfg(const char *path) {
  RKADK_PARAM_COMM_CFG_S *pstCommCfg = &g_stPARAMCtx.stCfg.stCommCfg;

  memset(pstCommCfg, 0, sizeof(RKADK_PARAM_COMM_CFG_S));
  pstCommCfg->sensor_count = 1;
  pstCommCfg->rec_unmute = true;
  pstCommCfg->enable_speaker = true;
  pstCommCfg->speaker_volume = 70;
  pstCommCfg->mic_unmute = true;
  pstCommCfg->mic_volume = 70;
  pstCommCfg->osd_time_format = 0;
  pstCommCfg->osd = true;
  pstCommCfg->boot_sound = true;
  pstCommCfg->osd_speed = false;
  RKADK_PARAM_SaveCommCfg(path);
}

static void RKADK_PARAM_DefAudioCfg(const char *path) {
  RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg = &g_stPARAMCtx.stCfg.stAudioCfg;

  memset(pstAudioCfg, 0, sizeof(RKADK_PARAM_AUDIO_CFG_S));
  memcpy(pstAudioCfg->audio_node, AI_DEVICE_NAME, strlen(AI_DEVICE_NAME));
  pstAudioCfg->sample_format = AUDIO_SAMPLE_FORMAT;
  pstAudioCfg->channels = AUDIO_CHANNEL;
  pstAudioCfg->samplerate = AUDIO_SAMPLE_RATE;
  pstAudioCfg->samples_per_frame = AUDIO_FRAME_COUNT;
  pstAudioCfg->bitrate = AUDIO_BIT_REAT;
  pstAudioCfg->ai_layout = AI_LAYOUT_NORMAL;
  pstAudioCfg->vqe_mode = RKADK_VQE_MODE_AI_RECORD;
  RKADK_PARAM_SaveAudioCfg(path);
}

static void RKADK_PARAM_DefThumbCfg(const char *path) {
  RKADK_PARAM_THUMB_CFG_S *pstThumbCfg = &g_stPARAMCtx.stCfg.stThumbCfg;

  memset(pstThumbCfg, 0, sizeof(RKADK_PARAM_THUMB_CFG_S));
  pstThumbCfg->thumb_width = THUMB_WIDTH;
  pstThumbCfg->thumb_height = THUMB_HEIGHT;
  pstThumbCfg->venc_chn = THUMB_VENC_CHN;
  RKADK_PARAM_SaveThumbCfg(path);
}

static void RKADK_PARAM_DefSensorCfg(RKADK_U32 u32CamId, const char *path) {
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = &g_stPARAMCtx.stCfg.stSensorCfg[0];

  memset(pstSensorCfg, 0, sizeof(RKADK_PARAM_SENSOR_CFG_S));
  if (u32CamId == 0) {
    pstSensorCfg->max_width = SENSOR_MAX_WIDTH;
    pstSensorCfg->max_height = SENSOR_MAX_HEIGHT;
  }

  pstSensorCfg->framerate = VIDEO_FRAME_RATE;
  pstSensorCfg->enable_record = true;
  pstSensorCfg->enable_photo = true;
  pstSensorCfg->flip = false;
  pstSensorCfg->mirror = false;
  pstSensorCfg->ldc = 0;
  pstSensorCfg->wdr = 0;
  pstSensorCfg->hdr = 0;
  pstSensorCfg->antifog = 0;
  RKADK_PARAM_SaveSensorCfg(path, u32CamId);
}

static void RKADK_PARAM_DefViCfg(RKADK_U32 u32CamId, RKADK_U32 u32ViIndex,
                                 const char *path) {
  RKADK_PARAM_VI_CFG_S *pstViCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stViCfg[u32ViIndex];

  memset(pstViCfg, 0, sizeof(RKADK_PARAM_VI_CFG_S));
  pstViCfg->chn_id = u32ViIndex + (u32CamId * RKADK_ISPP_VI_NODE_CNT);
  switch (pstViCfg->chn_id) {
  case 0:
    memcpy(pstViCfg->device_name, "rkispp_m_bypass", strlen("rkispp_m_bypass"));
    pstViCfg->buf_cnt = 4;
    pstViCfg->width = RECORD_VIDEO_WIDTH;
    pstViCfg->height = RECORD_VIDEO_HEIGHT;
    memcpy(pstViCfg->pix_fmt, "FBC0", strlen("FBC0"));
    memcpy(pstViCfg->module, "RECORD_MAIN|PHOTO", strlen("RECORD_MAIN|PHOTO"));
    break;
  case 1:
    memcpy(pstViCfg->device_name, "rkispp_scale0", strlen("rkispp_scale0"));
    pstViCfg->buf_cnt = 4;
    memcpy(pstViCfg->pix_fmt, "NV12", strlen("NV12"));
    memcpy(pstViCfg->module, "RECORD_MAIN|PHOTO", strlen("RECORD_MAIN|PHOTO"));
    break;
  case 2:
    memcpy(pstViCfg->device_name, "rkispp_scale1", strlen("rkispp_scale1"));
    pstViCfg->buf_cnt = 4;
    memcpy(pstViCfg->pix_fmt, "NV12", strlen("NV12"));
    memcpy(pstViCfg->module, "NONE", strlen("NONE"));
    break;
  case 3:
    memcpy(pstViCfg->device_name, "rkispp_scale2", strlen("rkispp_scale2"));
    pstViCfg->buf_cnt = 4;
    pstViCfg->width = STREAM_VIDEO_WIDTH;
    pstViCfg->height = STREAM_VIDEO_HEIGHT;
    memcpy(pstViCfg->pix_fmt, "NV12", strlen("NV12"));
    memcpy(pstViCfg->module, "RECORD_SUB|PREVIEW|LIVE|DISP",
           strlen("RECORD_SUB|PREVIEW|LIVE|DISP"));
    break;
  default:
    RKADK_LOGE("Nonsupport vi index: %d", u32ViIndex);
    return;
  }

  RKADK_PARAM_SaveViCfg(path, u32ViIndex, u32CamId);
}

static void RKADK_PARAM_DefVencParam(const char *path, RKADK_U32 u32CamId,
                                     RKADK_STREAM_TYPE_E enStrmType) {
  RKADK_PARAM_VENC_PARAM_S *pstVencParam = NULL;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;

  if (u32CamId >= RKADK_MAX_SENSOR_CNT) {
    RKADK_LOGE("invalid camera id: %d", u32CamId);
    return;
  }

  pstRecCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;
  switch (enStrmType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    pstVencParam = &(pstRecCfg->attribute[0].venc_param);
    break;
  case RKADK_STREAM_TYPE_VIDEO_SUB:
    pstVencParam = &(pstRecCfg->attribute[1].venc_param);
    break;
  case RKADK_STREAM_TYPE_PREVIEW:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stStreamCfg;
    pstVencParam = &(pstStreamCfg->attribute.venc_param);
    break;
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stLiveCfg;
    pstVencParam = &(pstStreamCfg->attribute.venc_param);
    break;
  default:
    return;
  }

  memset(pstVencParam, 0, sizeof(RKADK_PARAM_VENC_PARAM_S));
  pstVencParam->max_qp = 49;
  pstVencParam->min_qp = 25;
  pstVencParam->full_range = true;
  pstVencParam->scaling_list = true;
  pstVencParam->hier_qp_en = true;
  strcpy(pstVencParam->hier_qp_delta, "-3,0,0,0");
  strcpy(pstVencParam->hier_frame_num, "3,0,0,0");

  RKADK_PARAM_SaveVencParamCfg(path, u32CamId, enStrmType);
}

static void RKADK_PARAM_DefRecTime(RKADK_U32 u32CamId,
                                   RKADK_STREAM_TYPE_E enStrmType,
                                   const char *path) {
  RKADK_PARAM_REC_TIME_CFG_S *pstTimeCfg = NULL;

  if (u32CamId >= RKADK_MAX_SENSOR_CNT) {
    RKADK_LOGE("invalid camera id: %d", u32CamId);
    return;
  }

  if (enStrmType == RKADK_STREAM_TYPE_VIDEO_MAIN)
    pstTimeCfg =
        &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg.record_time_cfg[0];
  else
    pstTimeCfg =
        &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg.record_time_cfg[1];

  memset(pstTimeCfg, 0, sizeof(RKADK_PARAM_REC_TIME_CFG_S));
  pstTimeCfg->record_time = 60;
  pstTimeCfg->splite_time = 60;
  pstTimeCfg->lapse_interval = 60;

  RKADK_PARAM_SaveRecTime(path, u32CamId, enStrmType);
}

static void RKADK_PARAM_DefRecAttr(RKADK_U32 u32CamId,
                                   RKADK_STREAM_TYPE_E enStrmType,
                                   const char *path) {
  RKADK_U32 u32Width, u32Height;
  RKADK_U32 u32VecnChn;
  RKADK_U32 u32Bitrate;
  RKADK_PARAM_VENC_ATTR_S *pstAttr = NULL;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;

  if (u32CamId >= RKADK_MAX_SENSOR_CNT) {
    RKADK_LOGE("invalid camera id: %d", u32CamId);
    return;
  }

  pstRecCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;
  switch (enStrmType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    pstAttr = &pstRecCfg->attribute[0];
    u32Width = RECORD_VIDEO_WIDTH;
    u32Height = RECORD_VIDEO_HEIGHT;
    u32VecnChn = 0;
    u32Bitrate = 30 * 1024 * 1024;
    break;
  case RKADK_STREAM_TYPE_VIDEO_SUB:
    pstAttr = &pstRecCfg->attribute[1];
    u32Width = RECORD_VIDEO_WIDTH_S;
    u32Height = RECORD_VIDEO_HEIGHT_S;
    u32VecnChn = 1;
    u32Bitrate = 4 * 1024 * 1024;
    break;
  default:
    return;
  }

  memset(pstAttr, 0, sizeof(RKADK_PARAM_VENC_ATTR_S));
  pstAttr->width = u32Width;
  pstAttr->height = u32Height;
  pstAttr->venc_chn = u32VecnChn;
  pstAttr->bitrate = u32Bitrate;
  pstAttr->gop = VIDEO_GOP;
  pstAttr->profile = VIDEO_PROFILE;
  pstAttr->codec_type = RKADK_CODEC_TYPE_H264;
  memcpy(pstAttr->rc_mode, "CBR", strlen("CBR"));

  RKADK_PARAM_SaveRecAttr(path, u32CamId, enStrmType);
}

static void RKADK_PARAM_DefRecCfg(RKADK_U32 u32CamId, const char *path) {
  RKADK_PARAM_REC_CFG_S *pstRecCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;

  memset(pstRecCfg, 0, sizeof(RKADK_PARAM_REC_CFG_S));
  pstRecCfg->record_type = RKADK_REC_TYPE_NORMAL;
  pstRecCfg->pre_record_time = 0;
  pstRecCfg->pre_record_mode = MUXER_PRE_RECORD_NONE;
  pstRecCfg->lapse_multiple = 30;
  pstRecCfg->file_num = 2;
  RKADK_PARAM_SaveRecCfg(path, u32CamId);
}

static void RKADK_PARAM_DefPhotoCfg(RKADK_U32 u32CamId, const char *path) {
  RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stPhotoCfg;

  memset(pstPhotoCfg, 0, sizeof(RKADK_PARAM_PHOTO_CFG_S));
  pstPhotoCfg->snap_num = 1;
  if (u32CamId == 0) {
    pstPhotoCfg->image_width = PHOTO_VIDEO_WIDTH;
    pstPhotoCfg->image_height = PHOTO_VIDEO_HEIGHT;
    pstPhotoCfg->venc_chn = 2;
  }

  RKADK_PARAM_SavePhotoCfg(path, u32CamId);
}

static void RKADK_PARAM_DefStreamCfg(RKADK_U32 u32CamId, const char *path,
                                     RKADK_STREAM_TYPE_E enStrmType) {
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg = NULL;

  switch (enStrmType) {
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stLiveCfg;
    break;
  case RKADK_STREAM_TYPE_PREVIEW:
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stStreamCfg;
    break;
  default:
    RKADK_LOGW("unsupport enStrmType = %d", enStrmType);
    return;
  }

  memset(pstStreamCfg, 0, sizeof(RKADK_PARAM_STREAM_CFG_S));
  if (u32CamId == 0) {
    pstStreamCfg->attribute.width = STREAM_VIDEO_WIDTH;
    pstStreamCfg->attribute.height = STREAM_VIDEO_HEIGHT;
    pstStreamCfg->attribute.venc_chn = 1;
  }

  pstStreamCfg->attribute.gop = VIDEO_GOP;
  pstStreamCfg->attribute.bitrate = 4 * 1024 * 1024;
  pstStreamCfg->attribute.profile = VIDEO_PROFILE;
  pstStreamCfg->attribute.codec_type = RKADK_CODEC_TYPE_H264;
  memcpy(pstStreamCfg->attribute.rc_mode, "VBR", strlen("VBR"));

  RKADK_PARAM_SaveStreamCfg(path, u32CamId, enStrmType);
}

static void RKADK_PARAM_DefDispCfg(RKADK_U32 u32CamId, const char *path) {
  RKADK_PARAM_DISP_CFG_S *pstDispCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stDispCfg;

  memset(pstDispCfg, 0, sizeof(RKADK_PARAM_DISP_CFG_S));
  if (u32CamId == 0) {
    pstDispCfg->width = DISP_WIDTH;
    pstDispCfg->height = DISP_HEIGHT;
    // rga
    pstDispCfg->enable_buf_pool = true;
    pstDispCfg->buf_pool_cnt = 3;
    pstDispCfg->rotaion = 90;
    pstDispCfg->rga_chn = 0;
    // vo
    memcpy(pstDispCfg->device_node, "/dev/dri/card0", strlen("/dev/dri/card0"));
    memcpy(pstDispCfg->img_type, "RGB888", strlen("RGB888"));
    pstDispCfg->plane_type = VO_PLANE_PRIMARY;
    pstDispCfg->z_pos = 0;
    pstDispCfg->vo_chn = 0;
  }

  RKADK_PARAM_SaveDispCfg(path, u32CamId);
}

static void RKADK_PARAM_Dump() {
  int i, j;
  RKADK_PARAM_CFG_S *pstCfg = &g_stPARAMCtx.stCfg;

  printf("Version: %s\n", pstCfg->stVersion.version);

  printf("Common Config\n");
  printf("\tsensor_count: %d\n", pstCfg->stCommCfg.sensor_count);
  printf("\trec_unmute: %d\n", pstCfg->stCommCfg.rec_unmute);
  printf("\tenable_speaker: %d\n", pstCfg->stCommCfg.enable_speaker);
  printf("\tspeaker_volume: %d\n", pstCfg->stCommCfg.speaker_volume);
  printf("\tmic_unmute: %d\n", pstCfg->stCommCfg.mic_unmute);
  printf("\tmic_volume: %d\n", pstCfg->stCommCfg.mic_volume);
  printf("\tosd_time_format: %d\n", pstCfg->stCommCfg.osd_time_format);
  printf("\tshow osd: %d\n", pstCfg->stCommCfg.osd);
  printf("\tboot_sound: %d\n", pstCfg->stCommCfg.boot_sound);
  printf("\tosd_speed: %d\n", pstCfg->stCommCfg.osd_speed);

  printf("Audio Config\n");
  printf("\taudio_node: %s\n", pstCfg->stAudioCfg.audio_node);
  printf("\tsample_format: %d\n", pstCfg->stAudioCfg.sample_format);
  printf("\tchannels: %d\n", pstCfg->stAudioCfg.channels);
  printf("\tsamplerate: %d\n", pstCfg->stAudioCfg.samplerate);
  printf("\tsamples_per_frame: %d\n", pstCfg->stAudioCfg.samples_per_frame);
  printf("\tbitrate: %d\n", pstCfg->stAudioCfg.bitrate);
  printf("\tai_layout: %d\n", pstCfg->stAudioCfg.ai_layout);
  printf("\tvqe_mode: %d\n", pstCfg->stAudioCfg.vqe_mode);

  printf("Thumb Config\n");
  printf("\tthumb_width: %d\n", pstCfg->stThumbCfg.thumb_width);
  printf("\tthumb_height: %d\n", pstCfg->stThumbCfg.thumb_height);
  printf("\tvenc_chn: %d\n", pstCfg->stThumbCfg.venc_chn);

  for (i = 0; i < (int)pstCfg->stCommCfg.sensor_count; i++) {
    printf("Sensor[%d] Config\n", i);
    printf("\tsensor[%d] max_width: %d\n", i, pstCfg->stSensorCfg[i].max_width);
    printf("\tsensor[%d] max_height: %d\n", i,
           pstCfg->stSensorCfg[i].max_height);
    printf("\tsensor[%d] framerate: %d\n", i, pstCfg->stSensorCfg[i].framerate);
    printf("\tsensor[%d] enable_record: %d\n", i,
           pstCfg->stSensorCfg[i].enable_record);
    printf("\tsensor[%d] enable_photo: %d\n", i,
           pstCfg->stSensorCfg[i].enable_photo);
    printf("\tsensor[%d] flip: %d\n", i, pstCfg->stSensorCfg[i].flip);
    printf("\tsensor[%d] mirror: %d\n", i, pstCfg->stSensorCfg[i].mirror);
    printf("\tsensor[%d] wdr: %d\n", i, pstCfg->stSensorCfg[i].wdr);
    printf("\tsensor[%d] hdr: %d\n", i, pstCfg->stSensorCfg[i].hdr);
    printf("\tsensor[%d] ldc: %d\n", i, pstCfg->stSensorCfg[i].ldc);
    printf("\tsensor[%d] antifog: %d\n", i, pstCfg->stSensorCfg[i].antifog);

    printf("\tVI Config\n");
    for (j = 0; j < RKADK_ISPP_VI_NODE_CNT; j++) {
      printf("\t\tsensor[%d] VI[%d] device_name: %s\n", i, j,
             pstCfg->stMediaCfg[i].stViCfg[j].device_name);
      printf("\t\tsensor[%d] VI[%d] module: %s\n", i, j,
             pstCfg->stMediaCfg[i].stViCfg[j].module);
      printf("\t\tsensor[%d] VI[%d] chn_id: %d\n", i, j,
             pstCfg->stMediaCfg[i].stViCfg[j].chn_id);
      printf("\t\tsensor[%d] VI[%d] buf_cnt: %d\n", i, j,
             pstCfg->stMediaCfg[i].stViCfg[j].buf_cnt);
      printf("\t\tsensor[%d] VI[%d] pix_fmt: %s\n", i, j,
             pstCfg->stMediaCfg[i].stViCfg[j].pix_fmt);
      printf("\t\tsensor[%d] VI[%d] width: %d\n", i, j,
             pstCfg->stMediaCfg[i].stViCfg[j].width);
      printf("\t\tsensor[%d] VI[%d] height: %d\n", i, j,
             pstCfg->stMediaCfg[i].stViCfg[j].height);
    }

    printf("\tRecord Config\n");
    printf("\t\tsensor[%d] stRecCfg record_type: %d\n", i,
           pstCfg->stMediaCfg[i].stRecCfg.record_type);
    printf("\t\tsensor[%d] stRecCfg pre_record_time: %d\n", i,
           pstCfg->stMediaCfg[i].stRecCfg.pre_record_time);
    printf("\t\tsensor[%d] stRecCfg pre_record_mode: %d\n", i,
           pstCfg->stMediaCfg[i].stRecCfg.pre_record_mode);
    printf("\t\tsensor[%d] stRecCfg lapse_multiple: %d\n", i,
           pstCfg->stMediaCfg[i].stRecCfg.lapse_multiple);
    printf("\t\tsensor[%d] stRecCfg file_num: %d\n", i,
           pstCfg->stMediaCfg[i].stRecCfg.file_num);

    for (j = 0; j < (int)pstCfg->stMediaCfg[i].stRecCfg.file_num; j++) {
      printf("\t\tsensor[%d] stRecCfg record_time: %d\n", i,
             pstCfg->stMediaCfg[i].stRecCfg.record_time_cfg[j].record_time);
      printf("\t\tsensor[%d] stRecCfg splite_time: %d\n", i,
             pstCfg->stMediaCfg[i].stRecCfg.record_time_cfg[j].splite_time);
      printf("\t\tsensor[%d] stRecCfg lapse_interval: %d\n", i,
             pstCfg->stMediaCfg[i].stRecCfg.record_time_cfg[j].lapse_interval);
      printf("\t\tsensor[%d] stRecCfg[%d] width: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].width);
      printf("\t\tsensor[%d] stRecCfg[%d] height: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].height);
      printf("\t\tsensor[%d] stRecCfg[%d] bitrate: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].bitrate);
      printf("\t\tsensor[%d] stRecCfg[%d] gop: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].gop);
      printf("\t\tsensor[%d] stRecCfg[%d] profile: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].profile);
      printf("\t\tsensor[%d] stRecCfg[%d] venc_chn: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_chn);
      printf("\t\tsensor[%d] stRecCfg[%d] codec_type: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].codec_type);
      printf("\t\tsensor[%d] stRecCfg[%d] rc_mode: %s\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].rc_mode);
      printf("\t\tsensor[%d] stRecCfg[%d] first_frame_qp: %d\n", i, j,
             pstCfg->stMediaCfg[i]
                 .stRecCfg.attribute[j]
                 .venc_param.first_frame_qp);
      printf("\t\tsensor[%d] stRecCfg[%d] qp_step: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param.qp_step);
      printf("\t\tsensor[%d] stRecCfg[%d] max_qp: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param.max_qp);
      printf("\t\tsensor[%d] stRecCfg[%d] min_qp: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param.min_qp);
      printf("\t\tsensor[%d] stRecCfg[%d] row_qp_delta_i: %d\n", i, j,
             pstCfg->stMediaCfg[i]
                 .stRecCfg.attribute[j]
                 .venc_param.row_qp_delta_i);
      printf("\t\tsensor[%d] stRecCfg[%d] row_qp_delta_p: %d\n", i, j,
             pstCfg->stMediaCfg[i]
                 .stRecCfg.attribute[j]
                 .venc_param.row_qp_delta_p);
      printf("\t\tsensor[%d] stRecCfg[%d] full_range: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param.full_range);
      printf(
          "\t\tsensor[%d] stRecCfg[%d] scaling_list: %d\n", i, j,
          pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param.scaling_list);
      printf("\t\tsensor[%d] stRecCfg[%d] hier_qp_en: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param.hier_qp_en);
      printf(
          "\t\tsensor[%d] stRecCfg[%d] hier_qp_delta: %s\n", i, j,
          pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param.hier_qp_delta);
      printf("\t\tsensor[%d] stRecCfg[%d] hier_frame_num: %s\n", i, j,
             pstCfg->stMediaCfg[i]
                 .stRecCfg.attribute[j]
                 .venc_param.hier_frame_num);
    }

    printf("\tPhoto Config\n");
    printf("\t\tsensor[%d] stPhotoCfg image_width: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.image_width);
    printf("\t\tsensor[%d] stPhotoCfg image_height: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.image_height);
    printf("\t\tsensor[%d] stPhotoCfg snap_num: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.snap_num);
    printf("\t\tsensor[%d] stPhotoCfg venc_chn: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.venc_chn);

    printf("\tStream Config\n");
    printf("\t\tsensor[%d] stStreamCfg width: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.width);
    printf("\t\tsensor[%d] stStreamCfg height: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.height);
    printf("\t\tsensor[%d] stStreamCfg bitrate: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.bitrate);
    printf("\t\tsensor[%d] stStreamCfg gop: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.gop);
    printf("\t\tsensor[%d] stStreamCfg profile: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.profile);
    printf("\t\tsensor[%d] stStreamCfg venc_chn: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_chn);
    printf("\t\tsensor[%d] stStreamCfg codec_type: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.codec_type);
    printf("\t\tsensor[%d] stStreamCfg rc_mode: %s\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.rc_mode);
    printf(
        "\t\tsensor[%d] stStreamCfg first_frame_qp: %d\n", i,
        pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.first_frame_qp);
    printf("\t\tsensor[%d] stStreamCfg qp_step: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.qp_step);
    printf("\t\tsensor[%d] stStreamCfg max_qp: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.max_qp);
    printf("\t\tsensor[%d] stStreamCfg min_qp: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.min_qp);
    printf(
        "\t\tsensor[%d] stStreamCfg row_qp_delta_i: %d\n", i,
        pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.row_qp_delta_i);
    printf(
        "\t\tsensor[%d] stStreamCfg row_qp_delta_p: %d\n", i,
        pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.row_qp_delta_p);
    printf("\t\tsensor[%d] stStreamCfg full_range: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.full_range);
    printf("\t\tsensor[%d] stStreamCfg scaling_list: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.scaling_list);
    printf("\t\tsensor[%d] stStreamCfg hier_qp_en: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.hier_qp_en);
    printf(
        "\t\tsensor[%d] stStreamCfg hier_qp_delta: %s\n", i,
        pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.hier_qp_delta);
    printf(
        "\t\tsensor[%d] stStreamCfg hier_frame_num: %s\n", i,
        pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param.hier_frame_num);

    printf("\tLive Config\n");
    printf("\t\tsensor[%d] stLiveCfg width: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.width);
    printf("\t\tsensor[%d] stLiveCfg height: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.height);
    printf("\t\tsensor[%d] stLiveCfg bitrate: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.bitrate);
    printf("\t\tsensor[%d] stLiveCfg gop: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.gop);
    printf("\t\tsensor[%d] stLiveCfg profile: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.profile);
    printf("\t\tsensor[%d] stLiveCfg venc_chn: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_chn);
    printf("\t\tsensor[%d] stLiveCfg codec_type: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.codec_type);
    printf("\t\tsensor[%d] stLiveCfg rc_mode: %s\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.rc_mode);
    printf("\t\tsensor[%d] stLiveCfg first_frame_qp: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.first_frame_qp);
    printf("\t\tsensor[%d] stLiveCfg qp_step: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.qp_step);
    printf("\t\tsensor[%d] stLiveCfg max_qp: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.max_qp);
    printf("\t\tsensor[%d] stLiveCfg min_qp: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.min_qp);
    printf("\t\tsensor[%d] stLiveCfg row_qp_delta_i: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.row_qp_delta_i);
    printf("\t\tsensor[%d] stLiveCfg row_qp_delta_p: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.row_qp_delta_p);
    printf("\t\tsensor[%d] stLiveCfg full_range: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.full_range);
    printf("\t\tsensor[%d] stLiveCfg scaling_list: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.scaling_list);
    printf("\t\tsensor[%d] stLiveCfg hier_qp_en: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.hier_qp_en);
    printf("\t\tsensor[%d] stLiveCfg hier_qp_delta: %s\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.hier_qp_delta);
    printf("\t\tsensor[%d] stLiveCfg hier_frame_num: %s\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param.hier_frame_num);

    printf("\tDisplay Config\n");
    printf("\t\tsensor[%d] stDispCfg width: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.width);
    printf("\t\tsensor[%d] stDispCfg height: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.height);
    printf("\t\tsensor[%d] stDispCfg enable_buf_pool: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.enable_buf_pool);
    printf("\t\tsensor[%d] stDispCfg buf_pool_cnt: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.buf_pool_cnt);
    printf("\t\tsensor[%d] stDispCfg rotaion: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.rotaion);
    printf("\t\tsensor[%d] stDispCfg rga_chn: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.rga_chn);
    printf("\t\tsensor[%d] stDispCfg device_node: %s\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.device_node);
    printf("\t\tsensor[%d] stDispCfg img_type: %s\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.img_type);
    printf("\t\tsensor[%d] stDispCfg plane_type: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.plane_type);
    printf("\t\tsensor[%d] stDispCfg z_pos: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.z_pos);
    printf("\t\tsensor[%d] stDispCfg vo_chn: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vo_chn);
  }
}

static void RKADK_PARAM_DumpViAttr() {
  int i, j;
  RKADK_PARAM_CFG_S *pstCfg = &g_stPARAMCtx.stCfg;

  for (i = 0; i < (int)pstCfg->stCommCfg.sensor_count; i++) {
    for (j = 0; j < (int)pstCfg->stMediaCfg[i].stRecCfg.file_num; j++) {
      printf("\tRec VI Attribute\n");
      printf("\t\tsensor[%d] stRecCfg[%d] pcVideoNode: %s\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].stChnAttr.pcVideoNode);
      printf("\t\tsensor[%d] stRecCfg[%d] u32ViChn: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].u32ViChn);
      printf("\t\tsensor[%d] stRecCfg[%d] u32Width: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].stChnAttr.u32Width);
      printf("\t\tsensor[%d] stRecCfg[%d] u32Height: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].stChnAttr.u32Height);
      printf("\t\tsensor[%d] stRecCfg[%d] u32BufCnt: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].stChnAttr.u32BufCnt);
      printf("\t\tsensor[%d] stRecCfg[%d] enPixFmt: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].stChnAttr.enPixFmt);
      printf("\t\tsensor[%d] stRecCfg[%d] enBufType: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].stChnAttr.enBufType);
      printf("\t\tsensor[%d] stRecCfg[%d] enWorkMode: %d\n", i, j,
             pstCfg->stMediaCfg[i].stRecCfg.vi_attr[j].stChnAttr.enWorkMode);
    }

    printf("\tPhoto VI Attribute\n");
    printf("\t\tsensor[%d] stPhotoCfg pcVideoNode: %s\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.stChnAttr.pcVideoNode);
    printf("\t\tsensor[%d] stPhotoCfg u32ViChn: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.u32ViChn);
    printf("\t\tsensor[%d] stPhotoCfg u32Width: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.stChnAttr.u32Width);
    printf("\t\tsensor[%d] stPhotoCfg u32Height: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.stChnAttr.u32Height);
    printf("\t\tsensor[%d] stPhotoCfg u32BufCnt: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.stChnAttr.u32BufCnt);
    printf("\t\tsensor[%d] stPhotoCfg enPixFmt: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.stChnAttr.enPixFmt);
    printf("\t\tsensor[%d] stPhotoCfg enBufType: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.stChnAttr.enBufType);
    printf("\t\tsensor[%d] stPhotoCfg enWorkMode: %d\n", i,
           pstCfg->stMediaCfg[i].stPhotoCfg.vi_attr.stChnAttr.enWorkMode);

    printf("\tStream VI Attribute\n");
    printf("\t\tsensor[%d] stStreamCfg pcVideoNode: %s\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.stChnAttr.pcVideoNode);
    printf("\t\tsensor[%d] stStreamCfg u32ViChn: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.u32ViChn);
    printf("\t\tsensor[%d] stStreamCfg u32Width: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.stChnAttr.u32Width);
    printf("\t\tsensor[%d] stStreamCfg u32Height: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.stChnAttr.u32Height);
    printf("\t\tsensor[%d] stStreamCfg u32BufCnt: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.stChnAttr.u32BufCnt);
    printf("\t\tsensor[%d] stStreamCfg enPixFmt: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.stChnAttr.enPixFmt);
    printf("\t\tsensor[%d] stStreamCfg enBufType: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.stChnAttr.enBufType);
    printf("\t\tsensor[%d] stStreamCfg enWorkMode: %d\n", i,
           pstCfg->stMediaCfg[i].stStreamCfg.vi_attr.stChnAttr.enWorkMode);

    printf("\tLive VI Attribute\n");
    printf("\t\tsensor[%d] stLiveCfg pcVideoNode: %s\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.stChnAttr.pcVideoNode);
    printf("\t\tsensor[%d] stLiveCfg u32ViChn: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.u32ViChn);
    printf("\t\tsensor[%d] stLiveCfg u32Width: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.stChnAttr.u32Width);
    printf("\t\tsensor[%d] stLiveCfg u32Height: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.stChnAttr.u32Height);
    printf("\t\tsensor[%d] stLiveCfg u32BufCnt: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.stChnAttr.u32BufCnt);
    printf("\t\tsensor[%d] stLiveCfg enPixFmt: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.stChnAttr.enPixFmt);
    printf("\t\tsensor[%d] stLiveCfg enBufType: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.stChnAttr.enBufType);
    printf("\t\tsensor[%d] stLiveCfg enWorkMode: %d\n", i,
           pstCfg->stMediaCfg[i].stLiveCfg.vi_attr.stChnAttr.enWorkMode);

    printf("\tDisplay VI Attribute\n");
    printf("\t\tsensor[%d] stDispCfg pcVideoNode: %s\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.stChnAttr.pcVideoNode);
    printf("\t\tsensor[%d] stDispCfg u32ViChn: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.u32ViChn);
    printf("\t\tsensor[%d] stDispCfg u32Width: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.stChnAttr.u32Width);
    printf("\t\tsensor[%d] stDispCfg u32Height: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.stChnAttr.u32Height);
    printf("\t\tsensor[%d] stDispCfg u32BufCnt: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.stChnAttr.u32BufCnt);
    printf("\t\tsensor[%d] stDispCfg enPixFmt: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.stChnAttr.enPixFmt);
    printf("\t\tsensor[%d] stDispCfg enBufType: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.stChnAttr.enBufType);
    printf("\t\tsensor[%d] stDispCfg enWorkMode: %d\n", i,
           pstCfg->stMediaCfg[i].stDispCfg.vi_attr.stChnAttr.enWorkMode);
  }
}

static RKADK_S32 RKADK_PARAM_LoadParam(const char *path) {
  int i, j, ret = 0;
  RKADK_MAP_TABLE_CFG_S *pstMapTableCfg = NULL;
  RKADK_PARAM_MAP_TYPE_E enMapType = RKADK_PARAM_MAP_BUTT;
  RKADK_PARAM_CFG_S *pstCfg = &g_stPARAMCtx.stCfg;

  if (!strcmp(path, RKADK_PARAM_PATH)) {
    if (!RKADK_PARAM_CheckVersion())
      return -1;
  }

  // load common config
  memset(&pstCfg->stCommCfg, 0, sizeof(RKADK_PARAM_COMM_CFG_S));
  ret = RKADK_Ini2Struct(path, &pstCfg->stCommCfg, g_stCommCfgMapTable,
                         sizeof(g_stCommCfgMapTable) /
                             sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret == RKADK_PARAM_NOT_EXIST) {
    // use default
    RKADK_LOGW("common config param not exist, use default");
    RKADK_PARAM_DefCommCfg(path);
  } else if (ret) {
    RKADK_LOGE("load %s failed", path);
    return ret;
  }
  RKADK_PARAM_CheckCommCfg(path);

  // load audio config
  memset(&pstCfg->stAudioCfg, 0, sizeof(RKADK_PARAM_AUDIO_CFG_S));
  ret = RKADK_Ini2Struct(path, &pstCfg->stAudioCfg, g_stAudioCfgMapTable,
                         sizeof(g_stAudioCfgMapTable) /
                             sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret == RKADK_PARAM_NOT_EXIST) {
    // use default
    RKADK_LOGW("audio config param not exist, use default");
    RKADK_PARAM_DefAudioCfg(path);
  } else if (ret) {
    RKADK_LOGE("load audio param failed");
    return ret;
  }
  RKADK_PARAM_CheckAudioCfg(path);

  // load thumb config
  memset(&pstCfg->stThumbCfg, 0, sizeof(RKADK_PARAM_THUMB_CFG_S));
  ret = RKADK_Ini2Struct(path, &pstCfg->stThumbCfg, g_stThumbCfgMapTable,
                         sizeof(g_stThumbCfgMapTable) /
                             sizeof(RKADK_SI_CONFIG_MAP_S));
  if (ret == RKADK_PARAM_NOT_EXIST) {
    // use default
    RKADK_LOGW("thumb config param not exist, use default");
    RKADK_PARAM_DefThumbCfg(path);
  } else if (ret) {
    RKADK_LOGE("load thumb param failed");
    return ret;
  }
  RKADK_PARAM_CheckThumbCfg(path);

  // load sensor config
  for (i = 0; i < (int)pstCfg->stCommCfg.sensor_count; i++) {
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_SENSOR_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    memset(&pstCfg->stSensorCfg[i], 0, sizeof(RKADK_PARAM_SENSOR_CFG_S));
    ret = RKADK_Ini2Struct(path, &pstCfg->stSensorCfg[i],
                           pstMapTableCfg->pstMapTable,
                           pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] config param not exist, use default", i);
      RKADK_PARAM_DefSensorCfg(i, path);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] param failed", i);
      return ret;
    }
    RKADK_PARAM_CheckSensorCfg(path, i);

    // load vi config
    for (j = 0; j < RKADK_ISPP_VI_NODE_CNT; j++) {
      if (j == 0) {
        enMapType = RKADK_PARAM_VI0_MAP;
      } else if (j == 1) {
        enMapType = RKADK_PARAM_VI1_MAP;
      } else if (j == 2) {
        enMapType = RKADK_PARAM_VI2_MAP;
      } else {
        enMapType = RKADK_PARAM_VI3_MAP;
      }

      pstMapTableCfg = RKADK_PARAM_GetMapTable(i, enMapType);
      RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

      memset(&pstCfg->stMediaCfg[i].stViCfg[j], 0,
             sizeof(RKADK_PARAM_VI_CFG_S));
      ret = RKADK_Ini2Struct(path, &pstCfg->stMediaCfg[i].stViCfg[j],
                             pstMapTableCfg->pstMapTable,
                             pstMapTableCfg->u32TableLen);

      if (ret == RKADK_PARAM_NOT_EXIST) {
        // use default
        RKADK_LOGW("sensor[%d] vi[%d] config param not exist, use default", i,
                   j);
        RKADK_PARAM_DefViCfg(i, j, path);
      } else if (ret) {
        RKADK_LOGE("load sensor[%d] vi[%d] param failed", i, j);
        return ret;
      }
      RKADK_PARAM_CheckViCfg(path, i, j);
    }

    // load record config
    memset(&pstCfg->stMediaCfg[i].stRecCfg, 0, sizeof(RKADK_PARAM_REC_CFG_S));
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_REC_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    ret = RKADK_Ini2Struct(path, &pstCfg->stMediaCfg[i].stRecCfg,
                           pstMapTableCfg->pstMapTable,
                           pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] record param not exist, use default", i);
      RKADK_PARAM_DefRecCfg(i, path);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] record param failed", i);
      return ret;
    }

    for (j = 0; j < (int)pstCfg->stMediaCfg[i].stRecCfg.file_num; j++) {
      RKADK_MAP_TABLE_CFG_S *pstTimeMapTable = NULL;
      RKADK_MAP_TABLE_CFG_S *pstParamMapTable = NULL;
      RKADK_STREAM_TYPE_E enStrmType;

      memset(&pstCfg->stMediaCfg[i].stRecCfg.attribute[j], 0,
             sizeof(RKADK_PARAM_VENC_ATTR_S));
      if (j == 0) {
        enStrmType = RKADK_STREAM_TYPE_VIDEO_MAIN;
        pstTimeMapTable =
            RKADK_PARAM_GetMapTable(i, RKADK_PARAM_REC_MAIN_TIME_MAP);
        pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_REC_MAIN_MAP);
        pstParamMapTable =
            RKADK_PARAM_GetMapTable(i, RKADK_PARAM_REC_MAIN_PARAM_MAP);
      } else {
        enStrmType = RKADK_STREAM_TYPE_VIDEO_SUB;
        pstTimeMapTable =
            RKADK_PARAM_GetMapTable(i, RKADK_PARAM_REC_SUB_TIME_MAP);
        pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_REC_SUB_MAP);
        pstParamMapTable =
            RKADK_PARAM_GetMapTable(i, RKADK_PARAM_REC_SUB_PARAM_MAP);
      }

      if (!pstMapTableCfg || !pstParamMapTable || !pstTimeMapTable)
        return -1;

      ret = RKADK_Ini2Struct(
          path, &pstCfg->stMediaCfg[i].stRecCfg.record_time_cfg[j],
          pstTimeMapTable->pstMapTable, pstTimeMapTable->u32TableLen);
      if (ret == RKADK_PARAM_NOT_EXIST) {
        // use default
        RKADK_LOGW("sensor[%d] rec time[%d] not exist, use default", i, j);
        RKADK_PARAM_DefRecTime(i, enStrmType, path);
      } else if (ret) {
        RKADK_LOGE("load sensor[%d] record time[%d] failed", i, j);
        return ret;
      }

      ret = RKADK_Ini2Struct(path, &pstCfg->stMediaCfg[i].stRecCfg.attribute[j],
                             pstMapTableCfg->pstMapTable,
                             pstMapTableCfg->u32TableLen);
      if (ret == RKADK_PARAM_NOT_EXIST) {
        // use default
        RKADK_LOGW("sensor[%d] rec attribute[%d] param not exist, use default",
                   i, j);
        RKADK_PARAM_DefRecAttr(i, enStrmType, path);
      } else if (ret) {
        RKADK_LOGE("load sensor[%d] record attribute[%d] param failed", i, j);
        return ret;
      }

      ret = RKADK_Ini2Struct(
          path, &pstCfg->stMediaCfg[i].stRecCfg.attribute[j].venc_param,
          pstParamMapTable->pstMapTable, pstParamMapTable->u32TableLen);
      if (ret == RKADK_PARAM_NOT_EXIST) {
        // use default
        RKADK_LOGW(
            "sensor[%d] rec attribute[%d] venc param not exist, use default", i,
            j);
        RKADK_PARAM_DefVencParam(path, i, enStrmType);
      } else if (ret) {
        RKADK_LOGE("load sensor[%d] record attribute[%d] venc param failed", i,
                   j);
        return ret;
      }
    }
    RKADK_PARAM_CheckRecCfg(path, i);

    // load preview config
    memset(&pstCfg->stMediaCfg[i].stStreamCfg.attribute, 0,
           sizeof(RKADK_PARAM_VENC_ATTR_S));
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_PREVIEW_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    ret = RKADK_Ini2Struct(path, &pstCfg->stMediaCfg[i].stStreamCfg.attribute,
                           pstMapTableCfg->pstMapTable,
                           pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] stream config param not exist, use default", i);
      RKADK_PARAM_DefStreamCfg(i, path, RKADK_STREAM_TYPE_PREVIEW);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] stream param failed", i);
      return ret;
    }

    // load preview venc param
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_PREVIEW_PARAM_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    ret = RKADK_Ini2Struct(
        path, &pstCfg->stMediaCfg[i].stStreamCfg.attribute.venc_param,
        pstMapTableCfg->pstMapTable, pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] stream venc param not exist, use default", i);
      RKADK_PARAM_DefVencParam(path, i, RKADK_STREAM_TYPE_PREVIEW);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] stream venc param failed", i);
      return ret;
    }
    RKADK_PARAM_CheckStreamCfg(path, i, RKADK_STREAM_TYPE_PREVIEW);

    // load live config
    memset(&pstCfg->stMediaCfg[i].stLiveCfg.attribute, 0,
           sizeof(RKADK_PARAM_VENC_ATTR_S));
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_LIVE_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    ret = RKADK_Ini2Struct(path, &pstCfg->stMediaCfg[i].stLiveCfg.attribute,
                           pstMapTableCfg->pstMapTable,
                           pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] live config param not exist, use default", i);
      RKADK_PARAM_DefStreamCfg(i, path, RKADK_STREAM_TYPE_LIVE);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] live param failed", i);
      return ret;
    }

    // load live venc param
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_LIVE_PARAM_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    ret = RKADK_Ini2Struct(
        path, &pstCfg->stMediaCfg[i].stLiveCfg.attribute.venc_param,
        pstMapTableCfg->pstMapTable, pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] live venc param not exist, use default", i);
      RKADK_PARAM_DefVencParam(path, i, RKADK_STREAM_TYPE_LIVE);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] live venc param failed", i);
      return ret;
    }
    RKADK_PARAM_CheckStreamCfg(path, i, RKADK_STREAM_TYPE_LIVE);

    // load photo config
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_PHOTO_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    memset(&pstCfg->stMediaCfg[i].stPhotoCfg, 0,
           sizeof(RKADK_PARAM_PHOTO_CFG_S));
    ret = RKADK_Ini2Struct(path, &pstCfg->stMediaCfg[i].stPhotoCfg,
                           pstMapTableCfg->pstMapTable,
                           pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] photo config param not exist, use default", i);
      RKADK_PARAM_DefPhotoCfg(i, path);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] photo param failed", i);
      return ret;
    }
    RKADK_PARAM_CheckPhotoCfg(path, i);

    // load display config
    pstMapTableCfg = RKADK_PARAM_GetMapTable(i, RKADK_PARAM_DISP_MAP);
    RKADK_CHECK_POINTER(pstMapTableCfg, RKADK_FAILURE);

    memset(&pstCfg->stMediaCfg[i].stDispCfg, 0, sizeof(RKADK_PARAM_DISP_CFG_S));
    ret = RKADK_Ini2Struct(path, &pstCfg->stMediaCfg[i].stDispCfg,
                           pstMapTableCfg->pstMapTable,
                           pstMapTableCfg->u32TableLen);
    if (ret == RKADK_PARAM_NOT_EXIST) {
      // use default
      RKADK_LOGW("sensor[%d] display config param not exist, use default", i);
      RKADK_PARAM_DefDispCfg(i, path);
    } else if (ret) {
      RKADK_LOGE("load sensor[%d] display param failed", i);
      return ret;
    }
    RKADK_PARAM_CheckDispCfg(path, i);
  }

  return 0;
}

static void RKADK_PARAM_UseDefault() {
  int i;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_STREAM_TYPE_E enStrmType = RKADK_STREAM_TYPE_BUTT;

  // default version
  RKADK_PARAM_VERSION_S *pstVersion = &g_stPARAMCtx.stCfg.stVersion;
  memcpy(pstVersion->version, RKADK_PARAM_VERSION, strlen(RKADK_PARAM_VERSION));
  RKADK_PARAM_SaveVersion(RKADK_PARAM_PATH);

  // default common config
  RKADK_PARAM_DefCommCfg(RKADK_PARAM_PATH);

  // default audio config
  RKADK_PARAM_DefAudioCfg(RKADK_PARAM_PATH);

  // default thumb config
  RKADK_PARAM_DefThumbCfg(RKADK_PARAM_PATH);

  // default sensor.0 config
  RKADK_PARAM_DefSensorCfg(0, RKADK_PARAM_PATH);

  // default vi config
  for (i = 0; i < RKADK_ISPP_VI_NODE_CNT; i++)
    RKADK_PARAM_DefViCfg(0, i, RKADK_PARAM_PATH);

  // default sensor.0.rec config
  RKADK_PARAM_DefRecCfg(0, RKADK_PARAM_PATH);

  pstRecCfg = &g_stPARAMCtx.stCfg.stMediaCfg[0].stRecCfg;
  for (i = 0; i < (int)pstRecCfg->file_num; i++) {
    if (i == 0)
      enStrmType = RKADK_STREAM_TYPE_VIDEO_MAIN;
    else
      enStrmType = RKADK_STREAM_TYPE_VIDEO_SUB;

    RKADK_PARAM_DefRecAttr(0, enStrmType, RKADK_PARAM_PATH);
    RKADK_PARAM_DefVencParam(RKADK_PARAM_PATH, 0, enStrmType);
  }

  // default sensor.0.photo config
  RKADK_PARAM_DefPhotoCfg(0, RKADK_PARAM_PATH);

  // default sensor.0.stream config
  RKADK_PARAM_DefStreamCfg(0, RKADK_PARAM_PATH, RKADK_STREAM_TYPE_PREVIEW);
  RKADK_PARAM_DefVencParam(RKADK_PARAM_PATH, 0, RKADK_STREAM_TYPE_PREVIEW);

  // default sensor.0.live config
  RKADK_PARAM_DefStreamCfg(0, RKADK_PARAM_PATH, RKADK_STREAM_TYPE_LIVE);
  RKADK_PARAM_DefVencParam(RKADK_PARAM_PATH, 0, RKADK_STREAM_TYPE_LIVE);

  // default sensor.0.disp config
  RKADK_PARAM_DefDispCfg(0, RKADK_PARAM_PATH);
}

static RKADK_S32 RKADK_PARAM_LoadDefault() {
  int ret;
  char buffer[RKADK_BUFFER_LEN];

  ret = RKADK_PARAM_LoadParam(RKADK_DEFPARAM_PATH);
  if (ret) {
    RKADK_LOGE("load default ini failed");
    return ret;
  }

  memset(buffer, 0, RKADK_BUFFER_LEN);
  sprintf(buffer, "cp %s %s", RKADK_DEFPARAM_PATH, RKADK_PARAM_PATH);
  system(buffer);

  return 0;
}

static RKADK_S32 RKADK_PARAM_GetViIndex(const char *module, RKADK_S32 s32CamID,
                                        RKADK_U32 width, RKADK_U32 height) {
  int index = -1;
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;

  for (index = 0; index < RKADK_ISPP_VI_NODE_CNT; index++) {
    pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
    if (strstr(pstViCfg->module, module) && pstViCfg->width == width &&
        pstViCfg->height == height)
      return index;
  }

  for (index = 0; index < RKADK_ISPP_VI_NODE_CNT; index++) {
    pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
    if (pstViCfg->width == width && pstViCfg->height == height) {
      if (!strcmp(pstViCfg->module, "NONE")) {
        memcpy(pstViCfg->module, module, strlen(module));
      } else {
        strcat(pstViCfg->module, "|");
        strcat(pstViCfg->module, module);

        if (pstViCfg->buf_cnt < 4)
          pstViCfg->buf_cnt = 4;
      }
      RKADK_PARAM_SaveViCfg(RKADK_PARAM_PATH, index, s32CamID);
      break;
    }
  }

  return index;
}

static RKADK_S32 RKADK_PARAM_FindUsableVi(RKADK_S32 s32CamID, RKADK_U32 width,
                                          RKADK_U32 u32SensorWidth) {
  int index = -1;
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;

  if (width > RKISPP_SCALE1_WIDTH_MAX && width != u32SensorWidth)
    return -1;

  // VI[0] and VI[1] are reserved for Record MAIN_STREAM
  for (index = 2; index < RKADK_ISPP_VI_NODE_CNT; index++) {
    pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
    if (pstViCfg->width == 0 || pstViCfg->height == 0)
      break;
  }

  return index;
}

static RKADK_S32 RKADK_PARAM_FindClosestVi(RKADK_S32 s32CamID,
                                           const char *module, RKADK_U32 width,
                                           RKADK_U32 height) {
  int i, index = -1;
  RKADK_U32 u32WidthDiff = 0, u32PreDiff = 0;
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;

  for (i = 0; i < RKADK_ISPP_VI_NODE_CNT; i++) {
    pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[i];
    if (pstViCfg->width > width && pstViCfg->height > height) {
      u32WidthDiff = pstViCfg->width - width;

      if (!u32PreDiff || u32WidthDiff < u32PreDiff) {
        u32PreDiff = u32WidthDiff;
        index = i;
      }
    }
  }

  if (index == -1) {
    RKADK_LOGE("%s[%d*%d]: Not find closest vi config", module, width, height);
    return -1;
  }

  pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
  if (!strcmp(pstViCfg->module, "NONE")) {
    memcpy(pstViCfg->module, module, strlen(module));
  } else {
    if (!strstr(pstViCfg->module, module)) {
      strcat(pstViCfg->module, "|");
      strcat(pstViCfg->module, module);
    }

    if (pstViCfg->buf_cnt < 4)
      pstViCfg->buf_cnt = 4;
  }
  RKADK_PARAM_SaveViCfg(RKADK_PARAM_PATH, index, s32CamID);

  RKADK_LOGI("Find %s[%d*%d] closest vi config[%d*%d]", module, width, height,
             pstViCfg->width, pstViCfg->height);
  return index;
}

static RKADK_S32 RKADK_PARAM_FindViIndex(RKADK_STREAM_TYPE_E enStrmType,
                                         RKADK_S32 s32CamID, RKADK_U32 width,
                                         RKADK_U32 height) {
  int index;
  char module[RKADK_BUFFER_LEN];
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg =
      &g_stPARAMCtx.stCfg.stSensorCfg[s32CamID];

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);
  memset(module, 0, RKADK_BUFFER_LEN);

  switch (enStrmType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    memcpy(module, "RECORD_MAIN", strlen("RECORD_MAIN"));
    index = RKADK_PARAM_GetViIndex(module, s32CamID, width, height);
    if (index >= 0 && index < RKADK_ISPP_VI_NODE_CNT)
      return index;

    RKADK_LOGI("Sensor[%d] rec[0][%d*%d] not find matched VI", s32CamID, width,
               height);

    if ((width == pstSensorCfg->max_width) &&
        (width == pstSensorCfg->max_height)) {
      RKADK_LOGI("rec[0] default VI[0]");
      index = 0;
    } else {
      RKADK_LOGI("rec[0] default VI[1]");
      index = 1;
    }
    break;

  case RKADK_STREAM_TYPE_SNAP: {
    memcpy(module, "PHOTO", strlen("PHOTO"));
    index = RKADK_PARAM_GetViIndex(module, s32CamID, width, height);
    if (index >= 0 && index < RKADK_ISPP_VI_NODE_CNT)
      return index;

    RKADK_PARAM_VENC_ATTR_S *pstRecAttr =
        &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stRecCfg.attribute[0];
    RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg =
        &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stPhotoCfg;

    RKADK_LOGI("Sensor[%d] photo[%d*%d] not find matched VI", s32CamID, width,
               height);
    RKADK_LOGI("Force photo resolution = rec[0] resolution[%d*%d]",
               pstRecAttr->width, pstRecAttr->height);

    width = pstRecAttr->width;
    height = pstRecAttr->height;
    pstPhotoCfg->image_width = width;
    pstPhotoCfg->image_height = height;
    RKADK_PARAM_SavePhotoCfg(RKADK_PARAM_PATH, s32CamID);

    if ((width == pstSensorCfg->max_width) &&
        (height == pstSensorCfg->max_height))
      index = 0;
    else
      index = 1;
    break;
  }

  case RKADK_STREAM_TYPE_VIDEO_SUB:
  case RKADK_STREAM_TYPE_PREVIEW:
  case RKADK_STREAM_TYPE_LIVE:
  case RKADK_STREAM_TYPE_DISP:
    if (enStrmType == RKADK_STREAM_TYPE_VIDEO_SUB)
      memcpy(module, "RECORD_SUB", strlen("RECORD_SUB"));
    else if (enStrmType == RKADK_STREAM_TYPE_PREVIEW)
      memcpy(module, "PREVIEW", strlen("PREVIEW"));
    else if (enStrmType == RKADK_STREAM_TYPE_LIVE)
      memcpy(module, "LIVE", strlen("LIVE"));
    else
      memcpy(module, "DISP", strlen("DISP"));

    index = RKADK_PARAM_GetViIndex(module, s32CamID, width, height);
    if (index >= 0 && index < RKADK_ISPP_VI_NODE_CNT)
      return index;

    RKADK_LOGI("Sensor[%d] %s[%d*%d] not find matched VI", s32CamID, module,
               width, height);

    index = RKADK_PARAM_FindUsableVi(s32CamID, width, pstSensorCfg->max_width);
    if (index >= 0 && index < RKADK_ISPP_VI_NODE_CNT) {
      RKADK_LOGI("Sensor[%d] %s[%d*%d] find null VI[%d]", s32CamID, module,
                 width, height, index);
      break;
    }

    if (enStrmType == RKADK_STREAM_TYPE_DISP) {
      // Find the stViCfg with the closest resolution
      index = RKADK_PARAM_FindClosestVi(s32CamID, module, width, height);
      return index;
    }

    index = 3;
    pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
    RKADK_LOGI("Force sensor[%d] %s[%d*%d] to VI[3][%d*%d]", s32CamID, module,
               width, height, pstViCfg->width, pstViCfg->height);

    width = pstViCfg->width;
    height = pstViCfg->height;

    RKADK_PARAM_VENC_ATTR_S *pstAttrCfg;
    if (enStrmType == RKADK_STREAM_TYPE_VIDEO_SUB)
      pstAttrCfg =
          &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stRecCfg.attribute[1];
    else if (enStrmType == RKADK_STREAM_TYPE_PREVIEW)
      pstAttrCfg =
          &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stStreamCfg.attribute;
    else
      pstAttrCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stLiveCfg.attribute;

    pstAttrCfg->width = width;
    pstAttrCfg->height = height;

    if (enStrmType == RKADK_STREAM_TYPE_VIDEO_SUB)
      RKADK_PARAM_SaveRecAttr(RKADK_PARAM_PATH, s32CamID, enStrmType);
    else
      RKADK_PARAM_SaveStreamCfg(RKADK_PARAM_PATH, s32CamID, enStrmType);
    break;

  default:
    RKADK_LOGE("Invaild mode = %d", enStrmType);
    return -1;
  }

  pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
  pstViCfg->width = width;
  pstViCfg->height = height;

  if (pstViCfg->buf_cnt < 4)
    pstViCfg->buf_cnt = 4;

  if (!strcmp(pstViCfg->module, "NONE")) {
    memcpy(pstViCfg->module, module, strlen(module));
  } else {
    if (!strstr(pstViCfg->module, module)) {
      strcat(pstViCfg->module, "|");
      strcat(pstViCfg->module, module);
    }
  }

  if (!strcmp(pstViCfg->device_name, "rkispp_scale0")) {
    if ((pstViCfg->width != pstSensorCfg->max_width) &&
        (pstViCfg->width > RKISPP_SCALE0_NV12_WIDTH_MAX)) {
      if (strcmp(pstViCfg->pix_fmt, "NV16")) {
        RKADK_LOGW("rkispp_scale0 resolution[%d*%d] > 2K, default NV16",
                   pstViCfg->width, pstViCfg->height);
        memcpy(pstViCfg->pix_fmt, "NV16", strlen("NV16"));
      }
    }
  }

  RKADK_PARAM_SaveViCfg(RKADK_PARAM_PATH, index, s32CamID);
  return index;
}

IMAGE_TYPE_E RKADK_PARAM_GetPixFmt(char *pixFmt) {
  IMAGE_TYPE_E enPixFmt = IMAGE_TYPE_UNKNOW;

  RKADK_CHECK_POINTER(pixFmt, IMAGE_TYPE_UNKNOW);

  if (!strcmp(pixFmt, "NV12"))
    enPixFmt = IMAGE_TYPE_NV12;
  else if (!strcmp(pixFmt, "NV16"))
    enPixFmt = IMAGE_TYPE_NV16;
  else if (!strcmp(pixFmt, "YUYV422"))
    enPixFmt = IMAGE_TYPE_YUYV422;
  else if (!strcmp(pixFmt, "FBC0"))
    enPixFmt = IMAGE_TYPE_FBC0;
  else if (!strcmp(pixFmt, "FBC2"))
    enPixFmt = IMAGE_TYPE_FBC2;
  else if (!strcmp(pixFmt, "RGB565"))
    enPixFmt = IMAGE_TYPE_RGB565;
  else if (!strcmp(pixFmt, "RGB888"))
    enPixFmt = IMAGE_TYPE_RGB888;
  else if (!strcmp(pixFmt, "GRAY8"))
    enPixFmt = IMAGE_TYPE_GRAY8;
  else if (!strcmp(pixFmt, "GRAY16"))
    enPixFmt = IMAGE_TYPE_GRAY16;
  else if (!strcmp(pixFmt, "YUV420P"))
    enPixFmt = IMAGE_TYPE_YUV420P;
  else if (!strcmp(pixFmt, "NV21"))
    enPixFmt = IMAGE_TYPE_NV21;
  else if (!strcmp(pixFmt, "YV12"))
    enPixFmt = IMAGE_TYPE_YV12;
  else if (!strcmp(pixFmt, "YUV422P"))
    enPixFmt = IMAGE_TYPE_YUV422P;
  else if (!strcmp(pixFmt, "NV61"))
    enPixFmt = IMAGE_TYPE_NV61;
  else if (!strcmp(pixFmt, "YV16"))
    enPixFmt = IMAGE_TYPE_YV16;
  else if (!strcmp(pixFmt, "UYVY422"))
    enPixFmt = IMAGE_TYPE_UYVY422;
  else if (!strcmp(pixFmt, "YUV444SP"))
    enPixFmt = IMAGE_TYPE_YUV444SP;
  else if (!strcmp(pixFmt, "RGB332"))
    enPixFmt = IMAGE_TYPE_RGB332;
  else if (!strcmp(pixFmt, "BGR565"))
    enPixFmt = IMAGE_TYPE_BGR565;
  else if (!strcmp(pixFmt, "BGR888"))
    enPixFmt = IMAGE_TYPE_BGR888;
  else if (!strcmp(pixFmt, "ARGB8888"))
    enPixFmt = IMAGE_TYPE_ARGB8888;
  else if (!strcmp(pixFmt, "ABGR8888"))
    enPixFmt = IMAGE_TYPE_ABGR8888;
  else if (!strcmp(pixFmt, "RGBA8888"))
    enPixFmt = IMAGE_TYPE_RGBA8888;
  else if (!strcmp(pixFmt, "BGRA8888"))
    enPixFmt = IMAGE_TYPE_BGRA8888;
  else
    RKADK_LOGE("Invalid image type: %s", pixFmt);

  return enPixFmt;
}

static RKADK_S32 RKADK_PARAM_SetStreamViAttr(RKADK_S32 s32CamID,
                                             RKADK_STREAM_TYPE_E enStrmType) {
  int index;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);

  if (enStrmType == RKADK_STREAM_TYPE_PREVIEW)
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stStreamCfg;
  else
    pstStreamCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stLiveCfg;

  index = RKADK_PARAM_FindViIndex(enStrmType, s32CamID,
                                  pstStreamCfg->attribute.width,
                                  pstStreamCfg->attribute.height);
  if (index < 0 || index >= RKADK_ISPP_VI_NODE_CNT) {
    RKADK_LOGE("not find match vi index");
    return -1;
  }

  pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
  pstStreamCfg->vi_attr.u32ViChn = pstViCfg->chn_id;
  pstStreamCfg->vi_attr.stChnAttr.pcVideoNode = pstViCfg->device_name;
  pstStreamCfg->vi_attr.stChnAttr.u32Width = pstViCfg->width;
  pstStreamCfg->vi_attr.stChnAttr.u32Height = pstViCfg->height;
  pstStreamCfg->vi_attr.stChnAttr.u32BufCnt = pstViCfg->buf_cnt;
  pstStreamCfg->vi_attr.stChnAttr.enPixFmt =
      RKADK_PARAM_GetPixFmt(pstViCfg->pix_fmt);
  pstStreamCfg->vi_attr.stChnAttr.enBufType = VI_CHN_BUF_TYPE_MMAP;
  pstStreamCfg->vi_attr.stChnAttr.enWorkMode = VI_WORK_MODE_NORMAL;

  return 0;
}

static RKADK_S32 RKADK_PARAM_SetPhotoViAttr(RKADK_S32 s32CamID) {
  int index;
  RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stPhotoCfg;
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);

  index = RKADK_PARAM_FindViIndex(RKADK_STREAM_TYPE_SNAP, s32CamID,
                                  pstPhotoCfg->image_width,
                                  pstPhotoCfg->image_height);
  if (index < 0 || index >= RKADK_ISPP_VI_NODE_CNT) {
    RKADK_LOGE("not find match vi index");
    return -1;
  }

  pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
  pstPhotoCfg->vi_attr.u32ViChn = pstViCfg->chn_id;
  pstPhotoCfg->vi_attr.stChnAttr.pcVideoNode = pstViCfg->device_name;
  pstPhotoCfg->vi_attr.stChnAttr.u32Width = pstViCfg->width;
  pstPhotoCfg->vi_attr.stChnAttr.u32Height = pstViCfg->height;
  pstPhotoCfg->vi_attr.stChnAttr.u32BufCnt = pstViCfg->buf_cnt;
  pstPhotoCfg->vi_attr.stChnAttr.enPixFmt =
      RKADK_PARAM_GetPixFmt(pstViCfg->pix_fmt);
  pstPhotoCfg->vi_attr.stChnAttr.enBufType = VI_CHN_BUF_TYPE_MMAP;
  pstPhotoCfg->vi_attr.stChnAttr.enWorkMode = VI_WORK_MODE_NORMAL;

  return 0;
}

static RKADK_S32 RKADK_PARAM_SetRecViAttr(RKADK_S32 s32CamID) {
  int i, index;
  RKADK_STREAM_TYPE_E enStrmType = RKADK_STREAM_TYPE_BUTT;
  RKADK_PARAM_REC_CFG_S *pstRecCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stRecCfg;
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);

  for (i = 0; i < (int)pstRecCfg->file_num; i++) {
    if (i == 0)
      enStrmType = RKADK_STREAM_TYPE_VIDEO_MAIN;
    else
      enStrmType = RKADK_STREAM_TYPE_VIDEO_SUB;

    index = RKADK_PARAM_FindViIndex(enStrmType, s32CamID,
                                    pstRecCfg->attribute[i].width,
                                    pstRecCfg->attribute[i].height);
    if (index < 0 || index >= RKADK_ISPP_VI_NODE_CNT) {
      RKADK_LOGE("not find match vi index");
      return -1;
    }

    pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
    pstRecCfg->vi_attr[i].u32ViChn = pstViCfg->chn_id;
    pstRecCfg->vi_attr[i].stChnAttr.pcVideoNode = pstViCfg->device_name;
    pstRecCfg->vi_attr[i].stChnAttr.u32Width = pstViCfg->width;
    pstRecCfg->vi_attr[i].stChnAttr.u32Height = pstViCfg->height;
    pstRecCfg->vi_attr[i].stChnAttr.u32BufCnt = pstViCfg->buf_cnt;
    pstRecCfg->vi_attr[i].stChnAttr.enPixFmt =
        RKADK_PARAM_GetPixFmt(pstViCfg->pix_fmt);
    pstRecCfg->vi_attr[i].stChnAttr.enBufType = VI_CHN_BUF_TYPE_MMAP;
    pstRecCfg->vi_attr[i].stChnAttr.enWorkMode = VI_WORK_MODE_NORMAL;
  }

  return 0;
}

static RKADK_S32 RKADK_PARAM_SetDispViAttr(RKADK_S32 s32CamID) {
  int index;
  bool bSensorHorizontal = false, bDispHorizontal = false;
  RKADK_U32 width, height;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg =
      &g_stPARAMCtx.stCfg.stSensorCfg[s32CamID];
  RKADK_PARAM_DISP_CFG_S *pstDispCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stDispCfg;
  RKADK_PARAM_VI_CFG_S *pstViCfg = NULL;

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);

  if (pstSensorCfg->max_width > pstSensorCfg->max_height)
    bSensorHorizontal = true;

  if (pstDispCfg->width > pstDispCfg->height)
    bDispHorizontal = true;

  if ((bSensorHorizontal && bDispHorizontal) ||
      (!bSensorHorizontal && !bDispHorizontal)) {
    width = pstDispCfg->width;
    height = pstDispCfg->height;
  } else {
    width = pstDispCfg->height;
    height = pstDispCfg->width;
  }

  index =
      RKADK_PARAM_FindViIndex(RKADK_STREAM_TYPE_DISP, s32CamID, width, height);
  if (index < 0 || index >= RKADK_ISPP_VI_NODE_CNT) {
    RKADK_LOGE("not find match vi index");
    return -1;
  }

  pstViCfg = &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stViCfg[index];
  pstDispCfg->vi_attr.u32ViChn = pstViCfg->chn_id;
  pstDispCfg->vi_attr.stChnAttr.pcVideoNode = pstViCfg->device_name;
  pstDispCfg->vi_attr.stChnAttr.u32Width = pstViCfg->width;
  pstDispCfg->vi_attr.stChnAttr.u32Height = pstViCfg->height;
  pstDispCfg->vi_attr.stChnAttr.u32BufCnt = pstViCfg->buf_cnt;
  pstDispCfg->vi_attr.stChnAttr.enPixFmt =
      RKADK_PARAM_GetPixFmt(pstViCfg->pix_fmt);
  pstDispCfg->vi_attr.stChnAttr.enBufType = VI_CHN_BUF_TYPE_MMAP;
  pstDispCfg->vi_attr.stChnAttr.enWorkMode = VI_WORK_MODE_NORMAL;

  return 0;
}

static RKADK_S32 RKADK_PARAM_SetMediaViAttr() {
  int i, ret = 0;

  for (i = 0; i < (int)g_stPARAMCtx.stCfg.stCommCfg.sensor_count; i++) {
    // Must be called before setRecattr
    ret = RKADK_PARAM_SetStreamViAttr(i, RKADK_STREAM_TYPE_PREVIEW);
    if (ret)
      break;

    ret = RKADK_PARAM_SetStreamViAttr(i, RKADK_STREAM_TYPE_LIVE);
    if (ret)
      break;

    // Must be called before SetPhotoAttr
    ret = RKADK_PARAM_SetRecViAttr(i);
    if (ret)
      break;

    ret = RKADK_PARAM_SetPhotoViAttr(i);
    if (ret)
      break;

    ret = RKADK_PARAM_SetDispViAttr(i);
    if (ret)
      break;
  }

#ifdef RKADK_DUMP_CONFIG
  RKADK_PARAM_DumpViAttr();
#endif

  return ret;
}

static void RKADK_PARAM_SetMicVolume(RKADK_U32 volume) {
  char buffer[RKADK_BUFFER_LEN];

  memset(buffer, 0, RKADK_BUFFER_LEN);
  sprintf(buffer, "amixer sset MasterC %d%%", volume);
  system(buffer);
}

static void RKADK_PARAM_SetSpeakerVolume(RKADK_U32 volume) {
  char buffer[RKADK_BUFFER_LEN];

  memset(buffer, 0, RKADK_BUFFER_LEN);
  sprintf(buffer, "amixer sset MasterP %d%%", volume);
  system(buffer);
}

static void RKADK_PARAM_MicMute(bool mute) {
  char buffer[RKADK_BUFFER_LEN];

  memset(buffer, 0, RKADK_BUFFER_LEN);
  if (mute)
    sprintf(buffer, "amixer sset 'ADC SDP MUTE' on");
  else
    sprintf(buffer, "amixer sset 'ADC SDP MUTE' off");

  system(buffer);
}

static void RKADK_PARAM_SetVolume() {
  RKADK_PARAM_COMM_CFG_S *pstCommCfg = &g_stPARAMCtx.stCfg.stCommCfg;

  RKADK_PARAM_MicMute(!pstCommCfg->mic_unmute);
  RKADK_PARAM_SetMicVolume(pstCommCfg->mic_volume);

  if (!pstCommCfg->enable_speaker)
    RKADK_PARAM_SetSpeakerVolume(0);
  else
    RKADK_PARAM_SetSpeakerVolume(pstCommCfg->speaker_volume);
}

VENC_RC_MODE_E RKADK_PARAM_GetRcMode(char *rcMode,
                                     RKADK_CODEC_TYPE_E enCodecType) {
  VENC_RC_MODE_E enRcMode = VENC_RC_MODE_BUTT;

  RKADK_CHECK_POINTER(rcMode, VENC_RC_MODE_BUTT);

  switch (enCodecType) {
  case RKADK_CODEC_TYPE_H264:
    if (!strcmp(rcMode, "CBR"))
      enRcMode = VENC_RC_MODE_H264CBR;
    else if (!strcmp(rcMode, "VBR"))
      enRcMode = VENC_RC_MODE_H264VBR;
    else if (!strcmp(rcMode, "AVBR"))
      enRcMode = VENC_RC_MODE_H264AVBR;
    break;
  case RKADK_CODEC_TYPE_H265:
    if (!strcmp(rcMode, "CBR"))
      enRcMode = VENC_RC_MODE_H265CBR;
    else if (!strcmp(rcMode, "VBR"))
      enRcMode = VENC_RC_MODE_H265VBR;
    else if (!strcmp(rcMode, "AVBR"))
      enRcMode = VENC_RC_MODE_H265AVBR;
    break;
  case RKADK_CODEC_TYPE_MJPEG:
    if (!strcmp(rcMode, "CBR"))
      enRcMode = VENC_RC_MODE_MJPEGCBR;
    else if (!strcmp(rcMode, "VBR"))
      enRcMode = VENC_RC_MODE_MJPEGVBR;
    break;
  default:
    RKADK_LOGE("Nonsupport codec type: %d", enCodecType);
    break;
  }

  return enRcMode;
}

static void RKADK_PARAM_Strtok(char *input, RKADK_S32 *s32Output,
                               RKADK_S32 u32OutputLen, const char *delim) {
  char *p;

  for (int i = 0; i < u32OutputLen; i++) {
    if (!i)
      p = strtok(input, delim);
    else
      p = strtok(NULL, delim);

    if (!p)
      break;

    s32Output[i] = atoi(p);
  }
}

RKADK_S32 RKADK_PARAM_GetRcParam(RKADK_PARAM_VENC_ATTR_S stVencAttr,
                                 VENC_RC_PARAM_S *pstRcParam) {
  RKADK_S32 s32FirstFrameStartQp;
  RKADK_U32 u32RowQpDeltaI;
  RKADK_U32 u32RowQpDeltaP;
  RKADK_U32 u32StepQp;

  RKADK_CHECK_POINTER(pstRcParam, RKADK_FAILURE);
  memset(pstRcParam, 0, sizeof(VENC_RC_PARAM_S));

  s32FirstFrameStartQp = stVencAttr.venc_param.first_frame_qp > 0
                             ? stVencAttr.venc_param.first_frame_qp
                             : -1;

  u32StepQp =
      stVencAttr.venc_param.qp_step > 0 ? stVencAttr.venc_param.qp_step : 2;

  u32RowQpDeltaI = stVencAttr.venc_param.row_qp_delta_i > 0
                       ? stVencAttr.venc_param.row_qp_delta_i
                       : 1;

  u32RowQpDeltaP = stVencAttr.venc_param.row_qp_delta_p > 0
                       ? stVencAttr.venc_param.row_qp_delta_p
                       : 2;

  pstRcParam->s32FirstFrameStartQp = s32FirstFrameStartQp;
  pstRcParam->u32RowQpDeltaI = u32RowQpDeltaI;
  pstRcParam->u32RowQpDeltaP = u32RowQpDeltaP;
  pstRcParam->bEnableHierQp = (RK_BOOL)stVencAttr.venc_param.hier_qp_en;
  RKADK_PARAM_Strtok(stVencAttr.venc_param.hier_qp_delta,
                     pstRcParam->s32HierQpDelta, RC_HEIR_SIZE, ",");
  RKADK_PARAM_Strtok(stVencAttr.venc_param.hier_frame_num,
                     pstRcParam->s32HierFrameNum, RC_HEIR_SIZE, ",");

  switch (stVencAttr.codec_type) {
  case RKADK_CODEC_TYPE_H264:
    pstRcParam->stParamH264.u32StepQp = u32StepQp;
    pstRcParam->stParamH264.u32MaxQp = stVencAttr.venc_param.max_qp;
    pstRcParam->stParamH264.u32MinQp = stVencAttr.venc_param.min_qp;
    pstRcParam->stParamH264.u32MaxIQp = stVencAttr.venc_param.max_qp;
    pstRcParam->stParamH264.u32MinIQp = stVencAttr.venc_param.min_qp;
    break;
  case RKADK_CODEC_TYPE_H265:
    pstRcParam->stParamH265.u32StepQp = u32StepQp;
    pstRcParam->stParamH265.u32MaxQp = stVencAttr.venc_param.max_qp;
    pstRcParam->stParamH265.u32MinQp = stVencAttr.venc_param.min_qp;
    pstRcParam->stParamH265.u32MaxIQp = stVencAttr.venc_param.max_qp;
    pstRcParam->stParamH265.u32MinIQp = stVencAttr.venc_param.min_qp;
    break;
  case RKADK_CODEC_TYPE_MJPEG:
    break;
  default:
    RKADK_LOGE("Nonsupport codec type: %d", stVencAttr.codec_type);
    return -1;
  }

  return 0;
}

RKADK_PARAM_CONTEXT_S *RKADK_PARAM_GetCtx() {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  return &g_stPARAMCtx;
}

RKADK_PARAM_COMM_CFG_S *RKADK_PARAM_GetCommCfg() {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  return &g_stPARAMCtx.stCfg.stCommCfg;
}

RKADK_PARAM_REC_CFG_S *RKADK_PARAM_GetRecCfg(RKADK_U32 u32CamId) {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  RKADK_CHECK_CAMERAID(u32CamId, NULL);
  return &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stRecCfg;
}

RKADK_PARAM_SENSOR_CFG_S *RKADK_PARAM_GetSensorCfg(RKADK_U32 u32CamId) {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  RKADK_CHECK_CAMERAID(u32CamId, NULL);
  return &g_stPARAMCtx.stCfg.stSensorCfg[u32CamId];
}

RKADK_PARAM_STREAM_CFG_S *
RKADK_PARAM_GetStreamCfg(RKADK_U32 u32CamId, RKADK_STREAM_TYPE_E enStrmType) {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  RKADK_CHECK_CAMERAID(u32CamId, NULL);

  if (enStrmType == RKADK_STREAM_TYPE_PREVIEW)
    return &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stStreamCfg;
  else
    return &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stLiveCfg;
}

RKADK_PARAM_PHOTO_CFG_S *RKADK_PARAM_GetPhotoCfg(RKADK_U32 u32CamId) {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  RKADK_CHECK_CAMERAID(u32CamId, NULL);
  return &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stPhotoCfg;
}

RKADK_PARAM_DISP_CFG_S *RKADK_PARAM_GetDispCfg(RKADK_U32 u32CamId) {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  RKADK_CHECK_CAMERAID(u32CamId, NULL);
  return &g_stPARAMCtx.stCfg.stMediaCfg[u32CamId].stDispCfg;
}

RKADK_PARAM_AUDIO_CFG_S *RKADK_PARAM_GetAudioCfg() {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  return &g_stPARAMCtx.stCfg.stAudioCfg;
}

RKADK_PARAM_THUMB_CFG_S *RKADK_PARAM_GetThumbCfg(RKADK_VOID) {
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, NULL);
  return &g_stPARAMCtx.stCfg.stThumbCfg;
}

RKADK_S32 RKADK_PARAM_GetVencChnId(RKADK_U32 u32CamId,
                                   RKADK_STREAM_TYPE_E enStrmType) {
  RKADK_S32 s32VencChnId = -1;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);

  switch (enStrmType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN: {
    RKADK_PARAM_REC_CFG_S *pstRecCfg = RKADK_PARAM_GetRecCfg(u32CamId);
    if (!pstRecCfg)
      RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    else
      s32VencChnId = pstRecCfg->attribute[0].venc_chn;
    break;
  }
  case RKADK_STREAM_TYPE_VIDEO_SUB: {
    RKADK_PARAM_REC_CFG_S *pstRecCfg = RKADK_PARAM_GetRecCfg(u32CamId);
    if (!pstRecCfg)
      RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    else
      s32VencChnId = pstRecCfg->attribute[1].venc_chn;
    break;
  }
  case RKADK_STREAM_TYPE_SNAP: {
    RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg = RKADK_PARAM_GetPhotoCfg(u32CamId);
    if (!pstPhotoCfg)
      RKADK_LOGE("RKADK_PARAM_GetPhotoCfg failed");
    else
      s32VencChnId = pstPhotoCfg->venc_chn;
    break;
  }
  case RKADK_STREAM_TYPE_PREVIEW:
  case RKADK_STREAM_TYPE_LIVE: {
    RKADK_PARAM_STREAM_CFG_S *pstStreamCfg =
        RKADK_PARAM_GetStreamCfg(u32CamId, enStrmType);
    if (!pstStreamCfg)
      RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    else
      s32VencChnId = pstStreamCfg->attribute.venc_chn;
    break;
  }
  default:
    RKADK_LOGE("Unsupport stream type: %d", enStrmType);
    break;
  }

  return s32VencChnId;
}

RKADK_PARAM_RES_E RKADK_PARAM_GetResType(RKADK_U32 width, RKADK_U32 height) {
  RKADK_PARAM_RES_E type = RKADK_RES_BUTT;

  if (width == RKADK_WIDTH_720P && height == RKADK_HEIGHT_720P)
    type = RKADK_RES_720P;
  else if (width == RKADK_WIDTH_1080P && height == RKADK_HEIGHT_1080P)
    type = RKADK_RES_1080P;
  else if (width == RKADK_WIDTH_1296P && height == RKADK_HEIGHT_1296P)
    type = RKADK_RES_1296P;
  else if (width == RKADK_WIDTH_1440P && height == RKADK_HEIGHT_1440P)
    type = RKADK_RES_1440P;
  else if (width == RKADK_WIDTH_1520P && height == RKADK_HEIGHT_1520P)
    type = RKADK_RES_1520P;
  else if (width == RKADK_WIDTH_1600P && height == RKADK_HEIGHT_1600P)
    type = RKADK_RES_1600P;
  else if (width == RKADK_WIDTH_1620P && height == RKADK_HEIGHT_1620P)
    type = RKADK_RES_1620P;
  else if (width == RKADK_WIDTH_1944P && height == RKADK_HEIGHT_1944P)
    type = RKADK_RES_1944P;
  else if (width == RKADK_WIDTH_3840P && height == RKADK_HEIGHT_2160P)
    type = RKADK_RES_2160P;
  else
    RKADK_LOGE("Unsupport resolution(%d*%d)", width, height);

  return type;
}

RKADK_S32 RKADK_PARAM_GetResolution(RKADK_PARAM_RES_E type, RKADK_U32 *width,
                                    RKADK_U32 *height) {
  switch (type) {
  case RKADK_RES_720P:
    *width = RKADK_WIDTH_720P;
    *height = RKADK_HEIGHT_720P;
    break;
  case RKADK_RES_1080P:
    *width = RKADK_WIDTH_1080P;
    *height = RKADK_HEIGHT_1080P;
    break;
  case RKADK_RES_1296P:
    *width = RKADK_WIDTH_1296P;
    *height = RKADK_HEIGHT_1296P;
    break;
  case RKADK_RES_1440P:
    *width = RKADK_WIDTH_1440P;
    *height = RKADK_HEIGHT_1440P;
    break;
  case RKADK_RES_1520P:
    *width = RKADK_WIDTH_1520P;
    *height = RKADK_HEIGHT_1520P;
    break;
  case RKADK_RES_1600P:
    *width = RKADK_WIDTH_1600P;
    *height = RKADK_HEIGHT_1600P;
    break;
  case RKADK_RES_1620P:
    *width = RKADK_WIDTH_1620P;
    *height = RKADK_HEIGHT_1620P;
    break;
  case RKADK_RES_1944P:
    *width = RKADK_WIDTH_1944P;
    *height = RKADK_HEIGHT_1944P;
    break;
  case RKADK_RES_2160P:
    *width = RKADK_WIDTH_3840P;
    *height = RKADK_HEIGHT_2160P;
    break;
  default:
    RKADK_LOGE("Unsupport resolution type: %d, set to 1080P", type);
    *width = RKADK_WIDTH_1080P;
    *height = RKADK_HEIGHT_1080P;
    break;
  }

  return 0;
}

static RKADK_CODEC_TYPE_E
RKADK_PARAM_GetCodecType(RKADK_S32 s32CamId, RKADK_STREAM_TYPE_E enStreamType) {
  RKADK_CODEC_TYPE_E enCodecType = RKADK_CODEC_TYPE_BUTT;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;

  switch (enStreamType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    enCodecType = pstRecCfg->attribute[0].codec_type;
    break;

  case RKADK_STREAM_TYPE_VIDEO_SUB:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    enCodecType = pstRecCfg->attribute[1].codec_type;
    break;

  case RKADK_STREAM_TYPE_PREVIEW:
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg = RKADK_PARAM_GetStreamCfg(s32CamId, enStreamType);
    enCodecType = pstStreamCfg->attribute.codec_type;
    break;
  default:
    RKADK_LOGE("Unsupport enStreamType: %d", enStreamType);
    break;
  }

  return enCodecType;
}

static RKADK_S32
RKADK_PARAM_SetCodecType(RKADK_S32 s32CamId,
                         RKADK_PARAM_CODEC_CFG_S *pstCodecCfg) {
  RKADK_S32 ret;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;

  switch (pstCodecCfg->enStreamType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    if (pstRecCfg->attribute[0].codec_type == pstCodecCfg->enCodecType)
      return 0;

    pstRecCfg->attribute[0].codec_type = pstCodecCfg->enCodecType;
    ret = RKADK_PARAM_SaveRecAttr(RKADK_PARAM_PATH, s32CamId,
                                  RKADK_STREAM_TYPE_VIDEO_MAIN);
    break;

  case RKADK_STREAM_TYPE_VIDEO_SUB:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    if (pstRecCfg->attribute[1].codec_type == pstCodecCfg->enCodecType)
      return 0;

    pstRecCfg->attribute[1].codec_type = pstCodecCfg->enCodecType;
    ret = RKADK_PARAM_SaveRecAttr(RKADK_PARAM_PATH, s32CamId,
                                  RKADK_STREAM_TYPE_VIDEO_SUB);
    break;

  case RKADK_STREAM_TYPE_PREVIEW:
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg =
        RKADK_PARAM_GetStreamCfg(s32CamId, pstCodecCfg->enStreamType);
    if (pstStreamCfg->attribute.codec_type == pstCodecCfg->enCodecType)
      return 0;

    pstStreamCfg->attribute.codec_type = pstCodecCfg->enCodecType;
    ret = RKADK_PARAM_SaveStreamCfg(RKADK_PARAM_PATH, s32CamId,
                                    pstCodecCfg->enStreamType);
    break;

  default:
    RKADK_LOGE("Unsupport enStreamType: %d", pstCodecCfg->enStreamType);
    break;
  }

  return ret;
}

static RKADK_U32 RKADK_PARAM_GetBitrate(RKADK_S32 s32CamId,
                                        RKADK_STREAM_TYPE_E enStreamType) {
  RKADK_U32 bitrate = 0;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;

  switch (enStreamType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    bitrate = pstRecCfg->attribute[0].bitrate;
    break;

  case RKADK_STREAM_TYPE_VIDEO_SUB:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    bitrate = pstRecCfg->attribute[1].bitrate;
    break;

  case RKADK_STREAM_TYPE_PREVIEW:
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg = RKADK_PARAM_GetStreamCfg(s32CamId, enStreamType);
    bitrate = pstStreamCfg->attribute.bitrate;
    break;
  default:
    RKADK_LOGE("Unsupport enStreamType: %d", enStreamType);
    break;
  }

  return bitrate;
}

static RKADK_S32 RKADK_PARAM_SetBitrate(RKADK_S32 s32CamId,
                                        RKADK_PARAM_BITRATE_S *pstBitrate) {
  RKADK_S32 ret = -1;
  RKADK_PARAM_REC_CFG_S *pstRecCfg;
  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg;

  switch (pstBitrate->enStreamType) {
  case RKADK_STREAM_TYPE_VIDEO_MAIN:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    if (pstRecCfg->attribute[0].bitrate == pstBitrate->u32Bitrate)
      return 0;

    pstRecCfg->attribute[0].bitrate = pstBitrate->u32Bitrate;
    ret = RKADK_PARAM_SaveRecAttr(RKADK_PARAM_PATH, s32CamId,
                                  RKADK_STREAM_TYPE_VIDEO_MAIN);
    break;

  case RKADK_STREAM_TYPE_VIDEO_SUB:
    pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamId);
    if (pstRecCfg->attribute[1].bitrate == pstBitrate->u32Bitrate)
      return 0;

    pstRecCfg->attribute[1].bitrate = pstBitrate->u32Bitrate;
    ret = RKADK_PARAM_SaveRecAttr(RKADK_PARAM_PATH, s32CamId,
                                  RKADK_STREAM_TYPE_VIDEO_SUB);
    break;

  case RKADK_STREAM_TYPE_PREVIEW:
  case RKADK_STREAM_TYPE_LIVE:
    pstStreamCfg = RKADK_PARAM_GetStreamCfg(s32CamId, pstBitrate->enStreamType);
    if (pstStreamCfg->attribute.bitrate == pstBitrate->u32Bitrate)
      return 0;

    pstStreamCfg->attribute.bitrate = pstBitrate->u32Bitrate;
    ret = RKADK_PARAM_SaveStreamCfg(RKADK_PARAM_PATH, s32CamId,
                                    pstBitrate->enStreamType);
    break;

  default:
    RKADK_LOGE("Unsupport enStreamType: %d", pstBitrate->enStreamType);
    break;
  }

  return ret;
}

static RKADK_U32 RKADK_PARAM_GetRecTime(RKADK_S32 s32CamId,
                                        RKADK_STREAM_TYPE_E enStreamType,
                                        RKADK_PARAM_TYPE_E enParamType) {
  RKADK_U32 time = -1;
  RKADK_PARAM_REC_TIME_CFG_S *pstRecTimeCfg;

  if (enStreamType == RKADK_STREAM_TYPE_VIDEO_MAIN)
    pstRecTimeCfg =
        &g_stPARAMCtx.stCfg.stMediaCfg[s32CamId].stRecCfg.record_time_cfg[0];
  else
    pstRecTimeCfg =
        &g_stPARAMCtx.stCfg.stMediaCfg[s32CamId].stRecCfg.record_time_cfg[1];

  switch (enParamType) {
  case RKADK_PARAM_TYPE_RECORD_TIME:
    time = pstRecTimeCfg->record_time;
    break;
  case RKADK_PARAM_TYPE_SPLITTIME:
    time = pstRecTimeCfg->splite_time;
    break;
  case RKADK_PARAM_TYPE_LAPSE_INTERVAL:
    time = pstRecTimeCfg->lapse_interval;
    break;
  default:
    RKADK_LOGE("Invalid enParamType: %d", enParamType);
    break;
  }

  return time;
}

static RKADK_S32 RKADK_PARAM_SetRecTime(RKADK_S32 s32CamId,
                                        RKADK_PARAM_REC_TIME_S *pstRecTime,
                                        RKADK_PARAM_TYPE_E enParamType) {
  RKADK_S32 ret;
  RKADK_PARAM_REC_TIME_CFG_S *pstRecTimeCfg;

  if (pstRecTime->enStreamType == RKADK_STREAM_TYPE_VIDEO_MAIN)
    pstRecTimeCfg =
        &g_stPARAMCtx.stCfg.stMediaCfg[s32CamId].stRecCfg.record_time_cfg[0];
  else
    pstRecTimeCfg =
        &g_stPARAMCtx.stCfg.stMediaCfg[s32CamId].stRecCfg.record_time_cfg[1];

  switch (enParamType) {
  case RKADK_PARAM_TYPE_RECORD_TIME:
    if (pstRecTimeCfg->record_time == pstRecTime->time)
      return 0;

    pstRecTimeCfg->record_time = pstRecTime->time;
    break;
  case RKADK_PARAM_TYPE_SPLITTIME:
    if (pstRecTimeCfg->splite_time == pstRecTime->time)
      return 0;

    pstRecTimeCfg->splite_time = pstRecTime->time;
    break;
  case RKADK_PARAM_TYPE_LAPSE_INTERVAL:
    if (pstRecTimeCfg->lapse_interval == pstRecTime->time)
      return 0;

    pstRecTimeCfg->lapse_interval = pstRecTime->time;
    break;
  default:
    RKADK_LOGE("Invalid enParamType: %d", enParamType);
    return -1;
  }

  ret = RKADK_PARAM_SaveRecTime(RKADK_PARAM_PATH, s32CamId,
                                pstRecTime->enStreamType);
  return ret;
}

RKADK_S32 RKADK_PARAM_GetCamParam(RKADK_S32 s32CamID,
                                  RKADK_PARAM_TYPE_E enParamType,
                                  RKADK_VOID *pvParam) {
  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pvParam, RKADK_FAILURE);

  // RKADK_LOGD("s32CamID: %d, enParamType: %d, u32_pvParam: %d, b_pvParam: %d",
  // s32CamID, enParamType, *(RKADK_U32 *)pvParam, *(bool *)pvParam);

  RKADK_MUTEX_LOCK(g_stPARAMCtx.mutexLock);

  RKADK_PARAM_REC_CFG_S *pstRecCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stRecCfg;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg =
      &g_stPARAMCtx.stCfg.stSensorCfg[s32CamID];
  RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stPhotoCfg;

  switch (enParamType) {
  case RKADK_PARAM_TYPE_FPS:
    *(RKADK_U32 *)pvParam = pstSensorCfg->framerate;
    break;
  case RKADK_PARAM_TYPE_FLIP:
    *(bool *)pvParam = pstSensorCfg->flip;
    break;
  case RKADK_PARAM_TYPE_MIRROR:
    *(bool *)pvParam = pstSensorCfg->mirror;
    break;
  case RKADK_PARAM_TYPE_LDC:
    *(RKADK_U32 *)pvParam = pstSensorCfg->ldc;
    break;
  case RKADK_PARAM_TYPE_ANTIFOG:
    *(RKADK_U32 *)pvParam = pstSensorCfg->antifog;
    break;
  case RKADK_PARAM_TYPE_WDR:
    *(RKADK_U32 *)pvParam = pstSensorCfg->wdr;
    break;
  case RKADK_PARAM_TYPE_HDR:
    *(RKADK_U32 *)pvParam = pstSensorCfg->hdr;
    break;
  case RKADK_PARAM_TYPE_REC:
    *(bool *)pvParam = pstSensorCfg->enable_record;
    break;
  case RKADK_PARAM_TYPE_PHOTO_ENABLE:
    *(bool *)pvParam = pstSensorCfg->enable_photo;
    break;
  case RKADK_PARAM_TYPE_RES:
    *(RKADK_PARAM_RES_E *)pvParam = RKADK_PARAM_GetResType(
        pstRecCfg->attribute[0].width, pstRecCfg->attribute[0].height);
    break;
  case RKADK_PARAM_TYPE_CODEC_TYPE: {
    RKADK_PARAM_CODEC_CFG_S *pstCodecCfg;

    pstCodecCfg = (RKADK_PARAM_CODEC_CFG_S *)pvParam;
    pstCodecCfg->enCodecType =
        RKADK_PARAM_GetCodecType(s32CamID, pstCodecCfg->enStreamType);
    break;
  }
  case RKADK_PARAM_TYPE_BITRATE: {
    RKADK_PARAM_BITRATE_S *pstBitrate;

    pstBitrate = (RKADK_PARAM_BITRATE_S *)pvParam;
    pstBitrate->u32Bitrate =
        RKADK_PARAM_GetBitrate(s32CamID, pstBitrate->enStreamType);
    break;
  }
  case RKADK_PARAM_TYPE_RECORD_TIME:
  case RKADK_PARAM_TYPE_SPLITTIME:
  case RKADK_PARAM_TYPE_LAPSE_INTERVAL: {
    RKADK_PARAM_REC_TIME_S *pstRecTime;

    pstRecTime = (RKADK_PARAM_REC_TIME_S *)pvParam;
    pstRecTime->time =
        RKADK_PARAM_GetRecTime(s32CamID, pstRecTime->enStreamType, enParamType);
    break;
  }
  case RKADK_PARAM_TYPE_RECORD_TYPE:
    *(RKADK_REC_TYPE_E *)pvParam = pstRecCfg->record_type;
    break;
  case RKADK_PARAM_TYPE_FILE_CNT:
    *(RKADK_U32 *)pvParam = pstRecCfg->file_num;
    break;
  case RKADK_PARAM_TYPE_LAPSE_MULTIPLE:
    *(RKADK_U32 *)pvParam = pstRecCfg->lapse_multiple;
    break;
  case RKADK_PARAM_TYPE_PRE_RECORD_TIME:
    *(RKADK_U32 *)pvParam = pstRecCfg->pre_record_time;
    break;
  case RKADK_PARAM_TYPE_PRE_RECORD_MODE:
    *(MUXER_PRE_RECORD_MODE_E *)pvParam = pstRecCfg->pre_record_mode;
    break;
  case RKADK_PARAM_TYPE_PHOTO_RES:
    *(RKADK_PARAM_RES_E *)pvParam = RKADK_PARAM_GetResType(
        pstPhotoCfg->image_width, pstPhotoCfg->image_height);
    break;
  case RKADK_PARAM_TYPE_SNAP_NUM:
    *(RKADK_U32 *)pvParam = pstPhotoCfg->snap_num;
    break;
  default:
    RKADK_LOGE("Unsupport enParamType(%d)", enParamType);
    RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
    return -1;
  }

  RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
  return 0;
}

RKADK_S32 RKADK_PARAM_SetCamParam(RKADK_S32 s32CamID,
                                  RKADK_PARAM_TYPE_E enParamType,
                                  const RKADK_VOID *pvParam) {
  RKADK_S32 ret;
  bool bSaveRecCfg = false;
  bool bSaveRecAttr = false;
  bool bSavePhotoCfg = false;
  bool bSaveSensorCfg = false;
  RKADK_STREAM_TYPE_E enStrmType = RKADK_STREAM_TYPE_BUTT;
  RKADK_PARAM_RES_E type = RKADK_RES_BUTT;

  // RKADK_LOGD("s32CamID: %d, enParamType: %d, u32_pvParam: %d, b_pvParam: %d",
  // s32CamID, enParamType, *(RKADK_U32 *)pvParam, *(bool *)pvParam);

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pvParam, RKADK_FAILURE);

  RKADK_MUTEX_LOCK(g_stPARAMCtx.mutexLock);

  RKADK_PARAM_REC_CFG_S *pstRecCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stRecCfg;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg =
      &g_stPARAMCtx.stCfg.stSensorCfg[s32CamID];
  RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg =
      &g_stPARAMCtx.stCfg.stMediaCfg[s32CamID].stPhotoCfg;

  switch (enParamType) {
  case RKADK_PARAM_TYPE_FPS:
    RKADK_CHECK_EQUAL(pstSensorCfg->flip, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->framerate = *(RKADK_U32 *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_FLIP:
    RKADK_CHECK_EQUAL(pstSensorCfg->flip, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->flip = *(bool *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_MIRROR:
    RKADK_CHECK_EQUAL(pstSensorCfg->mirror, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->mirror = *(bool *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_LDC:
    RKADK_CHECK_EQUAL(pstSensorCfg->ldc, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->ldc = *(RKADK_U32 *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_ANTIFOG:
    RKADK_CHECK_EQUAL(pstSensorCfg->antifog, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->antifog = *(RKADK_U32 *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_WDR:
    RKADK_CHECK_EQUAL(pstSensorCfg->wdr, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->wdr = *(RKADK_U32 *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_HDR:
    RKADK_CHECK_EQUAL(pstSensorCfg->hdr, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->hdr = *(RKADK_U32 *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_REC:
    RKADK_CHECK_EQUAL(pstSensorCfg->enable_record, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->enable_record = *(bool *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_PHOTO_ENABLE:
    RKADK_CHECK_EQUAL(pstSensorCfg->enable_photo, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstSensorCfg->enable_photo = *(bool *)pvParam;
    bSaveSensorCfg = true;
    break;
  case RKADK_PARAM_TYPE_RES:
    type = *(RKADK_PARAM_RES_E *)pvParam;
    RKADK_PARAM_GetResolution(type, &(pstRecCfg->attribute[0].width),
                              &(pstRecCfg->attribute[0].height));
    enStrmType = RKADK_STREAM_TYPE_VIDEO_MAIN;
    bSaveRecAttr = true;
    break;
  case RKADK_PARAM_TYPE_CODEC_TYPE:
    ret =
        RKADK_PARAM_SetCodecType(s32CamID, (RKADK_PARAM_CODEC_CFG_S *)pvParam);
    RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
    return ret;
  case RKADK_PARAM_TYPE_BITRATE:
    ret = RKADK_PARAM_SetBitrate(s32CamID, (RKADK_PARAM_BITRATE_S *)pvParam);
    RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
    return ret;
  case RKADK_PARAM_TYPE_RECORD_TIME:
  case RKADK_PARAM_TYPE_SPLITTIME:
  case RKADK_PARAM_TYPE_LAPSE_INTERVAL:
    ret = RKADK_PARAM_SetRecTime(s32CamID, (RKADK_PARAM_REC_TIME_S *)pvParam,
                                 enParamType);
    RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
    return ret;
  case RKADK_PARAM_TYPE_RECORD_TYPE:
    RKADK_CHECK_EQUAL(pstRecCfg->record_type, *(RKADK_REC_TYPE_E *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstRecCfg->record_type = *(RKADK_REC_TYPE_E *)pvParam;
    bSaveRecCfg = true;
    break;
  case RKADK_PARAM_TYPE_FILE_CNT:
    RKADK_CHECK_EQUAL(pstRecCfg->file_num, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstRecCfg->file_num = *(RKADK_U32 *)pvParam;
    bSaveRecCfg = true;
    break;
  case RKADK_PARAM_TYPE_LAPSE_MULTIPLE:
    RKADK_CHECK_EQUAL(pstRecCfg->lapse_multiple, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstRecCfg->lapse_multiple = *(RKADK_U32 *)pvParam;
    bSaveRecCfg = true;
    break;
  case RKADK_PARAM_TYPE_PRE_RECORD_TIME:
    RKADK_CHECK_EQUAL(pstRecCfg->pre_record_time, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstRecCfg->pre_record_time = *(RKADK_U32 *)pvParam;
    bSaveRecCfg = true;
    break;
  case RKADK_PARAM_TYPE_PRE_RECORD_MODE:
    RKADK_CHECK_EQUAL(pstRecCfg->pre_record_mode,
                      *(MUXER_PRE_RECORD_MODE_E *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstRecCfg->pre_record_mode = *(MUXER_PRE_RECORD_MODE_E *)pvParam;
    bSaveRecCfg = true;
    break;
  case RKADK_PARAM_TYPE_PHOTO_RES:
    type = *(RKADK_PARAM_RES_E *)pvParam;
    RKADK_PARAM_GetResolution(type, &(pstPhotoCfg->image_width),
                              &(pstPhotoCfg->image_height));
    bSavePhotoCfg = true;
    break;
  case RKADK_PARAM_TYPE_SNAP_NUM:
    RKADK_CHECK_EQUAL(pstPhotoCfg->snap_num, *(RKADK_U32 *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);
    pstPhotoCfg->snap_num = *(RKADK_U32 *)pvParam;
    bSavePhotoCfg = true;
    break;
  default:
    RKADK_LOGE("Unsupport enParamType(%d)", enParamType);
    RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
    return -1;
  }

  if (bSaveSensorCfg)
    RKADK_PARAM_SaveSensorCfg(RKADK_PARAM_PATH, s32CamID);

  if (bSaveRecCfg)
    RKADK_PARAM_SaveRecCfg(RKADK_PARAM_PATH, s32CamID);

  if (bSaveRecAttr) {
    RKADK_PARAM_SaveRecAttr(RKADK_PARAM_PATH, s32CamID, enStrmType);
    RKADK_PARAM_SetRecViAttr(s32CamID);
  }

  if (bSavePhotoCfg) {
    RKADK_PARAM_SavePhotoCfg(RKADK_PARAM_PATH, s32CamID);
    RKADK_PARAM_SetPhotoViAttr(s32CamID);
  }

  RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
  return 0;
}

RKADK_S32 RKADK_PARAM_SetCommParam(RKADK_PARAM_TYPE_E enParamType,
                                   const RKADK_VOID *pvParam) {
  RKADK_CHECK_POINTER(pvParam, RKADK_FAILURE);
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, RKADK_FAILURE);

  // RKADK_LOGD("enParamType: %d, u32_pvParam: %d, b_pvParam: %d", enParamType,
  // *(RKADK_U32 *)pvParam, *(bool *)pvParam);

  RKADK_MUTEX_LOCK(g_stPARAMCtx.mutexLock);
  RKADK_PARAM_COMM_CFG_S *pstCommCfg = &g_stPARAMCtx.stCfg.stCommCfg;
  switch (enParamType) {
  case RKADK_PARAM_TYPE_REC_UNMUTE:
    RKADK_CHECK_EQUAL(pstCommCfg->rec_unmute, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->rec_unmute = *(bool *)pvParam;
    break;
  case RKADK_PARAM_TYPE_AUDIO:
    RKADK_CHECK_EQUAL(pstCommCfg->enable_speaker, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->enable_speaker = *(bool *)pvParam;
    if (!pstCommCfg->enable_speaker)
      RKADK_PARAM_SetSpeakerVolume(0);
    else
      RKADK_PARAM_SetSpeakerVolume(pstCommCfg->speaker_volume);
    break;
  case RKADK_PARAM_TYPE_VOLUME:
    RKADK_CHECK_EQUAL(pstCommCfg->speaker_volume, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->speaker_volume = *(RKADK_U32 *)pvParam;
    RKADK_PARAM_SetSpeakerVolume(pstCommCfg->speaker_volume);
    break;
  case RKADK_PARAM_TYPE_MIC_UNMUTE:
    RKADK_CHECK_EQUAL(pstCommCfg->mic_unmute, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->mic_unmute = *(bool *)pvParam;
    RKADK_PARAM_MicMute(!pstCommCfg->mic_unmute);
    break;
  case RKADK_PARAM_TYPE_MIC_VOLUME:
    RKADK_CHECK_EQUAL(pstCommCfg->mic_volume, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->mic_volume = *(RKADK_U32 *)pvParam;
    RKADK_PARAM_SetMicVolume(pstCommCfg->mic_volume);
    break;
  case RKADK_PARAM_TYPE_OSD_TIME_FORMAT:
    RKADK_CHECK_EQUAL(pstCommCfg->osd_time_format, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->osd_time_format = *(RKADK_U32 *)pvParam;
    break;
  case RKADK_PARAM_TYPE_OSD:
    RKADK_CHECK_EQUAL(pstCommCfg->osd, *(bool *)pvParam, g_stPARAMCtx.mutexLock,
                      RKADK_SUCCESS);

    pstCommCfg->osd = *(bool *)pvParam;
    break;
  case RKADK_PARAM_TYPE_BOOTSOUND:
    RKADK_CHECK_EQUAL(pstCommCfg->boot_sound, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->boot_sound = *(bool *)pvParam;
    break;
  case RKADK_PARAM_TYPE_OSD_SPEED:
    RKADK_CHECK_EQUAL(pstCommCfg->osd_speed, *(bool *)pvParam,
                      g_stPARAMCtx.mutexLock, RKADK_SUCCESS);

    pstCommCfg->osd_speed = *(bool *)pvParam;
    break;
  default:
    RKADK_LOGE("Unsupport enParamType(%d)", enParamType);
    RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
    return -1;
  }

  RKADK_PARAM_SaveCommCfg(RKADK_PARAM_PATH);
  RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
  return 0;
}

RKADK_S32 RKADK_PARAM_GetCommParam(RKADK_PARAM_TYPE_E enParamType,
                                   RKADK_VOID *pvParam) {
  RKADK_CHECK_POINTER(pvParam, RKADK_FAILURE);
  RKADK_CHECK_INIT(g_stPARAMCtx.bInit, RKADK_FAILURE);

  // RKADK_LOGD("enParamType: %d, u32_pvParam: %d, b_pvParam: %d", enParamType,
  // *(RKADK_U32 *)pvParam, *(bool *)pvParam);

  RKADK_MUTEX_LOCK(g_stPARAMCtx.mutexLock);
  RKADK_PARAM_COMM_CFG_S *pstCommCfg = &g_stPARAMCtx.stCfg.stCommCfg;
  switch (enParamType) {
  case RKADK_PARAM_TYPE_REC_UNMUTE:
    *(bool *)pvParam = pstCommCfg->rec_unmute;
    break;
  case RKADK_PARAM_TYPE_AUDIO:
    *(bool *)pvParam = pstCommCfg->enable_speaker;
    break;
  case RKADK_PARAM_TYPE_VOLUME:
    *(RKADK_U32 *)pvParam = pstCommCfg->speaker_volume;
    break;
  case RKADK_PARAM_TYPE_MIC_UNMUTE:
    *(bool *)pvParam = pstCommCfg->mic_unmute;
    break;
  case RKADK_PARAM_TYPE_MIC_VOLUME:
    *(RKADK_U32 *)pvParam = pstCommCfg->mic_volume;
    break;
  case RKADK_PARAM_TYPE_OSD_TIME_FORMAT:
    *(RKADK_U32 *)pvParam = pstCommCfg->osd_time_format;
    break;
  case RKADK_PARAM_TYPE_OSD:
    *(bool *)pvParam = pstCommCfg->osd;
    break;
  case RKADK_PARAM_TYPE_BOOTSOUND:
    *(bool *)pvParam = pstCommCfg->boot_sound;
    break;
  case RKADK_PARAM_TYPE_OSD_SPEED:
    *(bool *)pvParam = pstCommCfg->osd_speed;
    break;
  default:
    RKADK_LOGE("Unsupport enParamType(%d)", enParamType);
    RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
    return -1;
  }

  RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
  return 0;
}

RKADK_S32 RKADK_PARAM_SetDefault() {
  RKADK_S32 ret = RKADK_SUCCESS;

  RKADK_MUTEX_LOCK(g_stPARAMCtx.mutexLock);

  memset(&g_stPARAMCtx.stCfg, 0, sizeof(RKADK_PARAM_CFG_S));
  ret = RKADK_PARAM_LoadDefault();
  if (ret) {
    RKADK_LOGE("load default ini failed");
  } else {
    ret = RKADK_PARAM_SetMediaViAttr();
    if (ret)
      RKADK_LOGE("set media vi attribute failed");

    RKADK_PARAM_SetVolume();

#ifdef RKADK_DUMP_CONFIG
    RKADK_PARAM_Dump();
#endif
  }

  RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
  return ret;
}

RKADK_S32 RKADK_PARAM_Init() {
  RKADK_S32 ret = RKADK_SUCCESS;

  /* Check Module Init Status */
  if (g_stPARAMCtx.bInit)
    return RKADK_SUCCESS;

  RKADK_MUTEX_LOCK(g_stPARAMCtx.mutexLock);

  memset(&g_stPARAMCtx.stCfg, 0, sizeof(RKADK_PARAM_CFG_S));
  ret = RKADK_PARAM_LoadParam(RKADK_PARAM_PATH);
  if (ret) {
    RKADK_LOGE("load setting ini failed, load default ini");
    ret = RKADK_PARAM_LoadDefault();
    if (ret) {
      RKADK_LOGE("load default ini failed, use default param");
      RKADK_PARAM_UseDefault();
    }
  }

  ret = RKADK_PARAM_SetMediaViAttr();
  if (ret) {
    RKADK_LOGE("set media vi attribute failed");
    goto end;
  }

  RKADK_PARAM_SetVolume();

#ifdef RKADK_DUMP_CONFIG
  RKADK_PARAM_Dump();
#endif

  g_stPARAMCtx.bInit = true;

end:
  RKADK_MUTEX_UNLOCK(g_stPARAMCtx.mutexLock);
  return ret;
}

RKADK_S32 RKADK_PARAM_Deinit() {
  // reserved
  return 0;
}

RKADK_STREAM_TYPE_E RKADK_PARAM_VencChnMux(RKADK_U32 u32CamId,
                                           RKADK_U32 u32ChnId) {
  int i;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_STREAM_TYPE_BUTT);

  RKADK_PARAM_REC_CFG_S *pstRecCfg = RKADK_PARAM_GetRecCfg(u32CamId);
  if (pstRecCfg) {
    for (i = 0; i < (int)pstRecCfg->file_num; i++) {
      if (u32ChnId == pstRecCfg->attribute[i].venc_chn) {
        if (i == 0)
          return RKADK_STREAM_TYPE_VIDEO_MAIN;
        else
          return RKADK_STREAM_TYPE_VIDEO_SUB;
      }
    }
  }

  RKADK_PARAM_STREAM_CFG_S *pstStreamCfg =
      RKADK_PARAM_GetStreamCfg(0, RKADK_STREAM_TYPE_PREVIEW);
  if (pstStreamCfg && (u32ChnId == pstStreamCfg->attribute.venc_chn))
    return RKADK_STREAM_TYPE_PREVIEW;

  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg =
      RKADK_PARAM_GetStreamCfg(0, RKADK_STREAM_TYPE_LIVE);
  if (pstLiveCfg && (u32ChnId == pstLiveCfg->attribute.venc_chn))
    return RKADK_STREAM_TYPE_LIVE;

  return RKADK_STREAM_TYPE_BUTT;
}
