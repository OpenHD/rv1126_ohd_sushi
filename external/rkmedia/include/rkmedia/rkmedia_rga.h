// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_RGA_
#define __RKMEDIA_RGA_

#ifdef __cplusplus
extern "C" {
#endif

#include "rkmedia_common.h"

typedef struct rkRGA_INFO_S {
  IMAGE_TYPE_E imgType;
  RK_U32 u32X;
  RK_U32 u32Y;
  RK_U32 u32Width;
  RK_U32 u32Height;
  RK_U32 u32HorStride; // horizontal stride
  RK_U32 u32VirStride; // virtual stride
} RGA_INFO_S;

typedef enum rkRGA_FLIP_E {
  RGA_FLIP_NULL,
  RGA_FLIP_H,
  RGA_FLIP_V,
  RGA_FLIP_HV,
} RGA_FLIP_E;

typedef struct rkRGA_ATTR_S {
  RGA_INFO_S stImgIn;  // input image info
  RGA_INFO_S stImgOut; // output image info
  RK_U16 u16Rotaion;   // support 0/90/180/270.
  RK_BOOL bEnBufPool;
  RK_U16 u16BufPoolCnt;
  RGA_FLIP_E enFlip;
} RGA_ATTR_S;

#ifdef __cplusplus
}
#endif

#endif // #ifndef __RKMEDIA_RGA
