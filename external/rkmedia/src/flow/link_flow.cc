// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffer.h"
#include "flow.h"
#include "link_config.h"
#include "stream.h"
#include "utils.h"

namespace easymedia {

static bool process_buffer(Flow *f, MediaBufferVector &input_vector);

class _API LinkFlow : public Flow {
public:
  LinkFlow(const char *param);
  virtual ~LinkFlow();
  static const char *GetFlowName() { return "link_flow"; }

private:
  friend bool process_buffer(Flow *f, MediaBufferVector &input_vector);

private:
  LinkType link_type_;
  std::string extra_data;
};

LinkFlow::LinkFlow(const char *param) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }

  SetVideoHandler(nullptr);
  SetAudioHandler(nullptr);
  SetCaptureHandler(nullptr);
  SetUserCallBack(nullptr, nullptr);

  SlotMap sm;
  sm.input_slots.push_back(0);
  if (sm.thread_model == Model::NONE)
    sm.thread_model =
        !params[KEY_FPS].empty() ? Model::ASYNCATOMIC : Model::ASYNCCOMMON;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::DROPCURRENT;

  sm.input_maxcachenum.push_back(0);
  sm.process = process_buffer;

  if (!InstallSlotMap(sm, "LinkFLow", 0)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap for LinkFLow\n");
    return;
  }
  SetFlowTag("LinkFLow");
  std::string &type = params[KEY_INPUTDATATYPE];
  link_type_ = LINK_NONE;
  if (type.find(VIDEO_PREFIX) != std::string::npos) {
    link_type_ = LINK_VIDEO;
  } else if (type.find(AUDIO_PREFIX) != std::string::npos) {
    link_type_ = LINK_AUDIO;
  } else if (type.find(IMAGE_PREFIX) != std::string::npos) {
    link_type_ = LINK_PICTURE;
  } else if (type.find(NN_MODEL_PREFIX) != std::string::npos) {
    link_type_ = LINK_NNDATA;
    int position = type.find(":", 0);
    extra_data = type.substr(position + 1, type.length());
  }
}

LinkFlow::~LinkFlow() { StopAllThread(); }

bool process_buffer(Flow *f, MediaBufferVector &input_vector) {
  LinkFlow *flow = static_cast<LinkFlow *>(f);
  auto &buffer = input_vector[0];
  if (!buffer || !flow)
    return false;

  if (flow->link_type_ == LINK_VIDEO) {
    auto link_handler = flow->GetVideoHandler();
    auto nal_type = (buffer->GetUserFlag() & MediaBuffer::kIntra) ? 1 : 0;
    if (link_handler)
      link_handler((unsigned char *)buffer->GetPtr(), buffer->GetValidSize(),
                   buffer->GetUSTimeStamp(), nal_type);
  } else if (flow->link_type_ == LINK_AUDIO) {
    auto link_audio_handler = flow->GetAudioHandler();
    if (link_audio_handler)
      link_audio_handler((unsigned char *)buffer->GetPtr(),
                         buffer->GetValidSize(), buffer->GetUSTimeStamp());
  } else if (flow->link_type_ == LINK_PICTURE) {
    auto link_handler = flow->GetCaptureHandler();
    if (link_handler)
      link_handler((unsigned char *)buffer->GetPtr(), buffer->GetValidSize(), 0,
                   NULL);
  } else if (flow->link_type_ == LINK_NNDATA) {
    auto user_callback = flow->GetUserCallBack();
    auto timestamp = easymedia::gettimeofday() / 1000;
    if (user_callback) {
      if (buffer) {
        auto input_buffer =
            std::static_pointer_cast<easymedia::ImageBuffer>(buffer);
        static linknndata_s link_nn_data;
        auto &rknn_result = input_buffer->GetRknnResult();
        int size = rknn_result.size();
        RknnResult *infos = (RknnResult *)malloc(size * sizeof(RknnResult));
        if (infos) {
          int i = 0;
          for (auto &iter : rknn_result) {
            memcpy(&infos[i], &iter, sizeof(RknnResult));
            i++;
          }
        }
        link_nn_data.rknn_result = infos;
        link_nn_data.size = size;
        link_nn_data.nn_model_name = (flow->extra_data).c_str();
        link_nn_data.timestamp = timestamp;
        user_callback(nullptr, LINK_NNDATA, &link_nn_data, 1);
        if (infos)
          free(infos);
      } else {
        return true;
      }
    }
  }

  return false;
}

DEFINE_FLOW_FACTORY(LinkFlow, Flow)
const char *FACTORY(LinkFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(LinkFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
