// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_MOVE_DETECTION_
#define __RKMEDIA_MOVE_DETECTION_

#ifdef __cplusplus
extern "C" {
#endif

#include "rkmedia_common.h"

#define ALGO_MD_ROI_RET_MAX 4096

typedef struct rkMD_EVENT_S {
  RK_U16 u16Cnt;
  RK_U32 u32Width;
  RK_U32 u32Height;
  RECT_S stRects[ALGO_MD_ROI_RET_MAX];
} MD_EVENT_S;

/* the attribute of the Venc*/
typedef struct rkALGO_MD_ATTR_S {
  IMAGE_TYPE_E imageType; // the type of input image
  RK_U32 u32Width;
  RK_U32 u32Height;
  RK_U16 u16RoiCnt; // RW; Range:[0, ALGO_MD_ROI_RET_MAX].
  RECT_S stRoiRects[ALGO_MD_ROI_RET_MAX];
  RK_U16 u16Sensitivity; // value 0(sys default) or [1 - 100].
} ALGO_MD_ATTR_S;

#ifdef __cplusplus
}
#endif

#endif // #ifndef __RKMEDIA_MOVE_DETECTION
