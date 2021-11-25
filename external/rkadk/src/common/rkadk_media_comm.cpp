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

#include "rkadk_media_comm.h"
#include "rkadk_log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  bool bUsed;
  RKADK_S32 s32BindCnt;
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
} RKADK_BIND_INFO_S;

typedef struct {
  bool bGetBuffer;
  RKADK_S32 s32GetCnt;
  pthread_t tid;
  RKADK_VOID *pHandle[RKADK_MEDIA_VENC_MAX_CNT];
  OutCbFuncEx cbList[RKADK_MEDIA_VENC_MAX_CNT];
} RKADK_GET_MB_ATTR_S;

typedef struct {
  bool bUsed;
  RKADK_S32 s32InitCnt;
  RKADK_S32 s32ChnId;
  CODEC_TYPE_E enCodecType;
  RKADK_GET_MB_ATTR_S stGetMBAttr;
} RKADK_MEDIA_INFO_S;

typedef struct {
  pthread_mutex_t aiMutex;
  pthread_mutex_t aencMutex;
  pthread_mutex_t viMutex;
  pthread_mutex_t vencMutex;
  pthread_mutex_t bindMutex;
  RKADK_MEDIA_INFO_S stAiInfo[RKADK_MEDIA_AI_MAX_CNT];
  RKADK_MEDIA_INFO_S stAencInfo[RKADK_MEDIA_AENC_MAX_CNT];
  RKADK_MEDIA_INFO_S stViInfo[RKADK_MEDIA_VI_MAX_CNT];
  RKADK_MEDIA_INFO_S stVencInfo[RKADK_MEDIA_VENC_MAX_CNT];
  RKADK_BIND_INFO_S stAiAencInfo[RKADK_AI_AENC_MAX_BIND_CNT];
  RKADK_BIND_INFO_S stViVencInfo[RKADK_VI_VENC_MAX_BIND_CNT];
} RKADK_MEDIA_CONTEXT_S;

static bool g_bMediaCtxInit = false;
static RKADK_MEDIA_CONTEXT_S g_stMediaCtx;

static void RKADK_MEDIA_CtxInit() {
  if (g_bMediaCtxInit)
    return;

  memset((void *)&g_stMediaCtx, 0, sizeof(RKADK_MEDIA_CONTEXT_S));
  g_stMediaCtx.aiMutex = PTHREAD_MUTEX_INITIALIZER;
  g_stMediaCtx.aencMutex = PTHREAD_MUTEX_INITIALIZER;
  g_stMediaCtx.viMutex = PTHREAD_MUTEX_INITIALIZER;
  g_stMediaCtx.vencMutex = PTHREAD_MUTEX_INITIALIZER;
  g_stMediaCtx.bindMutex = PTHREAD_MUTEX_INITIALIZER;
  g_bMediaCtxInit = true;
}

static RKADK_U32 RKADK_MEDIA_FindUsableIdx(RKADK_MEDIA_INFO_S *pstInfo,
                                           int count, const char *mode) {
  for (int i = 0; i < count; i++) {
    if (!pstInfo[i].bUsed) {
      RKADK_LOGD("%s: find usable index[%d]", mode, i);
      return i;
    }
  }

  return -1;
}

static RKADK_S32 RKADK_MEDIA_GetIdx(RKADK_MEDIA_INFO_S *pstInfo, int count,
                                    RKADK_S32 s32ChnId, const char *mode) {
  for (int i = 0; i < count; i++) {
    if (!pstInfo[i].bUsed)
      continue;

    if (pstInfo[i].s32ChnId == s32ChnId) {
      RKADK_LOGD("%s: find matched index[%d] ChnId[%d]", mode, i, s32ChnId);
      return i;
    }
  }

  return -1;
}

static RKADK_S32 RKADK_MPI_AI_EnableVqe(RKADK_S32 s32AiChnId,
                                        RK_U32 u32SampleRate,
                                        RKADK_VQE_MODE_E enMode) {
  int ret;
  AI_RECORDVQE_CONFIG_S stAiVqeRecordAttr;

  if (enMode != RKADK_VQE_MODE_AI_RECORD) {
    RKADK_LOGW("NonSupport enMode: %d", enMode);
    return -1;
  }

  memset(&stAiVqeRecordAttr, 0, sizeof(AI_RECORDVQE_CONFIG_S));
  stAiVqeRecordAttr.s32WorkSampleRate = u32SampleRate;
  stAiVqeRecordAttr.s32FrameSample = 320;
  stAiVqeRecordAttr.stAnrConfig.fPostAddGain = 0;
  stAiVqeRecordAttr.stAnrConfig.fGmin = -20;
  stAiVqeRecordAttr.stAnrConfig.fNoiseFactor = 0.98;
  stAiVqeRecordAttr.stAnrConfig.enHpfSwitch = 0;
  stAiVqeRecordAttr.stAnrConfig.fHpfFc = 100.0f;
  stAiVqeRecordAttr.stAnrConfig.enLpfSwitch = 0;
  stAiVqeRecordAttr.stAnrConfig.fLpfFc = 10000.0f;
  stAiVqeRecordAttr.u32OpenMask = AI_RECORDVQE_MASK_ANR;

  ret = RK_MPI_AI_SetRecordVqeAttr(s32AiChnId, &stAiVqeRecordAttr);
  if (ret) {
    RKADK_LOGE("AI[%d] SetRecordVqeAttr failed: %d", s32AiChnId, ret);
    return -1;
  }

  ret = RK_MPI_AI_EnableVqe(s32AiChnId);
  if (ret) {
    RKADK_LOGE("AI[%d] EnableVqe failed: %d", s32AiChnId, ret);
    return -1;
  }

  return 0;
}

RKADK_S32 RKADK_MPI_AI_Init(RKADK_S32 s32AiChnId, AI_CHN_ATTR_S *pstAiChnAttr,
                            RKADK_VQE_MODE_E enMode) {
  int ret = -1;
  RKADK_S32 i;

  RKADK_CHECK_POINTER(pstAiChnAttr, RKADK_FAILURE);
  RKADK_MEDIA_CtxInit();

  RKADK_MUTEX_LOCK(g_stMediaCtx.aiMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAiInfo, RKADK_MEDIA_AI_MAX_CNT,
                         s32AiChnId, "AI_INIT");
  if (i < 0) {
    i = RKADK_MEDIA_FindUsableIdx(g_stMediaCtx.stAiInfo, RKADK_MEDIA_AI_MAX_CNT,
                                  "AI_INIT");
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      goto exit;
    }
  }

  if (0 == g_stMediaCtx.stAiInfo[i].s32InitCnt) {
    ret = RK_MPI_AI_SetChnAttr(s32AiChnId, pstAiChnAttr);
    if (ret) {
      RKADK_LOGE("Set AI[%d] attribute failed(%d)", s32AiChnId, ret);
      goto exit;
    }

    ret = RK_MPI_AI_EnableChn(s32AiChnId);
    if (ret) {
      RKADK_LOGE("Create AI[%d] failed(%d)", s32AiChnId, ret);
      goto exit;
    }

    if (enMode != RKADK_VQE_MODE_BUTT)
      RKADK_MPI_AI_EnableVqe(s32AiChnId, pstAiChnAttr->u32SampleRate, enMode);

    g_stMediaCtx.stAiInfo[i].bUsed = true;
    g_stMediaCtx.stAiInfo[i].s32ChnId = s32AiChnId;
  }

  g_stMediaCtx.stAiInfo[i].s32InitCnt++;
  RKADK_LOGD("aiChnId[%d], InitCnt[%d]", s32AiChnId,
             g_stMediaCtx.stAiInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.aiMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_AI_DeInit(RKADK_S32 s32AiChnId, RKADK_VQE_MODE_E enMode) {
  int ret = -1;
  RKADK_S32 i;
  RKADK_S32 s32InitCnt;

  RKADK_MUTEX_LOCK(g_stMediaCtx.aiMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAiInfo, RKADK_MEDIA_AI_MAX_CNT,
                         s32AiChnId, "AI_DEINIT");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32AiChnId[%d]", i, s32AiChnId);
    goto exit;
  }

  s32InitCnt = g_stMediaCtx.stAiInfo[i].s32InitCnt;
  if (0 == s32InitCnt) {
    RKADK_LOGD("aiChnId[%d] has already deinit", s32AiChnId);
    RKADK_MUTEX_UNLOCK(g_stMediaCtx.aiMutex);
    return 0;
  } else if (1 == s32InitCnt) {
    if (enMode != RKADK_VQE_MODE_BUTT) {
      ret = RK_MPI_AI_DisableVqe(s32AiChnId);
      if (ret)
        RKADK_LOGE("AI[%d] DisableVqe failed: %d", s32AiChnId, ret);
    }

    ret = RK_MPI_AI_DisableChn(s32AiChnId);
    if (ret) {
      RKADK_LOGE("Disable AI[%d] error %d", s32AiChnId, ret);
      goto exit;
    }

    g_stMediaCtx.stAiInfo[i].bUsed = false;
    g_stMediaCtx.stAiInfo[i].s32ChnId = 0;
  }

  g_stMediaCtx.stAiInfo[i].s32InitCnt--;
  RKADK_LOGD("aiChnId[%d], InitCnt[%d]", s32AiChnId,
             g_stMediaCtx.stAiInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.aiMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_AENC_Init(RKADK_S32 s32AencChnId,
                              AENC_CHN_ATTR_S *pstAencChnAttr) {
  int ret = -1;
  RKADK_S32 i;

  RKADK_CHECK_POINTER(pstAencChnAttr, RKADK_FAILURE);
  RKADK_MEDIA_CtxInit();

  RKADK_MUTEX_LOCK(g_stMediaCtx.aencMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAencInfo, RKADK_MEDIA_AENC_MAX_CNT,
                         s32AencChnId, "AENC_INIT");
  if (i < 0) {
    i = RKADK_MEDIA_FindUsableIdx(g_stMediaCtx.stAencInfo,
                                  RKADK_MEDIA_AENC_MAX_CNT, "AENC_INIT");
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      goto exit;
    }
  } else {
    if (g_stMediaCtx.stAencInfo[i].enCodecType != pstAencChnAttr->enCodecType) {
      RKADK_LOGW("find matched index[%d], but CodecType inequality[%d, %d]", i,
                 g_stMediaCtx.stAencInfo[i].enCodecType,
                 pstAencChnAttr->enCodecType);
    }
  }

  if (0 == g_stMediaCtx.stAencInfo[i].s32InitCnt) {
    ret = RK_MPI_AENC_CreateChn(s32AencChnId, pstAencChnAttr);
    if (ret) {
      RKADK_LOGE("Create AENC[%d] failed(%d)", s32AencChnId, ret);
      goto exit;
    }

    g_stMediaCtx.stAencInfo[i].bUsed = true;
    g_stMediaCtx.stAencInfo[i].s32ChnId = s32AencChnId;
    g_stMediaCtx.stAencInfo[i].enCodecType = pstAencChnAttr->enCodecType;
  }

  g_stMediaCtx.stAencInfo[i].s32InitCnt++;
  RKADK_LOGD("aencChnId[%d], InitCnt[%d]", s32AencChnId,
             g_stMediaCtx.stAencInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.aencMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_AENC_DeInit(RKADK_S32 s32AencChnId) {
  int ret = -1;
  RKADK_S32 i;
  RKADK_S32 s32InitCnt;

  RKADK_MUTEX_LOCK(g_stMediaCtx.aencMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAencInfo, RKADK_MEDIA_AENC_MAX_CNT,
                         s32AencChnId, "AENC_DEINIT");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32AencChnId[%d]", i, s32AencChnId);
    goto exit;
  }

  s32InitCnt = g_stMediaCtx.stAencInfo[i].s32InitCnt;
  if (0 == s32InitCnt) {
    RKADK_LOGD("aencChnId[%d] has already deinit", s32AencChnId);
    RKADK_MUTEX_UNLOCK(g_stMediaCtx.aencMutex);
    return 0;
  } else if (1 == s32InitCnt) {
    ret = RK_MPI_AENC_DestroyChn(s32AencChnId);
    if (ret) {
      RKADK_LOGE("Destroy AENC[%d] error %d", s32AencChnId, ret);
      goto exit;
    }

    g_stMediaCtx.stAencInfo[i].bUsed = false;
    g_stMediaCtx.stAencInfo[i].s32ChnId = 0;
  }

  g_stMediaCtx.stAencInfo[i].s32InitCnt--;
  RKADK_LOGD("aencChnId[%d], InitCnt[%d]", s32AencChnId,
             g_stMediaCtx.stAencInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.aencMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_VI_Init(RKADK_U32 u32CamId, RKADK_S32 s32ViChnId,
                            VI_CHN_ATTR_S *pstViChnAttr) {
  int ret = -1;
  RKADK_S32 i;

  RKADK_CHECK_POINTER(pstViChnAttr, RKADK_FAILURE);
  RKADK_MEDIA_CtxInit();

  RKADK_MUTEX_LOCK(g_stMediaCtx.viMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stViInfo, RKADK_MEDIA_VI_MAX_CNT,
                         s32ViChnId, "VI_INIT");
  if (i < 0) {
    i = RKADK_MEDIA_FindUsableIdx(g_stMediaCtx.stViInfo, RKADK_MEDIA_VI_MAX_CNT,
                                  "VI_INIT");
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      goto exit;
    }
  }

  if (0 == g_stMediaCtx.stViInfo[i].s32InitCnt) {
    ret = RK_MPI_VI_SetChnAttr(u32CamId, s32ViChnId, pstViChnAttr);
    if (ret) {
      RKADK_LOGE("Set VI[%d] attribute error %d", s32ViChnId, ret);
      goto exit;
    }

    ret = RK_MPI_VI_EnableChn(u32CamId, s32ViChnId);
    if (ret) {
      RKADK_LOGE("Create VI[%d] error %d", s32ViChnId, ret);
      goto exit;
    }

    g_stMediaCtx.stViInfo[i].bUsed = true;
    g_stMediaCtx.stViInfo[i].s32ChnId = s32ViChnId;
  }

  g_stMediaCtx.stViInfo[i].s32InitCnt++;
  RKADK_LOGD("viChnId[%d], InitCnt[%d]", s32ViChnId,
             g_stMediaCtx.stViInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.viMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_VI_DeInit(RKADK_U32 u32CamId, RKADK_S32 s32ViChnId) {
  int ret = -1;
  RKADK_S32 i;
  RKADK_S32 s32InitCnt;

  RKADK_MUTEX_LOCK(g_stMediaCtx.viMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stViInfo, RKADK_MEDIA_VI_MAX_CNT,
                         s32ViChnId, "VI_DEINIT");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32ChnId[%d]", i, s32ViChnId);
    goto exit;
  }

  s32InitCnt = g_stMediaCtx.stViInfo[i].s32InitCnt;
  if (0 == s32InitCnt) {
    RKADK_LOGD("viChnId[%d] has already deinit", s32ViChnId);
    RKADK_MUTEX_UNLOCK(g_stMediaCtx.viMutex);
    return 0;
  } else if (1 == s32InitCnt) {
    ret = RK_MPI_VI_DisableChn(u32CamId, s32ViChnId);
    if (ret) {
      RKADK_LOGE("Destory VI[%d] failed, ret=%d", s32ViChnId, ret);
      goto exit;
    }

    g_stMediaCtx.stViInfo[i].bUsed = false;
    g_stMediaCtx.stViInfo[i].s32ChnId = 0;
  }

  g_stMediaCtx.stViInfo[i].s32InitCnt--;
  RKADK_LOGD("viChnId[%d], InitCnt[%d]", s32ViChnId,
             g_stMediaCtx.stViInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.viMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_VENC_Init(RKADK_S32 s32ChnId,
                              VENC_CHN_ATTR_S *pstVencChnAttr) {
  int ret = -1;
  RKADK_S32 i;

  RKADK_CHECK_POINTER(pstVencChnAttr, RKADK_FAILURE);
  RKADK_MEDIA_CtxInit();

  RKADK_MUTEX_LOCK(g_stMediaCtx.vencMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stVencInfo, RKADK_MEDIA_VENC_MAX_CNT,
                         s32ChnId, "VENC_INIT");
  if (i < 0) {
    i = RKADK_MEDIA_FindUsableIdx(g_stMediaCtx.stVencInfo,
                                  RKADK_MEDIA_VENC_MAX_CNT, "VENC_INIT");
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      goto exit;
    }
  } else {
    if (g_stMediaCtx.stVencInfo[i].enCodecType !=
        pstVencChnAttr->stVencAttr.enType) {
      RKADK_LOGW("find matched index[%d], but CodecType inequality[%d, %d]", i,
                 g_stMediaCtx.stVencInfo[i].enCodecType,
                 pstVencChnAttr->stVencAttr.enType);
    }
  }

  if (0 == g_stMediaCtx.stVencInfo[i].s32InitCnt) {
    ret = RK_MPI_VENC_CreateChn(s32ChnId, pstVencChnAttr);
    if (ret) {
      RKADK_LOGE("Create VENC[%d] error %d", s32ChnId, ret);
      goto exit;
    }

    g_stMediaCtx.stVencInfo[i].bUsed = true;
    g_stMediaCtx.stVencInfo[i].s32ChnId = s32ChnId;
    g_stMediaCtx.stVencInfo[i].enCodecType = pstVencChnAttr->stVencAttr.enType;
  }

  g_stMediaCtx.stVencInfo[i].s32InitCnt++;
  RKADK_LOGD("vencChnId[%d], InitCnt[%d]", s32ChnId,
             g_stMediaCtx.stVencInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.vencMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_VENC_DeInit(RKADK_S32 s32ChnId) {
  int ret = -1;
  RKADK_S32 i;
  RKADK_S32 s32InitCnt;

  RKADK_MUTEX_LOCK(g_stMediaCtx.vencMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stVencInfo, RKADK_MEDIA_VENC_MAX_CNT,
                         s32ChnId, "VENC_DEINIT");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32ChnId[%d]", i, s32ChnId);
    goto exit;
  }

  s32InitCnt = g_stMediaCtx.stVencInfo[i].s32InitCnt;
  if (0 == s32InitCnt) {
    RKADK_LOGD("vencChnId[%d] has already deinit", s32ChnId);
    RKADK_MUTEX_UNLOCK(g_stMediaCtx.vencMutex);
    return 0;
  } else if (1 == s32InitCnt) {
    ret = RK_MPI_VENC_DestroyChn(s32ChnId);
    if (ret) {
      RKADK_LOGE("Destroy VENC[%d] error %d", s32ChnId, ret);
      goto exit;
    }

    g_stMediaCtx.stVencInfo[i].bUsed = false;
    g_stMediaCtx.stVencInfo[i].s32ChnId = 0;
  }

  g_stMediaCtx.stVencInfo[i].s32InitCnt--;
  RKADK_LOGD("vencChnId[%d], InitCnt[%d]", s32ChnId,
             g_stMediaCtx.stVencInfo[i].s32InitCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.vencMutex);
  return ret;
}

static void *RKADK_MEDIA_GetMb(void *params) {
  MEDIA_BUFFER mb = NULL;

  RKADK_MEDIA_INFO_S *pstMediaInfo = (RKADK_MEDIA_INFO_S *)params;
  if (!pstMediaInfo) {
    RKADK_LOGE("Get MB thread invalid param");
    return NULL;
  }

  while (pstMediaInfo->stGetMBAttr.bGetBuffer) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, pstMediaInfo->s32ChnId, -1);
    if (!mb) {
      RKADK_LOGE("RK_MPI_SYS_GetMediaBuffer get null buffer!");
      break;
    }

    for (int i = 0; i < (int)pstMediaInfo->stGetMBAttr.s32GetCnt; i++)
      if (pstMediaInfo->stGetMBAttr.cbList[i])
        pstMediaInfo->stGetMBAttr.cbList[i](
            mb, pstMediaInfo->stGetMBAttr.pHandle[i]);

    RK_MPI_MB_ReleaseBuffer(mb);
  }

  RK_MPI_SYS_StopGetMediaBuffer(RK_ID_VDEC, pstMediaInfo->s32ChnId);
  RKADK_LOGI("Exit get mb thread");
  return NULL;
}

RKADK_S32 RKADK_MEDIA_GetMediaBuffer(MPP_CHN_S *pstChn, OutCbFuncEx pfnDataCB,
                                     RKADK_VOID *pHandle) {
  int ret = -1;
  RKADK_S32 i;
  RKADK_MEDIA_INFO_S *pstMediaInfo;

  RKADK_MUTEX_LOCK(g_stMediaCtx.vencMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stVencInfo, RKADK_MEDIA_VENC_MAX_CNT,
                         pstChn->s32ChnId, "VENC_GET_MB");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32ChnId[%d]", i, pstChn->s32ChnId);
    goto exit;
  }

  pstMediaInfo = &g_stMediaCtx.stVencInfo[i];
  if (pstMediaInfo->s32InitCnt == 0) {
    RKADK_LOGE("vencChnId[%d] don't init", pstChn->s32ChnId);
    goto exit;
  }

  pstMediaInfo->stGetMBAttr.cbList[pstMediaInfo->stGetMBAttr.s32GetCnt] = pfnDataCB;
  pstMediaInfo->stGetMBAttr.pHandle[pstMediaInfo->stGetMBAttr.s32GetCnt] = pHandle;
  pstMediaInfo->stGetMBAttr.s32GetCnt++;

  if (pstMediaInfo->stGetMBAttr.bGetBuffer) {
    RKADK_LOGE("Get vencChnId[%d] MB thread has been created",
               pstChn->s32ChnId);
    ret = 0;
    goto exit;
  }

  pstMediaInfo->stGetMBAttr.bGetBuffer = true;
  ret = pthread_create(&pstMediaInfo->stGetMBAttr.tid, NULL, RKADK_MEDIA_GetMb,
                       pstMediaInfo);
  if (ret) {
    RKADK_LOGE("Create get mb(%d) thread failed %d", pstChn->s32ChnId, ret);
    goto exit;
  }

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.vencMutex);
  return ret;
}

RKADK_S32 RKADK_MEDIA_StopGetMediaBuffer(MPP_CHN_S *pstChn,
                                         OutCbFuncEx pfnDataCB) {
  int i, ret = 0;
  RKADK_MEDIA_INFO_S *pstMediaInfo;

  RKADK_MUTEX_LOCK(g_stMediaCtx.vencMutex);

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stVencInfo, RKADK_MEDIA_VENC_MAX_CNT,
                         pstChn->s32ChnId, "VENC_GET_MB");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32ChnId[%d]", i, pstChn->s32ChnId);
    ret = -1;
    goto exit;
  }

  pstMediaInfo = &g_stMediaCtx.stVencInfo[i];
  if (pstMediaInfo->s32InitCnt == 0) {
    RKADK_LOGE("vencChnId[%d] don't init", pstChn->s32ChnId);
    ret = -1;
    goto exit;
  }

  for (i = 0; i < pstMediaInfo->stGetMBAttr.s32GetCnt; i++) {
    if (pstMediaInfo->stGetMBAttr.cbList[i] == pfnDataCB) {
      RKADK_LOGD("remove cbList, i = %d", i);
      pstMediaInfo->stGetMBAttr.cbList[i] = NULL;
      pstMediaInfo->stGetMBAttr.s32GetCnt--;
      break;
    }
  }

  if (!pstMediaInfo->stGetMBAttr.s32GetCnt) {
    pstMediaInfo->stGetMBAttr.bGetBuffer = false;
    if (pstMediaInfo->stGetMBAttr.tid) {
      ret = pthread_join(pstMediaInfo->stGetMBAttr.tid, NULL);
      if (ret)
        RKADK_LOGE("Exit get mb thread failed!");
      else
        RKADK_LOGI("Exit get mb thread ok");
      pstMediaInfo->stGetMBAttr.tid = 0;
    }
  }

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.vencMutex);
  return ret;
}

static RKADK_U32 RKADK_BIND_FindUsableIdx(RKADK_BIND_INFO_S *pstInfo,
                                          int count) {
  for (int i = 0; i < count; i++) {
    if (!pstInfo[i].bUsed) {
      RKADK_LOGD("find usable index[%d]", i);
      return i;
    }
  }

  return -1;
}

static RKADK_S32 RKADK_BIND_GetIdx(RKADK_BIND_INFO_S *pstInfo, int count,
                                   const MPP_CHN_S *pstSrcChn,
                                   const MPP_CHN_S *pstDestChn) {
  RKADK_CHECK_POINTER(pstSrcChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstDestChn, RKADK_FAILURE);

  for (int i = 0; i < count; i++) {
    if (!pstInfo[i].bUsed)
      continue;

    if (pstInfo[i].stSrcChn.s32ChnId == pstSrcChn->s32ChnId &&
        pstInfo[i].stSrcChn.enModId == pstSrcChn->enModId &&
        pstInfo[i].stDestChn.s32ChnId == pstDestChn->s32ChnId &&
        pstInfo[i].stDestChn.enModId == pstDestChn->enModId) {
      RKADK_LOGD("find matched index[%d]: src ChnId[%d] dest ChnId[%d]", i,
                 pstSrcChn->s32ChnId, pstDestChn->s32ChnId);
      return i;
    }
  }

  return -1;
}

RKADK_S32 RKADK_MPI_SYS_Bind(const MPP_CHN_S *pstSrcChn,
                             const MPP_CHN_S *pstDestChn) {
  int ret = -1, count;
  RKADK_S32 i;
  RKADK_BIND_INFO_S *pstInfo = NULL;

  RKADK_CHECK_POINTER(pstSrcChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstDestChn, RKADK_FAILURE);

  if (pstSrcChn->enModId == RK_ID_AI && pstDestChn->enModId == RK_ID_AENC) {
    count = RKADK_AI_AENC_MAX_BIND_CNT;
    pstInfo = g_stMediaCtx.stAiAencInfo;
  } else if (pstSrcChn->enModId == RK_ID_VI &&
             pstDestChn->enModId == RK_ID_VENC) {
    count = RKADK_VI_VENC_MAX_BIND_CNT;
    pstInfo = g_stMediaCtx.stViVencInfo;
  } else {
    RKADK_LOGE("Nonsupport: src enModId: %d, dest enModId: %d",
               pstSrcChn->enModId, pstDestChn->enModId);
    return -1;
  }

  RKADK_MUTEX_LOCK(g_stMediaCtx.bindMutex);

  i = RKADK_BIND_GetIdx(pstInfo, count, pstSrcChn, pstDestChn);
  if (i < 0) {
    i = RKADK_BIND_FindUsableIdx(pstInfo, count);
    if (i < 0) {
      RKADK_LOGE("not find usable index, src chn[%d], dst chn[%d]",
                 pstSrcChn->s32ChnId, pstDestChn->s32ChnId);
      goto exit;
    }
  }

  if (0 == pstInfo[i].s32BindCnt) {
    ret = RK_MPI_SYS_Bind(pstSrcChn, pstDestChn);
    if (ret) {
      RKADK_LOGE("Bind src[%d %d] and dest[%d %d] failed(%d)",
                 pstSrcChn->enModId, pstSrcChn->s32ChnId, pstDestChn->enModId,
                 pstDestChn->s32ChnId, ret);
      goto exit;
    }

    pstInfo[i].bUsed = true;
    memcpy(&pstInfo[i].stSrcChn, pstSrcChn, sizeof(MPP_CHN_S));
    memcpy(&pstInfo[i].stDestChn, pstDestChn, sizeof(MPP_CHN_S));
  }

  pstInfo[i].s32BindCnt++;
  RKADK_LOGD("src[%d, %d], dest[%d, %d], BindCnt[%d]", pstSrcChn->enModId,
             pstSrcChn->s32ChnId, pstDestChn->enModId, pstDestChn->s32ChnId,
             pstInfo[i].s32BindCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.bindMutex);
  return ret;
}

RKADK_S32 RKADK_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn,
                               const MPP_CHN_S *pstDestChn) {
  int ret = -1, count;
  RKADK_S32 i;
  RKADK_BIND_INFO_S *pstInfo = NULL;

  RKADK_CHECK_POINTER(pstSrcChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstDestChn, RKADK_FAILURE);

  if (pstSrcChn->enModId == RK_ID_AI && pstDestChn->enModId == RK_ID_AENC) {
    count = RKADK_AI_AENC_MAX_BIND_CNT;
    pstInfo = g_stMediaCtx.stAiAencInfo;
  } else if (pstSrcChn->enModId == RK_ID_VI &&
             pstDestChn->enModId == RK_ID_VENC) {
    count = RKADK_VI_VENC_MAX_BIND_CNT;
    pstInfo = g_stMediaCtx.stViVencInfo;
  } else {
    RKADK_LOGE("Nonsupport");
    return -1;
  }

  RKADK_MUTEX_LOCK(g_stMediaCtx.bindMutex);

  i = RKADK_BIND_GetIdx(pstInfo, count, pstSrcChn, pstDestChn);
  if (i < 0) {
    RKADK_MUTEX_UNLOCK(g_stMediaCtx.bindMutex);
    RKADK_LOGE("not find usable index");
    return -1;
  }

  if (0 == pstInfo[i].s32BindCnt) {
    RKADK_LOGD("src[%d, %d], dest[%d, %d] has already UnBind",
               pstSrcChn->enModId, pstSrcChn->s32ChnId, pstDestChn->enModId,
               pstDestChn->s32ChnId);
    RKADK_MUTEX_UNLOCK(g_stMediaCtx.bindMutex);
    return 0;
  } else if (1 == pstInfo[i].s32BindCnt) {
    ret = RK_MPI_SYS_UnBind(pstSrcChn, pstDestChn);
    if (ret) {
      RKADK_LOGE("UnBind src[%d, %d] and dest[%d, %d] failed(%d)",
                 pstSrcChn->enModId, pstSrcChn->s32ChnId, pstDestChn->enModId,
                 pstDestChn->s32ChnId, ret);
      goto exit;
    }

    pstInfo[i].bUsed = false;
    memset(&pstInfo[i].stSrcChn, 0, sizeof(MPP_CHN_S));
    memset(&pstInfo[i].stDestChn, 0, sizeof(MPP_CHN_S));
  }

  pstInfo[i].s32BindCnt--;
  RKADK_LOGD("src[%d, %d], dest[%d, %d], BindCnt[%d]", pstSrcChn->enModId,
             pstSrcChn->s32ChnId, pstDestChn->enModId, pstDestChn->s32ChnId,
             pstInfo[i].s32BindCnt);
  ret = 0;

exit:
  RKADK_MUTEX_UNLOCK(g_stMediaCtx.bindMutex);
  return ret;
}

CODEC_TYPE_E RKADK_MEDIA_GetRkCodecType(RKADK_CODEC_TYPE_E enType) {
  CODEC_TYPE_E enCodecType;

  switch (enType) {
  case RKADK_CODEC_TYPE_H264:
    enCodecType = RK_CODEC_TYPE_H264;
    break;

  case RKADK_CODEC_TYPE_H265:
    enCodecType = RK_CODEC_TYPE_H265;
    break;

  case RKADK_CODEC_TYPE_MJPEG:
    enCodecType = RK_CODEC_TYPE_MJPEG;
    break;

  case RKADK_CODEC_TYPE_JPEG:
    enCodecType = RK_CODEC_TYPE_JPEG;
    break;

  case RKADK_CODEC_TYPE_MP3:
    enCodecType = RK_CODEC_TYPE_MP3;
    break;

  case RKADK_CODEC_TYPE_G711A:
    enCodecType = RK_CODEC_TYPE_G711A;
    break;

  case RKADK_CODEC_TYPE_G711U:
    enCodecType = RK_CODEC_TYPE_G711U;
    break;

  case RKADK_CODEC_TYPE_G726:
    enCodecType = RK_CODEC_TYPE_G726;
    break;

  case RKADK_CODEC_TYPE_MP2:
    enCodecType = RK_CODEC_TYPE_MP2;
    break;

  default:
    enCodecType = RK_CODEC_TYPE_NONE;
    break;
  }

  return enCodecType;
}

RKADK_CODEC_TYPE_E RKADK_MEDIA_GetCodecType(CODEC_TYPE_E enType) {
  RKADK_CODEC_TYPE_E enCodecType;

  switch (enType) {
  case RK_CODEC_TYPE_H264:
    enCodecType = RKADK_CODEC_TYPE_H264;
    break;

  case RK_CODEC_TYPE_H265:
    enCodecType = RKADK_CODEC_TYPE_H265;
    break;

  case RK_CODEC_TYPE_MJPEG:
    enCodecType = RKADK_CODEC_TYPE_MJPEG;
    break;

  case RK_CODEC_TYPE_JPEG:
    enCodecType = RKADK_CODEC_TYPE_JPEG;
    break;

  case RK_CODEC_TYPE_MP3:
    enCodecType = RKADK_CODEC_TYPE_MP3;
    break;
  case RK_CODEC_TYPE_G711A:
    enCodecType = RKADK_CODEC_TYPE_G711A;
    break;
  case RK_CODEC_TYPE_G711U:
    enCodecType = RKADK_CODEC_TYPE_G711U;
    break;

  case RK_CODEC_TYPE_G726:
    enCodecType = RKADK_CODEC_TYPE_G726;
    break;

  case RK_CODEC_TYPE_MP2:
    enCodecType = RKADK_CODEC_TYPE_MP2;
    break;

  default:
    enCodecType = RKADK_CODEC_TYPE_BUTT;
    break;
  }

  return enCodecType;
}

RKADK_S32 RKADK_MEDIA_SetRcAttr(VENC_RC_ATTR_S *pstRcAttr, RKADK_U32 u32Gop,
                                RKADK_U32 u32BitRate, RKADK_U32 u32SrcFrameRate,
                                RKADK_U32 u32DstFrameRate) {
  switch (pstRcAttr->enRcMode) {
  case VENC_RC_MODE_H265CBR:
    pstRcAttr->stH265Cbr.u32Gop = u32Gop;
    pstRcAttr->stH265Cbr.u32BitRate = u32BitRate;
    pstRcAttr->stH265Cbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stH265Cbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stH265Cbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stH265Cbr.u32SrcFrameRateNum = u32SrcFrameRate;
    break;
  case VENC_RC_MODE_H265VBR:
    pstRcAttr->stH265Vbr.u32Gop = u32Gop;
    pstRcAttr->stH265Vbr.u32MaxBitRate = u32BitRate;
    pstRcAttr->stH265Vbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stH265Vbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stH265Vbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stH265Vbr.u32SrcFrameRateNum = u32SrcFrameRate;
    break;
  case VENC_RC_MODE_H265AVBR:
    pstRcAttr->stH265Avbr.u32Gop = u32Gop;
    pstRcAttr->stH265Avbr.u32MaxBitRate = u32BitRate;
    pstRcAttr->stH265Avbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stH265Avbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stH265Avbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stH265Avbr.u32SrcFrameRateNum = u32SrcFrameRate;
    break;
  case VENC_RC_MODE_MJPEGCBR:
    pstRcAttr->stMjpegCbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stMjpegCbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stMjpegCbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stMjpegCbr.u32SrcFrameRateNum = u32SrcFrameRate;
    pstRcAttr->stMjpegCbr.u32BitRate = u32BitRate;
    break;
  case VENC_RC_MODE_MJPEGVBR:
    pstRcAttr->stMjpegVbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stMjpegVbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stMjpegVbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stMjpegVbr.u32SrcFrameRateNum = u32SrcFrameRate;
    pstRcAttr->stMjpegVbr.u32BitRate = u32BitRate;
    break;
  case VENC_RC_MODE_H264CBR:
    pstRcAttr->stH264Cbr.u32Gop = u32Gop;
    pstRcAttr->stH264Cbr.u32BitRate = u32BitRate;
    pstRcAttr->stH264Cbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stH264Cbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stH264Cbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stH264Cbr.u32SrcFrameRateNum = u32SrcFrameRate;
    break;
  case VENC_RC_MODE_H264VBR:
    pstRcAttr->stH264Vbr.u32Gop = u32Gop;
    pstRcAttr->stH264Vbr.u32MaxBitRate = u32BitRate;
    pstRcAttr->stH264Vbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stH264Vbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stH264Vbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stH264Vbr.u32SrcFrameRateNum = u32SrcFrameRate;
    break;
  case VENC_RC_MODE_H264AVBR:
    pstRcAttr->stH264Avbr.u32Gop = u32Gop;
    pstRcAttr->stH264Avbr.u32MaxBitRate = u32BitRate;
    pstRcAttr->stH264Avbr.fr32DstFrameRateDen = 1;
    pstRcAttr->stH264Avbr.fr32DstFrameRateNum = u32DstFrameRate;
    pstRcAttr->stH264Avbr.u32SrcFrameRateDen = 1;
    pstRcAttr->stH264Avbr.u32SrcFrameRateNum = u32SrcFrameRate;
    break;
  default:
    RKADK_LOGE("Invalid rc mode: %d", pstRcAttr->enRcMode);
    return -1;
  }

  return 0;
}
