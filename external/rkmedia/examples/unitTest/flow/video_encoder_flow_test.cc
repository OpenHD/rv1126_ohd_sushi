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
static FILE *save_file;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

void encoder_output_cb(void *handle,
                       std::shared_ptr<easymedia::MediaBuffer> mb) {
  printf("==Encoder Output CallBack recived: %p(%s), %p, %d, %dBytes\n", handle,
         (char *)handle, mb->GetPtr(), mb->GetFD(), mb->GetValidSize());

  if (save_file)
    fwrite(mb->GetPtr(), 1, mb->GetValidSize(), save_file);
}

static char optstr[] = "?:i:o:w:h:f:t:m:s:u:c:r:";

static void print_usage(char *name) {
  printf("usage example for normal mode: \n");
  printf("%s -i /dev/video0 -o output.h264 -w 1920 -h 1080 "
         "-f nv12 -t h264\n",
         name);
  printf("#[-t] enc type support list:\n\th264\n\th265\n\tjpeg\n\tmjpeg\n");
  printf(
      "#[-f] pix formate support list:\n\tyuyv422\n\tnv12\n\tfbc0\n\tfbc2\n");
  printf("#[-m] mode support list:\n\tnormal\n\tstressTest\n");
  printf("#[-s] Slice split mode:\n");
  printf("\t0: No slice is split\n");
  printf("\t1: Slice is split by byte number\n");
  printf("\t2: Slice is split by macroblock / ctu number\n");
  printf("#[-u] Enable userdata:\n");
  printf("\t0:disable\t1:enable\n");
  printf("#[-c] Enable Output Callback:\n");
  printf("\t0:disable\t1:enable\n");
  printf("#[-r] support rotation: 0, 90, 180, 270\n");
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
  int split_mode = 0;
  int userdata_enable = 0;
  int output_cb_enable = 0;
  char cb_str[32] = "<Output Callback Test>";
  int rotation = 0;

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
    case 'm':
      if (strstr(optarg, "stressTest")) {
        test_mode = 1;
        printf("======================================\n");
        printf("# Stress Test\n");
        printf("======================================\n");
      }
      break;
    case 's':
      if (video_enc_type == "jpeg") {
        printf("ERROR: JPEG not support split mode!\n");
        exit(0);
      }
      split_mode = atoi(optarg);
      printf("#IN ARGS: split mode: %d\n", split_mode);
      break;
    case 'u':
      userdata_enable = atoi(optarg);
      printf("#IN ARGS: userdata_enable: %d\n", userdata_enable);
      break;
    case 'c':
      output_cb_enable = atoi(optarg);
      printf("#IN ARGS: output_cb_enable: %d\n", output_cb_enable);
      break;
    case 'r':
      rotation = atoi(optarg);
      printf("#IN ARGS: rotation: %d\n", rotation);
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
  if ((pixel_format == "yuyv422") || (pixel_format == "nv12") ||
      (pixel_format == "fbc0") || (pixel_format == "fbc2"))
    pixel_format = "image:" + pixel_format;
  else {
    printf("ERROR: image type:%s not support!\n", pixel_format.c_str());
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // add prefix for encoder type
  if ((video_enc_type == "h264") || (video_enc_type == "h265") ||
      (video_enc_type == "mjpeg"))
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
  if ((video_enc_type == VIDEO_H264) || (video_enc_type == VIDEO_H265)) {
    int bps = video_width * video_height * video_fps / 14;
    PARAM_STRING_APPEND_TO(enc_param, KEY_COMPRESS_BITRATE_MAX, bps);
    PARAM_STRING_APPEND(enc_param, KEY_FPS, "30/1");
    PARAM_STRING_APPEND(enc_param, KEY_FPS_IN, "30/1");
    PARAM_STRING_APPEND_TO(enc_param, KEY_FULL_RANGE, 1);
    PARAM_STRING_APPEND_TO(enc_param, KEY_ROTATION, rotation);
    PARAM_STRING_APPEND(enc_param, KEY_COMPRESS_RC_MODE, KEY_CBR);
  } else if (video_enc_type == VIDEO_MJPEG) {
    int bps = video_width * video_height * 12;
    PARAM_STRING_APPEND_TO(enc_param, KEY_COMPRESS_BITRATE_MAX, bps);
    PARAM_STRING_APPEND(enc_param, KEY_FPS, "30/1");
    PARAM_STRING_APPEND(enc_param, KEY_FPS_IN, "30/1");
    PARAM_STRING_APPEND(enc_param, KEY_COMPRESS_RC_MODE, KEY_CBR);
  } else if (video_enc_type == IMAGE_JPEG) {
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

  if (!output_cb_enable) {
    flow_name = "file_write_flow";
    flow_param = "";
    if (video_enc_type == IMAGE_JPEG) {
      int pos = output_path.find_last_of(".");
      if (pos != -1)
        output_path = output_path.substr(0, pos);
      PARAM_STRING_APPEND(flow_param, KEY_SAVE_MODE, KEY_SAVE_MODE_SINGLE);
      PARAM_STRING_APPEND(flow_param, KEY_FILE_PREFIX, output_path);
      PARAM_STRING_APPEND(flow_param, KEY_FILE_SUFFIX, ".jpg");
    } else {
      PARAM_STRING_APPEND(flow_param, KEY_PATH, output_path.c_str());
    }
    PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                        "w+"); // read and close-on-exec
    printf("\n#FileWrite:\n%s\n", flow_param.c_str());
    video_save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
        flow_name.c_str(), flow_param.c_str());
    if (!video_save_flow) {
      fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
      exit(EXIT_FAILURE);
    }
  } else {
    save_file = fopen(output_path.c_str(), "w");
    if (!save_file)
      RKMEDIA_LOGE("open %s failed!\n", output_path.c_str());
  }

  if (split_mode == 1) {
    int split_size = video_width * video_height / 2;
    RKMEDIA_LOGI("Split frame to slice with size = %d...\n", split_size);
    video_encoder_set_split(video_encoder_flow, split_mode, split_size);
  } else if (split_mode == 2) {
    int split_mb_cnt;
    if (video_enc_type == VIDEO_H264) {
      split_mb_cnt = (video_width / 16) * ((video_height / 16) / 2);
      easymedia::video_encoder_set_split(video_encoder_flow, split_mode,
                                         split_mb_cnt);
    } else {
      split_mb_cnt = (video_width / 64) * ((video_height / 64) / 2);
      easymedia::video_encoder_set_split(video_encoder_flow, split_mode,
                                         split_mb_cnt);
    }
    RKMEDIA_LOGI("Split frame to 2 slice with MB cnt = %d...\n", split_mb_cnt);
  }

  if (userdata_enable) {
    char sei_str[] = "RockChip AVC/HEVC Codec";
    RKMEDIA_LOGI("Set userdata string:%s\n", sei_str);
    easymedia::video_encoder_set_userdata(video_encoder_flow, sei_str, 23);
  }

  // Demo code: Show how to feed data to the coded Flow.
  // YUV_IMG_SIZE is not defined
  /*
  std::shared_ptr<easymedia::MediaBuffer> mb =
    easymedia::MediaBuffer::Alloc(YUV_IMG_SIZE);
  memset(mb->GetPtr(), 0, YUV_IMG_SIZE);
  mb->SetValidSize(YUV_IMG_SIZE);
  video_encoder_flow->SendInput(mb, 0);
  */

  easymedia::video_encoder_enable_statistics(video_encoder_flow, 1);

  if (output_cb_enable) {
    RKMEDIA_LOGI("Regest callback handle:%p\n", cb_str);
    video_encoder_flow->SetOutputCallBack((void *)cb_str, encoder_output_cb);
  } else {
    video_encoder_flow->AddDownFlow(video_save_flow, 0, 0);
  }
  video_read_flow->AddDownFlow(video_encoder_flow, 0, 0);

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

  video_read_flow->RemoveDownFlow(video_encoder_flow);
  video_read_flow.reset();
  if (!output_cb_enable) {
    video_encoder_flow->RemoveDownFlow(video_save_flow);
    video_save_flow.reset();
  } else if (save_file) {
    fclose(save_file);
  }
  video_encoder_flow.reset();
  RKMEDIA_LOGI("%s deinitial finish\n", argv[0]);

  if (test_mode && !quit) {
    printf("=> Stress Test: resatr\n");
    goto RESTART;
  }

  return 0;
}
