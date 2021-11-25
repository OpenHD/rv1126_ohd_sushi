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

#include "buffer.h"
#include "key_string.h"

#include "control.h"
#include "stream.h"

static char optstr[] = "?i:f:w:h:r:p:z:";

int main(int argc, char **argv) {
  int c;
  std::string drm_path;
  std::string plane_type;
  std::string fmt;
  int prefer_width = 1920;
  int prefer_height = 1080;
  int prefer_fps = 60;
  int zindex = -1;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      drm_path = optarg;
      printf("input dev path: %s\n", drm_path.c_str());
      break;
    case 'f':
      fmt = optarg;
      printf("prefer format: %s\n", fmt.c_str());
      break;
    case 'w':
      prefer_width = atoi(optarg);
      break;
    case 'h':
      prefer_height = atoi(optarg);
      break;
    case 'p':
      plane_type = optarg;
      break;
    case 'r':
      prefer_fps = atoi(optarg);
      break;
    case 'z':
      zindex = atoi(optarg);
      break;
    case '?':
    default:
      printf("usage example: \n");
      printf("drm_display_test -i /dev/dri/card0\n");
      printf("drm_display_test -i /dev/dri/card0 -f image:rgb888 "
             "-w 1280 -h 720 -r 60 -p Primary -z 0\n");
      exit(0);
    }
  }
  if (drm_path.empty())
    assert(0);
  std::string param;
  PARAM_STRING_APPEND(param, KEY_DEVICE, drm_path);
  if (!fmt.empty())
    PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, fmt);
  if (prefer_width > 0 && prefer_height > 0) {
    PARAM_STRING_APPEND_TO(param, KEY_BUFFER_WIDTH, prefer_width);
    PARAM_STRING_APPEND_TO(param, KEY_BUFFER_HEIGHT, prefer_height);
  }
  if (prefer_fps > 0)
    PARAM_STRING_APPEND_TO(param, KEY_FPS, prefer_fps);
  if (!plane_type.empty()) {
    const char *pt = plane_type.c_str();
    const char *kpt = nullptr;
    if (!strcasecmp(pt, KEY_PRIMARY))
      kpt = KEY_PRIMARY;
    else if (!strcasecmp(pt, KEY_OVERLAY))
      kpt = KEY_OVERLAY;
    else if (!strcasecmp(pt, KEY_CURSOR))
      kpt = KEY_PRIMARY;
    if (!kpt)
      assert(0);
    PARAM_STRING_APPEND(param, KEY_PLANE_TYPE, kpt);
  }
  if (zindex >= 0)
    PARAM_STRING_APPEND_TO(param, KEY_ZPOS, zindex);

  easymedia::REFLECTOR(Stream)::DumpFactories();
  const char *stream_name = "drm_output_stream";
  auto out = easymedia::REFLECTOR(Stream)::Create<easymedia::Stream>(
      stream_name, param.c_str());
  if (!out) {
    fprintf(stderr, "Create stream %s failed\n", stream_name);
    exit(EXIT_FAILURE);
  }
  ImageInfo screen_info;
  int ret = out->IoCtrl(easymedia::G_PLANE_IMAGE_INFO, &screen_info);
  if (ret) {
    fprintf(stderr, "Get display screen info failed\n");
    exit(EXIT_FAILURE);
  }
  ImageRect screen_rect = {0, 0, screen_info.width, screen_info.height};
  ret = out->IoCtrl(easymedia::S_SOURCE_RECT, &screen_rect);
  if (ret) {
    fprintf(stderr, "Set display source rect failed\n");
    exit(EXIT_FAILURE);
  }
  // screen_rect = {screen_info.width >> 1, screen_info.height >> 1,
  // screen_info.width, screen_info.height};
  ret = out->IoCtrl(easymedia::S_DESTINATION_RECT, &screen_rect);
  if (ret) {
    fprintf(stderr, "Set display destination rect failed\n");
    exit(EXIT_FAILURE);
  }
  int size = CalPixFmtSize(screen_info);
  auto &&mb1 = easymedia::MediaBuffer::Alloc2(
      size, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert((int)mb1.GetSize() >= size);
  auto fb1 = std::make_shared<easymedia::ImageBuffer>(mb1, screen_info);
  assert(fb1);
  auto &&mb2 = easymedia::MediaBuffer::Alloc2(
      size, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
  assert((int)mb2.GetSize() >= size);
  auto fb2 = std::make_shared<easymedia::ImageBuffer>(mb2, screen_info);
  assert(fb2);

  int index = 0;
  int pixel = 0;
  while (pixel < 255 * 255) {
    std::shared_ptr<easymedia::ImageBuffer> input;
    if (index == 0) {
      input = fb1;
      index = 1;
    } else if (index == 1) {
      input = fb2;
      index = 0;
    }
    srand((unsigned)time(0));
    screen_rect = {rand() % (screen_info.width >> 1),
                   rand() % (screen_info.height >> 1),
                   (rand() % ((screen_info.width >> 1) - 100)) + 100,
                   (rand() % ((screen_info.height >> 1) - 100) + 100)};
    ImageRect rects[2] = {screen_rect, screen_rect};
    ret = out->IoCtrl(easymedia::S_SRC_DST_RECT, rects);
    if (ret)
      fprintf(stderr,
              "Set display source/destination rect[%d,%d %d,%d] failed\n",
              screen_rect.x, screen_rect.y, screen_rect.w, screen_rect.h);
    memset(input->GetPtr(), pixel++, input->GetSize());
    if (!out->Write(input))
      fprintf(stderr, "Write buffer to stream failed\n");
    // easymedia::msleep(16);
  }

  return 0;
}
