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

#define ROI_REGION_CNT 4

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static char optstr[] = "?:i:o:w:h:f:t:";

static void print_usage(char *name) {
  printf("usage example: \n");
  printf("%s -i /dev/video0 -o output.h264 -w 1920 -h 1080 "
         "-f nv12 -t h264\n",
         name);
  printf("#[-t] support list:\n\th264\n\th265\n");
  printf("#[-f] support list:\n\tyuyv422\n\tnv12\n");
}

int main(int argc, char **argv) {
  int c, local_file_flag;
  int video_width = 1920;
  int video_height = 1080;
  int vir_width, vir_height;
  std::string pixel_format;
  int video_fps = 30;
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

  if (strstr(input_path.c_str(), "/dev/video")) {
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

  VideoEncoderCfg vcfg;
  memset(&vcfg, 0, sizeof(vcfg));
  vcfg.type = (char *)video_enc_type.c_str();
  vcfg.fps = video_fps;
  vcfg.max_bps = video_width * video_height * video_fps / 14;
  ImageInfo image_info;
  image_info.pix_fmt = StringToPixFmt(pixel_format.c_str());
  image_info.width = video_width;
  image_info.height = video_height;
  image_info.vir_width = vir_width;
  image_info.vir_height = vir_height;
  enc_param = easymedia::get_video_encoder_config_string(image_info, vcfg);
  PARAM_STRING_APPEND(enc_param, KEY_ROI_REGIONS,
                      "(0,0,256,256,0,18,0,1,1)(256,256,512,512,0,18,0,1,1)("
                      "512,512,768,768,0,18,0,1,1)");
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

#if 0
  int region_width = UPALIGNTO16(video_width / ROI_REGION_CNT);
  int region_height = UPALIGNTO16(video_height / ROI_REGION_CNT);
  if ((region_width * ROI_REGION_CNT) > video_width)
    region_width -= 16;
  if ((region_height * ROI_REGION_CNT) > video_height)
    region_height -= 16;
  printf("Automic generate roi regions[%d]:\n", ROI_REGION_CNT);
  EncROIRegion region_data[ROI_REGION_CNT];
  memset(region_data, 0, sizeof(region_data));
  for (int i = 0; i < ROI_REGION_CNT; i++) {
    region_data[i].x = i * region_width;
    region_data[i].y = i * region_height;
    region_data[i].w = region_width;
    region_data[i].h = region_height;
    region_data[i].abs_qp_en = (i % 2)?1:0;
    region_data[i].intra = (i % 2)?1:0;
    region_data[i].quality = 20 + i * 5;
    printf("#%02d x:%d, y:%d, w:%d, h:%d, abs_qp_en:%d, intra:%d, quality:%d\n",
      i, region_data[i].x, region_data[i].y, region_data[i].w, region_data[i].h,
      region_data[i].abs_qp_en, region_data[i].intra, region_data[i].quality);
  }
  easymedia::video_encoder_set_roi_regions(video_encoder_flow, region_data, ROI_REGION_CNT);
#endif

  video_encoder_flow->AddDownFlow(video_save_flow, 0, 0);
  video_read_flow->AddDownFlow(video_encoder_flow, 0, 0);

  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

  sleep(10);
  printf("clear encoder roi function...\n");
  video_encoder_set_roi_regions(video_encoder_flow, NULL, 0);

  while (!quit) {
    easymedia::msleep(100);
  }

  video_read_flow->RemoveDownFlow(video_encoder_flow);
  video_read_flow.reset();
  video_encoder_flow->RemoveDownFlow(video_save_flow);
  video_encoder_flow.reset();
  video_save_flow.reset();

  return 0;
}
