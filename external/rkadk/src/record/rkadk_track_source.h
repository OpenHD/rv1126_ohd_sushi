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

#ifndef __RKADK_TRACK_SOURCE_H__
#define __RKADK_TRACK_SOURCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rkadk_common.h"
#include "rkmedia_common.h"

typedef enum {
  RKADK_TRACK_SOURCE_TYPE_VIDEO = 0,
  RKADK_TRACK_SOURCE_TYPE_AUDIO,
  RKADK_TRACK_SOURCE_TYPE_BUTT
} RKADK_TRACK_SOURCE_TYPE_E;

typedef struct {
  CODEC_TYPE_E enCodecType;
  IMAGE_TYPE_E enImageType;
  RKADK_U32 u32Width;
  RKADK_U32 u32Height;
  RKADK_U32 u32BitRate;
  RKADK_U32 u32FrameRate;
  RKADK_U16 u16Profile;
  RKADK_U16 u16Level;
} RKADK_TRACK_VIDEO_SOURCE_INFO_S;

typedef struct {
  CODEC_TYPE_E enCodecType;
  SAMPLE_FORMAT_E enSampFmt;
  RKADK_U32 u32ChnCnt;
  RKADK_U32 u32SampleRate;
  RKADK_U32 u32SamplesPerFrame;
} RKADK_TRACK_AUDIO_SOURCE_INFO_S;

typedef struct {
  RKADK_S32 s32PrivateHandle; /* venc or aenc handle */

  RKADK_TRACK_SOURCE_TYPE_E enTrackType;
  union {
    RKADK_TRACK_VIDEO_SOURCE_INFO_S stVideoInfo; /* <video track info */
    RKADK_TRACK_AUDIO_SOURCE_INFO_S stAudioInfo; /* <audio track info */
  } unTrackSourceAttr;
} RKADK_TRACK_SOURCE_S;

#ifdef __cplusplus
}
#endif
#endif