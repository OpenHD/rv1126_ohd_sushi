// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <rockface/rockface.h>

#include "buffer.h"
#include "filter.h"
#include "lock.h"
#include "rknn_user.h"
#include "rknn_utils.h"

namespace easymedia {

static int GetFaceSize(FaceInfo &face) {
  return (face.base.box.right - face.base.box.left) *
         (face.base.box.bottom - face.base.box.top);
}

class RockFaceEvaluate : public Filter {
public:
  RockFaceEvaluate(const char *param) { UNUSED(param); }
  virtual ~RockFaceEvaluate() {}

  static const char *GetFilterName() { return "rockface_evaluate"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;
  virtual int IoCtrl(unsigned long int request, ...) override;

protected:
  bool Evaluate(std::list<RknnResult> &list, RknnResult *result);
};

int RockFaceEvaluate::Process(std::shared_ptr<MediaBuffer> input,
                              std::shared_ptr<MediaBuffer> &output) {
  auto input_buffer = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  if (!input_buffer)
    return -1;

  RknnResult best;
  auto &list = input_buffer->GetRknnResult();
  bool ret = Evaluate(list, &best);
  // Clear all faces in the list
  std::list<RknnResult>::iterator it = list.begin();
  for (; it != list.end();) {
    if ((*it).type == NNRESULT_TYPE_FACE)
      it = list.erase(it);
    else
      it++;
  }
  if (ret)
    list.push_back(best);
  output = input;
  return 0;
}

bool RockFaceEvaluate::Evaluate(std::list<RknnResult> &list,
                                RknnResult *result) {
  int max_size = -1;
  for (RknnResult &it : list) {
    if (it.type != NNRESULT_TYPE_FACE)
      continue;

    int face_size = GetFaceSize(it.face_info);
    if (face_size > max_size) {
      *result = it;
      max_size = face_size;
    }
  }
  return (max_size > 0 ? true : false);
}

int RockFaceEvaluate::IoCtrl(unsigned long int request, ...) {
  va_list vl;
  va_start(vl, request);
  void *arg = va_arg(vl, void *);
  va_end(vl);

  if (!arg)
    return -1;

  int ret = 0;
  switch (request) {
  default:
    ret = -1;
    break;
  }
  return ret;
}

DEFINE_COMMON_FILTER_FACTORY(RockFaceEvaluate)
const char *FACTORY(RockFaceEvaluate)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(RockFaceEvaluate)::OutPutDataType() {
  return TYPE_ANYTHING;
}

} // namespace easymedia
