// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <assert.h>
#include <math.h>

#include "uvc_flow.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif

namespace easymedia {

bool do_uvc(Flow *f, MediaBufferVector &input_vector) {
  UvcFlow *flow = (UvcFlow *)f;
  auto img_buf = input_vector[0];
  // printf("do_uvc:++++++");
  if (!img_buf || img_buf->GetType() != Type::Image)
    return false;

  auto img = std::static_pointer_cast<ImageBuffer>(img_buf);
  MppFrameFormat ifmt = ConvertToMppPixFmt(img->GetPixelFormat());
  assert(ifmt == MPP_FMT_YUV420SP);
  // RKMEDIA_LOGI("ifmt: %d,size: %d\n", ifmt, (int)img_buf->GetValidSize());
  if (ifmt < 0) {
    RKMEDIA_LOGE("no support frame format,only nv12,uvc_width:%d",
                 flow->uvc_width);
    return false;
  }
  mpi_enc_set_format(ifmt);

  uvc_read_camera_buffer(img_buf->GetPtr(), img_buf->GetFD(),
                         img_buf->GetValidSize(), NULL, 0);

  return true;
}

UvcFlow::UvcFlow(const char *param)
    : uvc_event_code(-1), uvc_width(640), uvc_height(480), uvc_format("mjpeg") {
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;

  if (!ParseWrapFlowParams(param, params, separate_list)) {
    RKMEDIA_LOGE("UVC: flow param error!\n");
    SetError(-EINVAL);
    return;
  }

  std::string key_name = params[KEY_NAME];
  // check input/output type
  std::string &&rule = gen_datatype_rule(params);
  if (rule.empty()) {
    SetError(-EINVAL);
    return;
  }

  if (!REFLECTOR(Flow)::IsMatch("uvc_flow", rule.c_str())) {
    RKMEDIA_LOGE("Unsupport for uvc_flow : [%s]\n", rule.c_str());
    SetError(-EINVAL);
    return;
  }

  const std::string &md_param_str = separate_list.back();
  std::map<std::string, std::string> uvc_params;
  if (!parse_media_param_map(md_param_str.c_str(), uvc_params)) {
    RKMEDIA_LOGE("UVC: uvc param error!\n");
    SetError(-EINVAL);
    return;
  }

  std::string value;
  CHECK_EMPTY_SETERRNO(value, uvc_params, KEY_UVC_EVENT_CODE, 0)
  uvc_event_code = std::stoi(value);
  CHECK_EMPTY_SETERRNO(value, uvc_params, KEY_UVC_WIDTH, 0)
  uvc_width = std::stoi(value);
  CHECK_EMPTY_SETERRNO(value, uvc_params, KEY_UVC_HEIGHT, 0)
  uvc_height = std::stoi(value);
  CHECK_EMPTY_SETERRNO(value, uvc_params, KEY_UVC_FORMAT, 0)
  uvc_format = value;

  RKMEDIA_LOGI("UVC: param: uvc_event_code=%d\n", uvc_event_code);
  RKMEDIA_LOGI("UVC: param: uvc_width=%d\n", uvc_width);
  RKMEDIA_LOGI("UVC: param: uvc_height=%d\n", uvc_height);
  RKMEDIA_LOGI("UVC: param: uvc_format=%s\n", uvc_format.c_str());

  SlotMap sm;
  sm.thread_model = Model::ASYNCCOMMON;
  sm.mode_when_full = InputMode::DROPFRONT;
  sm.input_slots.push_back(0);
  sm.input_maxcachenum.push_back(2);
  sm.fetch_block.push_back(true);
  if (true) {
    sm.input_slots.push_back(1);
    sm.input_maxcachenum.push_back(1);
    sm.fetch_block.push_back(false);
  }
  sm.process = do_uvc;
  if (!InstallSlotMap(sm, "UvcFlow", 0)) {
    fprintf(stderr, "Fail to InstallSlotMap, %s\n", "uvc_join");
    SetError(-EINVAL);
    return;
  }
  if (uvc_event_code) {
    uvc_control_run(UVC_CONTROL_LOOP_ONCE);
  } else {
    uvc_control_run(UVC_CONTROL_CHECK_STRAIGHT);
  }
}

UvcFlow::~UvcFlow() {
  AutoPrintLine apl(__func__);
  StopAllThread();
  if (uvc_event_code) {
    uvc_control_join(UVC_CONTROL_LOOP_ONCE);
  } else {
    uvc_control_join(UVC_CONTROL_CHECK_STRAIGHT);
  }
}

int UvcFlow::Control(unsigned long int request, ...) {
  va_list ap;
  va_start(ap, request);
  // auto value = va_arg(ap, std::shared_ptr<ParameterBuffer>);
  va_end(ap);
  // assert(value);
  return 0;
}

DEFINE_FLOW_FACTORY(UvcFlow, Flow)
// type depends on encoder
const char *FACTORY(UvcFlow)::ExpectedInputDataType() { return ""; }
const char *FACTORY(UvcFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
