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

#ifndef __RKADK_SURFACE_INTERFACE_H__
#define __RKADK_SURFACE_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "RTSurfaceInterface.h"
#include "rkadk_common.h"
#include "rkadk_player.h"

typedef RKADK_PLAYER_FRAMEINFO_S VIDEO_FRAMEINFO_S;

#define MAX_BUFFER_NUM 3

class RKADKSurfaceInterface : RTSurfaceInterface {
public:
  RKADKSurfaceInterface(VIDEO_FRAMEINFO_S *pstFrmInfo);
  ~RKADKSurfaceInterface() { RKADK_LOGD("done"); }

  INT32 connect(INT32 mode) { return 0; }
  INT32 disconnect(INT32 mode) { return 0; }

  INT32 allocateBuffer(RTNativeWindowBufferInfo *info) { return 0; }
  INT32 freeBuffer(void *buf, INT32 fence) { return 0; }
  INT32 remainBuffer(void *buf, INT32 fence) { return 0; }
  INT32 queueBuffer(void *buf, INT32 fence);
  INT32 dequeueBuffer(void **buf) { return 0; }
  INT32 dequeueBufferAndWait(RTNativeWindowBufferInfo *info) { return 0; }
  INT32 mmapBuffer(RTNativeWindowBufferInfo *info, void **ptr) { return 0; }
  INT32 munmapBuffer(void **ptr, INT32 size, void *buf) { return 0; }

  INT32 setCrop(INT32 left, INT32 top, INT32 right, INT32 bottom) { return 0; }
  INT32 setUsage(INT32 usage) { return 0; }
  INT32 setScalingMode(INT32 mode) { return 0; }
  INT32 setDataSpace(INT32 dataSpace) { return 0; }
  INT32 setTransform(INT32 transform) { return 0; }
  INT32 setSwapInterval(INT32 interval) { return 0; }
  INT32 setBufferCount(INT32 bufferCount) { return 0; }
  INT32 setBufferGeometry(INT32 width, INT32 height, INT32 format) { return 0; }
  INT32 setSidebandStream(RTSidebandInfo info) { return 0; }

  INT32 query(INT32 cmd, INT32 *param) { return 0; }
  void *getNativeWindow() { return NULL; }
  void replay();

private:
  void *pCbMblk;
  INT32 s32Flag;
  VIDEO_FRAMEINFO_S stFrmInfo;
};

#ifdef __cplusplus
}
#endif
#endif