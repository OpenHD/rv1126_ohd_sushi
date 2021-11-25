// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffer.h"
#include "filter.h"
#include "link_config.h"
#include "lock.h"
#include "rknn_user.h"
#include "rknn_utils.h"
#include "utils.h"
#include <assert.h>
#include <rockx/rockx.h>

namespace easymedia {

static char *enable_skip_frame = NULL;
static char *enable_rockx_debug = NULL;

class RockxContrl {
public:
  RockxContrl() = delete;
  RockxContrl(bool enable, int interval) {
    SetEnable(enable);
    SetInterval(interval);
  }
  virtual ~RockxContrl() {}

  bool CheckIsRun();

  void SetEnable(int enable) { enable_ = enable; }
  void SetInterval(int interval) { interval_ = interval; }
  // bool PercentageFilter(rockface_det_t *body);
  // bool DurationFilter(std::list<RknnResult> &list, int64_t timeval_ms);

  int GetEnable(void) const { return enable_; }
  int GetInterval(void) const { return interval_; }

private:
  bool enable_;
  int interval_;
};

bool RockxContrl::CheckIsRun() {
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

int64_t SystemTime2() {
  struct timespec t;
  t.tv_sec = t.tv_nsec = 0;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (int64_t)(t.tv_sec) * 1000 + t.tv_nsec / 1000000;
}

void DumpFps() {

  static int mFrameCount;
  static int mLastFrameCount = 0;
  static int mLastFpsTime = 0;
  static float mFps = 0;
  mFrameCount++;
  int64_t now = SystemTime2();
  int64_t diff = now - mLastFpsTime;
  if (diff > 500) {
    mFps = ((mFrameCount - mLastFrameCount) * 1000) / diff;
    mLastFpsTime = now;
    mLastFrameCount = mFrameCount;
    RKMEDIA_LOGI("---mFps = %2.3f\n", mFps);
  }
}

void SavePoseBodyImg(rockx_image_t input_image,
                     rockx_keypoints_array_t *body_array) {
  static int frame_num = 0;
  const std::vector<std::pair<int, int>> pose_pairs = {
      {2, 3},   {3, 4},   {5, 6},  {6, 7},  {8, 9},   {9, 10},
      {11, 12}, {12, 13}, {1, 0},  {0, 14}, {14, 16}, {0, 15},
      {15, 17}, {2, 5},   {8, 11}, {2, 8},  {5, 11}};
  // process result
  for (int i = 0; i < body_array->count; i++) {
    printf("asx   person %d:\n", i);

    for (int j = 0; j < body_array->keypoints[i].count; j++) {
      int x = body_array->keypoints[i].points[j].x;
      int y = body_array->keypoints[i].points[j].y;
      float score = body_array->keypoints[i].score[j];
      printf("  %s [%d, %d] %f\n", ROCKX_POSE_BODY_KEYPOINTS_NAME[j], x, y,
             score);
      rockx_image_draw_circle(&input_image, {x, y}, 3, {255, 0, 0}, -1);
    }

    for (int j = 0; j < (int)pose_pairs.size(); j++) {
      const std::pair<int, int> &posePair = pose_pairs[j];
      int x0 = body_array->keypoints[i].points[posePair.first].x;
      int y0 = body_array->keypoints[i].points[posePair.first].y;
      int x1 = body_array->keypoints[i].points[posePair.second].x;
      int y1 = body_array->keypoints[i].points[posePair.second].y;

      if (x0 > 0 && y0 > 0 && x1 > 0 && y1 > 0)
        rockx_image_draw_line(&input_image, {x0, y0}, {x1, y1}, {0, 255, 0}, 1);
    }
  }
  if (body_array != NULL) {
    char path[64];
    snprintf(path, 64, "/data/%d.jpg", frame_num++);
    rockx_image_write(path, &input_image);
  } else {
    char path[64];
    snprintf(path, 64, "/data/err_%d.rgb", frame_num++);
    FILE *fp = fopen(path, "wb");
    if (fp) {
      int stride =
          (input_image.pixel_format == ROCKX_PIXEL_FORMAT_RGB888) ? 3 : 2;
      int size = input_image.width * input_image.height * stride;
      fwrite(input_image.data, size, 1, fp);
      fclose(fp);
    }
  }
}

class ROCKXContent {
public:
  ROCKXContent(std::shared_ptr<easymedia::ImageBuffer> input,
               RknnCallBack callback)
      : input_buffer(input), callback_(callback){};
  virtual ~ROCKXContent() {
    if (input_buffer)
      input_buffer.reset();
  };

  void SetInputWidth(const int &w) { input_w = w; };
  void SetInputHeight(const int &h) { input_h = h; };
  void SetRockxHandle(const rockx_handle_t hdl) { hdl_ = hdl; };
  void SetInputType(const std::string &type) { input_type_ = type; };
  std::shared_ptr<easymedia::ImageBuffer> &GetInputBuffer() {
    return input_buffer;
  };
  std::string &GetInputType() { return input_type_; };
  rockx_handle_t GetRockxHdl() { return hdl_; };
  RknnCallBack GetCallbackPtr() { return callback_; };
  int GetInputWidth() { return input_w; };
  int GetInputHeight() { return input_h; };

private:
  std::shared_ptr<easymedia::ImageBuffer> input_buffer;
  std::string input_type_;
  RknnCallBack callback_;
  rockx_handle_t hdl_;
  int input_w;
  int input_h;
};

class ROCKXFilter : public Filter {
public:
  ROCKXFilter(const char *param);
  virtual ~ROCKXFilter();
  static const char *GetFilterName() { return "rockx_filter"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;
  virtual int IoCtrl(unsigned long int request, ...) override;

private:
  std::string model_name_;
  std::vector<rockx_handle_t> rockx_handles_;
  std::string input_type_;
  int async_enable;
  static RknnCallBack callback_;
  std::shared_ptr<RockxContrl> contrl_;
  ReadWriteLockMutex cb_mtx_;

  int ProcessRockxFaceDetect(
      std::shared_ptr<easymedia::ImageBuffer> input_buffer,
      rockx_image_t input_img, std::vector<rockx_handle_t> handles);
  int ProcessRockxFaceLandmark(
      std::shared_ptr<easymedia::ImageBuffer> input_buffer,
      rockx_image_t input_img, std::vector<rockx_handle_t> handles);
  int ProcessRockxPoseBody(std::shared_ptr<easymedia::ImageBuffer> input_buffer,
                           rockx_image_t input_img,
                           std::vector<rockx_handle_t> handles);
  int ProcessRockxFinger(std::shared_ptr<easymedia::ImageBuffer> input_buffer,
                         rockx_image_t input_img,
                         std::vector<rockx_handle_t> handles);
  int ProcessRockxObjectDetect(
      std::shared_ptr<easymedia::ImageBuffer> input_buffer,
      rockx_image_t input_img, std::vector<rockx_handle_t> handles);
  static void RockxPoseBodyAsyncCallback(void *result, size_t result_size,
                                         void *extra_data);
  static void RockxFaceDetectAsyncCallback(void *result, size_t result_size,
                                           void *extra_data);
};

RknnCallBack ROCKXFilter::callback_ = NULL;
std::mutex nn_mtx;
void RockxSendNNData(std::list<RknnResult> &nn_results,
                     const std::string &model_name,
                     const RknnCallBack callback) {
  int size = nn_results.size();
  if (!callback || size <= 0)
    return;

  linknndata_s link_nn_data;
  RknnResult *infos = (RknnResult *)malloc(size * sizeof(RknnResult));
  if (infos) {
    int i = 0;
    for (auto &iter : nn_results) {
      memcpy(&infos[i], &iter, sizeof(RknnResult));
      i++;
    }
  }
  link_nn_data.rknn_result = infos;
  link_nn_data.size = size;
  link_nn_data.nn_model_name = model_name.c_str();
  link_nn_data.timestamp = 0;
  nn_mtx.lock();
  callback(nullptr, LINK_NNDATA, &link_nn_data, 1);
  nn_mtx.unlock();
  if (infos)
    free(infos);
}

void RockxLandmarkPostProcess(rockx_image_t &input_img,
                              rockx_object_array_t *face_array,
                              const rockx_handle_t &hdl,
                              std::list<RknnResult> &nn_result) {
  if (hdl == 0)
    return;
  rockx_ret_t ret = ROCKX_RET_SUCCESS;
  RknnResult result_item;
  memset(&result_item, 0, sizeof(RknnResult));
  result_item.type = NNRESULT_TYPE_LANDMARK;
  for (int i = 0; i < face_array->count; i++) {
    rockx_face_landmark_t out_landmark;
    memset(&out_landmark, 0, sizeof(rockx_face_landmark_t));
    ret = rockx_face_landmark(hdl, &input_img, &face_array->object[i].box,
                              &out_landmark);
    if (ret != ROCKX_RET_SUCCESS && ret != -2) {
      fprintf(stderr, "rockx_face_landmark error %d\n", ret);
      return;
    }
    // memcpy(&result_item.landmark_info.object, &out_landmark,
    //        sizeof(rockx_face_landmark_t));
    result_item.img_w = input_img.width;
    result_item.img_h = input_img.height;
    result_item.landmark_info.object.image_width = out_landmark.image_width;
    result_item.landmark_info.object.image_height = out_landmark.image_height;
    result_item.landmark_info.object.face_box.left = out_landmark.face_box.left;
    result_item.landmark_info.object.face_box.top = out_landmark.face_box.top;
    result_item.landmark_info.object.face_box.right =
        out_landmark.face_box.right;
    result_item.landmark_info.object.face_box.bottom =
        out_landmark.face_box.bottom;
    result_item.landmark_info.object.score = out_landmark.score;
    result_item.landmark_info.object.landmarks_count =
        out_landmark.landmarks_count;
    for (int j = 0; j < out_landmark.landmarks_count &&
                    j < RKMEDIA_ROCKX_LANDMARK_MAX_COUNT;
         j++) {
      result_item.landmark_info.object.landmarks[j].x =
          out_landmark.landmarks[j].x;
      result_item.landmark_info.object.landmarks[j].y =
          out_landmark.landmarks[j].y;
    }
    nn_result.push_back(result_item);
  }
}

void ROCKXFilter::RockxPoseBodyAsyncCallback(void *result, size_t result_size,
                                             void *extra_data) {
  rockx_keypoints_array_t *key_points_array = (rockx_keypoints_array_t *)result;
  ROCKXContent *ctx = static_cast<ROCKXContent *>(extra_data);
  if (result_size <= 0 || !ctx)
    return;

  std::list<RknnResult> body_results;
  RknnResult result_item;

  memset(&result_item, 0, sizeof(RknnResult));
  result_item.type = NNRESULT_TYPE_BODY;
  for (int i = 0; i < key_points_array->count; i++) {
    rockx_keypoints_t *object = &key_points_array->keypoints[i];
    result_item.body_info.object.count = object->count;
    for (int j = 0; j < object->count; j++) {
      result_item.body_info.object.points[j].x = object->points[j].x;
      result_item.body_info.object.points[j].y = object->points[j].y;
      result_item.body_info.object.score[j] = object->score[j];
    }
    // memcpy(&result_item.body_info.object, object, sizeof(rockx_keypoints_t));
    result_item.img_w = ctx->GetInputWidth();
    result_item.img_h = ctx->GetInputHeight();
    body_results.push_back(result_item);
  }

  RknnCallBack nn_callback = ctx->GetCallbackPtr();
  std::string model_name = "rockx_pose_body";
  RockxSendNNData(body_results, model_name, nn_callback);
  delete ctx;
}

void ROCKXFilter::RockxFaceDetectAsyncCallback(void *result, size_t result_size,
                                               void *extra_data) {
  UNUSED(result_size);
  rockx_object_array_t *face_array = (rockx_object_array_t *)result;
  ROCKXContent *ctx = static_cast<ROCKXContent *>(extra_data);
  if (!ctx)
    return;

  if (!face_array || face_array->count <= 0) {
    if (ctx)
      delete ctx;
    return;
  }

  std::list<RknnResult> landmark_results;
  auto input_buffer = ctx->GetInputBuffer();
  rockx_image_t input_img;
  input_img.width = input_buffer->GetWidth();
  input_img.height = input_buffer->GetHeight();
  input_img.pixel_format = StrToRockxPixelFMT(ctx->GetInputType().c_str());
  input_img.data = (uint8_t *)input_buffer->GetPtr();

  RknnResult result_item;
  memset(&result_item, 0, sizeof(RknnResult));
  result_item.type = NNRESULT_TYPE_LANDMARK;
  RockxLandmarkPostProcess(input_img, face_array, ctx->GetRockxHdl(),
                           landmark_results);

  RknnCallBack nn_callback = ctx->GetCallbackPtr();
  std::string model_name = "rockx_face_landmark";
  RockxSendNNData(landmark_results, model_name, nn_callback);
  delete ctx;
}

ROCKXFilter::ROCKXFilter(const char *param) : model_name_("") {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }

  if (params[KEY_ROCKX_MODEL].empty()) {
    RKMEDIA_LOGI("lost rockx model info!\n");
    return;
  } else {
    model_name_ = params[KEY_ROCKX_MODEL];
  }

  if (params[KEY_INPUTDATATYPE].empty()) {
    RKMEDIA_LOGI("rockx lost input type.\n");
    return;
  } else {
    input_type_ = params[KEY_INPUTDATATYPE];
  }

  if (params[KEY_ROCKX_ASYNC_CALLBACK].empty()) {
    async_enable = 0;
  } else {
    async_enable = std::stoi(params[KEY_ROCKX_ASYNC_CALLBACK]);
  }

  std::vector<rockx_module_t> models;
  void *config = nullptr;
  size_t config_size = 0;
  if (model_name_ == "rockx_face_gender_age") {
    models.push_back(ROCKX_MODULE_FACE_DETECTION_V3);
    models.push_back(ROCKX_MODULE_FACE_LANDMARK_5);
    models.push_back(ROCKX_MODULE_FACE_ANALYZE);
  } else if (model_name_ == "rockx_face_detect") {
    models.push_back(ROCKX_MODULE_FACE_DETECTION_V3);
    models.push_back(ROCKX_MODULE_OBJECT_TRACK);
  } else if (model_name_ == "rockx_face_landmark") {
    models.push_back(ROCKX_MODULE_FACE_DETECTION_V3);
    models.push_back(ROCKX_MODULE_FACE_LANDMARK_68);
  } else if (model_name_ == "rockx_pose_body") {
    models.push_back(ROCKX_MODULE_POSE_BODY);
  } else if (model_name_ == "rockx_pose_finger") {
    models.push_back(ROCKX_MODULE_POSE_FINGER_21);
  } else if (model_name_ == "rockx_pose_body_v2") {
    models.push_back(ROCKX_MODULE_POSE_BODY_V2);
  } else if (model_name_ == "rockx_object_detect") {
    models.push_back(ROCKX_MODULE_OBJECT_DETECTION);
  } else {
    assert(0);
  }
  for (size_t i = 0; i < models.size(); i++) {
    rockx_handle_t npu_handle = nullptr;
    rockx_module_t &model = models[i];
    rockx_ret_t ret = rockx_create(&npu_handle, model, config, config_size);
    if (ret != ROCKX_RET_SUCCESS) {
      fprintf(stderr, "init rockx module %d error=%d\n", model, ret);
      SetError(-EINVAL);
      return;
    }
    rockx_handles_.push_back(npu_handle);
  }

  bool enable = false;
  const std::string &enable_str = params[KEY_ENABLE];
  if (!enable_str.empty())
    enable = std::stoi(enable_str);

  int interval = 0;
  const std::string &interval_str = params[KEY_FRAME_INTERVAL];
  if (!interval_str.empty())
    interval = std::stoi(interval_str);

  contrl_ = std::make_shared<RockxContrl>(enable, interval);

  if (!contrl_) {
    RKMEDIA_LOGI("rockx contrl is nullptr.\n");
    return;
  }

  callback_ = nullptr;

  enable_rockx_debug = getenv("ENABLE_ROCKX_DEBUG");
  enable_skip_frame = getenv("ENABLE_SKIP_FRAME");
}

ROCKXFilter::~ROCKXFilter() {
  for (auto handle : rockx_handles_)
    rockx_destroy(handle);
}

int ROCKXFilter::ProcessRockxFaceDetect(
    std::shared_ptr<easymedia::ImageBuffer> input_buffer,
    rockx_image_t input_img, std::vector<rockx_handle_t> handles) {
  rockx_handle_t &face_det_handle = handles[0];
  rockx_handle_t &object_track_handle = handles[1];
  rockx_object_array_t face_array;
  rockx_object_array_t face_array_track;
  memset(&face_array, 0, sizeof(rockx_object_array_t));
  rockx_ret_t ret =
      rockx_face_detect(face_det_handle, &input_img, &face_array, nullptr);
  if (ret != ROCKX_RET_SUCCESS) {
    fprintf(stderr, "rockx_face_detect error %d\n", ret);
    return -1;
  }
  if (face_array.count <= 0)
    return -1;
  ret = rockx_object_track(object_track_handle, input_img.width,
                           input_img.height, 5, &face_array, &face_array_track);
  if (ret != ROCKX_RET_SUCCESS) {
    fprintf(stderr, "rockx_object_track error %d\n", ret);
    return -1;
  }

  RknnResult result_item;
  memset(&result_item, 0, sizeof(RknnResult));
  result_item.type = NNRESULT_TYPE_FACE;
  auto &nn_result = input_buffer->GetRknnResult();
  for (int i = 0; i < face_array_track.count; i++) {
    rockx_object_t *object = &face_array_track.object[i];
    // memcpy(&result_item.face_info.object, object, sizeof(rockx_object_t));
    result_item.face_info.object.cls_idx = object->cls_idx;
    result_item.face_info.object.score = object->score;
    result_item.face_info.object.box.left = object->box.left;
    result_item.face_info.object.box.top = object->box.top;
    result_item.face_info.object.box.right = object->box.right;
    result_item.face_info.object.box.bottom = object->box.bottom;
    result_item.img_w = input_img.width;
    result_item.img_h = input_img.height;
    nn_result.push_back(result_item);
  }
  return 0;
}

int ROCKXFilter::ProcessRockxFaceLandmark(
    std::shared_ptr<easymedia::ImageBuffer> input_buffer,
    rockx_image_t input_img, std::vector<rockx_handle_t> handles) {
  if (enable_skip_frame) {
    int static count = 0;
    int static divisor = atoi(enable_skip_frame);
    if (count++ % divisor != 0)
      return -1;
    if (count == 1000)
      count = 0;
  }
  rockx_handle_t &face_det_handle = handles[0];
  rockx_handle_t &face_landmark_handle = handles[1];
  rockx_object_array_t face_array;
  memset(&face_array, 0, sizeof(rockx_object_array_t));
  rockx_ret_t ret = ROCKX_RET_SUCCESS;

  if (async_enable) {
    rockx_async_callback async_callback;
    ROCKXContent *ctx = new ROCKXContent(input_buffer, callback_);
    ctx->SetInputWidth(input_img.width);
    ctx->SetInputHeight(input_img.height);
    ctx->SetRockxHandle(face_landmark_handle);
    ctx->SetInputType(input_type_);
    async_callback.callback_func = RockxFaceDetectAsyncCallback;
    async_callback.extra_data = (void *)ctx;
    ret = rockx_face_detect(face_det_handle, &input_img, &face_array,
                            &async_callback);
  } else {
    ret = rockx_face_detect(face_det_handle, &input_img, &face_array, NULL);
  }

  if (ret != ROCKX_RET_SUCCESS) {
    RKMEDIA_LOGI("rockx_face_detect error %d\n", ret);
    return -1;
  }

  if (face_array.count <= 0) {
    return -1;
  }

  auto &nn_result = input_buffer->GetRknnResult();
  RockxLandmarkPostProcess(input_img, &face_array, face_landmark_handle,
                           nn_result);

  return 0;
}

int ROCKXFilter::ProcessRockxPoseBody(
    std::shared_ptr<easymedia::ImageBuffer> input_buffer,
    rockx_image_t input_img, std::vector<rockx_handle_t> handles) {
  if (enable_skip_frame) {
    int static count = 0;
    int static divisor = atoi(enable_skip_frame);
    if (count++ % divisor != 0)
      return -1;
    if (count == 1000)
      count = 0;
  }

  rockx_handle_t &pose_body_handle = handles[0];
  rockx_keypoints_array_t key_points_array;
  memset(&key_points_array, 0, sizeof(rockx_keypoints_array_t));
  rockx_ret_t ret = ROCKX_RET_SUCCESS;
  if (async_enable > 0) {
    rockx_async_callback async_callback;
    ROCKXContent *ctx = new ROCKXContent(nullptr, callback_);
    ctx->SetInputWidth(input_img.width);
    ctx->SetInputHeight(input_img.height);
    async_callback.callback_func = RockxPoseBodyAsyncCallback;
    async_callback.extra_data = (void *)ctx;
    ret = rockx_pose_body(pose_body_handle, &input_img, &key_points_array,
                          &async_callback);
  } else
    ret =
        rockx_pose_body(pose_body_handle, &input_img, &key_points_array, NULL);

  if (ret != ROCKX_RET_SUCCESS) {
    RKMEDIA_LOGI("rockx_face_detect error %d\n", ret);
    return -1;
  }
  if (key_points_array.count <= 0) {
#ifdef DEBUG_POSE_BODY
    SavePoseBodyImg(input_img, NULL);
#endif
    return -1;
  }

  RknnResult result_item;
  memset(&result_item, 0, sizeof(RknnResult));
  result_item.type = NNRESULT_TYPE_BODY;
  auto &nn_result = input_buffer->GetRknnResult();
  for (int i = 0; i < key_points_array.count; i++) {
    rockx_keypoints_t *object = &key_points_array.keypoints[i];
    result_item.body_info.object.count = object->count;
    for (int j = 0; j < object->count; j++) {
      result_item.body_info.object.points[j].x = object->points[j].x;
      result_item.body_info.object.points[j].y = object->points[j].y;
      result_item.body_info.object.score[j] = object->score[j];
    }
    // memcpy(&result_item.body_info.object, object, sizeof(rockx_keypoints_t));
    result_item.img_w = input_img.width;
    result_item.img_h = input_img.height;
    nn_result.push_back(result_item);
  }
#ifdef DEBUG_POSE_BODY
  savePoseBodyImg(input_img, &key_points_array);
#endif
  return 0;
}

int ROCKXFilter::ProcessRockxFinger(
    std::shared_ptr<easymedia::ImageBuffer> input_buffer,
    rockx_image_t input_img, std::vector<rockx_handle_t> handles) {
  (void)input_buffer;
  rockx_handle_t &pose_finger_handle = handles[0];
  rockx_keypoints_t finger;
  int ret;

  memset(&finger, 0, sizeof(rockx_keypoints_t));
  ret = rockx_pose_finger(pose_finger_handle, &input_img, &finger);
  if (ret != ROCKX_RET_SUCCESS) {
    printf("rockx_pose_finger error %d\n", ret);
    return -1;
  }
  return 0;
}

int ROCKXFilter::ProcessRockxObjectDetect(
    std::shared_ptr<easymedia::ImageBuffer> input_buffer,
    rockx_image_t input_img, std::vector<rockx_handle_t> handles) {
  (void)input_buffer;
  rockx_handle_t &object_detect_handle = handles[0];
  rockx_object_array_t object_array;
  rockx_ret_t ret;

  memset(&object_array, 0, sizeof(rockx_object_array_t));
  ret = rockx_object_detect(object_detect_handle, &input_img, &object_array,
                            NULL);
  if (ret != ROCKX_RET_SUCCESS) {
    printf("rockx_object_detect error %d\n", ret);
    return -1;
  }

  auto image = std::static_pointer_cast<easymedia::ImageBuffer>(input_buffer);
  auto &nn_list = image->GetRknnResult();
  RknnResult nn_result;
  memset(&nn_result, 0, sizeof(RknnResult));
  for (int i = 0; i < object_array.count; i++) {
    // rockx_object_t *object_g = &(object_array.object[i]);
    nn_result.object_info.cls_idx = object_array.object[i].cls_idx;
    nn_result.object_info.score = object_array.object[i].score;
    nn_result.object_info.box.left = object_array.object[i].box.left;
    nn_result.object_info.box.top = object_array.object[i].box.top;
    nn_result.object_info.box.right = object_array.object[i].box.right;
    nn_result.object_info.box.bottom = object_array.object[i].box.bottom;
    nn_list.push_back(nn_result);
  }

  RknnResult nn_array[object_array.count];
  for (int i = 0; i < object_array.count; i++) {
    nn_array[i].timeval = input_buffer->GetAtomicClock();
    nn_array[i].img_w = input_img.width;
    nn_array[i].img_h = input_img.height;
    nn_array[i].type = NNRESULT_TYPE_OBJECT_DETECT;
    // memcpy(&nn_array[i].object_info, &object_array.object[i],
    //        sizeof(rockx_object_t));
    nn_array[i].object_info.score = object_array.object[i].score;
    nn_array[i].object_info.cls_idx = object_array.object[i].cls_idx;
    nn_array[i].object_info.box.left = object_array.object[i].box.left;
    nn_array[i].object_info.box.top = object_array.object[i].box.top;
    nn_array[i].object_info.box.right = object_array.object[i].box.right;
    nn_array[i].object_info.box.bottom = object_array.object[i].box.bottom;
  }
  if (callback_)
    callback_(this, NNRESULT_TYPE_OBJECT_DETECT, nn_array, object_array.count);

  return 0;
}

int ROCKXFilter::Process(std::shared_ptr<MediaBuffer> input,
                         std::shared_ptr<MediaBuffer> &output) {
  auto input_buffer = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  rockx_image_t input_img;
  input_img.width = input_buffer->GetWidth();
  input_img.height = input_buffer->GetHeight();
  input_img.pixel_format = StrToRockxPixelFMT(input_type_.c_str());
  input_img.data = (uint8_t *)input_buffer->GetPtr();

  output = input;

  if (!contrl_->CheckIsRun())
    return 0;

  auto &name = model_name_;
  auto &handles = rockx_handles_;
  if (enable_rockx_debug)
    RKMEDIA_LOGI("ROCKXFilter::Process %s begin \n", model_name_.c_str());
  if (name == "rockx_face_detect") {
    ProcessRockxFaceDetect(input_buffer, input_img, handles);
  } else if (name == "rockx_face_landmark") {
    ProcessRockxFaceLandmark(input_buffer, input_img, handles);
  } else if (name == "rockx_pose_body" || name == "rockx_pose_body_v2") {
    ProcessRockxPoseBody(input_buffer, input_img, handles);
  } else if (name == "rockx_pose_finger") {
    ProcessRockxFinger(input_buffer, input_img, handles);
  } else if (name == "rockx_object_detect") {
    ProcessRockxObjectDetect(input_buffer, input_img, handles);
  } else {
    assert(0);
  }
  if (enable_rockx_debug)
    RKMEDIA_LOGI("ROCKXFilter::Process %s end \n", model_name_.c_str());
  return 0;
}

int ROCKXFilter::IoCtrl(unsigned long int request, ...) {
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
    RockxFilterArg *arg = va_arg(vl, RockxFilterArg *);
    if (arg) {
      contrl_->SetEnable(arg->enable);
      contrl_->SetInterval(arg->interval);
    }
  } break;
  case G_NN_INFO: {
    RockxFilterArg *arg = va_arg(vl, RockxFilterArg *);
    if (arg) {
      arg->enable = contrl_->GetEnable();
      arg->interval = contrl_->GetInterval();
    }
  } break;
  default:
    ret = -1;
    break;
  }
  va_end(vl);
  return ret;
}

DEFINE_COMMON_FILTER_FACTORY(ROCKXFilter)
const char *FACTORY(ROCKXFilter)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(ROCKXFilter)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace easymedia
