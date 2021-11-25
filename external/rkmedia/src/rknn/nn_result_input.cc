// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <queue>

#include "buffer.h"
#include "control.h"
#include "encoder.h"
#include "filter.h"
#include "lock.h"
#include "media_config.h"

namespace easymedia {

class NNResultInput : public Filter {
public:
  NNResultInput(const char *param);
  virtual ~NNResultInput() = default;
  static const char *GetFilterName() { return "nn_result_input"; }

  void PushResult(std::list<RknnResult> &results);
  std::list<RknnResult> PopResult(int64_t atomic_clock);

  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;
  virtual int IoCtrl(unsigned long int request, ...) override;

private:
  static uint32_t kImagePoolSize;

  bool enable_;
  uint32_t cache_size_;
  uint32_t clock_delta_ms_; // millisecond

  std::mutex mutex_;
  std::condition_variable cond_;
  ReadWriteLockMutex result_mutex_;

  std::deque<std::list<RknnResult>> nn_cache_;
  std::deque<std::shared_ptr<ImageBuffer>> image_pool_;
};

uint32_t NNResultInput::kImagePoolSize = 1;

NNResultInput::NNResultInput(const char *param) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }

  cache_size_ = 10;
  const std::string &cache_size_str = params[KEY_CACHE_SIZE];
  if (!cache_size_str.empty())
    cache_size_ = std::stoi(cache_size_str);

  clock_delta_ms_ = 90;
  const std::string &clock_delta_str = params[KEY_CLOCK_DELTA];
  if (!clock_delta_str.empty())
    clock_delta_ms_ = std::stoi(clock_delta_str);

  enable_ = false;
  const std::string &enable_str = params[KEY_ENABLE];
  if (!enable_str.empty())
    enable_ = std::stoi(enable_str);
}

void NNResultInput::PushResult(std::list<RknnResult> &results) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (nn_cache_.size() > cache_size_)
    nn_cache_.pop_front();
  nn_cache_.push_back(results);
}

std::list<RknnResult> NNResultInput::PopResult(int64_t atomic_clock) {
  std::unique_lock<std::mutex> lock(mutex_);

  int64_t min_delta = 10000000LL;
  std::list<RknnResult> result;

  for (auto &iter : nn_cache_) {
    for (RknnResult &tmp : iter) {
      int64_t delta = std::llabs(tmp.timeval - atomic_clock);
      if (delta < min_delta) {
        min_delta = delta;
        result = iter;
      }
      break;
    }
  }
  int64_t clock_delta = clock_delta_ms_ * 1000;
  if (!result.empty() && clock_delta < min_delta)
    result.clear();
  return std::move(result);
}

int NNResultInput::Process(std::shared_ptr<MediaBuffer> input,
                           std::shared_ptr<MediaBuffer> &output) {
  if (!input || input->GetType() != Type::Image)
    return -EINVAL;
  if (!output || output->GetType() != Type::Image)
    return -EINVAL;

  if (!enable_) {
    output = input;
  } else {
    auto image = std::static_pointer_cast<easymedia::ImageBuffer>(input);
    image_pool_.push_back(image);

    if (image_pool_.size() <= kImagePoolSize)
      return -1;

    auto output_image = image_pool_.front();
    image_pool_.pop_front();

    auto tobe_input_result = PopResult(output_image->GetAtomicClock());
    if (!tobe_input_result.empty()) {
      auto &nn_results = output_image->GetRknnResult();
      for (auto &iter : tobe_input_result)
        nn_results.push_back(iter);
    }
    output = output_image;
  }
  return 0;
}

int NNResultInput::IoCtrl(unsigned long int request, ...) {
  va_list vl;
  va_start(vl, request);
  void *arg = va_arg(vl, void *);
  va_end(vl);

  int ret = 0;
  AutoLockMutex rw_mtx(result_mutex_);
  switch (request) {
  case S_SUB_REQUEST: {
    SubRequest *req = (SubRequest *)arg;
    if (S_NN_INFO == req->sub_request) {
      int size = req->size;
      std::list<RknnResult> infos_list;
      RknnResult *infos = (RknnResult *)req->arg;
      if (infos) {
        for (int i = 0; i < size; i++)
          infos_list.push_back(infos[i]);
      }
      PushResult(infos_list);
    }
  } break;
  case S_NN_INFO: {
    if (arg) {
      NNinputArg *nn_input_arg = (NNinputArg *)arg;
      enable_ = nn_input_arg->enable;
    }
  } break;
  case G_NN_INFO: {
    if (arg) {
      NNinputArg *nn_input_arg = (NNinputArg *)arg;
      nn_input_arg->enable = enable_;
    }
  } break;
  default:
    ret = -1;
    break;
  }
  return ret;
}

DEFINE_COMMON_FILTER_FACTORY(NNResultInput)
const char *FACTORY(NNResultInput)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(NNResultInput)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace easymedia
