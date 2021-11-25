// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include "buffer.h"
#include "decoder.h"
#include "flow.h"
#include "image.h"
#include "key_string.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_VDEC

namespace easymedia {

static bool do_decode(Flow *f, MediaBufferVector &input_vector);
class VideoDecoderFlow : public Flow {
public:
  VideoDecoderFlow(const char *param);
  virtual ~VideoDecoderFlow() { StopAllThread(); }
  static const char *GetFlowName() { return "video_dec"; }
  std::vector<std::shared_ptr<MediaBuffer>> input_buffers;

private:
  std::shared_ptr<Decoder> decoder;
  bool support_async;
  Model thread_model;
  std::vector<std::shared_ptr<MediaBuffer>> out_buffers;
  size_t out_index;

  bool is_single_frame_out;

  friend bool do_decode(Flow *f, MediaBufferVector &input_vector);
};

VideoDecoderFlow::VideoDecoderFlow(const char *param)
    : support_async(true), thread_model(Model::NONE), out_index(0) {
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;
  if (!ParseWrapFlowParams(param, params, separate_list)) {
    SetError(-EINVAL);
    return;
  }
  std::string &name = params[KEY_NAME];
  const char *decoder_name = name.c_str();
  const std::string &decode_param = separate_list.back();
  decoder = REFLECTOR(Decoder)::Create<VideoDecoder>(decoder_name,
                                                     decode_param.c_str());
  if (!decoder) {
    RKMEDIA_LOGI("Create decoder %s failed\n", decoder_name);
    SetError(-EINVAL);
    return;
  }
  SlotMap sm;
  int input_maxcachenum = 2;
  ParseParamToSlotMap(params, sm, input_maxcachenum);
  if (sm.thread_model == Model::NONE)
    sm.thread_model = Model::ASYNCCOMMON;
  thread_model = sm.thread_model;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::BLOCKING;
  sm.input_slots.push_back(0);
  sm.input_maxcachenum.push_back(input_maxcachenum);
  sm.output_slots.push_back(0);
  sm.process = do_decode;
  if (!InstallSlotMap(sm, "VideoDecoderFlow", -1)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap for VideoDecoderFlow\n");
    SetError(-EINVAL);
    return;
  }

  std::string split_mode;
  std::string stimeout;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_MPP_SPLIT_MODE, split_mode));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_OUTPUT_TIMEOUT, stimeout));
  parse_media_param_match(param, params, req_list);

  int auto_split = 0;
  int tout = 0;
  if (!split_mode.empty())
    auto_split = std::stoi(split_mode);
  if (!stimeout.empty())
    tout = std::stoi(stimeout);

  is_single_frame_out = false;
  if (!auto_split && (tout < 0)) {
    RKMEDIA_LOGI("MPP Decoder: Enable single frame output mode!\n");
    is_single_frame_out = true;
  }

  if (decoder->SendInput(nullptr) < 0 && errno == ENOSYS)
    support_async = false;
  SetFlowTag("VideoDecoderFlow");
}

bool do_decode(Flow *f, MediaBufferVector &input_vector) {
  VideoDecoderFlow *flow = static_cast<VideoDecoderFlow *>(f);
  auto decoder = flow->decoder;
  auto &in = input_vector[0];
  int pkt_done = 0;
  int frm_got = 0;
  int times = 0;
  if (!in)
    return false;
  bool ret = true;
  std::shared_ptr<MediaBuffer> output;
  if (!flow->input_buffers.empty()) {
    flow->input_buffers.emplace_back(input_vector[0]);
    in = flow->input_buffers[0];
  }
  if (flow->support_async) {
    int send_ret = 0;
    do {
      send_ret = decoder->SendInput(in);
      if (!send_ret) {
        pkt_done = 1;
      } else if (times == 0) {
        flow->input_buffers.emplace_back(input_vector[0]);
        in = flow->input_buffers[0];
        times = 1;
      }

      do {
        output = decoder->FetchOutput();
        if (!output) {
          break;
        }

        frm_got = 1;
        ret = flow->SetOutput(output, 0);
        if (flow->is_single_frame_out)
          break;
      } while (true);

      if (pkt_done && !flow->input_buffers.empty()) {
        std::vector<std::shared_ptr<MediaBuffer>>::iterator head =
            flow->input_buffers.begin();
        flow->input_buffers.erase(head);
      }
      if (pkt_done || frm_got)
        break;

      msleep(3);
    } while (true);
  } else {
    output = std::make_shared<ImageBuffer>();
    if (decoder->Process(in, output))
      return false;
    ret = flow->SetOutput(output, 0);
  }
  return ret;
}

DEFINE_FLOW_FACTORY(VideoDecoderFlow, Flow)
// TODO!
const char *FACTORY(VideoDecoderFlow)::ExpectedInputDataType() { return ""; }
// TODO!
const char *FACTORY(VideoDecoderFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
