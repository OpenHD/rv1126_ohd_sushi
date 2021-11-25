// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "rga_filter.h"

#include <assert.h>

#include <vector>

#include "buffer.h"
#include "filter.h"
#include "media_config.h"

#include <rga/im2d.h>
#include <rga/rga.h>

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_RGA

namespace easymedia {

RockchipRga RgaFilter::gRkRga;

static int rga_rect_check(ImageRect *rect, int max_w, int max_h) {
  if (!rect)
    return -1;
  // check x && w
  if ((rect->x < 0) || (rect->w < 0) ||
      ((max_w > 0) && ((rect->x + rect->w) > max_w)))
    return -1;

  // check y && h
  if ((rect->y < 0) || (rect->h < 0) ||
      ((max_h > 0) && ((rect->y + rect->h) > max_h)))
    return -1;

  return 0;
}

RgaFilter::RgaFilter(const char *param) : rotate(0), flip(FLIP_NULL), hide(0) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  const std::string &value = params[KEY_BUFFER_RECT];
  auto &&rects = StringToTwoImageRect(value);
  if (rects.empty()) {
    RKMEDIA_LOGE("Missing src and dst rects\n");
    SetError(-EINVAL);
    return;
  }
  vec_rect = std::move(rects);

  if (rga_rect_check(&vec_rect[0], 0, 0) ||
      rga_rect_check(&vec_rect[1], 0, 0)) {
    RKMEDIA_LOGE("Invalid src rect:<%d,%d,%d,%d> or dst rect:<%d,%d,%d,%d>\n",
                 vec_rect[0].x, vec_rect[0].y, vec_rect[0].w, vec_rect[0].h,
                 vec_rect[1].x, vec_rect[1].y, vec_rect[1].w, vec_rect[1].h);
    SetError(-EINVAL);
    return;
  }

  // The initialized rectangular frame is the maximum range,
  // and subsequent dynamic modification of the rectangular
  // frame must be within this range.
  src_max_width = vec_rect[0].w;
  src_max_height = vec_rect[0].h;
  dst_max_width = vec_rect[1].w;
  dst_max_height = vec_rect[1].h;

  const std::string &v = params[KEY_BUFFER_ROTATE];
  if (!v.empty())
    rotate = std::stoi(v);
  const std::string &f = params[KEY_BUFFER_FLIP];
  if (!f.empty())
    flip = (FlipEnum)std::stoi(f);
  memset(&region_luma, 0, sizeof(ImageRegionLuma));
}

void RgaFilter::SetRects(std::vector<ImageRect> rects) {
  vec_rect = std::move(rects);
}

int RgaFilter::Process(std::shared_ptr<MediaBuffer> input,
                       std::shared_ptr<MediaBuffer> &output) {
  if (vec_rect.size() < 2)
    return -EINVAL;
  if (!input || input->GetType() != Type::Image)
    return -EINVAL;
  if (!output || output->GetType() != Type::Image)
    return -EINVAL;

  auto src = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  auto dst = std::static_pointer_cast<easymedia::ImageBuffer>(output);
  if (!src->IsValid() || !dst->IsValid()) {
    RKMEDIA_LOGE("Src(%zuBytes) or Dst(%zuBytes) Buffer is invalid!\n",
                 src->GetValidSize(), dst->GetValidSize());
    return -EINVAL;
  }

  ImageRect src_rect;
  ImageRect dst_rect;
  int cur_rotate = 0;
  FlipEnum cur_flip = FLIP_NULL;
  int cur_hide = 0;

  memset(&src_rect, 0, sizeof(ImageRect));
  memset(&dst_rect, 0, sizeof(ImageRect));
  param_mtx.lock();
  src_rect = vec_rect[0];
  dst_rect = vec_rect[1];
  cur_rotate = rotate;
  cur_flip = flip;
  std::vector<ImageBorder> cur_lines = lines;
  cur_hide = hide;
  std::map<int, OsdInfo> cur_osds = osds;
  ImageRegionLuma cur_region_luma;
  memcpy(&cur_region_luma, &region_luma, sizeof(ImageRegionLuma));
  param_mtx.unlock();

  if ((!dst_rect.w || !dst_rect.h) && !dst->IsValid()) {
    // the same to src
    ImageInfo info = src->GetImageInfo();
    info.pix_fmt = dst->GetPixelFormat();
    size_t size =
        CalPixFmtSize(info.pix_fmt, info.vir_width, info.vir_height, 16);
    if (size == 0)
      return -EINVAL;
    auto &&mb = MediaBuffer::Alloc2(size, MediaBuffer::MemType::MEM_HARD_WARE);
    ImageBuffer ib(mb, info);
    if (ib.GetSize() >= size) {
      ib.SetValidSize(size);
      *dst.get() = ib;
    }
    assert(dst->IsValid());
  }
  int ret = rga_blit(src, dst, cur_lines, cur_osds, &cur_region_luma, &src_rect,
                     &dst_rect, cur_rotate, cur_flip, cur_hide);
  if (cur_region_luma.region_num > 0) {
    param_mtx.lock();
    memcpy(&region_luma, &cur_region_luma, sizeof(ImageRegionLuma));
    param_mtx.unlock();
    luma_mutex.lock();
    luma_cond.notify_all();
    luma_mutex.unlock();
  }
  return ret;
}
RgaFilter::~RgaFilter() {}
int get_rga_format(PixelFormat f) {
  static std::map<PixelFormat, int> rga_format_map = {
      {PIX_FMT_YUV420P, RK_FORMAT_YCbCr_420_P},
      {PIX_FMT_NV12, RK_FORMAT_YCbCr_420_SP},
      {PIX_FMT_NV21, RK_FORMAT_YCrCb_420_SP},
      {PIX_FMT_YUV422P, RK_FORMAT_YCbCr_422_P},
      {PIX_FMT_NV16, RK_FORMAT_YCbCr_422_SP},
      {PIX_FMT_NV61, RK_FORMAT_YCrCb_422_SP},
      {PIX_FMT_YUYV422, RK_FORMAT_YUYV_422},
      {PIX_FMT_UYVY422, RK_FORMAT_UYVY_422},
      {PIX_FMT_RGB565, RK_FORMAT_RGB_565},
      {PIX_FMT_BGR565, -1},
      {PIX_FMT_RGB888, RK_FORMAT_BGR_888},
      {PIX_FMT_BGR888, RK_FORMAT_RGB_888},
      {PIX_FMT_ARGB8888, RK_FORMAT_BGRA_8888},
      {PIX_FMT_ABGR8888, RK_FORMAT_RGBA_8888}};
  auto it = rga_format_map.find(f);
  if (it != rga_format_map.end())
    return it->second;
  return -1;
}

int RgaFilter::IoCtrl(unsigned long int request, ...) {
  int ret = 0;
  va_list vl;
  va_start(vl, request);
  void *arg = va_arg(vl, void *);
  va_end(vl);

  if (!arg) {
    RKMEDIA_LOGE("Invalid IoCtrl args(request:%ld, args:NULL)\n", request);
    return -1;
  }

  switch (request) {
  case S_RGA_CFG: {
    RgaConfig *new_rga_cfg = (RgaConfig *)arg;
    ImageRect *new_src_rect = &(new_rga_cfg->src_rect);
    ImageRect *new_dst_rect = &(new_rga_cfg->dst_rect);
    int new_rotation = new_rga_cfg->rotation;
    int max_width, max_height;
    if (new_rotation == 90 || new_rotation == 270) {
      max_width = src_max_height;
      max_height = src_max_width;
    } else {
      max_width = src_max_width;
      max_height = src_max_height;
    }
    if (rga_rect_check(new_src_rect, max_width, max_height) ||
        rga_rect_check(new_dst_rect, max_width, max_height)) {
      RKMEDIA_LOGE(
          "IoCtrl: Invalid srcRect:<%d,%d,%d,%d> or dstRect:<%d,%d,%d,%d>\n",
          new_src_rect->x, new_src_rect->y, new_src_rect->w, new_src_rect->h,
          new_dst_rect->x, new_dst_rect->y, new_dst_rect->w, new_dst_rect->h);
      ret = -1;
      break;
    }

    if ((new_rotation != 0) && (new_rotation != 90) && (new_rotation != 180) &&
        (new_rotation != 270)) {
      RKMEDIA_LOGE("IoCtrl: Invalid rotation:%d\n", new_rotation);
      ret = -1;
      break;
    }

    param_mtx.lock();
    vec_rect[0].x = new_src_rect->x;
    vec_rect[0].y = new_src_rect->y;
    vec_rect[0].w = new_src_rect->w;
    vec_rect[0].h = new_src_rect->h;
    vec_rect[1].x = new_dst_rect->x;
    vec_rect[1].y = new_dst_rect->y;
    vec_rect[1].w = new_dst_rect->w;
    vec_rect[1].h = new_dst_rect->h;
    rotate = new_rotation;
    param_mtx.unlock();
    break;
  }
  case G_RGA_CFG: {
    RgaConfig *rd_rga_cfg = (RgaConfig *)arg;
    ImageRect *rd_src_rect = &(rd_rga_cfg->src_rect);
    ImageRect *rd_dst_rect = &(rd_rga_cfg->dst_rect);
    int *rd_rotation = &(rd_rga_cfg->rotation);

    param_mtx.lock();
    rd_src_rect->x = vec_rect[0].x;
    rd_src_rect->y = vec_rect[0].y;
    rd_src_rect->w = vec_rect[0].w;
    rd_src_rect->h = vec_rect[0].h;
    rd_dst_rect->x = vec_rect[1].x;
    rd_dst_rect->y = vec_rect[1].y;
    rd_dst_rect->w = vec_rect[1].w;
    rd_dst_rect->h = vec_rect[1].h;
    *rd_rotation = rotate;
    param_mtx.unlock();
    break;
  }
  case S_RGA_LINE_INFO: {
    ImageBorder *line = (ImageBorder *)arg;
    param_mtx.lock();
    int exist = 0;
    for (auto it = lines.begin(); it != lines.end(); ++it) {
      ImageBorder &l = *it;
      if (l.id == line->id) {
        exist = 1;
        memcpy(&l, line, sizeof(ImageBorder));
        break;
      }
    }
    if (!exist)
      lines.push_back(*line);
    param_mtx.unlock();
    break;
  }
  case G_RGA_LINE_INFO:
    /* TODO */
    break;
  case S_RGA_SHOW:
    param_mtx.lock();
    hide = 0;
    param_mtx.unlock();
    break;
  case S_RGA_HIDE:
    param_mtx.lock();
    hide = 1;
    param_mtx.unlock();
    break;
  case S_RGA_OSD_INFO: {
    ImageOsd *osd = (ImageOsd *)arg;
    OsdInfo info = {osd->x,    osd->y,       osd->w,     osd->h,
                    osd->data, osd->pix_fmt, osd->enable};
    param_mtx.lock();
    osds[osd->id] = info;
    param_mtx.unlock();
    break;
  }
  case G_RGA_OSD_INFO:
    /* TODO */
    break;
  case G_RGA_REGION_LUMA: {
    ImageRegionLuma *rl = (ImageRegionLuma *)arg;
    param_mtx.lock();
    memcpy(&region_luma, rl, sizeof(ImageRegionLuma));
    param_mtx.unlock();
    std::unique_lock<std::mutex> lck(luma_mutex);
    if (rl->ms <= 0) {
      luma_cond.wait(lck);
      memcpy(rl, &region_luma, sizeof(ImageRegionLuma));
    } else if (rl->ms > 0) {
      if (luma_cond.wait_for(lck, std::chrono::milliseconds(rl->ms)) ==
          std::cv_status::timeout) {
        RKMEDIA_LOGI("INFO: %s: G_RGA_REGION_LUMA timeout!\n", __func__);
        memset(rl->luma_data, 0, sizeof(rl->luma_data));
      } else {
        memcpy(rl, &region_luma, sizeof(ImageRegionLuma));
      }
    }
    param_mtx.lock();
    memset(&region_luma, 0, sizeof(ImageRegionLuma));
    param_mtx.unlock();
    break;
  }
  default:
    ret = -1;
    RKMEDIA_LOGE("Not support IoCtrl cmd(%ld)\n", request);
    break;
  }

  return ret;
}

#ifndef NDEBUG
static void dummp_rga_info(rga_info_t info, std::string name) {
  RKMEDIA_LOGD("### %s dummp info:\n", name.c_str());
  RKMEDIA_LOGD("\t info.fd = %d\n", info.fd);
  RKMEDIA_LOGD("\t info.mmuFlag = %d\n", info.mmuFlag);
  RKMEDIA_LOGD("\t info.rotation = %d\n", info.rotation);
  RKMEDIA_LOGD("\t info.blend = %d\n", info.blend);
  RKMEDIA_LOGD("\t info.virAddr = %p\n", info.virAddr);
  RKMEDIA_LOGD("\t info.phyAddr = %p\n", info.phyAddr);
  RKMEDIA_LOGD("\t info.rect.xoffset = %d\n", info.rect.xoffset);
  RKMEDIA_LOGD("\t info.rect.yoffset = %d\n", info.rect.yoffset);
  RKMEDIA_LOGD("\t info.rect.width = %d\n", info.rect.width);
  RKMEDIA_LOGD("\t info.rect.height = %d\n", info.rect.height);
  RKMEDIA_LOGD("\t info.rect.wstride = %d\n", info.rect.wstride);
  RKMEDIA_LOGD("\t info.rect.hstride = %d\n", info.rect.hstride);
  RKMEDIA_LOGD("\t info.rect.format = %d\n", info.rect.format);
  RKMEDIA_LOGD("\t info.rect.size = %d\n", info.rect.size);
}
#endif

int rga_blit(std::shared_ptr<ImageBuffer> src, std::shared_ptr<ImageBuffer> dst,
             std::vector<ImageBorder> &lines, std::map<int, OsdInfo> osds,
             ImageRegionLuma *region_luma, ImageRect *src_rect,
             ImageRect *dst_rect, int rotate, FlipEnum flip, int hide) {
  if (!src || !src->IsValid())
    return -EINVAL;
  if (!dst || !dst->IsValid())
    return -EINVAL;
  rga_info_t src_info, dst_info, pat_info;
  memset(&src_info, 0, sizeof(src_info));
  src_info.fd = src->GetFD();
  if (src_info.fd < 0)
    src_info.virAddr = src->GetPtr();
  src_info.mmuFlag = 1;
  switch (rotate) {
  case 0:
    src_info.rotation = 0;
    break;
  case 90:
    src_info.rotation = HAL_TRANSFORM_ROT_90;
    break;
  case 180:
    src_info.rotation = HAL_TRANSFORM_ROT_180;
    break;
  case 270:
    src_info.rotation = HAL_TRANSFORM_ROT_270;
    break;
  default:
    RKMEDIA_LOGW("rotate is not valid! use default:0");
    src_info.rotation = 0;
    break;
  }
  if (!src_info.rotation) {
    switch (flip) {
    case FLIP_H:
      src_info.rotation = HAL_TRANSFORM_FLIP_H;
      break;
    case FLIP_V:
      src_info.rotation = HAL_TRANSFORM_FLIP_V;
      break;
    case FLIP_HV:
      src_info.rotation = HAL_TRANSFORM_ROT_180;
      break;
    default:
      break;
    }
  }
  if (src_rect)
    rga_set_rect(&src_info.rect, src_rect->x, src_rect->y, src_rect->w,
                 src_rect->h, src->GetVirWidth(), src->GetVirHeight(),
                 get_rga_format(src->GetPixelFormat()));
  else
    rga_set_rect(&src_info.rect, 0, 0, src->GetWidth(), src->GetHeight(),
                 src->GetVirWidth(), src->GetVirHeight(),
                 get_rga_format(src->GetPixelFormat()));

  memset(&dst_info, 0, sizeof(dst_info));
  dst_info.fd = dst->GetFD();
  if (dst_info.fd < 0)
    dst_info.virAddr = dst->GetPtr();
  dst_info.mmuFlag = 1;
  if (dst_rect)
    rga_set_rect(&dst_info.rect, dst_rect->x, dst_rect->y, dst_rect->w,
                 dst_rect->h, dst->GetVirWidth(), dst->GetVirHeight(),
                 get_rga_format(dst->GetPixelFormat()));
  else
    rga_set_rect(&dst_info.rect, 0, 0, dst->GetWidth(), dst->GetHeight(),
                 dst->GetVirWidth(), dst->GetVirHeight(),
                 get_rga_format(dst->GetPixelFormat()));

#ifndef NDEBUG
  dummp_rga_info(src_info, "SrcInfo");
  dummp_rga_info(dst_info, "DstInfo");
#endif

  // flush cache,  2688x1520 NV12 cost 1399us, 1080P cost 905us
  // src->BeginCPUAccess(false);
  // src->EndCPUAccess(false);

  int ret;
  if (hide)
    ret = RgaFilter::gRkRga.RkRgaCollorFill(&dst_info);
  else
    ret = RgaFilter::gRkRga.RkRgaBlit(&src_info, &dst_info, NULL);
  if (ret) {
    dst->SetValidSize(0);
    RKMEDIA_LOGI("Fail to RkRgaBlit, ret=%d\n", ret);
  } else {
    size_t valid_size = CalPixFmtSize(dst->GetPixelFormat(), dst->GetVirWidth(),
                                      dst->GetVirHeight(), 0);
    dst->SetValidSize(valid_size);
    if (src->GetUSTimeStamp() > dst->GetUSTimeStamp())
      dst->SetUSTimeStamp(src->GetUSTimeStamp());
    dst->SetAtomicClock(src->GetAtomicClock());
  }

  for (auto &line : lines) {
    if (!line.enable)
      continue;
    int x, y, w, h;
    if (dst_rect && line.offset) {
      x = dst_rect->x + line.x;
      y = dst_rect->y + line.y;
      w = line.w;
      h = line.h;
    } else {
      x = line.x;
      y = line.y;
      w = line.w;
      h = line.h;
    }
    if (x + w > dst->GetWidth() || y + h > dst->GetHeight())
      continue;
    dst_info.color = line.color;
    rga_set_rect(&dst_info.rect, x, y, w, h, dst->GetVirWidth(),
                 dst->GetVirHeight(), get_rga_format(dst->GetPixelFormat()));
    RgaFilter::gRkRga.RkRgaCollorFill(&dst_info);
  }

  for (auto &osd : osds) {
    if (!osd.second.enable)
      continue;

    memcpy(&src_info, &dst_info, sizeof(rga_info_t));
    src_info.blend = 0xff0501;
    rga_set_rect(&src_info.rect, osd.second.x, osd.second.y, osd.second.w,
                 osd.second.h, dst->GetVirWidth(), dst->GetVirHeight(),
                 get_rga_format(dst->GetPixelFormat()));
    memset(&pat_info, 0, sizeof(rga_info_t));
    pat_info.virAddr = osd.second.data;
    pat_info.mmuFlag = 1;
    rga_set_rect(&pat_info.rect, 0, 0, osd.second.w, osd.second.h, osd.second.w,
                 osd.second.h, get_rga_format(osd.second.fmt));
    RgaFilter::gRkRga.RkRgaBlit(&src_info, &src_info, &pat_info);
  }

  // invalidate cache, 2688x1520 NV12 cost  1072us, 1080P cost 779us
  // dst->BeginCPUAccess(true);
  // dst->EndCPUAccess(true);

  if (region_luma && region_luma->region_num > 0) {
    int x, y, w, h;
    if (dst_rect && region_luma->offset) {
      x = dst_rect->x;
      y = dst_rect->y;
      w = dst_rect->w;
      h = dst_rect->h;
    } else {
      x = 0;
      y = 0;
      w = dst->GetWidth();
      h = dst->GetHeight();
    }
    memset(region_luma->luma_data, 0, sizeof(region_luma->luma_data));
    for (unsigned int i = 0; i < region_luma->region_num; i++) {
      if (region_luma->region[i].x + region_luma->region[i].w > w ||
          region_luma->region[i].y + region_luma->region[i].h > h)
        continue;
      int line_size = dst->GetVirWidth();
      int rx = x + region_luma->region[i].x;
      int ry = y + region_luma->region[i].y;
      int rw = region_luma->region[i].w;
      int rh = region_luma->region[i].h;
      char *rect_start = (char *)dst->GetPtr() + ry * line_size + rx;
      for (int k = 0; k < rh; k++) {
        char *line_start = rect_start + k * line_size;
        for (int j = 0; j < rw; j++)
          region_luma->luma_data[i] += *(line_start + j);
      }
    }
  }

  return ret;
}

class _PRIVATE_SUPPORT_FMTS : public SupportMediaTypes {
public:
  _PRIVATE_SUPPORT_FMTS() {
    types.append(TYPENEAR(IMAGE_YUV420P));
    types.append(TYPENEAR(IMAGE_NV12));
    types.append(TYPENEAR(IMAGE_NV21));
    types.append(TYPENEAR(IMAGE_YUV422P));
    types.append(TYPENEAR(IMAGE_NV16));
    types.append(TYPENEAR(IMAGE_NV61));
    types.append(TYPENEAR(IMAGE_YUYV422));
    types.append(TYPENEAR(IMAGE_UYVY422));
    types.append(TYPENEAR(IMAGE_RGB565));
    types.append(TYPENEAR(IMAGE_BGR565));
    types.append(TYPENEAR(IMAGE_RGB888));
    types.append(TYPENEAR(IMAGE_BGR888));
    types.append(TYPENEAR(IMAGE_ARGB8888));
    types.append(TYPENEAR(IMAGE_ABGR8888));
  }
};
static _PRIVATE_SUPPORT_FMTS priv_fmts;

DEFINE_COMMON_FILTER_FACTORY(RgaFilter)
const char *FACTORY(RgaFilter)::ExpectedInputDataType() {
  return priv_fmts.types.c_str();
}
const char *FACTORY(RgaFilter)::OutPutDataType() {
  return priv_fmts.types.c_str();
}

} // namespace easymedia
