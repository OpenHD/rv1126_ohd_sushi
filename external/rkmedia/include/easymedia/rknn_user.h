// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_RKNN_USER_H_
#define EASYMEDIA_RKNN_USER_H_

#include <type_traits>

#ifdef USE_ROCKFACE
#include "rockface/rockface.h"
#endif
#ifdef USE_ROCKX
#include <rockx/rockx.h>
#endif

#define RKNN_PICTURE_PATH_LEN (512)
#define RKMEDIA_ROCKX_LANDMARK_MAX_COUNT (128)

using RknnCallBack =
    std::add_pointer<void(void *handler, int type, void *ptr, int size)>::type;
using RknnHandler = std::add_pointer<void *>::type;

typedef struct Rect {
  int left;
  int top;
  int right;
  int bottom;
} Rect;

typedef struct {
  int x;
  int y;
} RkmediaPoint;

typedef enum {
  FACE_REG_NONE = -1,
  FACE_REG_RECOGNIZE = 0,
  FACE_REG_REGISTER,
} FaceRegType;

/* recognize_type: -1, Unknow; register_type: -1, register repeaded, -99
 * register failed */
typedef struct {
  FaceRegType type;
  int user_id;
  float similarity;
  char pic_path[RKNN_PICTURE_PATH_LEN];
} FaceReg;

typedef struct {
  int cls_idx;
  float score;
  Rect box;
} RkmediaRockxObject;

typedef struct {
  int image_width;
  int image_height;
  Rect face_box;
  int landmarks_count;
  RkmediaPoint landmarks[RKMEDIA_ROCKX_LANDMARK_MAX_COUNT];
  float score;
} RkmediaRockxFaceLandmark;

typedef struct {
  int count;
  RkmediaPoint points[32];
  float score[32];
} RkmediaRockxKeypoints;

typedef struct {
#ifdef USE_ROCKFACE
  rockface_det_t base;
  rockface_attribute_t attr;
  rockface_landmark_t landmark;
  rockface_angle_t angle;
  rockface_feature_t feature;
#endif
  RkmediaRockxObject object;
  FaceReg face_reg;
} FaceInfo;

typedef struct {
  RkmediaRockxFaceLandmark object;
} LandmarkInfo;

typedef struct {
#ifdef USE_ROCKFACE
  rockface_det_t base;
#endif
  RkmediaRockxKeypoints object;
} BodyInfo;

typedef struct {
  RkmediaRockxKeypoints object;
} FingerInfo;

typedef enum {
  SUCCESS = 0,
  FAILURE,
  TIMEOUT,
  UNKNOW,
} AuthorizedStatus;

typedef enum {
  NNRESULT_TYPE_NONE = -1,
  NNRESULT_TYPE_FACE = 0,
  NNRESULT_TYPE_FACE_PICTURE_UPLOAD,
  NNRESULT_TYPE_BODY,
  NNRESULT_TYPE_FINGER,
  NNRESULT_TYPE_LANDMARK,
  NNRESULT_TYPE_AUTHORIZED_STATUS,
  NNRESULT_TYPE_FACE_REG,
  NNRESULT_TYPE_OBJECT_DETECT,
} RknnResultType;

typedef struct {
  int img_w;
  int img_h;
  int64_t timeval;
  RknnResultType type;
  AuthorizedStatus status;
  union {
    BodyInfo body_info;
    FaceInfo face_info;
    LandmarkInfo landmark_info;
    FingerInfo finger_info;
    RkmediaRockxObject object_info;
  };
} RknnResult;

#endif // #ifndef EASYMEDIA_RKNN_USER_H_
