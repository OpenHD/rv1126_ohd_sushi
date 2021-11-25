// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <rockface/rockface.h>

#include "buffer.h"
#include "filter.h"
#include "lock.h"
#include "rknn_user.h"
#include "rknn_utils.h"

namespace easymedia {

#define DEFAULT_LIC_PATH "/userdata/key.lic"

class BodyContrl {
public:
  BodyContrl() = delete;
  BodyContrl(bool enable, int interval, int duration_thr, float percentage_thr,
             ImageRect roi_rect) {
    SetEnable(enable);
    SetInterval(interval);
    SetRect(roi_rect);
    SetDuration(duration_thr);
    SetPercentage(percentage_thr);
  }
  virtual ~BodyContrl() {}

  bool CheckIsRun();
  bool PercentageFilter(rockface_det_t *body);
  bool DurationFilter(std::list<RknnResult> &list, int64_t timeval_ms);

  void SetEnable(int enable) { enable_ = enable; }
  void SetInterval(int interval) { interval_ = interval; }
  void SetDuration(int second) { duration_thr_ = second * 1000; }
  void SetRect(ImageRect &rect) { roi_rect_ = rect; }
  void SetPercentage(float percentage) { percentage_thr_ = percentage / 100; }

  int GetEnable(void) const { return enable_; }
  int GetInterval(void) const { return interval_; }
  int GetDuration(void) const { return duration_thr_ / 1000; }
  ImageRect GetRect(void) const { return roi_rect_; }
  float GetPercentage(void) const { return percentage_thr_ * 100; }

private:
  bool enable_;
  int interval_;
  int duration_thr_; /* millisecond */
  float percentage_thr_;
  ImageRect roi_rect_;
};

class BodyDetect : public Filter {
public:
  BodyDetect(const char *param);
  virtual ~BodyDetect();
  static const char *GetFilterName() { return "rockface_bodydetect"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;
  virtual int IoCtrl(unsigned long int request, ...) override;

protected:
  void SendNNResult(std::list<RknnResult> &list,
                    std::shared_ptr<ImageBuffer> image);

private:
  std::string input_type_;
  rockface_handle_t body_handle_;
  std::shared_ptr<BodyContrl> contrl_;

  RknnResult authorized_result_;
  RknnCallBack callback_;
  ReadWriteLockMutex cb_mtx_;
};

BodyDetect::BodyDetect(const char *param) : callback_(nullptr) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  input_type_ = params[KEY_INPUTDATATYPE];
  if (input_type_.empty()) {
    RKMEDIA_LOGI("bodydetect lost input type.\n");
    return;
  }
  bool enable = false;
  const std::string &enable_str = params[KEY_ENABLE];
  if (!enable_str.empty())
    enable = std::stoi(enable_str);

  int interval = 0;
  const std::string &interval_str = params[KEY_FRAME_INTERVAL];
  if (!interval_str.empty())
    interval = std::stoi(interval_str);

  int duration_thr = 0;
  const std::string &duration_str = params[KEY_BODY_DURATION];
  if (!duration_str.empty())
    duration_thr = std::stoi(duration_str);

  float percentage_thr = 50.0;
  const std::string &percentage_str = params[KEY_BODY_PERCENTAGE];
  if (!percentage_str.empty())
    percentage_thr = std::stof(percentage_str);

  auto &&rects = StringToImageRect(params[KEY_DETECT_RECT]);
  if (rects.empty()) {
    RKMEDIA_LOGI("missing rects\n");
    SetError(-EINVAL);
    return;
  }
  ImageRect roi_rect = rects[0];
  contrl_ = std::make_shared<BodyContrl>(enable, interval, duration_thr,
                                         percentage_thr, roi_rect);
  if (!contrl_) {
    RKMEDIA_LOGI("body contrl is nullptr.\n");
    return;
  }

  std::string license_path = DEFAULT_LIC_PATH;
  if (!params[KEY_PATH].empty())
    license_path = params[KEY_PATH];

  rockface_ret_t ret;
  body_handle_ = rockface_create_handle();

  authorized_result_.type = NNRESULT_TYPE_AUTHORIZED_STATUS;
  ret = rockface_set_licence(body_handle_, license_path.c_str());
  if (ret != ROCKFACE_RET_SUCCESS)
    authorized_result_.status = FAILURE;
  else
    authorized_result_.status = SUCCESS;

  ret = rockface_init_person_detector(body_handle_);
  if (ret != ROCKFACE_RET_SUCCESS) {
    RKMEDIA_LOGI("rockface_init_person_detector failed, ret = %d\n", ret);
    return;
  }
  if (authorized_result_.status != SUCCESS)
    RKMEDIA_LOGI("rockface bodydetect authorize failed.\n");
}

BodyDetect::~BodyDetect() {
  if (body_handle_)
    rockface_release_handle(body_handle_);
}

int BodyDetect::Process(std::shared_ptr<MediaBuffer> input,
                        std::shared_ptr<MediaBuffer> &output) {
  auto image = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  if (!image)
    return -1;

  output = input;
  rockface_image_t input_img;
  input_img.width = image->GetWidth();
  input_img.height = image->GetHeight();
  input_img.pixel_format = StrToRockFacePixelFMT(input_type_.c_str());
  input_img.data = (uint8_t *)image->GetPtr();

  if (!contrl_->CheckIsRun() || authorized_result_.status == TIMEOUT)
    return 0;

  // flush cache,  2688x1520 NV12 cost 1399us, 1080P cost 905us
  image->BeginCPUAccess(false);
  image->EndCPUAccess(false);

  AutoDuration cost_time;
  rockface_ret_t ret;
  rockface_det_person_array_t body_array;
  memset(&body_array, 0, sizeof(rockface_det_person_array_t));

  ret = rockface_person_detect(body_handle_, &input_img, &body_array);
  if (ret != ROCKFACE_RET_SUCCESS) {
    if (ret == ROCKFACE_RET_AUTH_FAIL) {
      authorized_result_.status = TIMEOUT;
      if (callback_) {
        AutoLockMutex lock(cb_mtx_);
        callback_(this, NNRESULT_TYPE_AUTHORIZED_STATUS, &authorized_result_,
                  1);
      }
    }
    RKMEDIA_LOGI("rockface_person_detect failed, ret = %d\n", ret);
    return -1;
  }
  RKMEDIA_LOGD("rockface_person_detect cost time %lldus\n", cost_time.Get());

  RknnResult nn_result;
  memset(&nn_result, 0, sizeof(RknnResult));
  nn_result.type = NNRESULT_TYPE_BODY;
  auto &nn_list = image->GetRknnResult();

  for (int i = 0; i < body_array.count; i++) {
    rockface_det_t *body = &body_array.person[i];
    if (!contrl_->PercentageFilter(body))
      continue;

    RKMEDIA_LOGD("body[%d], position:[%d, %d, %d, %d]\n", i, body->box.left,
                 body->box.top, body->box.right, body->box.bottom);

    memcpy(&nn_result.body_info.base, body, sizeof(rockface_det_t));
    nn_list.push_back(nn_result);
  }
  SendNNResult(nn_list, image);
  return 0;
}

void BodyDetect::SendNNResult(std::list<RknnResult> &list,
                              std::shared_ptr<ImageBuffer> image) {
  int64_t timeval_ms = image->GetAtomicClock() / 1000;
  if (contrl_->DurationFilter(list, timeval_ms)) {
    int count = 0;
    int size = list.size();
    RknnResult nn_array[size];
    for (auto &iter : list) {
      if (iter.type != NNRESULT_TYPE_BODY)
        continue;
      nn_array[count].img_w = image->GetWidth();
      nn_array[count].img_h = image->GetHeight();
      nn_array[count].body_info.base = iter.body_info.base;
      nn_array[count].type = NNRESULT_TYPE_BODY;
      count++;
    }
    AutoLockMutex lock(cb_mtx_);
    if (callback_)
      callback_(this, NNRESULT_TYPE_BODY, nn_array, count);
  }
}

int BodyDetect::IoCtrl(unsigned long int request, ...) {
  AutoLockMutex lock(cb_mtx_);
  int ret = 0;
  va_list vl;
  va_start(vl, request);
  switch (request) {
  case S_NN_CALLBACK: {
    void *arg = va_arg(vl, void *);
    if (arg)
      callback_ = (RknnCallBack)arg;
  } break;
  case G_NN_CALLBACK: {
    void *arg = va_arg(vl, void *);
    if (arg)
      arg = (void *)callback_;
  } break;
  case S_NN_INFO: {
    BodyDetectArg *arg = va_arg(vl, BodyDetectArg *);
    if (arg) {
      contrl_->SetEnable(arg->enable);
      contrl_->SetInterval(arg->interval);
      contrl_->SetDuration(arg->duration);
      contrl_->SetRect(arg->rect);
      contrl_->SetPercentage(arg->percentage);
    }
  } break;
  case G_NN_INFO: {
    BodyDetectArg *arg = va_arg(vl, BodyDetectArg *);
    if (arg) {
      arg->enable = contrl_->GetEnable();
      arg->interval = contrl_->GetInterval();
      arg->duration = contrl_->GetDuration();
      arg->rect = contrl_->GetRect();
      arg->percentage = contrl_->GetPercentage();
    }
  } break;
  default:
    ret = -1;
    break;
  }
  va_end(vl);
  return ret;
}

static bool CheckBodyEmpty(std::list<RknnResult> &list) {
  bool check = true;
  for (auto &iter : list) {
    if (iter.type == NNRESULT_TYPE_BODY) {
      check = false;
      break;
    }
  }
  return check;
}

bool BodyContrl::CheckIsRun() {
  bool check = false;
  static int count = 0;
  if (enable_) {
    if (count++ >= interval_) {
      count = 0;
      check = true;
    }
  }
  return check;
}

bool BodyContrl::PercentageFilter(rockface_det_t *body) {
  if (!body)
    return false;

  float percentage = 0;
  int left = body->box.left;
  int top = body->box.top;
  int right = body->box.right;
  int bottom = body->box.bottom;

  int rect_left = roi_rect_.x;
  int rect_top = roi_rect_.y;
  int rect_right = roi_rect_.x + roi_rect_.w;
  int rect_bottom = roi_rect_.y + roi_rect_.h;

  if (left > rect_right || right < rect_left || top > rect_bottom ||
      bottom < rect_top) {
    percentage = 0.0;
  } else if (left > rect_left && right < rect_right && top > rect_top &&
             bottom < rect_bottom) {
    percentage = 1.0;
  } else {
    int overlap_left = std::max(rect_left, left);
    int overlap_top = std::max(rect_top, top);
    int overlap_right = std::min(rect_right, right);
    int overlap_bottom = std::min(rect_bottom, bottom);
    float overlap_size =
        (overlap_right - overlap_left) * (overlap_bottom - overlap_top);

    float rect_size = (right - left) * (bottom - top);
    percentage = overlap_size / rect_size;
  }
  return (percentage > percentage_thr_) ? true : false;
}

bool BodyContrl::DurationFilter(std::list<RknnResult> &list,
                                int64_t timeval_ms) {
  static bool found = false;
  static int64_t first_timeval_ms = 0;

  int64_t diff = -1;
  if (CheckBodyEmpty(list)) {
    found = false;
  } else {
    if (!found) {
      first_timeval_ms = timeval_ms;
      found = true;
    }
    diff = timeval_ms - first_timeval_ms;
  }
  return (diff >= duration_thr_) ? true : false;
}

DEFINE_COMMON_FILTER_FACTORY(BodyDetect)
const char *FACTORY(BodyDetect)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(BodyDetect)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace easymedia
