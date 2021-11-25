// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_VI_
#define __RKMEDIA_VI_
#ifdef __cplusplus
extern "C" {
#endif
#include "rkmedia_common.h"

typedef enum rkVI_CHN_WORK_MODE {
  VI_WORK_MODE_NORMAL = 0,
  // for vi single caculate luma.
  // In this mode, vi has no output,
  // and data cannot be obtained from vi.
  VI_WORK_MODE_LUMA_ONLY,
  VI_WORK_MODE_BUTT
} VI_CHN_WORK_MODE;

typedef enum rkVI_CHN_BUF_TYPE {
  VI_CHN_BUF_TYPE_DMA = 0, // Default
  VI_CHN_BUF_TYPE_MMAP,
} VI_CHN_BUF_TYPE;

typedef struct rkVI_CHN_ATTR_S {
  const RK_CHAR *pcVideoNode;
  RK_U32 u32Width;
  RK_U32 u32Height;
  IMAGE_TYPE_E enPixFmt;
  RK_U32 u32BufCnt;          // VI capture video buffer cnt.
  VI_CHN_BUF_TYPE enBufType; // VI capture video buffer type.
  VI_CHN_WORK_MODE enWorkMode;
} VI_CHN_ATTR_S;

typedef enum rkVP_CHN_WORK_MODE {
  VP_WORK_MODE_NORMAL = 0,
  VP_WORK_MODE_BUTT
} VP_CHN_WORK_MODE;

typedef enum rkVP_CHN_BUF_TYPE {
  VP_CHN_BUF_TYPE_DMA = 0, // Default
  VP_CHN_BUF_TYPE_MMAP,
} VP_CHN_BUF_TYPE;

typedef struct rkVP_CHN_ATTR_S {
  const RK_CHAR *pcVideoNode;
  RK_U32 u32Width;
  RK_U32 u32Height;
  IMAGE_TYPE_E enPixFmt;
  RK_U32 u32BufCnt;          // VP output video buffer cnt.
  VP_CHN_BUF_TYPE enBufType; // VP output video buffer type.
  VP_CHN_WORK_MODE enWorkMode;
} VP_CHN_ATTR_S;

typedef struct rkVIDEO_REGION_INFO_S {
  RK_U32 u32RegionNum; /* count of the region */
  RECT_S *pstRegion;   /* region attribute */
} VIDEO_REGION_INFO_S;

typedef struct rkVI_USERPIC_ATTR_S {
  IMAGE_TYPE_E enPixFmt;
  RK_U32 u32Width;
  RK_U32 u32Height;
  RK_VOID *pvPicPtr;
} VI_USERPIC_ATTR_S;

#ifdef __cplusplus
}
#endif
#endif // #ifndef __RKMEDIA_VI_
