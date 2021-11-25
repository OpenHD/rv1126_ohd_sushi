// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "encoder.h"
#include "key_string.h"

static char optstr[] = "?i:o:w:h:f:";

static int read_yuv_image(void *buf, int fd, ImageInfo &info) {
  int ret = 0;
  int read_size;
  int row = 0;
  int width = info.width;
  int height = info.height;
  int hor_stride = info.vir_width;
  int ver_stride = info.vir_height;
  PixelFormat fmt = info.pix_fmt;
  unsigned char *buf_y = NULL;
  unsigned char *buf_u = NULL;
  unsigned char *buf_v = NULL;

  // NOTE: diff from gen_yuv_image
  buf_y = (unsigned char *)buf;
  buf_u = buf_y + hor_stride * ver_stride;
  buf_v = buf_u + hor_stride * ver_stride / 4;

  switch (fmt) {
  case PIX_FMT_NV12: {
    for (row = 0; row < height; row++) {
      read_size = read(fd, buf_y + row * hor_stride, width);
      if (read_size != width) {
        if (read_size == 0)
          printf("read the end of yuv file.\n");
        else
          printf("read ori yuv file luma failed\n");

        ret = -1;
        goto err;
      }
    }

    for (row = 0; row < height / 2; row++) {
      read_size = read(fd, buf_u + row * hor_stride, width);
      if (read_size != width) {
        printf("read ori yuv file cb failed\n");
        ret = -1;
        goto err;
      }
    }
  } break;
  case PIX_FMT_YUV420P: {
    for (row = 0; row < height; row++) {
      read_size = read(fd, buf_y + row * hor_stride, width);
      if (read_size != width) {
        if (read_size == 0)
          printf("read the end of yuv file.\n");
        else
          printf("read ori yuv file luma failed\n");

        ret = -1;
        goto err;
      }
    }

    for (row = 0; row < height / 2; row++) {
      read_size = read(fd, buf_u + row * hor_stride / 2, width / 2);
      if (read_size != width / 2) {
        printf("read ori yuv file cb failed\n");
        ret = -1;
        goto err;
      }
    }

    for (row = 0; row < height / 2; row++) {
      read_size = read(fd, buf_v + row * hor_stride / 2, width / 2);
      if (read_size != width / 2) {
        printf("read ori yuv file cr failed\n");
        ret = -1;
        goto err;
      }
    }
  } break;
  case PIX_FMT_ARGB8888: {
    for (row = 0; row < height; row++) {
      read_size = read(fd, buf_y + row * hor_stride * 4, width * 4);
    }
  } break;
  case PIX_FMT_YUYV422:
  case PIX_FMT_UYVY422: {
    for (row = 0; row < height; row++) {
      read_size = read(fd, buf_y + row * hor_stride, width * 2);
    }
  } break;
  default: {
    printf("read image do not support fmt %d\n", fmt);
    ret = -1;
  } break;
  }

err:

  return ret;
}

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;
  std::string output_format;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input file path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("output file path: %s\n", output_path.c_str());
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 'f': {
      char *cut = strchr(optarg, '_');
      if (!cut) {
        fprintf(stderr, "input/output format must be cut by \'_\'\n");
        exit(EXIT_FAILURE);
      }
      cut[0] = 0;
      input_format = optarg;
      output_format = cut + 1;
      if (output_format == "jpg" || output_format == "mjpg" ||
          output_format == "mjpeg")
        output_format = IMAGE_JPEG;
      else if (output_format == "h264")
        output_format = VIDEO_H264;
      else if (output_format == "h265")
        output_format = VIDEO_H265;
      printf("input format: %s\n", input_format.c_str());
      printf("output format: %s\n", output_format.c_str());
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rkmpp_enc_test -i input.yuv -o output.h264 -w 320 -h 240 -f "
             "nv12_h264\n");
      printf("rkmpp_enc_test -i input.yuv -o output.hevc -w 320 -h 240 -f "
             "nv12_h265\n");
      printf("rkmpp_enc_test -i input.yuv -o output.mjpeg -w 320 -h 240 -f "
             "nv12_jpeg\n");
      exit(0);
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  if (input_format.empty() || output_format.empty())
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  int input_file_fd = open(input_path.c_str(), O_RDONLY | O_CLOEXEC);
  assert(input_file_fd >= 0);
  unlink(output_path.c_str());
  int output_file_fd =
      open(output_path.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC, 0664);
  assert(output_file_fd >= 0);

  PixelFormat fmt = PIX_FMT_NONE;
  if (input_format == "nv12") {
    fmt = PIX_FMT_NV12;
  } else {
    fprintf(stderr, "TO BE TESTED <%s:%s,%d>\n", __FILE__, __FUNCTION__,
            __LINE__);
    exit(EXIT_FAILURE);
  }

  easymedia::REFLECTOR(Encoder)::DumpFactories();

  std::string mpp_codec = "rkmpp";
  std::string param;
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, output_format);
  auto mpp_enc = easymedia::REFLECTOR(Encoder)::Create<easymedia::VideoEncoder>(
      mpp_codec.c_str(), param.c_str());

  if (!mpp_enc) {
    fprintf(stderr, "Create encoder %s failed\n", mpp_codec.c_str());
    exit(EXIT_FAILURE);
  }

  ImageInfo info = {fmt, w, h, UPALIGNTO16(w), UPALIGNTO16(h)};
  MediaConfig enc_config;

  if (output_format == VIDEO_H264 || output_format == VIDEO_H265) {
    VideoConfig &vid_cfg = enc_config.vid_cfg;
    ImageConfig &img_cfg = vid_cfg.image_cfg;
    img_cfg.image_info = info;
    vid_cfg.qp_init = 24;
    vid_cfg.qp_step = 4;
    vid_cfg.qp_min = 12;
    vid_cfg.qp_max = 48;
    vid_cfg.bit_rate = w * h * 7;
    if (vid_cfg.bit_rate > 1000000) {
      vid_cfg.bit_rate /= 1000000;
      vid_cfg.bit_rate *= 1000000;
    }
    vid_cfg.frame_rate = 30;
    vid_cfg.level = 52;
    vid_cfg.gop_size = vid_cfg.frame_rate;
    vid_cfg.profile = 100;
    // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
    vid_cfg.rc_quality = KEY_HIGHEST;
    vid_cfg.rc_mode = KEY_CBR;
  } else if (output_format == IMAGE_JPEG) {
    ImageConfig &img_cfg = enc_config.img_cfg;
    img_cfg.image_info = info;
    img_cfg.qfactor = 50;
  }

  if (!mpp_enc->InitConfig(enc_config)) {
    fprintf(stderr, "Init config of encoder %s failed\n", mpp_codec.c_str());
    exit(EXIT_FAILURE);
  }

  void *extra_data = nullptr;
  size_t extra_data_size = 0;
  mpp_enc->GetExtraData(&extra_data, &extra_data_size);
  fprintf(stderr, "extra_data: %p, extra_data_size: %d\n", extra_data,
          (int)extra_data_size);
  if (extra_data && extra_data_size > 0) {
    // if there is extra data, write it first
    ssize_t count = write(output_file_fd, extra_data, extra_data_size);
    if (count < 0) {
      RKMEDIA_LOGI("write extra data failed\n");
    }
  }

  size_t len = CalPixFmtSize(fmt, info.vir_width, info.vir_height);
  auto &&src_mb = easymedia::MediaBuffer::Alloc2(
      len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert(src_mb.GetSize() > 0);
  auto src_buffer = std::make_shared<easymedia::ImageBuffer>(src_mb, info);
  assert(src_buffer && src_buffer->GetSize() >= len);

  auto dst_buffer = easymedia::MediaBuffer::Alloc(
      len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert(dst_buffer && dst_buffer->GetSize() >= len);

  int index = 0;
  while (read_yuv_image(src_buffer->GetPtr(), input_file_fd, info) >= 0) {
    src_buffer->SetValidSize(len);
    dst_buffer->SetValidSize(dst_buffer->GetSize());
    index++;
    if (0 != mpp_enc->Process(src_buffer, dst_buffer, nullptr)) {
      fprintf(stderr, "frame %d encode failed\n", index);
      continue;
    }
    size_t out_len = dst_buffer->GetValidSize();
    fprintf(stderr, "frame %d encoded, type %s, out %d bytes\n", index,
            dst_buffer->GetUserFlag() & easymedia::MediaBuffer::kIntra
                ? "I frame"
                : "P frame",
            (int)out_len);
    ssize_t count = write(output_file_fd, dst_buffer->GetPtr(), out_len);
    if (count < 0) {
      RKMEDIA_LOGI("write dst buffer, failed\n");
    }
  }

  close(input_file_fd);
  close(output_file_fd);

  return 0;
}
