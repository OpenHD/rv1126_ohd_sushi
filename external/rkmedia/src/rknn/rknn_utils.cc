// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "media_type.h"
#include "rknn_utils.h"
#include "utils.h"

namespace easymedia {

#ifdef USE_ROCKFACE
static const struct RockfacePixelFmtEntry {
  rockface_pixel_format fmt;
  const char *fmt_str;
} rockface_pixel_fmt_string_map[] = {
    {ROCKFACE_PIXEL_FORMAT_GRAY8, IMAGE_GRAY8},
    {ROCKFACE_PIXEL_FORMAT_RGB888, IMAGE_BGR888},
    {ROCKFACE_PIXEL_FORMAT_BGR888, IMAGE_RGB888},
    {ROCKFACE_PIXEL_FORMAT_RGBA8888, IMAGE_ABGR8888},
    {ROCKFACE_PIXEL_FORMAT_BGRA8888, IMAGE_ARGB8888},
    {ROCKFACE_PIXEL_FORMAT_YUV420P_YU12, IMAGE_YUV420P},
    {ROCKFACE_PIXEL_FORMAT_YUV420P_YV12, IMAGE_YV12},
    {ROCKFACE_PIXEL_FORMAT_YUV420SP_NV12, IMAGE_NV12},
    {ROCKFACE_PIXEL_FORMAT_YUV420SP_NV21, IMAGE_NV21},
    {ROCKFACE_PIXEL_FORMAT_YUV422P_YU16, IMAGE_UYVY422},
    {ROCKFACE_PIXEL_FORMAT_YUV422P_YV16, IMAGE_YV16},
    {ROCKFACE_PIXEL_FORMAT_YUV422SP_NV16, IMAGE_NV16},
    {ROCKFACE_PIXEL_FORMAT_YUV422SP_NV61, IMAGE_NV61}};

rockface_pixel_format StrToRockFacePixelFMT(const char *fmt_str) {
  FIND_ENTRY_TARGET_BY_STRCMP(fmt_str, rockface_pixel_fmt_string_map, fmt_str,
                              fmt)
  return ROCKFACE_PIXEL_FORMAT_MAX;
}
#endif // USE_ROCKFACE

#ifdef USE_ROCKX
static const struct RockxPixelFmtEntry {
  rockx_pixel_format fmt;
  const char *fmt_str;
} rockx_pixel_fmt_string_map[] = {
    {ROCKX_PIXEL_FORMAT_GRAY8, IMAGE_GRAY8},
    {ROCKX_PIXEL_FORMAT_RGB888, IMAGE_BGR888},
    {ROCKX_PIXEL_FORMAT_BGR888, IMAGE_RGB888},
    {ROCKX_PIXEL_FORMAT_RGBA8888, IMAGE_ABGR8888},
    {ROCKX_PIXEL_FORMAT_BGRA8888, IMAGE_ARGB8888},
    {ROCKX_PIXEL_FORMAT_YUV420P_YU12, IMAGE_YUV420P},
    {ROCKX_PIXEL_FORMAT_YUV420P_YV12, IMAGE_YV12},
    {ROCKX_PIXEL_FORMAT_YUV420SP_NV12, IMAGE_NV12},
    {ROCKX_PIXEL_FORMAT_YUV420SP_NV21, IMAGE_NV21},
    {ROCKX_PIXEL_FORMAT_YUV422P_YU16, IMAGE_UYVY422},
    {ROCKX_PIXEL_FORMAT_YUV422P_YV16, IMAGE_YV16},
    {ROCKX_PIXEL_FORMAT_YUV422SP_NV16, IMAGE_NV16},
    {ROCKX_PIXEL_FORMAT_YUV422SP_NV61, IMAGE_NV61},
    {ROCKX_PIXEL_FORMAT_GRAY16, IMAGE_GRAY16}};

rockx_pixel_format StrToRockxPixelFMT(const char *fmt_str) {
  FIND_ENTRY_TARGET_BY_STRCMP(fmt_str, rockx_pixel_fmt_string_map, fmt_str, fmt)
  return ROCKX_PIXEL_FORMAT_MAX;
}
#endif // USE_ROCKX

} // namespace easymedia
