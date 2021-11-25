// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "v4l2_stream.h"

#include <cstring>
#include <fcntl.h>

#include "control.h"

namespace easymedia {

V4L2Context::V4L2Context(enum v4l2_buf_type cap_type, v4l2_io io_func,
                         const std::string device)
    : fd(-1), capture_type(cap_type), vio(io_func), started(false)
#ifndef NDEBUG
      ,
      path(device)
#endif
{
  const char *dev = device.c_str();
  fd = v4l2_open(dev, O_RDWR | O_CLOEXEC, 0);
  if (fd < 0)
    RKMEDIA_LOGE("V4L2-CTX: open %s failed %m\n", dev);
  else
    RKMEDIA_LOGI("#V4L2Ctx: open %s, fd %d\n", dev, fd);
}

V4L2Context::~V4L2Context() {
  if (fd >= 0) {
    SetStarted(false);
    v4l2_close(fd);
    RKMEDIA_LOGI("#V4L2Ctx: close %s, fd %d\n", path.c_str(), fd);
  }
}

bool V4L2Context::SetStarted(bool val) {
  std::lock_guard<std::mutex> _lk(mtx);
  if (started == val)
    return true;
  enum v4l2_buf_type cap_type = capture_type;
  unsigned int request = val ? VIDIOC_STREAMON : VIDIOC_STREAMOFF;
  if (IoCtrl(request, &cap_type) < 0) {
    RKMEDIA_LOGI("ioctl(%d): %m\n", (int)request);
    return false;
  }
  started = val;
  return true;
}

int V4L2Context::IoCtrl(unsigned long int request, void *arg) {
  if (fd < 0) {
    errno = EINVAL;
    return -1;
  }
  return V4L2IoCtl(&vio, fd, request, arg);
}

V4L2MediaCtl::V4L2MediaCtl() {}

V4L2MediaCtl::~V4L2MediaCtl() {}

V4L2Stream::V4L2Stream(const char *param)
    : use_libv4l2(false), camera_id(0), fd(-1),
      capture_type(V4L2_BUF_TYPE_VIDEO_CAPTURE), plane_cnt(1),
      enable_user_picture(0), recent_time(0) {
  memset(&vio, 0, sizeof(vio));
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  std::string str_libv4l2;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_USE_LIBV4L2, str_libv4l2));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_DEVICE, device));
  std::string str_camera_id;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_CAMERA_ID, str_camera_id));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_SUB_DEVICE, sub_device));
  std::string cap_type;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_V4L2_CAP_TYPE, cap_type));
  int ret = parse_media_param_match(param, params, req_list);
  if (ret == 0)
    return;
  if (!str_camera_id.empty())
    camera_id = std::stoi(str_camera_id);
  if (!str_libv4l2.empty())
    use_libv4l2 = !!std::stoi(str_libv4l2);
  if (!cap_type.empty())
    capture_type =
        static_cast<enum v4l2_buf_type>(GetV4L2Type(cap_type.c_str()));
  v4l2_medctl = std::make_shared<V4L2MediaCtl>();

  RKMEDIA_LOGI("#V4l2Stream: camraID:%d, Device:%s\n", camera_id,
               device.c_str());
}

int V4L2Stream::Open() {
  if (!SetV4L2IoFunction(&vio, use_libv4l2))
    return -EINVAL;
  if (!sub_device.empty()) {
    // TODO:
  }

  if (!strcmp(device.c_str(), MB_ENTITY_NAME) ||
      !strcmp(device.c_str(), S0_ENTITY_NAME) ||
      !strcmp(device.c_str(), S1_ENTITY_NAME) ||
      !strcmp(device.c_str(), S2_ENTITY_NAME)) {
#ifdef RKAIQ
    devname =
        v4l2_medctl->media_ctl_infos.GetVideoNode(camera_id, device.c_str());
#else
    RKMEDIA_LOGE("#V4l2Stream: VideoNode: %s is invalid without librkaiq\n",
                 device.c_str());
    return -EINVAL;
#endif
  } else
    devname = device;
  RKMEDIA_LOGI("#V4l2Stream: camera id:%d, VideoNode:%s\n", camera_id,
               devname.c_str());
  v4l2_ctx = std::make_shared<V4L2Context>(capture_type, vio, devname);
  if (!v4l2_ctx)
    return -ENOMEM;
  fd = v4l2_ctx->GetDeviceFd();
  if (fd < 0) {
    v4l2_ctx = nullptr;
    return -1;
  }
  user_picture = nullptr;
  return 0;
}

int V4L2Stream::Close() {
  if (v4l2_ctx) {
    v4l2_ctx->SetStarted(false);
    v4l2_ctx = nullptr; // release reference
    RKMEDIA_LOGI("#V4L2Stream: v4l2 ctx reset to nullptr!\n");
  }
  fd = -1;
  return 0;
}

int V4L2Stream::IoCtrl(unsigned long int request, ...) {
  va_list vl;

  switch (request) {
  case S_SUB_REQUEST: {
    va_start(vl, request);
    void *arg = va_arg(vl, void *);
    va_end(vl);
    auto sub = (SubRequest *)arg;
    return V4L2IoCtl(&vio, fd, sub->sub_request, sub->arg);
  }
  case S_STREAM_OFF: {
    return v4l2_ctx->SetStarted(false) ? 0 : -1;
  }
  case G_STREAM_RECENT_TIME: {
    va_start(vl, request);
    int64_t *arg = va_arg(vl, int64_t *);
    va_end(vl);
    *arg = recent_time;

    return 0;
  }
  case S_INSERT_USER_PICTURE: {
    va_start(vl, request);
    UserPicArg *arg = va_arg(vl, UserPicArg *);
    va_end(vl);

    if (arg->data && (arg->size > 0)) {
      RKMEDIA_LOGI("V4L2: Insert user picture: ptr:%p, size:%d\n", arg->data,
                   arg->size);
      if (user_picture == nullptr) {
        auto mb = MediaBuffer::Alloc((size_t)arg->size,
                                     MediaBuffer::MemType::MEM_HARD_WARE);
        if (!mb) {
          RKMEDIA_LOGE("V4L2: Enable user picture: no mem left!\n");
          return -1;
        }
        user_picture = mb;
      }
      user_picture->BeginCPUAccess(true);
      memcpy(user_picture->GetPtr(), arg->data, arg->size);
      user_picture->EndCPUAccess(true);
      user_picture->SetValidSize(arg->size);
    } else {
      RKMEDIA_LOGI("V4L2: Reset user picture!\n");
      user_picture.reset();
    }

    return 0;
  }
  case S_ENABLE_USER_PICTURE: {
    if (!user_picture || !user_picture->GetValidSize()) {
      RKMEDIA_LOGE("V4L2: Enable user picture failed, Do insert first!\n");
      return -1;
    }
    RKMEDIA_LOGI("V4L2: Enable user picture!\n");
    enable_user_picture = 1;
    return 0;
  }
  case S_DISABLE_USER_PICTURE: {
    RKMEDIA_LOGI("V4L2: Disable user picture!\n");
    enable_user_picture = 0;
    return 0;
  }
  case S_RGA_LINE_INFO: {
    va_start(vl, request);
    void *arg = va_arg(vl, void *);
    va_end(vl);
    ImageBorder *line = (ImageBorder *)arg;
    param_mtx.lock();
    int exist = 0;
    for (auto it = lines.begin(); it != lines.end(); ++it) {
      ImageBorder &l = *it;
      if (l.id == line->id) {
        exist = 1;
        memcpy(&l, line, sizeof(ImageBorder));
        break;
      }
    }
    if (!exist)
      lines.push_back(*line);
    param_mtx.unlock();
    return 0;
  }
  case S_RGA_OSD_INFO: {
    va_start(vl, request);
    void *arg = va_arg(vl, void *);
    va_end(vl);
    ImageOsd *osd = (ImageOsd *)arg;
    OsdInfo info = {osd->x,    osd->y,       osd->w,     osd->h,
                    osd->data, osd->pix_fmt, osd->enable};
    param_mtx.lock();
    osds[osd->id] = info;
    param_mtx.unlock();
    break;
  }
  }
  return -1;
}

} // namespace easymedia
