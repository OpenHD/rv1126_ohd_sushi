// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <condition_variable>
#include <mutex>
#include <thread>

#include "buffer.h"
#include "filter.h"
#include "lock.h"
#include "rga_filter.h"
#include "rknn_utils.h"
#include "rockface_db_manager.h"
#include "utils.h"

#define DEFAULT_LIC_PATH "/userdata/key.lic"
#define FACE_DETECT_DATA_VERSION (4)

namespace easymedia {

class RockFaceRecognize;

enum class RequestType { NONE, RECOGNIZE, REGISTER };

class FaceRecognizeRequest {
public:
  FaceRecognizeRequest(RockFaceRecognize *handle,
                       std::shared_ptr<ImageBuffer> image,
                       RequestType type = RequestType::NONE);
  FaceRecognizeRequest(RockFaceRecognize *handle, std::string pic_path,
                       RequestType type = RequestType::NONE);
  virtual ~FaceRecognizeRequest();
  rockface_image_t &GetInputImage();

  std::vector<rockface_det_t> &GetFaceArray(void) { return face_array_; }

  std::string GetPicPath(void) const { return pic_path_; }
  RequestType GetType(void) const { return type_; }

  void SetType(RequestType type) { type_ = type; }

private:
  RockFaceRecognize *handle_;
  RequestType type_;

  std::string pic_path_;
  std::shared_ptr<ImageBuffer> image_;

  rockface_image_t input_image_;
  std::vector<rockface_det_t> face_array_;
};

class RockFaceRecognize : public Filter {
public:
  RockFaceRecognize(const char *param);
  virtual ~RockFaceRecognize();
  static const char *GetFilterName() { return "rockface_recognize"; }
  virtual int SendInput(std::shared_ptr<MediaBuffer> input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

  virtual int IoCtrl(unsigned long int request, ...) override;

  void ThreadLoop(void);

  int MatchFeature(rockface_feature_t *feature, float *out_similarity);

  FaceReg Recognize(rockface_feature_t &feature,
                    std::shared_ptr<FaceRecognizeRequest> request);
  bool FindRecognizedWithId(int face_id, FaceReg *face_reg,
                            std::vector<RknnResult> &last_result);
  void BatchProcesPicture(RequestType type,
                          std::list<std::string> &picture_list);

  bool PushRequest(std::shared_ptr<FaceRecognizeRequest> request);
  std::shared_ptr<FaceRecognizeRequest> PopRequest(void);

  bool wait_for(std::unique_lock<std::mutex> &lock, uint32_t seconds) {
    if (cond_.wait_for(lock, std::chrono::seconds(seconds)) ==
        std::cv_status::timeout)
      return false;
    else
      return true;
  }
  void signal(void) { cond_.notify_all(); }

  class RequestFinder {
  public:
    RequestFinder(RequestType type) : type_(type) {}
    bool operator()(
        std::list<std::shared_ptr<FaceRecognizeRequest>>::value_type &request) {
      if (request->GetType() == type_)
        return true;
      else
        return false;
    }

  private:
    const RequestType type_;
  };

protected:
  bool CheckIsRunning(void);
  bool CheckFaceReturn(rockface_ret_t ret, const char *fun,
                       bool is_abort = false);
  bool ThreadIsRunning(void) const { return thread_running_; }

private:
  static unsigned int kMaxCacheSize;
  static float kFaceSimilarityThreshod;

  bool enable_;
  bool thread_running_;
  bool enable_face_detect_;

  std::atomic_int tobe_registered_count_;      /* for camera register */
  std::list<std::string> tobe_registered_pic_; /* for picture register */
  std::list<std::string> tobe_recognized_pic_; /* for picture recognize */

  std::mutex mutex_;
  std::condition_variable cond_;

  RknnCallBack callback_;
  ReadWriteLockMutex cb_mtx_;
  RknnResult authorized_result_;

  rockface_pixel_format pixel_fmt_;
  rockface_handle_t face_handle_;

  std::shared_ptr<std::thread> thread_;
  std::shared_ptr<FaceDBManager> db_manager_;
  std::list<std::shared_ptr<FaceRecognizeRequest>> request_list_;

  friend class FaceRecognizeRequest;
};

static void ThrFunc(void *param) {
  RockFaceRecognize *process = (RockFaceRecognize *)param;
  if (process)
    process->ThreadLoop();
}

unsigned int RockFaceRecognize::kMaxCacheSize = 3;
float RockFaceRecognize::kFaceSimilarityThreshod = 0.7;

RockFaceRecognize::RockFaceRecognize(const char *param)
    : tobe_registered_count_(0), callback_(nullptr) {
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

  enable_ = false;
  const std::string &reg_enable_str = params[KEY_ENABLE_FACE_REG];
  if (!reg_enable_str.empty())
    enable_ = std::stoi(reg_enable_str);

  thread_running_ = false;
  const std::string &enable_str = params[KEY_ENABLE];
  if (!enable_str.empty())
    thread_running_ = std::stoi(enable_str);

  const std::string &db_path = params[KEY_DB_PATH];
  if (!db_path.empty())
    db_manager_ = std::make_shared<FaceDBManager>(db_path);
  else {
    RKMEDIA_LOGI("lost db path.\n");
    return;
  }

  enable_face_detect_ = false;
  const std::string &enable_face_detect = params[KEY_ENBALE_FACE_DETECT];
  if (!enable_face_detect.empty())
    enable_face_detect_ = std::stoi(enable_face_detect);

  std::string license_path = DEFAULT_LIC_PATH;
  if (!params[KEY_PATH].empty())
    license_path = params[KEY_PATH];

  rockface_ret_t ret;
  face_handle_ = rockface_create_handle();

  authorized_result_.type = NNRESULT_TYPE_AUTHORIZED_STATUS;
  ret = rockface_set_licence(face_handle_, license_path.c_str());
  if (ret != ROCKFACE_RET_SUCCESS) {
    authorized_result_.status = FAILURE;
    RKMEDIA_LOGI("rockface_set_licence failed, ret = %d\n", ret);
  } else
    authorized_result_.status = SUCCESS;

  if (enable_face_detect_) {
    ret = rockface_init_detector2(face_handle_, FACE_DETECT_DATA_VERSION);
    CheckFaceReturn(ret, "rockface_init_detector2");
  }
  ret = rockface_init_analyzer(face_handle_);
  CheckFaceReturn(ret, "rockface_init_analyzer");

  ret = rockface_init_landmark(face_handle_, 5);
  CheckFaceReturn(ret, "rockface_init_landmark");

  ret = rockface_init_recognizer(face_handle_);
  CheckFaceReturn(ret, "rockface_init_recognizer");

  if (thread_running_) {
    thread_ = std::make_shared<std::thread>(ThrFunc, this);
    if (!thread_) {
      RKMEDIA_LOGI("new thread failed.\n");
      return;
    }
  }
}

RockFaceRecognize::~RockFaceRecognize() {
  if (thread_) {
    thread_running_ = false;
    signal();
    thread_->join();
    thread_.reset();
  }
  if (db_manager_)
    db_manager_.reset();
  if (face_handle_)
    rockface_release_handle(face_handle_);
}

void RockFaceRecognize::BatchProcesPicture(
    RequestType type, std::list<std::string> &picture_list) {
  std::string &pic_path = picture_list.front();
  auto request = std::make_shared<FaceRecognizeRequest>(this, pic_path, type);
  if (request && PushRequest(request))
    picture_list.pop_front();
}

int RockFaceRecognize::SendInput(std::shared_ptr<MediaBuffer> input) {
  auto image = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  if (!image || !CheckIsRunning())
    return -1;

  if (!tobe_registered_pic_.empty()) {
    BatchProcesPicture(RequestType::REGISTER, tobe_registered_pic_);
  } else if (!tobe_recognized_pic_.empty()) {
    BatchProcesPicture(RequestType::RECOGNIZE, tobe_recognized_pic_);
  } else {
    auto &nn_list = image->GetRknnResult();
    if (nn_list.empty())
      return -1;

    auto request = std::make_shared<FaceRecognizeRequest>(this, image);
    auto &face_array = request->GetFaceArray();

    for (auto &nn : nn_list) {
      if (nn.type != NNRESULT_TYPE_FACE)
        continue;
      face_array.push_back(nn.face_info.base);
    }

    if (!face_array.empty()) {
      if (tobe_registered_count_ > 0) {
        if (face_array.size() == 1) {
          request->SetType(RequestType::REGISTER);
        } else
          RKMEDIA_LOGI(
              "there are muti faces(%d) in the image, drop the frame.\n",
              face_array.size());
        tobe_registered_count_--;
      }
      PushRequest(request);
    }
  }

  if (!request_list_.empty())
    signal();
  return 0;
}

std::shared_ptr<MediaBuffer> RockFaceRecognize::FetchOutput() {
  return nullptr;
}

int RockFaceRecognize::MatchFeature(rockface_feature_t *feature,
                                    float *out_similarity) {
  std::vector<FaceDb> vec = db_manager_->GetAllFaceDb();
  if (vec.empty())
    return -1;

  FaceDb *out = nullptr;
  float similarity = 99.9;
  for (FaceDb &iter : vec) {
    rockface_ret_t ret =
        rockface_feature_compare(feature, &iter.feature, &similarity);
    if (ret != ROCKFACE_RET_SUCCESS) {
      RKMEDIA_LOGI("rockface_feature_compare failed, ret = %d\n", ret);
      continue;
    }
    if (similarity < *out_similarity) {
      *out_similarity = similarity;
      out = &iter;
    }
  }
  return (*out_similarity - kFaceSimilarityThreshod < 0 ? out->user_id : -1);
}

bool RockFaceRecognize::CheckIsRunning(void) {
  bool check = true;
  if (!thread_running_ || authorized_result_.status == TIMEOUT || !enable_)
    check = false;
  return check;
}

bool RockFaceRecognize::CheckFaceReturn(rockface_ret_t ret,
                                        const char *fun_name, bool is_abort) {
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
  if (!check) {
    RKMEDIA_LOGI("%s run failed, ret = %d\n", fun_name, ret);
    if (is_abort)
      abort();
  }
  return check;
}

static FaceRegType RequestTypeToRegType(RequestType type) {
  FaceRegType reg_type;
  switch (type) {
  case RequestType::RECOGNIZE:
  case RequestType::NONE:
    reg_type = FACE_REG_RECOGNIZE;
    break;
  case RequestType::REGISTER:
    reg_type = FACE_REG_REGISTER;
    break;
  default:
    reg_type = FACE_REG_NONE;
    break;
  }
  return reg_type;
}

bool RockFaceRecognize::FindRecognizedWithId(
    int face_id, FaceReg *face_reg, std::vector<RknnResult> &last_result) {
  bool found = false;
  for (RknnResult &iter : last_result) {
    if (iter.face_info.base.id == face_id &&
        iter.face_info.face_reg.user_id >= 0 &&
        iter.face_info.face_reg.type == FACE_REG_RECOGNIZE) {
      *face_reg = iter.face_info.face_reg;
      found = true;
      break;
    }
  }
  return found;
}

FaceReg
RockFaceRecognize::Recognize(rockface_feature_t &feature,
                             std::shared_ptr<FaceRecognizeRequest> request) {
  FaceReg face_reg;
  FaceRegType type = RequestTypeToRegType(request->GetType());

  float similarity = 99.9;
  int matched_index = MatchFeature(&feature, &similarity);
  if (matched_index >= 0) {
    if (type == FACE_REG_REGISTER) {
      face_reg.user_id = -1;
    } else if (type == FACE_REG_RECOGNIZE) {
      face_reg.user_id = matched_index;
      face_reg.similarity = similarity;
    }
  } else {
    if (type == FACE_REG_REGISTER) {
      int user_id = db_manager_->AddUser(&feature);
      if (user_id >= 0)
        face_reg.user_id = user_id;
      else
        face_reg.user_id = -99;
    } else if (type == FACE_REG_RECOGNIZE) {
      face_reg.user_id = -1;
      face_reg.similarity = similarity;
    }
  }
  face_reg.type = type;

  std::string pic_path = request->GetPicPath();
  if (!pic_path.empty())
    strcpy(face_reg.pic_path, pic_path.c_str());
  else
    snprintf(face_reg.pic_path, RKNN_PICTURE_PATH_LEN, "%s_%d", "user",
             face_reg.user_id);
  return face_reg;
}

void RockFaceRecognize::ThreadLoop(void) {
  while (ThreadIsRunning()) {
    auto request = PopRequest();
    if (!request || !enable_)
      continue;

    static std::vector<RknnResult> last_result;
    rockface_image_t &input_image = request->GetInputImage();
    auto face_array = request->GetFaceArray();

    size_t count = 0;
    RknnResult nn_array[face_array.size()];

    for (; count < face_array.size(); count++) {
      rockface_det_t &face = face_array[count];
      nn_array[count].type = NNRESULT_TYPE_FACE_REG;
      nn_array[count].face_info.base = face;

      FaceReg *face_reg = &nn_array[count].face_info.face_reg;
      face_reg->type = RequestTypeToRegType(request->GetType());
      face_reg->user_id = -99;
      face_reg->similarity = -1;

      std::string pic_path = request->GetPicPath();
      if (!pic_path.empty())
        strcpy(face_reg->pic_path, pic_path.c_str());

      if (request->GetType() == RequestType::NONE &&
          FindRecognizedWithId(face.id, face_reg, last_result))
        continue;

      rockface_ret_t ret;
      rockface_feature_t feature;
      rockface_image_t aligned_image;

      ret = rockface_align(face_handle_, &input_image, &face.box, nullptr,
                           &aligned_image);
      if (!CheckFaceReturn(ret, "rockface_align"))
        continue;

      ret = rockface_feature_extract(face_handle_, &aligned_image, &feature);
      if (!CheckFaceReturn(ret, "rockface_feature_extract")) {
        rockface_image_release(&aligned_image);
        continue;
      }
      rockface_image_release(&aligned_image);

      FaceReg result = Recognize(feature, request);
      memcpy(face_reg, &result, sizeof(FaceReg));
    }
    if (last_result.size() > 0)
      last_result.clear();
    last_result.assign(nn_array, nn_array + count);

    AutoLockMutex lock(cb_mtx_);
    if (callback_)
      callback_(this, NNRESULT_TYPE_FACE_REG, nn_array, count);
  }
}

static float CalAverScore(std::shared_ptr<FaceRecognizeRequest> request) {
  float aver_score = 0.0;
  auto &face_array = request->GetFaceArray();
  for (rockface_det_t &face : face_array)
    aver_score += face.score;
  return aver_score / face_array.size();
}

bool RockFaceRecognize::PushRequest(
    std::shared_ptr<FaceRecognizeRequest> request) {
  std::lock_guard<std::mutex> lock(mutex_);

  bool ret = false;
  RequestType type = request->GetType();
  if (type == RequestType::NONE) {
    auto it = std::find_if(request_list_.begin(), request_list_.end(),
                           RequestFinder(RequestType::NONE));
    if (it != request_list_.end()) {
      float cur_aver_score = CalAverScore(request);
      float old_aver_score = CalAverScore(*it);
      if (cur_aver_score > old_aver_score) {
        request_list_.erase(it);
        request_list_.push_back(request);
        ret = true;
      }
    } else {
      if (request_list_.size() < kMaxCacheSize) {
        request_list_.push_back(request);
        ret = true;
      }
    }
  } else {
    if (request_list_.size() < kMaxCacheSize) {
      request_list_.push_back(request);
      ret = true;
    }
  }
  return ret;
}

std::shared_ptr<FaceRecognizeRequest> RockFaceRecognize::PopRequest(void) {
  std::unique_lock<std::mutex> lock(mutex_);
  std::shared_ptr<FaceRecognizeRequest> request = nullptr;
  if (request_list_.empty()) {
    if (wait_for(lock, 5) == false)
      return request;
  }
  for (auto it = request_list_.begin(); it != request_list_.end(); it++) {
    if ((*it)->GetType() == RequestType::REGISTER ||
        (*it)->GetType() == RequestType::RECOGNIZE) {
      request = *it;
      request_list_.erase(it);
      break;
    }
  }
  if (!request && !request_list_.empty()) {
    request = request_list_.back();
    request_list_.pop_back();
  }
  return request;
}

int RockFaceRecognize::IoCtrl(unsigned long int request, ...) {
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
    FaceRegArg *arg = va_arg(vl, FaceRegArg *);
    if (!arg) {
      RKMEDIA_LOGI("FaceRegArg params error.\n");
      break;
    }
    if (arg->type == USER_ADD_CAM) {
      tobe_registered_count_++;
    } else if (arg->type == USER_ADD_PIC) {
      std::string pic_path = arg->pic_path;
      if (!pic_path.empty())
        tobe_registered_pic_.push_back(pic_path);
    } else if (arg->type == USER_REG_PIC) {
      std::string pic_path = arg->pic_path;
      if (!pic_path.empty())
        tobe_recognized_pic_.push_back(pic_path);
    } else if (arg->type == USER_CLR) {
      db_manager_->ClearDb();
    } else if (arg->type == USER_DEL) {
      db_manager_->DeleteUser(arg->user_id);
    } else if (arg->type == USER_ENABLE) {
      enable_ = arg->enable;
    }
  } break;
  case G_NN_INFO: {
    FaceRegArg *arg = va_arg(vl, FaceRegArg *);
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

static void RockfaceRealeseImage(rockface_image_t &image) {
  if (!image.is_prealloc_buf && image.size > 0) {
    rockface_image_release(&image);
    memset(&image, 0, sizeof(rockface_image_t));
  }
}

FaceRecognizeRequest::FaceRecognizeRequest(RockFaceRecognize *handle,
                                           std::shared_ptr<ImageBuffer> image,
                                           RequestType type)
    : handle_(handle), type_(type), image_(image) {
  memset(&input_image_, 0, sizeof(rockface_image_t));
}
FaceRecognizeRequest::FaceRecognizeRequest(RockFaceRecognize *handle,
                                           std::string pic_path,
                                           RequestType type)
    : handle_(handle), type_(type), pic_path_(pic_path) {
  memset(&input_image_, 0, sizeof(rockface_image_t));
}

FaceRecognizeRequest::~FaceRecognizeRequest() {
  RockfaceRealeseImage(input_image_);
}

rockface_image_t &FaceRecognizeRequest::GetInputImage() {
  if (pic_path_.empty()) {
    input_image_.width = image_->GetVirWidth();
    input_image_.height = image_->GetVirHeight();
    input_image_.pixel_format = handle_->pixel_fmt_;
    input_image_.is_prealloc_buf = 1;
    input_image_.data = (uint8_t *)image_->GetPtr();
    input_image_.size = image_->GetValidSize();
  } else {
    rockface_ret_t ret =
        rockface_image_read(pic_path_.c_str(), &input_image_, 1);
    if (!handle_->CheckFaceReturn(ret, "rockface_image_read"))
      return input_image_;
    input_image_.size = input_image_.width * input_image_.height * 3;

    rockface_det_array_t det_array;
    ret = rockface_detect(handle_->face_handle_, &input_image_, &det_array);

    if (!handle_->CheckFaceReturn(ret, "rockface_detect"))
      return input_image_;

    for (int i = 0; i < det_array.count; i++)
      face_array_.push_back(det_array.face[i]);
  }
  return input_image_;
}

DEFINE_COMMON_FILTER_FACTORY(RockFaceRecognize)
const char *FACTORY(RockFaceRecognize)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(RockFaceRecognize)::OutPutDataType() {
  return TYPE_ANYTHING;
}

} // namespace easymedia
