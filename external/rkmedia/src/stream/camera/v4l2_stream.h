// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_V4L2_STREAM_H_
#define EASYMEDIA_V4L2_STREAM_H_

#include "stream.h"

#include <assert.h>
#include <stdint.h>

#include <mutex>

#include "buffer.h"
#include "v4l2_utils.h"
#ifdef RKAIQ
#include "rkaiq_media.h"
#endif

#define RKISP_SUBDEV_NAME "rkisp-isp-subdev"
#define RKIISPP_SUBDEV_NAME "rkispp-subdev"

namespace easymedia {

class V4L2Context {
public:
  V4L2Context(enum v4l2_buf_type cap_type, v4l2_io io_func,
              const std::string device);
  ~V4L2Context();
  int GetDeviceFd() { return fd; }
  void SetCapType(enum v4l2_buf_type type) { capture_type = type; }
  // val: if true, VIDIOC_STREAMON, else VIDIOC_STREAMOFF
  bool SetStarted(bool val);
  int IoCtrl(unsigned long int request, void *arg);

private:
  int fd;
  enum v4l2_buf_type capture_type;
  v4l2_io vio;
  std::mutex mtx;
  volatile bool started;
  std::string path;
};

class V4L2MediaCtl {
public:
  V4L2MediaCtl();
  ~V4L2MediaCtl();
  int InitHwInfos();
  int SetupLink(std::string devname, bool enable);

#ifdef RKAIQ
  RKAiqMedia media_ctl_infos;
#endif
};

class V4L2Stream : public Stream {
public:
  V4L2Stream(const char *param);
  virtual ~V4L2Stream() { V4L2Stream::Close(); }
  virtual size_t Read(void *ptr _UNUSED, size_t size _UNUSED,
                      size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int Seek(int64_t offset _UNUSED, int whence _UNUSED) final {
    return -1;
  }
  virtual long Tell() final { return -1; }
  virtual size_t Write(const void *ptr _UNUSED, size_t size _UNUSED,
                       size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int IoCtrl(unsigned long int request, ...) override;

#define v4l2_open vio.open_f
#define v4l2_close vio.close_f
#define v4l2_dup vio.dup_f
#define v4l2_ioctl vio.ioctl_f
#define v4l2_read vio.read_f
#define v4l2_mmap vio.mmap_f
#define v4l2_munmap vio.munmap_f

protected:
  virtual int Open() override;
  virtual int Close() override;

  bool use_libv4l2;
  v4l2_io vio;
  std::string devname;
  std::string device;
  std::string sub_device;
  int camera_id;
  int fd; // just for convenience
  // static sub_device_controller;
  enum v4l2_buf_type capture_type;
  enum v4l2_buf_type output_type;
  int plane_cnt;
  std::shared_ptr<V4L2Context> v4l2_ctx;
  std::shared_ptr<V4L2MediaCtl> v4l2_medctl;
  std::shared_ptr<MediaBuffer> user_picture;
  int enable_user_picture;
  ConditionLockMutex param_mtx;
  std::vector<ImageBorder> lines;
  std::map<int, OsdInfo> osds;
  int64_t recent_time; // steam read or wirte recent timestamp
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_V4L2_STREAM_H_
