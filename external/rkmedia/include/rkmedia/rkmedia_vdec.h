// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RKMEDIA_VDEC_
#define __RKMEDIA_VDEC_
#ifdef __cplusplus
extern "C" {
#endif
#include "rkmedia_common.h"

typedef enum rkVIDEO_MODE_E {
  VIDEO_MODE_STREAM = 0, // send by stream
  VIDEO_MODE_FRAME,      // send by frame
  VIDEO_MODE_COMPAT, // Not Support now ! One Frame supports multiple packets
                     // sending.
  // The current frame is considered to end when bEndOfFrame is equal to RK_TRUE
  VIDEO_MODE_BUTT
} VIDEO_MODE_E;

typedef enum rkVIDEO_DECODEC_MODE_E {
  VIDEO_DECODEC_SOFTWARE = 0,
  VIDEO_DECODEC_HADRWARE,
} VIDEO_DECODEC_MODE_E;

typedef struct rkVDEC_ATTR_VIDEO_S {

} VDEC_ATTR_VIDEO_S;

typedef struct rkVDEC_CHN_ATTR_S {
  CODEC_TYPE_E enCodecType; // RW; video type to be decoded
  // IMAGE_TYPE_E enImageType;           // RW; image type to be outputed
  VIDEO_MODE_E enMode;                // RW; send by stream or by frame
  VIDEO_DECODEC_MODE_E enDecodecMode; // RW; hardware or software
  union {
    VDEC_ATTR_VIDEO_S stVdecVideoAttr; // RW; structure with video
  };
} VDEC_CHN_ATTR_S;

#ifdef __cplusplus
}
#endif
#endif // #ifndef __RKMEDIA_VDEC_
