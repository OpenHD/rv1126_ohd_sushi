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

static volatile bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static char optstr[] = "?:i:o:w:h:f:t:m:";

static void print_usage(char *name) {
  printf("usage example for normal mode: \n");
  printf("%s -i /dev/video0 -w 1920 -h 1080 "
         "-f nv12 -t h264\n",
         name);
  printf("#[-t] uvc enc type support list:\n\th264\n\tyuyv\n\tmjpeg\n");
  printf("#[-f] pix formate support list:\n\tyuyv422\n\tnv12\n");
  printf("#[-m] mode support list:\n\tnormal\n\tstressTest\n");
}

int main(int argc, char **argv) {
  int c, local_file_flag;
  int video_width = 1920;
  int video_height = 1080;
  std::string pixel_format;
  int video_fps = 30;
  std::string video_enc_type = IMAGE_JPEG;
  int test_mode = 0; // 0 for normal,1 for stressTest.

  std::string output_path;
  std::string input_path;

  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::string uvc_param;

  std::shared_ptr<easymedia::Flow> video_read_flow;
  std::shared_ptr<easymedia::Flow> uvc_flow;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("#IN ARGS: input path: %s\n", input_path.c_str());
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
    case 't':
      video_enc_type = optarg;
      printf("#IN ARGS: video_enc_type: %s\n", video_enc_type.c_str());
      break;
    case 'm':
      if (strstr(optarg, "stressTest")) {
        test_mode = 1;
        printf("======================================\n");
        printf("# Stress Test\n");
        printf("======================================\n");
      }
      break;
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  if (input_path.empty()) {
    printf("ERROR: path is not valid!\n");
    exit(EXIT_FAILURE);
  }

  // add prefix for pixformat
  if ((pixel_format == "yuyv422") || (pixel_format == "nv12"))
    pixel_format = "image:" + pixel_format;
  else {
    printf("ERROR: image type:%s not support!\n", pixel_format.c_str());
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // add prefix for encoder type
  if ((video_enc_type == "h264") || (video_enc_type == "h265"))
    video_enc_type = "video:" + video_enc_type;
  else if ((video_enc_type == "mjpeg") || (video_enc_type == "yuyv"))
    video_enc_type = "image:jpeg";
  else {
    printf("ERROR: encoder type:%s not support!\n", video_enc_type.c_str());
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, sigterm_handler);
  printf("#Dump factroys:");
  easymedia::REFLECTOR(Flow)::DumpFactories();
  printf("#Dump streams:");
  easymedia::REFLECTOR(Stream)::DumpFactories();

  if (strstr(input_path.c_str(), "/dev/video") ||
      strstr(input_path.c_str(), "rkispp")) {
    printf("INFO: reading yuv frome camera!\n");
    local_file_flag = 0;
  } else {
    printf("INFO: reading yuv frome file!\n");
    local_file_flag = 1;
  }

RESTART:

  if (local_file_flag) {
    // Reading yuv from file.
    flow_name = "file_read_flow";
    flow_param = "";
    ImageInfo info;
    info.pix_fmt = StringToPixFmt(pixel_format.c_str());
    info.width = video_width;
    info.height = video_height;
    info.vir_width = UPALIGNTO16(video_width);
    info.vir_height = UPALIGNTO16(video_height);

    flow_param.append(easymedia::to_param_string(info, 1).c_str());
    PARAM_STRING_APPEND(flow_param, KEY_PATH, input_path);
    PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                        "re"); // read and close-on-exec
    PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, video_fps);
    PARAM_STRING_APPEND_TO(flow_param, KEY_LOOP_TIME, 0);
    RKMEDIA_LOGI("#FileRead flow param:\n%s\n", flow_param.c_str());

    video_read_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
        flow_name.c_str(), flow_param.c_str());
    if (!video_read_flow) {
      RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
      exit(EXIT_FAILURE);
    }
  } else {
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
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_VIR_WIDTH, video_width);
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_VIR_HEIGHT, video_height);

    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
    printf("\n#VideoCapture flow param:\n%s\n", flow_param.c_str());
    video_read_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
        flow_name.c_str(), flow_param.c_str());
    if (!video_read_flow) {
      fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
      exit(EXIT_FAILURE);
    }
  }

  flow_name = "uvc_flow";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "uvc_flow");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, "NULL");
  uvc_param = "";
  PARAM_STRING_APPEND_TO(uvc_param, KEY_UVC_EVENT_CODE, 0);
  PARAM_STRING_APPEND_TO(uvc_param, KEY_UVC_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(uvc_param, KEY_UVC_HEIGHT, video_height);
  PARAM_STRING_APPEND(uvc_param, KEY_UVC_FORMAT, video_enc_type);

  flow_param = easymedia::JoinFlowParam(flow_param, 1, uvc_param);
  printf("\n#UVC flow param:\n%s\n", flow_param.c_str());
  uvc_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!uvc_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  video_read_flow->AddDownFlow(uvc_flow, 0, 0);

  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

  while (!test_mode && !quit) {
    easymedia::msleep(100);
  }

  if (test_mode) {
    srand((unsigned)time(NULL));
    int delay_ms = rand() % 100000;
    printf("=> Stress Test: sleep %dms\n", delay_ms);
    easymedia::msleep(delay_ms);
  }

  video_read_flow->RemoveDownFlow(uvc_flow);
  video_read_flow.reset();

  uvc_flow.reset();
  RKMEDIA_LOGI("%s deinitial finish\n", argv[0]);

  if (test_mode && !quit) {
    printf("=> Stress Test: resatr\n");
    goto RESTART;
  }
  return 0;
}
