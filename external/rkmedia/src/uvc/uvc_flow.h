// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __UVC_FLOW__
#define __UVC_FLOW__

#include <assert.h>

#include "flow.h"
#include "media_reflector.h"

extern "C" {
#include <uvc/mpi_enc.h>
#include <uvc/uvc_control.h>
#include <uvc/uvc_video.h>
}

#include "buffer.h"
#include "image.h"
#include "key_string.h"
#include "media_config.h"
#include "media_type.h"
#include "mpp_inc.h"
#include "utils.h"

namespace easymedia {
bool do_uvc(Flow *f, MediaBufferVector &input_vector);

class UvcFlow : public Flow {
public:
  UvcFlow(const char *param);
  virtual ~UvcFlow();
  static const char *GetFlowName() { return "uvc_flow"; }
  int Control(unsigned long int request, ...);

private:
  uint32_t uvc_event_code;
  // std::string model_identifier;
  uint32_t uvc_width;
  uint32_t uvc_height;
  std::string uvc_format;
  friend bool do_uvc(Flow *f, MediaBufferVector &input_vector);
};

} // namespace easymedia

#endif // __UVC_FLOW__
