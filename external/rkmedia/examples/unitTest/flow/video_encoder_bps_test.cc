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

#include <iostream>
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

static char optstr[] = "?:i:o:w:h:f:t:b:";

static void print_usage(char *name) {
  printf("usage example: \n");
  printf("%s -i /dev/video0 -o output.h264 -w 1920 -h 1080 "
         "-f nv12 -t h264 -b 777600\n",
         name);
  printf("#[-t] support list:\n\th264\n\th265\n\tjpeg\n");
  printf("#[-f] support list:\n\tyuyv422\n\tnv12\n");
}

int main(int argc, char **argv) {
  int c, local_file_flag;
  int video_width = 1920;
  int video_height = 1080;
  int vir_width, vir_height;
  std::string pixel_format;
  int video_fps = 30;
  unsigned int bpsmax = 777600;
  std::string video_enc_type = VIDEO_H264;

  std::string output_path;
  std::string input_path;

  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::string enc_param;

  std::shared_ptr<easymedia::Flow> video_read_flow;
  std::shared_ptr<easymedia::Flow> video_encoder_flow;
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
    case 't':
      video_enc_type = optarg;
      printf("#IN ARGS: video_enc_type: %s\n", video_enc_type.c_str());
      break;
    case 'b':
      bpsmax = atoi(optarg);
      printf("#IN ARGS: bpsmax: %d\n", bpsmax);
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
  else if (video_enc_type == "jpeg")
    video_enc_type = "image:" + video_enc_type;
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
    vir_width = info.vir_width;
    vir_height = info.vir_height;

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
    vir_width = UPALIGNTO(video_width, 8);
    ;
    vir_height = UPALIGNTO(video_height, 8);
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
  }

  flow_name = "video_enc";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, video_enc_type);

  enc_param = "";
  PARAM_STRING_APPEND_TO(enc_param, KEY_BUFFER_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(enc_param, KEY_BUFFER_HEIGHT, video_height);
  PARAM_STRING_APPEND_TO(enc_param, KEY_BUFFER_VIR_WIDTH, vir_width);
  PARAM_STRING_APPEND_TO(enc_param, KEY_BUFFER_VIR_HEIGHT, vir_height);
  if (video_enc_type != IMAGE_JPEG) {
    int bps = video_width * video_height * video_fps / 14;
    PARAM_STRING_APPEND_TO(enc_param, KEY_COMPRESS_BITRATE, bps);
    PARAM_STRING_APPEND_TO(enc_param, KEY_COMPRESS_BITRATE_MAX, bps * 17 / 16);
    PARAM_STRING_APPEND_TO(enc_param, KEY_COMPRESS_BITRATE_MIN, bps / 16);
    PARAM_STRING_APPEND(enc_param, KEY_FPS, "30/1");
    PARAM_STRING_APPEND(enc_param, KEY_FPS_IN, "30/1");
    PARAM_STRING_APPEND_TO(enc_param, KEY_FULL_RANGE, 1);
    PARAM_STRING_APPEND_TO(enc_param, KEY_ROTATION, 0);
  } else {
    PARAM_STRING_APPEND_TO(enc_param, KEY_JPEG_QFACTOR, 50);
  }

  flow_param = easymedia::JoinFlowParam(flow_param, 1, enc_param);
  printf("\n#VideoEncoder flow param:\n%s\n", flow_param.c_str());
  video_encoder_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_encoder_flow) {
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

  easymedia::video_encoder_enable_statistics(video_encoder_flow, 1);

  video_encoder_flow->AddDownFlow(video_save_flow, 0, 0);
  video_read_flow->AddDownFlow(video_encoder_flow, 0, 0);
  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

  /*****************************************************
   * JPEG only supports quant configuration.
   *****************************************************/
  if (video_enc_type == IMAGE_JPEG) {
    easymedia::msleep(3000);
    for (int i = 0; i < 10; i++) {
      easymedia::jpeg_encoder_set_qfactor(video_encoder_flow, i * 10 + 9);
      RKMEDIA_LOGI("Change jpeg quant to %d, keep 3s....\n", i);
      easymedia::msleep(3000);
    }
  } else {
    /*****************************************************
     * AVC/HEVC: RC test.
     *****************************************************/
    RKMEDIA_LOGI("#Encoder in CBR MODE for 5s....\n");
    easymedia::msleep(5000);

    RKMEDIA_LOGI("#Encoder with new qp for 5s....\n");
    VideoEncoderQp qps;
    memset(&qps, 0, sizeof(qps));
    qps.qp_init = 30;
    qps.qp_max = 51;
    qps.qp_min = 6;
    qps.qp_step = 6;
    qps.qp_max_i = 48;
    qps.qp_min_i = 10;
    easymedia::video_encoder_set_qp(video_encoder_flow, qps);
    easymedia::msleep(5000);

    RKMEDIA_LOGI("#Encoder in VBR MODE for 5s....\n");
    easymedia::video_encoder_set_rc_mode(video_encoder_flow, KEY_VBR);
    easymedia::msleep(5000);

    const char *rc_level[7] = {KEY_HIGHEST, KEY_HIGHER, KEY_HIGH,  KEY_MEDIUM,
                               KEY_LOW,     KEY_LOWER,  KEY_LOWEST};

    for (int i = 0; !quit && (i < 30); i++) {
      srand((unsigned)time(NULL));
      int level_id = rand() % 7;
      RKMEDIA_LOGI("#Encoder in [%s] Quality for 20s....\n",
                   rc_level[level_id]);
      easymedia::video_encoder_set_rc_quality(video_encoder_flow,
                                              rc_level[level_id]);
      easymedia::msleep(20000);
    }

    RKMEDIA_LOGI("#Encoder start bps change test....\n");
    bpsmax = video_width * video_height * video_fps / 4;
    int bpsmin = video_width * video_height * video_fps / 60;
    int bpsstep = (bpsmax - bpsmin / 4);
    for (int i = 0; !quit && (i < 4); i++) {
      RKMEDIA_LOGI("[%d] bps:%d keep 5s...\n", i, bpsmin + i * bpsstep);
      easymedia::video_encoder_set_bps(video_encoder_flow,
                                       bpsmin + i * bpsstep);
      easymedia::msleep(5000);
    }
  }

  video_read_flow->RemoveDownFlow(video_encoder_flow);
  video_read_flow.reset();
  video_encoder_flow->RemoveDownFlow(video_save_flow);
  video_encoder_flow.reset();
  video_save_flow.reset();
  RKMEDIA_LOGI("%s deinitial finish\n", argv[0]);

  return 0;
}
