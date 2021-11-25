// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_RGA_H_
#define EASYMEDIA_RGA_H_

#include "buffer.h"
#include "filter.h"
#include "image.h"

#include <rga/RockchipRga.h>

namespace easymedia {

class ImageBuffer;

typedef enum { FLIP_NULL, FLIP_H, FLIP_V, FLIP_HV } FlipEnum;

class RgaFilter : public Filter {
public:
  RgaFilter(const char *param);
  virtual ~RgaFilter();
  static const char *GetFilterName() { return "rkrga"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;

  void SetRects(std::vector<ImageRect> vec_rect);
  static RockchipRga gRkRga;
  virtual int IoCtrl(unsigned long int request _UNUSED, ...) override;

private:
  // After initialization, these parameters cannot be changed.
  // Rect must be within the maximum range.
  unsigned int src_max_width;
  unsigned int src_max_height;
  unsigned int dst_max_width;
  unsigned int dst_max_height;
  ConditionLockMutex param_mtx;
  std::vector<ImageRect> vec_rect;
  int rotate;
  FlipEnum flip;
  std::vector<ImageBorder> lines;
  int hide;
  std::map<int, OsdInfo> osds;
  std::mutex luma_mutex;
  std::condition_variable luma_cond;
  ImageRegionLuma region_luma;
};

int rga_blit(std::shared_ptr<ImageBuffer> src, std::shared_ptr<ImageBuffer> dst,
             std::vector<ImageBorder> &lines, std::map<int, OsdInfo> osds,
             ImageRegionLuma *region_luma, ImageRect *src_rect = nullptr,
             ImageRect *dst_rect = nullptr, int rotate = 0,
             FlipEnum flip = FLIP_NULL, int hide = 0);

int get_rga_format(PixelFormat f);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_RGA_H_
