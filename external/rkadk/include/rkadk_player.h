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

#ifndef __RKADK_PLAYER_H__
#define __RKADK_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rk_comm_vo.h"
#include "rkadk_common.h"

/** Error information of the player*/
typedef enum {
  RKADK_PLAYER_ERROR_VID_PLAY_FAIL = 0x0, /**< The video fails to be played. */
  RKADK_PLAYER_ERROR_AUD_PLAY_FAIL,       /**< The audio fails to be played. */
  RKADK_PLAYER_ERROR_DEMUX_FAIL,          /**< The file fails to be played. */
  RKADK_PLAYER_ERROR_TIMEOUT, /**< Operation timeout. For example, reading data
                                 timeout. */
  RKADK_PLAYER_ERROR_NOT_SUPPORT, /**< The file format is not supportted. */
  RKADK_PLAYER_ERROR_UNKNOW,      /**< Unknown error. */
  RKADK_PLAYER_ERROR_ILLEGAL_STATEACTION, /**< illegal action at cur state. */
  RKADK_PLAYER_ERROR_BUTT,
} RKADK_PLAYER_ERROR_E;

/** Player status*/
typedef enum {
  RKADK_PLAYER_STATE_IDLE = 0, /**< The player state before init . */
  RKADK_PLAYER_STATE_INIT, /**< The player is in the initial state. It changes
                              to the initial state after being SetDataSource. */
  RKADK_PLAYER_STATE_PREPARED, /**< The player is in the prepared state. */
  RKADK_PLAYER_STATE_PLAY,     /**< The player is in the playing state. */
  RKADK_PLAYER_STATE_TPLAY,    /**< The player is in the trick playing state*/
  RKADK_PLAYER_STATE_PAUSE,    /**< The player is in the pause state. */
  RKADK_PLAYER_STATE_ERR,      /**< The player is in the err state. */
  RKADK_PLAYER_STATE_BUTT
} RKADK_PLAYER_STATE_E;

typedef enum {
  RKADK_PLAYER_EVENT_STATE_CHANGED = 0x0, /**< the player status changed */
  RKADK_PLAYER_EVENT_PREPARED,
  RKADK_PLAYER_EVENT_STARTED,
  RKADK_PLAYER_EVENT_PAUSED,
  RKADK_PLAYER_EVENT_STOPPED,
  RKADK_PLAYER_EVENT_EOF, /**< the player is playing the end */
  RKADK_PLAYER_EVENT_SOF, /**< the player backward tplay to the start of file*/
  RKADK_PLAYER_EVENT_PROGRESS, /**< current playing progress. it will be called
                                  every one second. the additional value that in
                                  the unit of ms is current playing time */
  RKADK_PLAYER_EVENT_SEEK_END, /**< seek time jump, the additional value is the
                                  seek value */
  RKADK_PLAYER_EVENT_ERROR,    /**< play error */
  RKADK_PLAYER_EVENT_BUTT
} RKADK_PLAYER_EVENT_E;

typedef enum {
  VO_FORMAT_ARGB8888 = 0,
  VO_FORMAT_ABGR8888,
  VO_FORMAT_RGB888,
  VO_FORMAT_BGR888,
  VO_FORMAT_ARGB1555,
  VO_FORMAT_ABGR1555,
  VO_FORMAT_NV12,
  VO_FORMAT_NV21
} RKADK_PLAYER_VO_FORMAT_E;

typedef enum { VO_DEV_HD0 = 0, VO_DEV_HD1 } RKADK_PLAYER_VO_DEV_E;

typedef enum {
  DISPLAY_TYPE_HDMI = 0,
  DISPLAY_TYPE_EDP,
  DISPLAY_TYPE_VGA,
  DISPLAY_TYPE_MIPI,
} RKADK_PLAYER_VO_INTF_TYPE_E;

typedef enum {
  CHNN_ASPECT_RATIO_AUTO = 1,
  CHNN_ASPECT_RATIO_MANUAL,
} RKADK_PLAYER_VO_CHNN_MODE_E;
typedef RKADK_VOID (*RKADK_PLAYER_EVENT_FN)(RKADK_MW_PTR pPlayer,
                                            RKADK_PLAYER_EVENT_E enEvent,
                                            RKADK_VOID *pData);

/** player configuration */
typedef struct {
  RKADK_BOOL bEnableVideo;
  RKADK_BOOL bEnableAudio;
  RKADK_PLAYER_EVENT_FN pfnPlayerCallback;
} RKADK_PLAYER_CFG_S;

/** video output frameinfo */
typedef struct {
  RKADK_U32 u32FrmInfoS32x;
  RKADK_U32 u32FrmInfoS32y;
  RKADK_U32 u32DispWidth;
  RKADK_U32 u32DispHeight;
  RKADK_U32 u32ImgWidth;
  RKADK_U32 u32ImgHeight;
  RKADK_U32 u32VoLayerMode;
  RKADK_U32 u32ChnnNum;
  RKADK_U32 u32BorderColor;
  RKADK_U32 u32BorderTopWidth;
  RKADK_U32 u32BorderBottomWidth;
  RKADK_U32 u32BorderLeftWidth;
  RKADK_U32 u32BorderRightWidth;
  RKADK_PLAYER_VO_CHNN_MODE_E u32EnMode;
  RKADK_PLAYER_VO_FORMAT_E u32VoFormat;
  RKADK_PLAYER_VO_DEV_E u32VoDev;
  RKADK_PLAYER_VO_INTF_TYPE_E u32EnIntfType;
  RKADK_U32 u32DispFrmRt;
  VO_INTF_SYNC_E enIntfSync;
  VO_SYNC_INFO_S stSyncInfo;
} RKADK_PLAYER_FRAMEINFO_S;

/**
 * @brief create the player
 * @param[in] pstPlayCfg : player config
 * @param[out] ppPlayer : RKADK_MW_PTR*: handle of the player
 * @retval 0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Create(RKADK_MW_PTR *ppPlayer,
                              RKADK_PLAYER_CFG_S *pstPlayCfg);

/**
 * @brief  destroy the player
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 *  @retval 0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Destroy(RKADK_MW_PTR pPlayer);

/**
 * @brief    set the file for playing
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[in] filePath : RKADK_CHAR: media file path
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_SetDataSource(RKADK_MW_PTR pPlayer,
                                     const RKADK_CHAR *pszfilePath);

/**
 * @brief prepare for the playing
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Prepare(RKADK_MW_PTR pPlayer);

/**
 * @brief setcallback for the playing
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[in] frameinfo : RKADK_FRAMEINFO_S: record displayer info
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_SetVideoSink(RKADK_MW_PTR pPlayer,
                                    RKADK_PLAYER_FRAMEINFO_S *pstFrameInfo);

/**
 * @brief  do play of the stream
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Play(RKADK_MW_PTR pPlayer);

/**
 * @brief stop the stream playing, and release the resource
 * @param[in] pPlayer : void *: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Stop(RKADK_MW_PTR pPlayer);

/**
 * @brief pause the stream playing
 * @param[in] hPlayer : RKADK_MW_PTR: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Pause(RKADK_MW_PTR pPlayer);

/**
 * @brief seek by the time
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[in] s64TimeInMs : RKADK_S64: seek time
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Seek(RKADK_MW_PTR pPlayer, RKADK_S64 s64TimeInMs);

/**
 * @brief get the  current play status
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[out] penState : RKADK_LITEPLAYER_STATE_E*: play state
 *  @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_GetPlayStatus(RKADK_MW_PTR pPlayer,
                                     RKADK_PLAYER_STATE_E *penState);

#ifdef __cplusplus
}
#endif
#endif
