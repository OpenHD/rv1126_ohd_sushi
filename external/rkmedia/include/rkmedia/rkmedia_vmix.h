// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_VIDEO_MIX_
#define __RKMEDIA_VIDEO_MIX_

#ifdef __cplusplus
extern "C" {
#endif

#include "rkmedia_common.h"

#define VMIX_MAX_CHN_NUM 16
#define VMIX_MAX_DEV_NUM 16
#define VMIX_MAX_LINE_NUM 64

typedef struct rkVMIX_CHN_INFO_S {
  IMAGE_TYPE_E enImgInType;
  IMAGE_TYPE_E enImgOutType;
  RECT_S stInRect;
  RECT_S stOutRect;
} VMIX_CHN_INFO_S;

typedef struct rkVMIX_DEV_INFO_S {
  RK_U16 u16ChnCnt;
  RK_U16 u16Fps;
  RK_U32 u32ImgWidth;
  RK_U32 u32ImgHeight;
  IMAGE_TYPE_E enImgType;
  RK_BOOL bEnBufPool;
  RK_U16 u16BufPoolCnt;
  VMIX_CHN_INFO_S stChnInfo[VMIX_MAX_CHN_NUM];
} VMIX_DEV_INFO_S;

typedef struct rkVMIX_LINE_INFO_S {
  RK_U32 u32LineCnt;
  RK_U32 u32Color;
  RECT_S stLines[VMIX_MAX_LINE_NUM];
  RK_U8 u8Enable[VMIX_MAX_LINE_NUM];
} VMIX_LINE_INFO_S;

#ifdef __cplusplus
}
#endif

#endif // #ifndef __RKMEDIA_VIDEO_MIX_
