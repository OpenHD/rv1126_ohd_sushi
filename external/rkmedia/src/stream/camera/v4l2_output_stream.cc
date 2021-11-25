// Copyright 2021 Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys/mman.h>
#include <vector>

#include "buffer.h"
#include "utils.h"
#include "v4l2_stream.h"

#include "rga_filter.h"

#define FMT_NUM_PLANES 2

namespace easymedia {

class V4L2OutputStream : public V4L2Stream {
public:
  V4L2OutputStream(const char *param);
  virtual ~V4L2OutputStream() {
    V4L2OutputStream::Close();
    assert(!started);
  }
  static const char *GetStreamName() { return "v4l2_output_stream"; }
  virtual bool Write(std::shared_ptr<MediaBuffer>) override;
  virtual int Open() final;
  virtual int Close() final;

private:
  int BufferExport(enum v4l2_buf_type bt, int index, int *dmafd);
  enum v4l2_memory memory_type;
  std::string data_type;
  PixelFormat pix_fmt;
  int width, height;
  int colorspace;
  int loop_num;
  int quantization;
  bool started;
};

V4L2OutputStream::V4L2OutputStream(const char *param)
    : V4L2Stream(param), memory_type(V4L2_MEMORY_DMABUF), data_type(IMAGE_NV12),
      pix_fmt(PIX_FMT_NONE), width(0), height(0), colorspace(-1), loop_num(2),
      quantization(-1), started(false) {
  if (device.empty())
    return;
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;

  std::string mem_type, str_loop_num;
  std::string str_width, str_height, str_color_space, str_quantization;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_V4L2_MEM_TYPE, mem_type));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_FRAMES, str_loop_num));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_INPUTDATATYPE, data_type));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_BUFFER_WIDTH, str_width));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_BUFFER_HEIGHT, str_height));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_V4L2_COLORSPACE, str_color_space));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_V4L2_QUANTIZATION, str_quantization));
  int ret = parse_media_param_match(param, params, req_list);
  if (ret == 0)
    return;
  if (!mem_type.empty())
    memory_type = static_cast<enum v4l2_memory>(GetV4L2Type(mem_type.c_str()));
  if (!str_loop_num.empty())
    loop_num = std::stoi(str_loop_num);
  assert(loop_num >= 2);
  if (!str_width.empty())
    width = std::stoi(str_width);
  if (!str_height.empty())
    height = std::stoi(str_height);
  if (!str_color_space.empty())
    colorspace = std::stoi(str_color_space);
  if (!str_quantization.empty())
    quantization = std::stoi(str_quantization);
}

int V4L2OutputStream::BufferExport(enum v4l2_buf_type bt, int index,
                                   int *dmafd) {
  struct v4l2_exportbuffer expbuf;

  memset(&expbuf, 0, sizeof(expbuf));
  expbuf.type = bt;
  expbuf.index = index;
  if (v4l2_ctx->IoCtrl(VIDIOC_EXPBUF, &expbuf) == -1) {
    RKMEDIA_LOGI("VIDIOC_EXPBUF  %d failed, %m\n", index);
    return -1;
  }
  *dmafd = expbuf.fd;

  return 0;
}

class V4L2Buffer {
public:
  V4L2Buffer() : dmafd(-1), ptr(nullptr), length(0), munmap_f(nullptr) {}
  ~V4L2Buffer() {
    if (dmafd >= 0)
      close(dmafd);
    if (ptr && ptr != MAP_FAILED && munmap_f)
      munmap_f(ptr, length);
    if (ptr1 && ptr1 != MAP_FAILED && munmap_f)
      munmap_f(ptr1, length1);
  }
  int dmafd;
  void *ptr;
  size_t length;
  void *ptr1;
  size_t length1;
  int (*munmap_f)(void *_start, size_t length);
};

int V4L2OutputStream::Open() {
  if (memory_type == V4L2_MEMORY_MMAP) {
    RKMEDIA_LOGE("V4L2OutputStream not support V4L2_MEMORY_MMAP\n");
    return -1;
  }
  const char *dev = device.c_str();
  if (width <= 0 || height <= 0) {
    RKMEDIA_LOGI("Invalid param, device=%s, width=%d, height=%d\n", dev, width,
                 height);
    return -EINVAL;
  }
  int ret = V4L2Stream::Open();
  if (ret)
    return ret;

  struct v4l2_capability cap;
  memset(&cap, 0, sizeof(cap));
  if (v4l2_ctx->IoCtrl(VIDIOC_QUERYCAP, &cap) < 0) {
    RKMEDIA_LOGE("Failed to ioctl(VIDIOC_QUERYCAP): %m\n");
    return -1;
  }
  if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) &&
      !(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE)) {
    RKMEDIA_LOGE("%s, Not a video output device.\n", dev);
    return -1;
  }
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    RKMEDIA_LOGE("%s does not support the streaming I/O method.\n", dev);
    return -1;
  }
  const char *data_type_str = data_type.c_str();
  struct v4l2_format fmt;
  memset(&fmt, 0, sizeof(fmt));
  if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) {
    RKMEDIA_LOGD("#RKMedia: V4L2: output_type=V4L2_CAP_VIDEO_OUTPUT\n");
    output_type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  } else if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE) {
    RKMEDIA_LOGD("#RKMedia: V4L2: output_type=V4L2_CAP_VIDEO_OUTPUT_MPLANE\n");
    output_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    // update new cap type.
    v4l2_ctx->SetCapType(output_type);
    if (memory_type == V4L2_MEMORY_DMABUF) {
      RKMEDIA_LOGE("V4L2: MPLANE not support DMA Buffer yet.\n");
      return -1;
    }
  }
  fmt.type = output_type;
  fmt.fmt.pix_mp.width = width;
  fmt.fmt.pix_mp.height = height;
  fmt.fmt.pix_mp.pixelformat = GetV4L2FmtByString(data_type_str);
  fmt.fmt.pix_mp.field = V4L2_FIELD_INTERLACED;
  fmt.fmt.pix_mp.quantization = V4L2_QUANTIZATION_FULL_RANGE;
  if (fmt.fmt.pix.pixelformat == 0) {
    RKMEDIA_LOGE("unsupport input format : %s\n", data_type_str);
    return -1;
  }
  if (v4l2_ctx->IoCtrl(VIDIOC_S_FMT, &fmt) < 0) {
    RKMEDIA_LOGE("%s, s fmt failed(cap type=%d, %c%c%c%c), %m\n", dev,
                 output_type, DUMP_FOURCC(fmt.fmt.pix.pixelformat));
    return -1;
  }

  if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == output_type)
    plane_cnt = fmt.fmt.pix_mp.num_planes;
  RKMEDIA_LOGD("#RKMedia: V4L2: output plane cnt:%d\n", plane_cnt);

  if (GetV4L2FmtByString(data_type_str) != fmt.fmt.pix.pixelformat) {
    RKMEDIA_LOGE("%s, expect %s, return %c%c%c%c\n", dev, data_type_str,
                 DUMP_FOURCC(fmt.fmt.pix.pixelformat));
    return -1;
  }
  pix_fmt = StringToPixFmt(data_type_str);
  if (width != (int)fmt.fmt.pix.width || height != (int)fmt.fmt.pix.height) {
    RKMEDIA_LOGE("%s change res from %dx%d to %dx%d\n", dev, width, height,
                 fmt.fmt.pix.width, fmt.fmt.pix.height);
    width = fmt.fmt.pix.width;
    height = fmt.fmt.pix.height;
    return -1;
  }
  if (fmt.fmt.pix.field == V4L2_FIELD_INTERLACED)
    RKMEDIA_LOGI("%s is using the interlaced mode\n", dev);

  struct v4l2_requestbuffers req;
  req.type = output_type;
  req.count = loop_num;
  req.memory = memory_type;
  if (v4l2_ctx->IoCtrl(VIDIOC_REQBUFS, &req) < 0) {
    RKMEDIA_LOGE("%s, count=%d, ioctl(VIDIOC_REQBUFS): %m\n", dev, loop_num);
    return -1;
  }

  SetWriteable(true);
  return 0;
}
int V4L2OutputStream::Close() {
  started = false;
  return V4L2Stream::Close();
}

class V4L2AutoQBUF {
public:
  V4L2AutoQBUF(std::shared_ptr<V4L2Context> ctx, struct v4l2_buffer buf)
      : v4l2_ctx(ctx), v4l2_buf(buf) {}

  void SetPlanes(struct v4l2_plane *plane_ptr, int plane_size) {
    memcpy(planes, plane_ptr, plane_size);
    v4l2_buf.m.planes = planes;
  }

  ~V4L2AutoQBUF() {
    if (v4l2_ctx->IoCtrl(VIDIOC_QBUF, &v4l2_buf) < 0)
      RKMEDIA_LOGE("index=%d, ioctl(VIDIOC_QBUF): %m\n", v4l2_buf.index);
  }

private:
  std::shared_ptr<V4L2Context> v4l2_ctx;
  struct v4l2_buffer v4l2_buf;
  struct v4l2_plane planes[FMT_NUM_PLANES];
};

class AutoQBUFMediaBuffer : public MediaBuffer {
public:
  AutoQBUFMediaBuffer(const MediaBuffer &mb, std::shared_ptr<V4L2Context> ctx,
                      struct v4l2_buffer buf)
      : MediaBuffer(mb), auto_qbuf(ctx, buf) {}

  void SetPlanes(struct v4l2_plane *plane_ptr, int plane_size) {
    auto_qbuf.SetPlanes(plane_ptr, plane_size);
  }

private:
  V4L2AutoQBUF auto_qbuf;
};

class AutoQBUFImageBuffer : public ImageBuffer {
public:
  AutoQBUFImageBuffer(const MediaBuffer &mb, const ImageInfo &info,
                      std::shared_ptr<V4L2Context> ctx, struct v4l2_buffer buf)
      : ImageBuffer(mb, info), auto_qbuf(ctx, buf) {}

  void SetPlanes(struct v4l2_plane *plane_ptr, int plane_size) {
    auto_qbuf.SetPlanes(plane_ptr, plane_size);
  }

private:
  V4L2AutoQBUF auto_qbuf;
};

bool V4L2OutputStream::Write(std::shared_ptr<MediaBuffer> input) {
  if (input->GetFD() < 0) {
    RKMEDIA_LOGE("V4L2OutputStream::Write input fd error!\n");
    return false;
  }
  if ((int)input->GetValidSize() != CalPixFmtSize(pix_fmt, width, height, 0)) {
    RKMEDIA_LOGE("V4L2OutputStream::Write input valid size error!\n");
    return false;
  }
  const char *dev = device.c_str();
  if (!started && v4l2_ctx->SetStarted(true))
    started = true;

  struct v4l2_buffer buf;

  memset(&buf, 0, sizeof(buf));
  buf.type = output_type;
  buf.memory = memory_type;
  buf.timestamp = input->GetTimeVal();
  buf.bytesused = input->GetValidSize();
  buf.length = input->GetValidSize();
  buf.m.fd = input->GetFD();

  if (v4l2_ctx->IoCtrl(VIDIOC_QBUF, &buf) < 0) {
    RKMEDIA_LOGE("%s, index=%d, ioctl(VIDIOC_QBUF): %m\n", dev, buf.index);
    return false;
  }

  int ret = v4l2_ctx->IoCtrl(VIDIOC_DQBUF, &buf);
  if (ret < 0) {
    RKMEDIA_LOGI("%s, ioctl(VIDIOC_DQBUF): %m\n", dev);
    return false;
  }

  return true;
}

DEFINE_STREAM_FACTORY(V4L2OutputStream, Stream)

const char *FACTORY(V4L2OutputStream)::ExpectedInputDataType() {
  return GetStringOfV4L2Fmts().c_str();
}

const char *FACTORY(V4L2OutputStream)::OutPutDataType() { return nullptr; }

} // namespace easymedia
