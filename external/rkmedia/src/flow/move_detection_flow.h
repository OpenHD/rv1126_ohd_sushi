// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __MOVE_DETECTION_FLOW__
#define __MOVE_DETECTION_FLOW__

#include <assert.h>

#include "flow.h"
#include "media_reflector.h"

#include <move_detect/move_detection.h>

#include "buffer.h"
#include "media_type.h"

namespace easymedia {

bool md_process(Flow *f, MediaBufferVector &input_vector);

class MoveDetectionFlow : public Flow {
public:
  MoveDetectionFlow(const char *param);
  virtual ~MoveDetectionFlow();
  static const char *GetFlowName() { return "move_detec"; }
  int Control(unsigned long int request, ...);
  std::shared_ptr<MediaBuffer> LookForMdResult(int64_t ustimestamp,
                                               int approximation);

protected:
  ROI_INFO *roi_in;
  int roi_cnt;
  struct md_ctx *md_ctx;
  int roi_enable;
  int Sensitivity;
  int update_mask;
  std::mutex md_roi_mtx;
  std::vector<ImageRect> new_roi;
  void InsertMdResult(std::shared_ptr<MediaBuffer> &buffer);

private:
  int is_single_ref;
  // orignal width, orignal height
  int ori_width, ori_height;
  int ds_width, ds_height;
  std::mutex md_results_mtx;
  std::condition_variable con_var;
  std::list<std::shared_ptr<MediaBuffer>> md_results;
  friend bool md_process(Flow *f, MediaBufferVector &input_vector);
};

} // namespace easymedia

#endif // __MOVE_DETECTION_FLOW__