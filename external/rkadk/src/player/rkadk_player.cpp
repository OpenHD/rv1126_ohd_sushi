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

#include "rkadk_player.h"
#include "RTMediaPlayer.h"
#include "RTPlayerDef.h"
#include "rkadk_surface_interface.h"
#include "rt_message.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  RKADK_BOOL bEnableVideo;
  RKADKSurfaceInterface *pSurface;
  RTMediaPlayer *pMediaPlayer;
  void *pListener;
} RKADK_PLAYER_HANDLE_S;

static RKADK_PLAYER_EVENT_FN g_pfnPlayerCallback = NULL;
static RTMediaPlayer *g_pPlayer = NULL;
static pthread_t g_thread = 0;
static bool g_getPosition = false;

void *GetPlayPositionThread(void *para) {
  RKADK_S64 position; // us

  while (g_getPosition) {
    if (!g_pPlayer)
      break;

    g_pPlayer->getCurrentPosition(&position);
    if (g_pfnPlayerCallback) {
      position = position / 1000; // ms
      g_pfnPlayerCallback((RKADK_MW_PTR)g_pPlayer, RKADK_PLAYER_EVENT_PROGRESS,
                          (void *)&position);
    }

    sleep(1);
  }

  RKADK_LOGD("Exit Thread");
  return NULL;
}

static int CreateGetPosThread() {
  if (!g_thread) {
    g_getPosition = true;
    if (pthread_create(&g_thread, NULL, GetPlayPositionThread, NULL)) {
      RKADK_LOGE("Create get play position thread failed");
      return -1;
    }
  }

  return 0;
}

static void DestoryGetPosThread() {
  int ret;

  g_getPosition = false;
  if (g_thread) {
    ret = pthread_join(g_thread, NULL);
    if (ret) {
      RKADK_LOGE("thread exit failed = %d", ret);
    } else {
      RKADK_LOGI("thread exit successed");
    }
    g_thread = 0;
  }
}

class RKADKPlayerListener : public RTPlayerListener {
public:
  ~RKADKPlayerListener() { RKADK_LOGD("done"); }
  virtual void notify(INT32 msg, INT32 ext1, INT32 ext2, void *ptr) {
    RKADK_PLAYER_EVENT_E event = RKADK_PLAYER_EVENT_BUTT;

    RKADK_LOGI("msg: %d, ext1: %d", msg, ext1);
    switch (msg) {
    case RT_MEDIA_PREPARED:
      event = RKADK_PLAYER_EVENT_PREPARED;
      break;
    case RT_MEDIA_STARTED:
      event = RKADK_PLAYER_EVENT_STARTED;
      CreateGetPosThread();
      break;
    case RT_MEDIA_PAUSED:
      event = RKADK_PLAYER_EVENT_PAUSED;
      break;
    case RT_MEDIA_STOPPED:
      DestoryGetPosThread();
      event = RKADK_PLAYER_EVENT_STOPPED;
      break;
    case RT_MEDIA_PLAYBACK_COMPLETE:
      DestoryGetPosThread();
      event = RKADK_PLAYER_EVENT_EOF;
      break;
    case RT_MEDIA_BUFFERING_UPDATE:
      break;
    case RT_MEDIA_SEEK_COMPLETE:
      event = RKADK_PLAYER_EVENT_SEEK_END;
      break;
    case RT_MEDIA_ERROR:
      DestoryGetPosThread();
      event = RKADK_PLAYER_EVENT_ERROR;
      break;
    case RT_MEDIA_SET_VIDEO_SIZE:
    case RT_MEDIA_SKIPPED:
    case RT_MEDIA_TIMED_TEXT:
    case RT_MEDIA_INFO:
    case RT_MEDIA_SUBTITLE_DATA:
    case RT_MEDIA_SEEK_ASYNC:
      break;
    default:
      RKADK_LOGI("Unknown event: %d", msg);
    }

    if (event == RKADK_PLAYER_EVENT_BUTT)
      return;

    if (g_pfnPlayerCallback)
      g_pfnPlayerCallback((RKADK_MW_PTR)g_pPlayer, event, NULL);
  }
};

RKADK_S32 RKADK_PLAYER_Create(RKADK_MW_PTR *ppPlayer,
                              RKADK_PLAYER_CFG_S *pstPlayCfg) {
  int ret;
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;
  RKADKPlayerListener *pListener = NULL;

  RKADK_CHECK_POINTER(pstPlayCfg, RKADK_FAILURE);
  g_pfnPlayerCallback = pstPlayCfg->pfnPlayerCallback;

  if (*ppPlayer) {
    RKADK_LOGE("player has been created");
    return -1;
  }

  RKADK_LOGI("Create Player[%d, %d] Start...", pstPlayCfg->bEnableVideo,
             pstPlayCfg->bEnableAudio);

  pstPlayer = (RKADK_PLAYER_HANDLE_S *)malloc(sizeof(RKADK_PLAYER_HANDLE_S));
  if (!pstPlayer) {
    RKADK_LOGE("malloc pstPlayer failed");
    return -1;
  }
  pstPlayer->bEnableVideo = pstPlayCfg->bEnableVideo;
  pstPlayer->pSurface = NULL;

  pstPlayer->pMediaPlayer = new RTMediaPlayer();
  if (!pstPlayer->pMediaPlayer) {
    RKADK_LOGE("Create RTMediaPlayer failed");
    goto failed;
  }
  g_pPlayer = pstPlayer->pMediaPlayer;

  pListener = new RKADKPlayerListener();
  if (!pListener) {
    RKADK_LOGE("Create RKADKPlayerListener failed");
    goto failed;
  }

  ret = pstPlayer->pMediaPlayer->setListener(pListener);
  if (ret) {
    RKADK_LOGE("setListener failed = %d", ret);
    goto failed;
  }
  pstPlayer->pListener = (void *)pListener;

  *ppPlayer = (RKADK_MW_PTR)pstPlayer;

  RKADK_LOGI("Create Player[%d, %d] End...", pstPlayCfg->bEnableVideo,
             pstPlayCfg->bEnableAudio);
  return 0;

failed:
  RKADK_LOGI("Create Player[%d, %d] failed...", pstPlayCfg->bEnableVideo,
             pstPlayCfg->bEnableAudio);

  if (pListener)
    delete pListener;

  if (pstPlayer->pMediaPlayer)
    delete pstPlayer->pMediaPlayer;

  if (pstPlayer)
    free(pstPlayer);

  return -1;
}

RKADK_S32 RKADK_PLAYER_Destroy(RKADK_MW_PTR pPlayer) {
  int ret;
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstPlayer->pListener, RKADK_FAILURE);

  RKADK_LOGI("Destory Player Start...");

  ret = pstPlayer->pMediaPlayer->stop();
  if (ret) {
    RKADK_LOGE("RTMediaPlayer stop failed(%d)", ret);
    return ret;
  }

  if (pstPlayer->bEnableVideo) {
    RKADK_CHECK_POINTER(pstPlayer->pSurface, RKADK_FAILURE);
    pstPlayer->pSurface->replay();
  }

  pstPlayer->pMediaPlayer->reset();
  delete pstPlayer->pMediaPlayer;
  g_pPlayer = NULL;

  if (pstPlayer->bEnableVideo)
    delete pstPlayer->pSurface;

  delete ((RKADKPlayerListener *)pstPlayer->pListener);
  free(pPlayer);

  RKADK_LOGI("Destory Player End...");
  return 0;
}

RKADK_S32 RKADK_PLAYER_SetDataSource(RKADK_MW_PTR pPlayer,
                                     const RKADK_CHAR *pszfilePath) {
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pszfilePath, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);

  return pstPlayer->pMediaPlayer->setDataSource(pszfilePath, NULL);
}

RKADK_S32 RKADK_PLAYER_Prepare(RKADK_MW_PTR pPlayer) {
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);

  return pstPlayer->pMediaPlayer->prepare();
}

RKADK_S32 RKADK_PLAYER_SetVideoSink(RKADK_MW_PTR pPlayer,
                                    RKADK_PLAYER_FRAMEINFO_S *pstFrameInfo) {
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pstFrameInfo, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);

  if (!pstPlayer->bEnableVideo) {
    RKADK_LOGE("don't enable video, bEnableVideo = %d",
               pstPlayer->bEnableVideo);
    return -1;
  }

  pstPlayer->pSurface = new RKADKSurfaceInterface(pstFrameInfo);
  if (!pstPlayer->pSurface) {
    RKADK_LOGE("RKADKSurfaceInterface failed");
    return -1;
  }

  return pstPlayer->pMediaPlayer->setVideoSink(
      static_cast<const void *>(pstPlayer->pSurface));
}

RKADK_S32 RKADK_PLAYER_Play(RKADK_MW_PTR pPlayer) {
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);

  return pstPlayer->pMediaPlayer->start();
}

RKADK_S32 RKADK_PLAYER_Stop(RKADK_MW_PTR pPlayer) {
  int ret;
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);

  ret = pstPlayer->pMediaPlayer->stop();
  if (ret) {
    RKADK_LOGE("RTMediaPlayer stop failed(%d)", ret);
    return ret;
  }

  if (pstPlayer->bEnableVideo) {
    RKADK_CHECK_POINTER(pstPlayer->pSurface, RKADK_FAILURE);
    pstPlayer->pSurface->replay();
  }

  return pstPlayer->pMediaPlayer->reset();
}

RKADK_S32 RKADK_PLAYER_Pause(RKADK_MW_PTR pPlayer) {
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);

  return pstPlayer->pMediaPlayer->pause();
}

RKADK_S32 RKADK_PLAYER_Seek(RKADK_MW_PTR pPlayer, RKADK_S64 s64TimeInMs) {
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);

  return pstPlayer->pMediaPlayer->seekTo(s64TimeInMs * 1000);
}

RKADK_S32 RKADK_PLAYER_GetPlayStatus(RKADK_MW_PTR pPlayer,
                                     RKADK_PLAYER_STATE_E *penState) {
  RKADK_U32 state;
  RKADK_PLAYER_STATE_E enState = RKADK_PLAYER_STATE_BUTT;
  RKADK_PLAYER_HANDLE_S *pstPlayer = NULL;

  RKADK_CHECK_POINTER(pPlayer, RKADK_FAILURE);
  pstPlayer = (RKADK_PLAYER_HANDLE_S *)pPlayer;
  RKADK_CHECK_POINTER(pstPlayer->pMediaPlayer, RKADK_FAILURE);
  RKADK_CHECK_POINTER(penState, RKADK_FAILURE);

  state = pstPlayer->pMediaPlayer->getState();
  switch (state) {
  case RT_STATE_IDLE:
    enState = RKADK_PLAYER_STATE_IDLE;
    break;
  case RT_STATE_INITIALIZED:
    enState = RKADK_PLAYER_STATE_INIT;
    break;
  case RT_STATE_PREPARED:
    enState = RKADK_PLAYER_STATE_PREPARED;
    break;
  case RT_STATE_STARTED:
    enState = RKADK_PLAYER_STATE_PLAY;
    break;
  case RT_STATE_PAUSED:
    enState = RKADK_PLAYER_STATE_PAUSE;
    break;
  case RT_STATE_ERROR:
    enState = RKADK_PLAYER_STATE_ERR;
    break;
  case RT_STATE_PREPARING:
  case RT_STATE_STOPPED:
  case RT_STATE_COMPLETE:
    break;
  default:
    RKADK_LOGE("UnKnown play state = %d", state);
    break;
  }

  *penState = enState;
  return 0;
}
