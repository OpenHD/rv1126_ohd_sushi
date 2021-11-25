// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <queue>

#include "buffer.h"
#include "encoder.h"
#include "filter.h"
#include "lock.h"
#include "media_config.h"
#ifdef USE_ROCKX
#include "rknn_user.h"
#endif
#define YUV_PIXEL_RED ((0x4C << 16) | (0x54 << 8) | 0xFF)

namespace easymedia {

static void draw_rect(std::shared_ptr<ImageBuffer> &buffer, Rect &rect,
                      int thick);
static int draw_nv12_rect(uint8_t *data, int img_w, int img_h, Rect &rect,
                          int thick, int yuv_color);
static Rect combine_rect(std::vector<Rect> &rect);
static void hw_draw_rect(uint8_t *data, int img_w, Rect &rect, int thick,
                         int palette_index);

class DrawFilter : public Filter {
public:
  DrawFilter(const char *param);
  virtual ~DrawFilter() = default;
  static const char *GetFilterName() { return "draw_filter"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;
  virtual int IoCtrl(unsigned long int request, ...) override;

  void DoDrawRect(std::shared_ptr<ImageBuffer> &buffer, Rect &rect);
  void DoDraw(std::shared_ptr<ImageBuffer> &buffer,
              std::list<RknnResult> &nn_result);

  void DoHwDrawRect(OsdRegionData *region_data, int enable = 1);
  void DoHwDraw(std::list<RknnResult> &nn_result);

  void ConvertRect(std::list<RknnResult> &nn_list);

private:
  bool enable_;
  bool need_hw_draw_;
  int draw_rect_thick_;
  int draw_frame_rate_;
  int min_rect_size_;
  float offset_x_;
  float offset_y_;
  ReadWriteLockMutex draw_mtx_;
  RknnHandler draw_handler_;
};

DrawFilter::DrawFilter(const char *param)
    : need_hw_draw_(false), draw_rect_thick_(2), draw_handler_(nullptr) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }

  if (params[KEY_NEED_HW_DRAW].empty()) {
    need_hw_draw_ = false;
  } else {
    need_hw_draw_ = atoi(params[KEY_NEED_HW_DRAW].c_str());
  }

  if (!params[KEY_DRAW_RECT_THICK].empty()) {
    draw_rect_thick_ = atoi(params[KEY_DRAW_RECT_THICK].c_str());
  }

  if (params[KEY_FRAME_RATE].empty()) {
    draw_frame_rate_ = 30;
  } else {
    draw_frame_rate_ = atoi(params[KEY_FRAME_RATE].c_str());
  }

  min_rect_size_ = 0;
  const std::string &min_rect = params[KEY_DRAW_MIN_RECT];
  if (!min_rect.empty())
    min_rect_size_ = atoi(min_rect.c_str());

  offset_x_ = 0.0;
  const std::string &offset_x = params[KEY_DRAW_OFFSET_X];
  if (!offset_x.empty())
    offset_x_ = atof(offset_x.c_str());

  offset_y_ = 0.0;
  const std::string &offset_y = params[KEY_DRAW_OFFSET_Y];
  if (!offset_y.empty())
    offset_y_ = atof(offset_y.c_str());

  enable_ = false;
  const std::string &enable_str = params[KEY_ENABLE];
  if (!enable_str.empty())
    enable_ = std::stoi(enable_str);
}

void DrawFilter::DoDrawRect(std::shared_ptr<ImageBuffer> &buffer, Rect &rect) {
  draw_rect(buffer, rect, draw_rect_thick_);
}

void DrawFilter::DoHwDrawRect(OsdRegionData *region_data, int enable) {
  Flow *flow = (Flow *)draw_handler_;
  if (region_data->enable &&
      ((region_data->width % 16) || (region_data->height % 16))) {
    RKMEDIA_LOGE("osd region size must be a multiple of 16x16.");
    return;
  }
  if (enable) {
    int buffer_size = region_data->width * region_data->height;
    OsdRegionData *rdata =
        (OsdRegionData *)malloc(sizeof(OsdRegionData) + buffer_size);
    memcpy((void *)rdata, (void *)region_data, sizeof(OsdRegionData));
    rdata->buffer = (uint8_t *)rdata + sizeof(OsdRegionData);
    memcpy(rdata->buffer, region_data->buffer, buffer_size);
    auto pbuff = std::make_shared<ParameterBuffer>(0);
    pbuff->SetPtr(rdata, sizeof(OsdRegionData) + buffer_size);
    flow->Control(VideoEncoder::kOSDDataChange, pbuff);
  } else {
    region_data->enable = enable;
    OsdRegionData *rdata = (OsdRegionData *)malloc(sizeof(OsdRegionData));
    memcpy((void *)rdata, (void *)region_data, sizeof(OsdRegionData));
    auto pbuff = std::make_shared<ParameterBuffer>(0);
    pbuff->SetPtr(rdata, sizeof(OsdRegionData));
    flow->Control(VideoEncoder::kOSDDataChange, pbuff);
  }
}

void DrawFilter::DoHwDraw(std::list<RknnResult> &nn_result) {
  int color_index = 0x23;
  OsdRegionData osd_region_data;
  memset(&osd_region_data, 0, sizeof(OsdRegionData));
  osd_region_data.enable = 1;
  osd_region_data.region_id = 7;

  std::vector<Rect> rects;
  for (auto info : nn_result) {
    Rect rect;
#ifdef USE_ROCKFACE
    if (info.type == NNRESULT_TYPE_FACE) {
      rockface_det_t face_det = info.face_info.base;
      rect.left = UPALIGNTO16(face_det.box.left);
      rect.right = DOWNALIGNTO16(face_det.box.right);
      rect.top = UPALIGNTO16(face_det.box.top);
      rect.bottom = DOWNALIGNTO16(face_det.box.bottom);
      rects.push_back(rect);
    }
#endif
#ifdef USE_ROCKX
    if (info.type == NNRESULT_TYPE_OBJECT_DETECT) {
      Rect object_det_box = info.object_info.box;
      rect.left = UPALIGNTO16(object_det_box.left);
      rect.right = DOWNALIGNTO16(object_det_box.right);
      rect.top = UPALIGNTO16(object_det_box.top);
      rect.bottom = DOWNALIGNTO16(object_det_box.bottom);
      rects.push_back(rect);
    }
#endif
  }
  Rect combine = combine_rect(rects);
  for (auto &rect : rects) {
    rect.left = rect.left - combine.left;
    rect.right = rect.right - combine.left;
    rect.top = rect.top - combine.top;
    rect.bottom = rect.bottom - combine.top;
  }

  osd_region_data.pos_x = combine.left;
  osd_region_data.pos_y = combine.top;
  osd_region_data.width = combine.right - combine.left;
  osd_region_data.height = combine.bottom - combine.top;
  int buffer_size = osd_region_data.width * osd_region_data.height;
#ifdef DRAW_HW_BUFFER
  auto mb = easymedia::MediaBuffer::Alloc(
      buffer_size, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  osd_region_data.buffer = mb->GetPtr();
#else
  osd_region_data.buffer = static_cast<uint8_t *>(malloc(buffer_size));
#endif
  if (!osd_region_data.buffer) {
    return;
  }
  memset(osd_region_data.buffer, 0xFF, buffer_size);
  for (auto &rect : rects) {
    hw_draw_rect(osd_region_data.buffer, osd_region_data.width, rect,
                 draw_rect_thick_, color_index);
  }
  DoHwDrawRect(&osd_region_data);

#ifndef DRAW_HW_BUFFER
  free(osd_region_data.buffer);
#endif
}

void DrawFilter::DoDraw(std::shared_ptr<ImageBuffer> &buffer,
                        std::list<RknnResult> &nn_result) {
  for (auto info_result : nn_result) {
#ifdef USE_ROCKFACE
    if (info_result.type == NNRESULT_TYPE_FACE) {
      rockface_det_t face_det = info_result.face_info.base;
      Rect rect_face = {face_det.box.left, face_det.box.top, face_det.box.right,
                        face_det.box.bottom};
      DoDrawRect(buffer, rect_face);
    }
#endif
#ifdef USE_ROCKX
    if (info_result.type == NNRESULT_TYPE_OBJECT_DETECT) {
      Rect object_det_box = info_result.object_info.box;
      Rect rect_rockx = {object_det_box.left, object_det_box.top,
                         object_det_box.right, object_det_box.bottom};
      DoDrawRect(buffer, rect_rockx);
    }
#endif
  }
}

void DrawFilter::ConvertRect(std::list<RknnResult> &nn_list) {
  for (RknnResult &nn : nn_list) {
#ifdef USE_ROCKFACE
    if (nn.type == NNRESULT_TYPE_FACE) {
      rockface_rect_t *rect = &nn.face_info.base.box;
      rect->left = rect->left + offset_x_;
      rect->top = rect->top + offset_y_;
      rect->right = rect->right + offset_x_;
      rect->bottom = rect->bottom + offset_y_;
      int rect_size = (rect->right - rect->left) * (rect->bottom - rect->top);
      if (rect_size < min_rect_size_)
        memset(rect, 0, sizeof(rockface_rect_t));
    }
#endif
#ifdef USE_ROCKX
    if (nn.type == NNRESULT_TYPE_OBJECT_DETECT) {
      Rect *rect = &nn.object_info.box;
      rect->left = rect->left + offset_x_;
      rect->top = rect->top + offset_y_;
      rect->right = rect->right + offset_x_;
      rect->bottom = rect->bottom + offset_y_;
      int rect_size = (rect->right - rect->left) * (rect->bottom - rect->top);
      if (rect_size < min_rect_size_)
        memset(rect, 0, sizeof(rockx_rect_t));
    }
#endif
  }
}

int DrawFilter::Process(std::shared_ptr<MediaBuffer> input,
                        std::shared_ptr<MediaBuffer> &output) {
  if (!input || input->GetType() != Type::Image)
    return -EINVAL;
  if (!output || output->GetType() != Type::Image)
    return -EINVAL;

  output = input;

  if (!enable_)
    return 0;

  auto src = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  auto dst = std::static_pointer_cast<easymedia::ImageBuffer>(output);

  std::list<RknnResult> &written_list = src->GetRknnResult();
  if (written_list.empty())
    return 0;
  ConvertRect(written_list);

  input->BeginCPUAccess(false);
  if (draw_handler_ && need_hw_draw_)
    DoHwDraw(written_list);
  else
    DoDraw(dst, written_list);
  input->EndCPUAccess(false);
  return 0;
}

int DrawFilter::IoCtrl(unsigned long int request, ...) {
  va_list vl;
  va_start(vl, request);
  void *arg = va_arg(vl, void *);
  va_end(vl);

  int ret = 0;
  AutoLockMutex rw_mtx(draw_mtx_);
  switch (request) {
  case S_NN_DRAW_HANDLER: {
    draw_handler_ = (RknnHandler)arg;
  } break;
  case G_NN_DRAW_HANDLER: {
    arg = (void *)draw_handler_;
  } break;
  case S_NN_INFO: {
    if (arg) {
      DrawFilterArg *draw_arg = (DrawFilterArg *)arg;
      enable_ = draw_arg->enable;
    }
  } break;
  case G_NN_INFO: {
    if (arg) {
      DrawFilterArg *draw_arg = (DrawFilterArg *)arg;
      draw_arg->enable = enable_;
    }
  } break;

  default:
    ret = -1;
    break;
  }

  return ret;
}

DEFINE_COMMON_FILTER_FACTORY(DrawFilter)
const char *FACTORY(DrawFilter)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(DrawFilter)::OutPutDataType() { return TYPE_ANYTHING; }

void draw_rect(std::shared_ptr<ImageBuffer> &buffer, Rect &rect, int thick) {
  ImageInfo info = buffer->GetImageInfo();
  uint8_t *img_data = (uint8_t *)buffer->GetPtr();
  int img_w = buffer->GetWidth();
  int img_h = buffer->GetHeight();

  if (rect.right > img_w - thick) {
    // RKMEDIA_LOGI("draw_rect right > img_w\n");
    rect.right = img_w - thick;
  }
  if (rect.left < 0) {
    // RKMEDIA_LOGI("draw_rect letf < 0\n");
    rect.left = 0;
  }
  if (rect.bottom > img_h - thick) {
    // RKMEDIA_LOGI("draw_rect bottom > img_h\n");
    rect.bottom = img_h - thick;
  }
  if (rect.top < 0) {
    // RKMEDIA_LOGI("draw_rect top < 0\n");
    rect.top = 0;
  }

  if (info.pix_fmt == PIX_FMT_NV12) {
    draw_nv12_rect(img_data, img_w, img_h, rect, thick, YUV_PIXEL_RED);
  } else {
    RKMEDIA_LOGI("RockFaceDebug:can't draw rect on this format yet!\n");
  }
}

int draw_nv12_rect(uint8_t *data, int img_w, int img_h, Rect &rect, int thick,
                   int yuv_color) {
  int j, k;
  int uv_offset = img_w * img_h;
  int y_offset, u_offset, v_offset;
  int rect_x, rect_y, rect_w, rect_h;
  rect_x = rect.left;
  rect_y = rect.top;
  rect_w = rect.right - rect.left;
  rect_h = rect.bottom - rect.top;

  int y = (yuv_color >> 16) & 0xFF;
  int u = (yuv_color >> 8) & 0xFF;
  int v = (yuv_color >> 0) & 0xFF;

  for (j = rect_y; j <= rect_y + rect_h; j++) {
    for (k = rect_x; k <= rect_x + rect_w; k++) {
      if (k <= (rect_x + thick) || k >= (rect_x + rect_w - thick) ||
          j <= (rect_y + thick) || j >= (rect_y + rect_h - thick)) {
        y_offset = j * img_w + k;
        u_offset = (j >> 1) * img_w + k - k % 2 + uv_offset;
        v_offset = u_offset + 1;
        data[y_offset] = y;
        data[u_offset] = u;
        data[v_offset] = v;
      }
    }
  }
  return 0;
}

Rect combine_rect(std::vector<Rect> &rect) {
  Rect combine;
  int size = rect.size();
  memset(&combine, 0, sizeof(Rect));
  if (!size)
    return combine;
  combine.left = rect[0].left;
  combine.right = rect[0].right;
  combine.top = rect[0].top;
  combine.bottom = rect[0].bottom;
  for (int i = 1; i < size; i++) {
    combine.left = VALUE_MIN(combine.left, rect[i].left);
    combine.right = VALUE_MAX(combine.right, rect[i].right);
    combine.top = VALUE_MIN(combine.top, rect[i].top);
    combine.bottom = VALUE_MAX(combine.bottom, rect[i].bottom);
  }
  return combine;
}

void hw_draw_rect(uint8_t *data, int img_w, Rect &rect, int thick, int index) {
  int j, k, offset;
  for (j = rect.top; j < rect.bottom; j++) {
    for (k = rect.left; k < rect.right; k++) {
      if (k < (rect.left + thick) || k > (rect.right - thick) ||
          j < (rect.top + thick) || j > (rect.bottom - thick)) {
        offset = j * img_w + k;
        data[offset] = index;
      }
    }
  }
}

} // namespace easymedia
