// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "encoder.h"
#include "flow.h"
#include "key_string.h"
#include "media_config.h"
#include "media_type.h"
#include "message.h"
#include "stream.h"
#include "utils.h"

static char optstr[] = "?i:o:w:h:";

static bool quit = false;

static void SigTermHandler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void PushVideoHandler(unsigned char *buffer, unsigned int buffer_size,
                      int64_t present_time, int nal_type) {
  printf("PushVideoHandler buffer %p, buffer_size %d present_time %lld "
         "nal_type %d\n",
         buffer, buffer_size, present_time, nal_type);
}

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;

  std::string flow_name;
  std::string flow_param;
  std::string stream_name;
  std::string stream_param;
  std::shared_ptr<easymedia::Flow> input_flow_;
  std::shared_ptr<easymedia::Flow> enc_flow_;
  std::shared_ptr<easymedia::Flow> link_flow_;

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
    case '?':
    default:
      printf("usage example: \n");
      printf("link_flow_test -i input.yuv -o output.h264 -w 320 -h 240\n");
      exit(0);
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  signal(SIGINT, SigTermHandler);

  easymedia::REFLECTOR(Flow)::DumpFactories();
  easymedia::REFLECTOR(Stream)::DumpFactories();

  int fps = 30;
  int w_align = UPALIGNTO16(w);
  int h_align = UPALIGNTO16(h);

  ImageInfo info;
  info.pix_fmt = PIX_FMT_NV12;
  info.width = w;
  info.height = h;
  info.vir_width = w_align;
  info.vir_height = h_align;

  flow_name = "file_read_flow";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_PATH, input_path);
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "re"); // read and close-on-exec
  PARAM_STRING_APPEND(flow_param, KEY_MEM_TYPE, KEY_MEM_HARDWARE);
  flow_param += easymedia::to_param_string(info);
  PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, fps); // rhythm reading
  PARAM_STRING_APPEND_TO(flow_param, KEY_LOOP_TIME, 9999);
  printf("\nflow_param 1:\n%s\n", flow_param.c_str());

  input_flow_ = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!input_flow_) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "video_enc";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE,
                      IMAGE_NV12); // IMAGE_NV12 IMAGE_YUYV422
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, VIDEO_H264);
  MediaConfig enc_config;
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
  vid_cfg.frame_rate = fps;
  vid_cfg.level = 52;
  vid_cfg.gop_size = fps;
  vid_cfg.profile = 100;
  // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
  vid_cfg.rc_quality = KEY_HIGHEST;
  vid_cfg.rc_mode = KEY_CBR;
  std::string enc_param;
  enc_param.append(easymedia::to_param_string(enc_config, VIDEO_H264));
  enc_param = easymedia::JoinFlowParam(flow_param, 1, enc_param);
  printf("\nparam 2:\n%s\n", enc_param.c_str());

  enc_flow_ = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), enc_param.c_str());
  if (!enc_flow_) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    input_flow_.reset();
    exit(EXIT_FAILURE);
  }

  flow_name = "link_flow";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, "video:h264");
  printf("\nparam 3:\n%s\n", flow_param.c_str());

  link_flow_ = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!link_flow_) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    input_flow_.reset();
    enc_flow_.reset();
    exit(EXIT_FAILURE);
  }

  enc_flow_->AddDownFlow(link_flow_, 0, 0);
  input_flow_->AddDownFlow(enc_flow_, 0, 0);

  link_flow_->SetVideoHandler(PushVideoHandler);

  while (!quit)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  enc_flow_->RemoveDownFlow(link_flow_);
  link_flow_.reset();
  input_flow_->RemoveDownFlow(enc_flow_);
  enc_flow_.reset();
  input_flow_.reset();
}
