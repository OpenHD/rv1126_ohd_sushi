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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "stream.h"

static char optstr[] = "?i:o:d:w:h:f:c:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;
  std::string output_format;
  bool display = false;
  int dump_frm = 100;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("output file path: %s\n", output_path.c_str());
      break;
    case 'd':
      display = !!atoi(optarg);
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 'f': {
      char *cut = strchr(optarg, ',');
      input_format = optarg;
      if (cut) {
        cut[0] = 0;
        output_format = cut + 1;
      }
      printf("input format: %s\n", input_format.c_str());
      printf("output format: %s\n", output_format.c_str());
    } break;
    case 'c':
      dump_frm = atoi(optarg);
      printf("dump frame count: %d\n", dump_frm);
      break;
    case '?':
    default:
      printf("usage example: \n");
      printf("camera_cap_test -i /dev/video0 -o output.yuv -w 1920 -h 1080 -f "
             "image:yuyv422\n");
      printf("camera_cap_test -d 1 -o output.yuv -w 1920 -h 1080 -f "
             "image:nv16,image:argb8888\n");
      exit(0);
    }
  }
  if (input_path.empty())
    exit(EXIT_FAILURE);
  if (output_path.empty() && !display)
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  if (input_format.empty())
    exit(EXIT_FAILURE);
  if (display && output_format.empty())
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  easymedia::REFLECTOR(Stream)::DumpFactories();
  std::string stream_name = "v4l2_capture_stream";
  std::string param;
  PARAM_STRING_APPEND_TO(param, KEY_USE_LIBV4L2, 1);
  PARAM_STRING_APPEND(param, KEY_DEVICE, input_path);
  // PARAM_STRING_APPEND(param, KEY_SUB_DEVICE, sub_input_path);
  PARAM_STRING_APPEND(param, KEY_V4L2_CAP_TYPE, KEY_V4L2_C_TYPE(VIDEO_CAPTURE));
  PARAM_STRING_APPEND(param, KEY_V4L2_MEM_TYPE, KEY_V4L2_M_TYPE(MEMORY_DMABUF));
  PARAM_STRING_APPEND_TO(param, KEY_FRAMES, 4); // if not set, default is 2
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, input_format);
  PARAM_STRING_APPEND_TO(param, KEY_BUFFER_WIDTH, w);
  PARAM_STRING_APPEND_TO(param, KEY_BUFFER_HEIGHT, h);
  auto input = easymedia::REFLECTOR(Stream)::Create<easymedia::Stream>(
      stream_name.c_str(), param.c_str());
  if (!input) {
    fprintf(stderr, "create stream \"%s\" failed\n", stream_name.c_str());
    exit(EXIT_FAILURE);
  }

  std::shared_ptr<easymedia::Stream> output;
  if (!output_path.empty()) {
    // test dump
    stream_name = "file_write_stream";
    param = "";
    PARAM_STRING_APPEND(param, KEY_PATH, output_path);
    PARAM_STRING_APPEND(param, KEY_OPEN_MODE, "we");
    output = easymedia::REFLECTOR(Stream)::Create<easymedia::Stream>(
        stream_name.c_str(), param.c_str());
  } else {
    fprintf(stderr, "TODO: display to screen");
    exit(EXIT_FAILURE);
  }
  assert(output);

  while (dump_frm-- > 0) {
    auto buffer = input->Read();
    assert(buffer && buffer->GetValidSize() > 0);
    output->Write(buffer->GetPtr(), 1, buffer->GetValidSize());
    if (buffer->GetType() == Type::Image) {
      // if type image, we can static cast it
      auto img_buffer =
          std::static_pointer_cast<easymedia::ImageBuffer>(buffer);
      printf("w: %d, h: %d\n", img_buffer->GetWidth(), img_buffer->GetHeight());
    }
  }

  output.reset();
  input.reset();

  return 0;
}
