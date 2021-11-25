// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
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

class V4L2CaptureStream : public V4L2Stream {
public:
  V4L2CaptureStream(const char *param);
  virtual ~V4L2CaptureStream() {
    V4L2CaptureStream::Close();
    assert(!started);
  }
  static const char *GetStreamName() { return "v4l2_capture_stream"; }
  virtual std::shared_ptr<MediaBuffer> Read();
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
  std::vector<MediaBuffer> buffer_vec;
  bool started;
};

V4L2CaptureStream::V4L2CaptureStream(const char *param)
    : V4L2Stream(param), memory_type(V4L2_MEMORY_MMAP), data_type(IMAGE_NV12),
      pix_fmt(PIX_FMT_NONE), width(0), height(0), colorspace(-1), loop_num(2),
      quantization(V4L2_QUANTIZATION_LIM_RANGE), started(false) {
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
      KEY_OUTPUTDATATYPE, data_type));
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

int V4L2CaptureStream::BufferExport(enum v4l2_buf_type bt, int index,
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

static int __free_v4l2buffer(void *arg) {
  delete static_cast<V4L2Buffer *>(arg);
  return 0;
}

int V4L2CaptureStream::Open() {
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
  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) &&
      !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
    RKMEDIA_LOGE("%s, Not a video capture device.\n", dev);
    return -1;
  }
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    RKMEDIA_LOGE("%s does not support the streaming I/O method.\n", dev);
    return -1;
  }
  const char *data_type_str = data_type.c_str();
  struct v4l2_format fmt;
  memset(&fmt, 0, sizeof(fmt));
  if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
    RKMEDIA_LOGD("#RKMedia: V4L2: capture_type=V4L2_CAP_VIDEO_CAPTURE\n");
    capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  } else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
    RKMEDIA_LOGD(
        "#RKMedia: V4L2: capture_type=V4L2_CAP_VIDEO_CAPTURE_MPLANE\n");
    capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // update new cap type.
    v4l2_ctx->SetCapType(capture_type);
    if (memory_type == V4L2_MEMORY_DMABUF) {
      RKMEDIA_LOGE("V4L2: MPLANE not support DMA Buffer yet.\n");
      return -1;
    }
  }
  fmt.type = capture_type;
  fmt.fmt.pix_mp.width = width;
  fmt.fmt.pix_mp.height = height;
  fmt.fmt.pix_mp.pixelformat = GetV4L2FmtByString(data_type_str);
  fmt.fmt.pix_mp.field = V4L2_FIELD_INTERLACED;
  fmt.fmt.pix_mp.quantization = quantization; // V4L2_QUANTIZATION_FULL_RANGE;
  if (fmt.fmt.pix.pixelformat == 0) {
    RKMEDIA_LOGE("unsupport input format : %s\n", data_type_str);
    return -1;
  }
  if (v4l2_ctx->IoCtrl(VIDIOC_S_FMT, &fmt) < 0) {
    RKMEDIA_LOGE("%s, s fmt failed(cap type=%d, %c%c%c%c), %m\n", dev,
                 capture_type, DUMP_FOURCC(fmt.fmt.pix.pixelformat));
    return -1;
  }

  if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type)
    plane_cnt = fmt.fmt.pix_mp.num_planes;
  RKMEDIA_LOGD("#RKMedia: V4L2: capture plane cnt:%d\n", plane_cnt);

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
  req.type = capture_type;
  req.count = loop_num;
  req.memory = memory_type;
  if (v4l2_ctx->IoCtrl(VIDIOC_REQBUFS, &req) < 0) {
    RKMEDIA_LOGE("%s, count=%d, ioctl(VIDIOC_REQBUFS): %m\n", dev, loop_num);
    return -1;
  }
  int w = UPALIGNTO16(width);
  int h = UPALIGNTO16(height);
  if (memory_type == V4L2_MEMORY_DMABUF) {
    int size = 0;
    if (pix_fmt != PIX_FMT_NONE)
      size = CalPixFmtSize(pix_fmt, w, h, 16);
    if (size == 0) // unknown pixel format
      size = w * h * 4;
    for (size_t i = 0; i < req.count; i++) {
      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(buf));
      buf.type = req.type;
      buf.index = i;
      buf.memory = req.memory;

      auto &&buffer =
          MediaBuffer::Alloc2(size, MediaBuffer::MemType::MEM_HARD_WARE);
      if (buffer.GetSize() == 0) {
        errno = ENOMEM;
        return -1;
      }
      buffer_vec.push_back(buffer);
      buf.m.fd = buffer.GetFD();
      buf.length = buffer.GetSize();
      if (v4l2_ctx->IoCtrl(VIDIOC_QBUF, &buf) < 0) {
        RKMEDIA_LOGE("%s ioctl(VIDIOC_QBUF): %m\n", dev);
        return -1;
      }
    }
  } else if (memory_type == V4L2_MEMORY_MMAP) {
    for (size_t i = 0; i < req.count; i++) {
      struct v4l2_buffer buf;
      struct v4l2_plane planes[FMT_NUM_PLANES];
      void *ptr_lane0 = MAP_FAILED;
      void *ptr_lane1 = MAP_FAILED;
      size_t len_lane0 = 0;
      size_t len_lane1 = 0;
      V4L2Buffer *buffer = new V4L2Buffer();
      if (!buffer) {
        errno = ENOMEM;
        return -1;
      }
      buffer_vec.push_back(
          MediaBuffer(nullptr, 0, -1, 0, -1, buffer, __free_v4l2buffer));
      memset(&buf, 0, sizeof(buf));
      memset(&planes, 0, sizeof(planes));

      buf.type = req.type;
      buf.index = i;
      buf.memory = req.memory;
      if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type) {
        buf.m.planes = planes;
        buf.length = FMT_NUM_PLANES;
      }
      if (v4l2_ctx->IoCtrl(VIDIOC_QUERYBUF, &buf) < 0) {
        RKMEDIA_LOGE("%s ioctl(VIDIOC_QUERYBUF): %m\n", dev);
        return -1;
      }

      if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type) {
        len_lane0 = buf.m.planes[0].length;
        ptr_lane0 = mmap(
            NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */, fd, buf.m.planes[0].m.mem_offset);
        if (plane_cnt > 1) {
          len_lane1 = buf.m.planes[1].length;
          ptr_lane1 = mmap(NULL, buf.m.planes[1].length,
                           PROT_READ | PROT_WRITE /* required */,
                           MAP_SHARED /* recommended */, fd,
                           buf.m.planes[1].m.mem_offset);
          if (ptr_lane1 == MAP_FAILED) {
            ptr_lane1 = nullptr;
            RKMEDIA_LOGW("V4L2: Mplane: lane1 mmap failed!\n");
          }
        }
      } else {
        len_lane0 = buf.length;
        ptr_lane0 = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                              MAP_SHARED, fd, buf.m.offset);
        len_lane1 = 0;
        ptr_lane1 = nullptr;
      }

      if (ptr_lane0 == MAP_FAILED) {
        RKMEDIA_LOGE("%s v4l2_mmap (%d): %m\n", dev, (int)i);
        return -1;
      }
      MediaBuffer &mb = buffer_vec[i];
      buffer->munmap_f = vio.munmap_f;
      buffer->ptr = ptr_lane0;
      mb.SetPtr(ptr_lane0);
      buffer->length = len_lane0;
      mb.SetSize(len_lane0);
      if (ptr_lane1) {
        mb.SetDbgInfo(ptr_lane1);
        mb.SetDbgInfoSize(len_lane1);
        buffer->ptr1 = ptr_lane1;
        buffer->length1 = len_lane1;
      }

      RKMEDIA_LOGD("query buf.length=%zu\n", len_lane0);
      int dmafd = -1;
      if (!BufferExport(capture_type, i, &dmafd)) {
        buffer->dmafd = dmafd;
        mb.SetFD(dmafd);
      }
    }

    for (size_t i = 0; i < req.count; ++i) {
      struct v4l2_buffer qbuf;
      struct v4l2_plane qplanes[FMT_NUM_PLANES];
      memset(&qbuf, 0, sizeof(qbuf));
      qbuf.type = req.type;
      qbuf.memory = req.memory;
      qbuf.index = i;
      if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type) {
        qbuf.m.planes = qplanes;
        qbuf.length = FMT_NUM_PLANES;
      }
      if (v4l2_ctx->IoCtrl(VIDIOC_QBUF, &qbuf) < 0) {
        RKMEDIA_LOGE("%s, ioctl(VIDIOC_QBUF): %m\n", dev);
        return -1;
      }
    }
  }

  SetReadable(true);
  return 0;
}
int V4L2CaptureStream::Close() {
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

std::shared_ptr<MediaBuffer> V4L2CaptureStream::Read() {
  const char *dev = device.c_str();
  if (!started && v4l2_ctx->SetStarted(true)) {
    RKMEDIA_LOGI("Camera %d stream %d is started\n", camera_id, fd);
    started = true;
  }

  struct v4l2_buffer buf;
  struct v4l2_plane planes[FMT_NUM_PLANES];

  memset(&buf, 0, sizeof(buf));
  memset(planes, 0, sizeof(planes));
  buf.type = capture_type;
  buf.memory = memory_type;
  if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type) {
    buf.m.planes = planes;
    buf.length = FMT_NUM_PLANES;
  }

  int ret = v4l2_ctx->IoCtrl(VIDIOC_DQBUF, &buf);
  if (ret < 0) {
    RKMEDIA_LOGI("%s, ioctl(VIDIOC_DQBUF): %m\n", dev);
    return nullptr;
  }
  struct timeval buf_ts = buf.timestamp;
  MediaBuffer &mb = buffer_vec[buf.index];
  std::shared_ptr<MediaBuffer> ret_buf;
  int bytes_used = 0;
  int bytes_used_plane1 = 0;

  if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type) {
    bytes_used = buf.m.planes[0].bytesused;
    bytes_used_plane1 = buf.m.planes[1].bytesused;
  } else {
    bytes_used = buf.bytesused;
    bytes_used_plane1 = 0;
  }

  if (enable_user_picture && user_picture) {
    RKMEDIA_LOGD("%s: Send user picture...\n", __func__);
    ImageInfo info{pix_fmt, width, height, width, height};
    auto user_pic_buf =
        std::make_shared<ImageBuffer>(*(user_picture.get()), info);
    if (v4l2_ctx->IoCtrl(VIDIOC_QBUF, &buf) < 0)
      RKMEDIA_LOGE("%s, index=%d, ioctl(VIDIOC_QBUF): %m\n", dev, buf.index);

    return user_pic_buf;
  }

  if (bytes_used > 0) {
    if (pix_fmt != PIX_FMT_NONE) {
      ImageInfo info{pix_fmt, width, height, width, height};
      auto tmp_buf =
          std::make_shared<AutoQBUFImageBuffer>(mb, info, v4l2_ctx, buf);
      if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type)
        tmp_buf->SetPlanes(planes, FMT_NUM_PLANES * sizeof(struct v4l2_plane));
      ret_buf = tmp_buf;
    } else {
      auto tmp_buf = std::make_shared<AutoQBUFMediaBuffer>(mb, v4l2_ctx, buf);
      if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type)
        tmp_buf->SetPlanes(planes, FMT_NUM_PLANES * sizeof(struct v4l2_plane));
      ret_buf = tmp_buf;
    }
  }

  if (ret_buf) {
    assert(ret_buf->GetFD() == mb.GetFD());
    if (buf.memory == V4L2_MEMORY_DMABUF) {
      assert(ret_buf->GetFD() == buf.m.fd);
    }
    ret_buf->SetAtomicTimeVal(buf_ts);
    ret_buf->SetTimeVal(buf_ts);
    ret_buf->SetValidSize(bytes_used);
    if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == capture_type)
      ret_buf->SetDbgInfoSize(bytes_used_plane1);

    recent_time = ret_buf->GetUSTimeStamp();

    /* set cover */
    param_mtx.lock();
    std::vector<ImageBorder> cur_lines = lines;
    param_mtx.unlock();
    for (auto &line : cur_lines) {
      if (!line.enable)
        continue;
      int x, y, w, h;
      x = line.x;
      y = line.y;
      w = line.w;
      h = line.h;
      if (x + w > width || y + h > height)
        continue;
      rga_info_t dst_info;
      memset(&dst_info, 0, sizeof(dst_info));
      dst_info.fd = ret_buf->GetFD();
      if (dst_info.fd < 0)
        dst_info.virAddr = ret_buf->GetPtr();
      dst_info.mmuFlag = 1;
      dst_info.color = line.color;
      rga_set_rect(&dst_info.rect, x, y, w, h, width, height,
                   get_rga_format(pix_fmt));
      RgaFilter::gRkRga.RkRgaCollorFill(&dst_info);
    }

    /* set osd */
    param_mtx.lock();
    auto cur_osds = osds;
    param_mtx.unlock();
    for (auto &osd : cur_osds) {
      if (!osd.second.enable)
        continue;

      rga_info_t src_info, pat_info;
      memset(&src_info, 0, sizeof(src_info));
      src_info.fd = ret_buf->GetFD();
      if (src_info.fd < 0)
        src_info.virAddr = ret_buf->GetPtr();
      src_info.mmuFlag = 1;
      src_info.blend = 0xff0501;
      rga_set_rect(&src_info.rect, osd.second.x, osd.second.y, osd.second.w,
                   osd.second.h, width, height, get_rga_format(pix_fmt));
      memset(&pat_info, 0, sizeof(rga_info_t));
      pat_info.virAddr = osd.second.data;
      pat_info.mmuFlag = 1;
      rga_set_rect(&pat_info.rect, 0, 0, osd.second.w, osd.second.h,
                   osd.second.w, osd.second.h, get_rga_format(osd.second.fmt));
      RgaFilter::gRkRga.RkRgaBlit(&src_info, &src_info, &pat_info);
    }
  } else {
    if (v4l2_ctx->IoCtrl(VIDIOC_QBUF, &buf) < 0)
      RKMEDIA_LOGE("%s, index=%d, ioctl(VIDIOC_QBUF): %m\n", dev, buf.index);
  }

  return ret_buf;
}

DEFINE_STREAM_FACTORY(V4L2CaptureStream, Stream)

const char *FACTORY(V4L2CaptureStream)::ExpectedInputDataType() {
  return nullptr;
}

const char *FACTORY(V4L2CaptureStream)::OutPutDataType() {
  return GetStringOfV4L2Fmts().c_str();
}

} // namespace easymedia
