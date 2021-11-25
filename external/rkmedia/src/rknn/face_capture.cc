// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <queue>
#include <sstream>

#include "buffer.h"
#include "encoder.h"
#include "filter.h"
#include "lock.h"
#include "media_config.h"
#include "rga_filter.h"
#include "rknn_utils.h"
#include "stream.h"

namespace easymedia {

class FaceCapture : public Filter {
public:
  FaceCapture(const char *param);
  virtual ~FaceCapture();
  static const char *GetFilterName() { return "face_capture"; }
  virtual int IoCtrl(unsigned long int request, ...) override;

  std::string GenFilePath(time_t curtime = 0, int face_id = 0);

  int InitPlugin(std::map<std::string, std::string> &params);
  int DeInitPlugin();

  int DoCrop(std::shared_ptr<MediaBuffer> src,
             std::shared_ptr<MediaBuffer> &dst, ImageRect rect);
  int DoEncode(std::shared_ptr<MediaBuffer> src,
               std::shared_ptr<MediaBuffer> &dst);
  int DoWrite(std::shared_ptr<MediaBuffer> buffer, int face_id);

  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;

private:
  bool enable_;
  std::shared_ptr<VideoEncoder> enc_;
  std::shared_ptr<Filter> rga_;
  std::shared_ptr<Stream> fstream_;
  std::string file_path_;
  std::string file_prefix_;
  std::string file_suffix_;
  size_t file_index_;
  int last_face_id_;
  RknnCallBack callback_;
  ReadWriteLockMutex cb_mtx_;
  std::string filepath_;
};

FaceCapture::FaceCapture(const char *param) : last_face_id_(-1) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }

  InitPlugin(params);
}

FaceCapture::~FaceCapture() { DeInitPlugin(); }

std::string FaceCapture::GenFilePath(time_t curtime, int face_id) {
  std::ostringstream ostr;

  if (!file_path_.empty()) {
    ostr << file_path_;
    ostr << "/";
  }

  if (!file_prefix_.empty()) {
    ostr << file_prefix_;
  }

  if (curtime == 0)
    curtime = time(NULL);

  char time_str[128] = {0};
  strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S", localtime(&curtime));

  ostr << time_str;

  ostr << "_" << face_id;

  if (!file_suffix_.empty()) {
    ostr << file_suffix_;
  }

  return ostr.str();
}

int FaceCapture::InitPlugin(std::map<std::string, std::string> &params) {
  std::string mpp_codec = "rkmpp";
  std::string enc_param;
  PARAM_STRING_APPEND(enc_param, KEY_OUTPUTDATATYPE, IMAGE_JPEG);
  enc_ = easymedia::REFLECTOR(Encoder)::Create<easymedia::VideoEncoder>(
      mpp_codec.c_str(), enc_param.c_str());
  if (!enc_) {
    RKMEDIA_LOGI("FaceCapture Create encoder %s failed\n", mpp_codec.c_str());
    exit(EXIT_FAILURE);
  }

  std::string rga_param = "";
  ImageRect rect = {0, 0, 0, 0};
  std::vector<ImageRect> rect_vect;
  rect_vect.push_back(rect);
  rect_vect.push_back(rect);
  PARAM_STRING_APPEND(rga_param, KEY_BUFFER_RECT,
                      easymedia::TwoImageRectToString(rect_vect).c_str());
  PARAM_STRING_APPEND_TO(rga_param, KEY_BUFFER_ROTATE, 0);

  rga_ = REFLECTOR(Filter)::Create<Filter>("rkrga", rga_param.c_str());
  if (!rga_) {
    RKMEDIA_LOGI("FaceCapture Create rga %s failed\n", rga_param.c_str());
    exit(EXIT_FAILURE);
  }

  std::string stream_param;
  std::string value;
  file_prefix_ = params[KEY_FILE_PREFIX];
  if (file_prefix_.empty()) {
    RKMEDIA_LOGI("FaceCapture file_prefix empty\n");
  }
  file_path_ = params[KEY_PATH];
  file_suffix_ = params[KEY_FILE_SUFFIX];
  auto path = GenFilePath();

  PARAM_STRING_APPEND(stream_param, KEY_PATH, path);
  PARAM_STRING_APPEND(stream_param, KEY_OPEN_MODE, "w+");
  PARAM_STRING_APPEND(stream_param, KEY_SAVE_MODE, KEY_SAVE_MODE_SINGLE);
  fstream_ = REFLECTOR(Stream)::Create<Stream>("file_write_stream",
                                               stream_param.c_str());
  if (!fstream_) {
    RKMEDIA_LOGI("FaceCapture Create file_write_stream %s failed\n",
                 stream_param.c_str());
    exit(EXIT_FAILURE);
  }

  enable_ = false;
  const std::string &enable_str = params[KEY_ENABLE];
  if (!enable_str.empty())
    enable_ = std::stoi(enable_str);

  return 0;
}

int FaceCapture::DeInitPlugin() {
  if (enc_) {
    enc_.reset();
    enc_ = nullptr;
  }
  if (rga_) {
    rga_.reset();
    rga_ = nullptr;
  }
  if (fstream_) {
    fstream_.reset();
    fstream_ = nullptr;
  }
  return 0;
}

int FaceCapture::DoCrop(std::shared_ptr<MediaBuffer> src,
                        std::shared_ptr<MediaBuffer> &dst, ImageRect rect) {
  auto img_buffer = std::static_pointer_cast<easymedia::ImageBuffer>(src);
  ImageInfo &img_info = img_buffer->GetImageInfo();

  std::shared_ptr<MediaBuffer> out_buffer;

  rect.x = UPALIGNTO(rect.x - 50, 2);
  if (rect.x < 0)
    rect.x = 0;

  rect.y = UPALIGNTO(rect.y - 50, 2);
  if (rect.y < 0)
    rect.y = 0;

  rect.w = UPALIGNTO(rect.w + 100, 2);
  if ((rect.x + rect.w) > img_info.width)
    rect.w = img_info.width - rect.x;

  rect.h = UPALIGNTO(rect.h + 100, 2);
  if ((rect.y + rect.h) > img_info.height)
    rect.h = img_info.height - rect.y;

  auto rga_filter = std::static_pointer_cast<RgaFilter>(rga_);
  std::vector<ImageRect> rects;
  rects.push_back({rect.x, rect.y, rect.w, rect.h});
  rects.push_back({0, 0, rect.w, rect.h});
  rga_filter->SetRects(rects);

  ImageInfo dst_info = img_info;
  dst_info.width = rect.w;
  dst_info.height = rect.h;
  dst_info.vir_width = UPALIGNTO16(rect.w);
  dst_info.vir_height = UPALIGNTO16(rect.h);

  size_t size = CalPixFmtSize(dst_info);
  if (size == 0) {
    RKMEDIA_LOGI("FaceCapture rga size empty\n");
    return -1;
  }

  auto &&mb = MediaBuffer::Alloc2(size, MediaBuffer::MemType::MEM_HARD_WARE);
  dst = std::make_shared<ImageBuffer>(mb, dst_info);
  dst->SetValidSize(size);

  int ret = rga_->Process(src, dst);
  if (ret < 0) {
    RKMEDIA_LOGI("FaceCapture rga Process failed\n");
    return -1;
  }

  return 0;
}

int FaceCapture::DoEncode(std::shared_ptr<MediaBuffer> src,
                          std::shared_ptr<MediaBuffer> &dst) {
  auto img_buffer = std::static_pointer_cast<easymedia::ImageBuffer>(src);
  MediaConfig enc_config;
  ImageConfig &img_cfg = enc_config.img_cfg;
  img_cfg.image_info = img_buffer->GetImageInfo();
  img_cfg.qfactor = 50;
  if (!enc_->InitConfig(enc_config)) {
    RKMEDIA_LOGI("FaceCapture Init config of encoder mjpeg failed\n");
    exit(EXIT_FAILURE);
  }
  dst = std::make_shared<MediaBuffer>();
  if (!dst) {
    LOG_NO_MEMORY();
    return -1;
  }
  if (0 != enc_->Process(src, dst, nullptr)) {
    RKMEDIA_LOGI("FaceCapture encoder Process failed\n");
    return -1;
  }
  return 0;
}

int FaceCapture::DoWrite(std::shared_ptr<MediaBuffer> buffer, int face_id) {
  int ret = 0;
  time_t curtime = buffer->GetUSTimeStamp() / 1000000LL;
  filepath_ = GenFilePath(curtime, face_id);
  ret = fstream_->NewStream(filepath_);
  if (ret < 0)
    return ret;
  return fstream_->WriteAndClose(buffer->GetPtr(), 1, buffer->GetValidSize());
}

int FaceCapture::Process(std::shared_ptr<MediaBuffer> input,
                         std::shared_ptr<MediaBuffer> &output) {
  if (!input || input->GetType() != Type::Image)
    return -EINVAL;
  if (!output || output->GetType() != Type::Image)
    return -EINVAL;

  output = input;

  if (!enable_)
    return 0;

  auto src = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  auto &nn_results = src->GetRknnResult();

  // search min id
  int min_face_id = last_face_id_;
  RknnResult min_id_result;
  memset(&min_id_result, 0, sizeof(RknnResult));
  min_id_result.type = NNRESULT_TYPE_NONE;

  for (auto &iter : nn_results) {
    if (last_face_id_ < iter.face_info.base.id &&
        min_face_id < iter.face_info.base.id) {
      min_id_result = iter;
      min_face_id = iter.face_info.base.id;

      break;
    }
  }

  // filtering by type
  if (min_id_result.type != NNRESULT_TYPE_FACE)
    return 0;

  // filtering by id
  if (min_face_id < 0 || min_face_id <= last_face_id_)
    return 0;

  last_face_id_ = min_face_id;

  ImageRect image_rect;
  rockface_rect_t min_face_rect = min_id_result.face_info.base.box;
  image_rect.x = min_face_rect.left;
  image_rect.y = min_face_rect.top;
  image_rect.w = min_face_rect.right - min_face_rect.left;
  image_rect.h = min_face_rect.bottom - min_face_rect.top;

  std::shared_ptr<MediaBuffer> rga_buffer;
  if (DoCrop(input, rga_buffer, image_rect))
    return 0;
  std::shared_ptr<MediaBuffer> enc_buffer;
  if (DoEncode(rga_buffer, enc_buffer))
    return 0;
  if (DoWrite(enc_buffer, min_face_id))
    return 0;

  if (callback_)
    callback_(this, NNRESULT_TYPE_FACE_PICTURE_UPLOAD,
              (void *)(filepath_.c_str()), filepath_.size());
  return 0;
}

int FaceCapture::IoCtrl(unsigned long int request, ...) {
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
    void *arg = va_arg(vl, void *);
    if (arg) {
      FaceCaptureArg *face_cap_arg = (FaceCaptureArg *)arg;
      enable_ = face_cap_arg->enable;
    }
  } break;
  case G_NN_INFO: {
    void *arg = va_arg(vl, void *);
    if (arg) {
      FaceCaptureArg *face_cap_arg = (FaceCaptureArg *)arg;
      face_cap_arg->enable = enable_;
    }
  } break;
  default:
    ret = -1;
    break;
  }
  va_end(vl);
  return ret;
}

DEFINE_COMMON_FILTER_FACTORY(FaceCapture)
const char *FACTORY(FaceCapture)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(FaceCapture)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace easymedia
