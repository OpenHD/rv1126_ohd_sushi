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
#include "filter.h"
#include "flow.h"
#include "image.h"
#include "key_string.h"
#include "media_config.h"
#include "media_type.h"
#include "message.h"
#include "stream.h"
#include "utils.h"

namespace easymedia {

class SoftProcess : public Filter {
public:
  SoftProcess(const char *param);
  virtual ~SoftProcess() = default;
  static const char *GetFilterName() { return "softprocess"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;

private:
};

SoftProcess::SoftProcess(const char *param) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
}

int SoftProcess::Process(std::shared_ptr<MediaBuffer> input,
                         std::shared_ptr<MediaBuffer> &output) {
  int w = 400;
  int h = 400;
  ImageRect screen_rect;
  srand((unsigned)time(0));
  screen_rect = {0, 0, w, h};
  auto buffer = std::static_pointer_cast<easymedia::ImageBuffer>(input);
  for (int j = 0; j < h; j++)
    memset(((unsigned int *)buffer->GetPtr() + j * buffer->GetWidth()),
           rand() % 255, w);
  output = input;
  return 0;
}

DEFINE_COMMON_FILTER_FACTORY(SoftProcess)
const char *FACTORY(SoftProcess)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(SoftProcess)::OutPutDataType() { return TYPE_ANYTHING; }
} // namespace easymedia

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static char optstr[] = "?:i:o:w:h:f:t:m:a";

static void print_usage(char *name) {
  printf("usage example for normal mode: \n");
  printf("%s -i /dev/video0 -o /data/photo0/main -w 1920 -h 1080 "
         "-f nv12 -t jpeg\n",
         name);
  printf("%s -i /dev/video1 -o /data/photo1/sub -w 1920 -h 1080 "
         "-f nv12 -t jpeg\n",
         name);
  printf("#[-f] pix formate support list:\n\tyuyv422\n\tnv12\n");
  printf("#[-a] do aysnc test mode\n");
}

int main(int argc, char **argv) {
  int c, local_file_flag;
  int video_width = 1920;
  int video_height = 1080;
  int vir_width, vir_height;
  std::string pixel_format;
  int video_fps = 30;
  std::string video_enc_type = VIDEO_H264;
  int test_mode = 0; // 0 for normal,1 for stressTest.
  int test_async_mode = 0;

  std::string output_path;
  std::string input_path;

  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::string enc_param;
  std::string filter_param;

  std::shared_ptr<easymedia::Flow> video_read_flow;
  std::shared_ptr<easymedia::Flow> through_guard_flow;
  std::shared_ptr<easymedia::Flow> video_encoder_flow;
  std::shared_ptr<easymedia::Flow> video_save_flow;
  std::shared_ptr<easymedia::Flow> soft_process_flow;
  std::shared_ptr<easymedia::Flow> soft_save_flow;

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
    case 'm':
      if (strstr(optarg, "stressTest")) {
        test_mode = 1;
        printf("======================================\n");
        printf("# Stress Test\n");
        printf("======================================\n");
      }
      break;
    case 'a':
      test_async_mode = 1;
      printf("#IN ARGS: test_async_mode: %d\n", test_async_mode);
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

  flow_name = "filter";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "through_guard");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, video_height);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH, vir_width);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, vir_height);
  if (test_async_mode)
    PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, 30);
  filter_param = "";
  PARAM_STRING_APPEND_TO(filter_param, KEY_ALLOW_THROUGH_COUNT, 1);
  flow_param = easymedia::JoinFlowParam(flow_param, 1, filter_param);
  printf("\n#Rkvirtual Filter flow param:\n%s\n", flow_param.c_str());
  through_guard_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!through_guard_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
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
  PARAM_STRING_APPEND(flow_param, KEY_FILE_PREFIX, output_path.c_str());
  PARAM_STRING_APPEND(flow_param, KEY_FILE_SUFFIX, ".jpeg");
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "w+"); // read and close-on-exec
  PARAM_STRING_APPEND(flow_param, KEY_SAVE_MODE, "single_frame");
  // PARAM_STRING_APPEND(flow_param, KEY_SAVE_MODE, "continuous_frame");
  printf("\n#FileWrite:\n%s\n", flow_param.c_str());
  video_save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_save_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  if (test_async_mode) {
    flow_name = "filter";
    flow_param = "";
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "softprocess");
    PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
    PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, pixel_format);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH, video_width);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, video_height);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH, vir_width);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, vir_height);
    PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, 30);
    filter_param = "";
    PARAM_STRING_APPEND_TO(filter_param, KEY_ALLOW_THROUGH_COUNT, 1);
    flow_param = easymedia::JoinFlowParam(flow_param, 1, filter_param);
    printf("\n#Rkvirtual Filter flow param:\n%s\n", flow_param.c_str());
    soft_process_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
        flow_name.c_str(), flow_param.c_str());
    if (!soft_process_flow) {
      fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
      exit(EXIT_FAILURE);
    }

    flow_name = "file_write_flow";
    flow_param = "";
    PARAM_STRING_APPEND(flow_param, KEY_FILE_PREFIX, output_path.c_str());
    PARAM_STRING_APPEND(flow_param, KEY_FILE_SUFFIX, ".yuv");
    PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                        "w+"); // read and close-on-exec
    PARAM_STRING_APPEND(flow_param, KEY_SAVE_MODE, "single_frame");
    printf("\n#FileWrite:\n%s\n", flow_param.c_str());
    soft_save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
        flow_name.c_str(), flow_param.c_str());
    if (!soft_save_flow) {
      fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
      exit(EXIT_FAILURE);
    }
  }

  video_encoder_flow->AddDownFlow(video_save_flow, 0, 0);
  through_guard_flow->AddDownFlow(video_encoder_flow, 0, 0);
  video_read_flow->AddDownFlow(through_guard_flow, 0, 0);
  if (test_async_mode) {
    soft_process_flow->AddDownFlow(soft_save_flow, 0, 0);
    video_read_flow->AddDownFlow(soft_process_flow, 0, 0);
  }

  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

  while (!test_mode && !quit) {
    easymedia::msleep(100);
    printf("Please enter the number of times you want to take a snap [0-9]\n");
    char count_c = getchar();
    if (count_c > '0' && count_c <= '9') {
      int count = count_c - '0';
      through_guard_flow->Control(easymedia::S_ALLOW_THROUGH_COUNT, &count);
    }
  }

  if (test_mode) {
    srand((unsigned)time(NULL));
    int delay_ms = rand() % 100000;
    printf("=> Stress Test: sleep %dms\n", delay_ms);
    easymedia::msleep(delay_ms);
  }

  video_read_flow->RemoveDownFlow(through_guard_flow);
  if (test_async_mode) {
    video_read_flow->RemoveDownFlow(soft_process_flow);
  }
  video_read_flow.reset();
  through_guard_flow->RemoveDownFlow(video_encoder_flow);
  through_guard_flow.reset();
  if (test_async_mode) {
    soft_process_flow->RemoveDownFlow(soft_save_flow);
    soft_process_flow.reset();
    soft_save_flow.reset();
  }
  video_encoder_flow->RemoveDownFlow(video_save_flow);
  video_encoder_flow.reset();
  video_save_flow.reset();
  RKMEDIA_LOGI("%s deinitial finish\n", argv[0]);

  if (test_mode && !quit) {
    printf("=> Stress Test: resatr\n");
    goto RESTART;
  }

  return 0;
}
