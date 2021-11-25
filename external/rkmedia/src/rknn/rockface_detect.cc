// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffer.h"
#include "filter.h"
#include "lock.h"
#include "rknn_utils.h"
#include "rockface/rockface.h"

#define DEFAULT_LIC_PATH "/userdata/key.lic"

namespace easymedia {

#define FACE_TRACK_START_FRAME (4)
#define FACE_DETECT_DATA_VERSION (4)

class RockFaceDetect : public Filter {
public:
  RockFaceDetect(const char *param);
  virtual ~RockFaceDetect();
  static const char *GetFilterName() { return "rockface_detect"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;
  virtual int IoCtrl(unsigned long int request, ...) override;

  bool FaceDetect(std::shared_ptr<easymedia::ImageBuffer> image,
                  rockface_det_array_t *face_array);
  void SendNNResult(std::list<RknnResult> &list,
                    std::shared_ptr<ImageBuffer> image);

protected:
  bool CheckFaceReturn(const char *fun, int ret);

private:
  bool enable_;

  int track_frame_;
  float score_threshod_;

  rockface_pixel_format pixel_fmt_;
  rockface_handle_t face_handle_;

  RknnResult authorized_result_;
  RknnCallBack callback_;
  ReadWriteLockMutex cb_mtx_;
};

RockFaceDetect::RockFaceDetect(const char *param) : callback_(nullptr) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  if (params[KEY_INPUTDATATYPE].empty()) {
    RKMEDIA_LOGI("lost input pixel format!\n");
    return;
  } else {
    pixel_fmt_ = StrToRockFacePixelFMT(params[KEY_INPUTDATATYPE].c_str());
    if (pixel_fmt_ >= ROCKFACE_PIXEL_FORMAT_MAX) {
      RKMEDIA_LOGI("input pixel format not support yet!\n");
      return;
    }
  }
  std::string license_path = DEFAULT_LIC_PATH;
  if (params[KEY_PATH].empty()) {
    RKMEDIA_LOGI("use default license file path:%s\n", license_path.c_str());
  } else {
    license_path = params[KEY_PATH];
  }

  enable_ = false;
  const std::string &enable_str = params[KEY_ENABLE];
  if (!enable_str.empty())
    enable_ = std::stoi(enable_str);

  score_threshod_ = 0.0;
  const std::string &score_threshod = params[KEY_SCORE_THRESHOD];
  if (!score_threshod.empty())
    score_threshod_ = std::stof(score_threshod);

  track_frame_ = 3;
  const std::string &track_frame = params[KEY_FACE_DETECT_TRACK_FRAME];
  if (!track_frame.empty())
    track_frame_ = atoi(track_frame.c_str());

  rockface_ret_t ret;
  face_handle_ = rockface_create_handle();

  authorized_result_.type = NNRESULT_TYPE_AUTHORIZED_STATUS;
  ret = rockface_set_licence(face_handle_, license_path.c_str());
  if (ret != ROCKFACE_RET_SUCCESS)
    authorized_result_.status = FAILURE;
  else
    authorized_result_.status = SUCCESS;

  ret = rockface_init_detector2(face_handle_, FACE_DETECT_DATA_VERSION);
  if (ret != ROCKFACE_RET_SUCCESS) {
    RKMEDIA_LOGI("rockface_init_detector2 failed. ret = %d\n", ret);
    return;
  }
  if (authorized_result_.status != SUCCESS)
    RKMEDIA_LOGI("rockface detect authorize failed.\n");
}

RockFaceDetect::~RockFaceDetect() {
  if (face_handle_)
    rockface_release_handle(face_handle_);
}

bool RockFaceDetect::FaceDetect(std::shared_ptr<easymedia::ImageBuffer> image,
                                rockface_det_array_t *face_array) {
  if (!image || !face_array)
    return false;

  static int64_t frame_idx = 0;
  rockface_image_t input_image;
  input_image.width = image->GetVirWidth();
  input_image.height = image->GetVirHeight();
  input_image.pixel_format = pixel_fmt_;
  input_image.is_prealloc_buf = 1;
  input_image.data = (uint8_t *)image->GetPtr();
  input_image.size = image->GetValidSize();
  if (authorized_result_.status == TIMEOUT)
    return false;

  rockface_ret_t ret;
  AutoDuration cost_time;
  bool auto_track = (frame_idx < FACE_TRACK_START_FRAME)
                        ? 0
                        : (frame_idx % (track_frame_ + 1) == 0 ? 0 : 1);
  if (!auto_track) {
    rockface_det_array_t det_array;
    ret = rockface_detect(face_handle_, &input_image, &det_array);
    if (!CheckFaceReturn("rockface_detect", ret))
      return false;
    *face_array = det_array;
  }
  RKMEDIA_LOGD("rockface_detect cost time %lldus\n", cost_time.GetAndReset());

  static int first_face_log = false;
  if (!first_face_log) {
    RKMEDIA_LOGI("First face detected\n");
    first_face_log = true;
  }

  rockface_det_array_t track_array;
  ret = rockface_autotrack(face_handle_, &input_image, 4, face_array,
                           &track_array, auto_track);
  if (!CheckFaceReturn("rockface_autotrack", ret))
    return false;
  *face_array = track_array;
  RKMEDIA_LOGD("rockface_autotrack cost time %lldus\n",
               cost_time.GetAndReset());

  frame_idx++;
  return true;
}

int RockFaceDetect::Process(std::shared_ptr<MediaBuffer> input,
                            std::shared_ptr<MediaBuffer> &output) {
  auto image = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  if (!image)
    return -1;

  RknnResult nn_result;
  memset(&nn_result, 0, sizeof(RknnResult));
  nn_result.type = NNRESULT_TYPE_FACE;
  auto &nn_list = image->GetRknnResult();

  if (!enable_)
    goto exit;

  // flush cache,  2688x1520 NV12 cost 1399us, 1080P cost 905us
  image->BeginCPUAccess(false);
  image->EndCPUAccess(false);

  rockface_det_array_t face_array;
  memset(&face_array, 0, sizeof(rockface_det_array_t));
  if (!FaceDetect(image, &face_array))
    goto exit;

  for (int i = 0; i < face_array.count; i++) {
    rockface_det_t *face = &(face_array.face[i]);
    if (face->score - score_threshod_ < 0) {
      RKMEDIA_LOGD("Drop the face, score = %f\n", face->score);
      continue;
    }
    RKMEDIA_LOGD("face[%d] rect[%d, %d, %d, %d]\n", i, face->box.left,
                 face->box.top, face->box.right, face->box.bottom);

    nn_result.face_info.base = *face;
    nn_list.push_back(nn_result);
  }
exit:
  SendNNResult(nn_list, image);
  output = input;
  return 0;
}

bool RockFaceDetect::CheckFaceReturn(const char *fun, int ret) {
  bool check = true;
  if (ret != ROCKFACE_RET_SUCCESS) {
    if (ret == ROCKFACE_RET_AUTH_FAIL) {
      authorized_result_.status = TIMEOUT;
      if (callback_) {
        AutoLockMutex lock(cb_mtx_);
        callback_(this, NNRESULT_TYPE_AUTHORIZED_STATUS, &authorized_result_,
                  1);
      }
    }
    check = false;
  }
  if (!check)
    RKMEDIA_LOGI("%s run failed, ret = %d\n", fun, ret);
  return check;
}

void RockFaceDetect::SendNNResult(std::list<RknnResult> &list,
                                  std::shared_ptr<ImageBuffer> image) {
  AutoLockMutex lock(cb_mtx_);
  if (!callback_ || list.empty())
    return;

  int count = 0;
  int size = list.size();
  RknnResult nn_array[size];
  for (auto &iter : list) {
    if (iter.type != NNRESULT_TYPE_FACE)
      continue;
    nn_array[count].timeval = image->GetAtomicClock();
    nn_array[count].img_w = image->GetWidth();
    nn_array[count].img_h = image->GetHeight();
    nn_array[count].face_info.base = iter.face_info.base;
    nn_array[count].type = NNRESULT_TYPE_FACE;
    count++;
  }
  callback_(this, NNRESULT_TYPE_FACE, nn_array, count);
}

int RockFaceDetect::IoCtrl(unsigned long int request, ...) {
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
    FaceDetectArg *arg = va_arg(vl, FaceDetectArg *);
    if (arg) {
      enable_ = arg->enable;
    }
  } break;
  case G_NN_INFO: {
    FaceDetectArg *arg = va_arg(vl, FaceDetectArg *);
    if (arg) {
      arg->enable = enable_;
    }
  } break;
  default:
    ret = -1;
    break;
  }
  va_end(vl);
  return ret;
}

DEFINE_COMMON_FILTER_FACTORY(RockFaceDetect)
const char *FACTORY(RockFaceDetect)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(RockFaceDetect)::OutPutDataType() { return TYPE_ANYTHING; }
} // namespace easymedia
