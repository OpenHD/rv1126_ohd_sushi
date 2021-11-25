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
#include "image.h"
#include "key_string.h"
#include "media_config.h"
#include "media_type.h"
#include "message.h"
#include "stream.h"
#include "utils.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static char optstr[] = "?:i:o:w:h:f:n:b:";

static void print_usage(char *name) {
  printf("@function:\nCrop input image to half of original.\n");
  printf("@usage example: \n");
  printf("%s -i /dev/video0 -o output.yuv -w 1920 -h 1080 "
         "-f nv12\n",
         name);
}

int main(int argc, char **argv) {
  int c;
  int video_width = 1920;
  int video_height = 1080;
  std::string pixel_format;
  bool bufferpool = false;

  std::string output_path;
  std::string input_path;

  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::string filter_param;

  std::shared_ptr<easymedia::Flow> video_read_flow;
  std::shared_ptr<easymedia::Flow> video_process_flow;
  std::shared_ptr<easymedia::Flow> video_save_flow;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("#IN ARGS: input path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("#IN ARGS: output file path: %s\n", output_path.c_str());
      break;
    case 'w':
      video_width = atoi(optarg);
      printf("#IN ARGS: video_width: %d\n", video_width);
      break;
    case 'h':
      video_height = atoi(optarg);
      printf("#IN ARGS: video_height: %d\n", video_height);
      break;
    case 'f':
      pixel_format = optarg;
      printf("#IN ARGS: pixel_format: %s\n", pixel_format.c_str());
      break;
    case 'b':
      bufferpool = atoi(optarg) ? true : false;
      printf("#IN ARGS: bufferpool: %d\n", bufferpool);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  if (input_path.empty() || output_path.empty()) {
    printf("ERROR: path is not valid!\n");
    exit(EXIT_FAILURE);
  }

  // add prefix for pixformat
  pixel_format = "image:" + pixel_format;
  if (StringToPixFmt(pixel_format.c_str()) == PIX_FMT_NONE) {
    printf("ERROR: image type:%s not support!\n", pixel_format.c_str());
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, sigterm_handler);
  printf("#Dump factroys:");
  easymedia::REFLECTOR(Flow)::DumpFactories();
  printf("#Dump streams:");
  easymedia::REFLECTOR(Stream)::DumpFactories();

  // init log:set log level and log method.
  LOG_INIT();

  // Reading yuv from camera
  flow_name = "source_stream";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "v4l2_capture_stream");
  PARAM_STRING_APPEND(flow_param, KEK_THREAD_SYNC_MODEL, KEY_SYNC);
  PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);
  PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 5);
  stream_param = "";
  PARAM_STRING_APPEND_TO(stream_param, KEY_USE_LIBV4L2, 1);
  PARAM_STRING_APPEND(stream_param, KEY_DEVICE, input_path);
  PARAM_STRING_APPEND(stream_param, KEY_V4L2_CAP_TYPE,
                      KEY_V4L2_C_TYPE(VIDEO_CAPTURE));
  PARAM_STRING_APPEND(stream_param, KEY_V4L2_MEM_TYPE,
                      KEY_V4L2_M_TYPE(MEMORY_DMABUF));
  PARAM_STRING_APPEND_TO(stream_param, KEY_FRAMES,
                         4); // if not set, default is 2
  PARAM_STRING_APPEND(stream_param, KEY_OUTPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, video_height);

  flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
  printf("\n#VideoCapture flow param:\n%s\n", flow_param.c_str());
  video_read_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_read_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  // Crop to half of the original image
  int target_width = video_width / 2;
  int target_height = video_height / 2;

  flow_name = "filter";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkrga");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  // Set output buffer type.
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, pixel_format);
  // Set output buffer size.
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH, target_width);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, target_height);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH, target_width);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, target_height);
  // enable buffer pool?
  if (bufferpool) {
    PARAM_STRING_APPEND(flow_param, KEY_MEM_TYPE, KEY_MEM_HARDWARE);
    PARAM_STRING_APPEND_TO(flow_param, KEY_MEM_CNT, 10);
  }

  filter_param = "";
  ImageRect src_rect = {0, 0, video_width, video_height};
  ImageRect dst_rect = {0, 0, target_width, target_height};
  std::vector<ImageRect> rect_vect;
  rect_vect.push_back(src_rect);
  rect_vect.push_back(dst_rect);
  PARAM_STRING_APPEND(filter_param, KEY_BUFFER_RECT,
                      easymedia::TwoImageRectToString(rect_vect).c_str());
  PARAM_STRING_APPEND_TO(filter_param, KEY_BUFFER_ROTATE, 0);
  flow_param = easymedia::JoinFlowParam(flow_param, 1, filter_param);
  printf("\n#Rkrga Filter flow param:\n%s\n", flow_param.c_str());
  video_process_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_process_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "file_write_flow";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_PATH, output_path.c_str());
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "w+"); // read and close-on-exec
  printf("\n#FileWrite:\n%s\n", flow_param.c_str());
  video_save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_save_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  video_process_flow->AddDownFlow(video_save_flow, 0, 0);
  video_read_flow->AddDownFlow(video_process_flow, 0, 0);

  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

  while (!quit) {
    easymedia::msleep(100);
  }

  RKMEDIA_LOGI("%s quit!\n", argv[0]);

  video_read_flow->RemoveDownFlow(video_process_flow);
  video_read_flow.reset();
  video_process_flow->RemoveDownFlow(video_save_flow);
  video_process_flow.reset();
  video_save_flow.reset();

  return 0;
}
