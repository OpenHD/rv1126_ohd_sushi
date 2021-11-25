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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "flow.h"
#include "key_string.h"
#include "media_config.h"
#include "media_type.h"
#include "utils.h"

static bool quit = false;

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

#define SIMPLE 1

#if SIMPLE

#include "buffer.h"
#include "codec.h"

#define MAX_FILE_NUM 10
static char optstr[] = "?d:p:c:u:t:";

enum { STREAM_TYPE_NONE, STREAM_TYPE_H264, STREAM_TYPE_H265 };

int main(int argc, char **argv) {
  int c, stream_type, stream_idr_type;
  std::string input_dir_path;
  int port = 554;                  // rtsp port
  std::string channel_name;        // rtsp channel name
  std::string user_name, user_pwd; // rtsp user name and password

  opterr = 1;
  stream_type = STREAM_TYPE_NONE;
  stream_idr_type = 0;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 't': {
      if (strstr(optarg, "h264")) {
        stream_type = STREAM_TYPE_H264;
        stream_idr_type = 5;
      } else if (strstr(optarg, "h265")) {
        stream_type = STREAM_TYPE_H265;
        stream_idr_type = 19;
      } else {
        fprintf(stderr, "stream type only support \"h264\" or \"h265\"\n");
        exit(EXIT_FAILURE);
      }
    } break;
    case 'd':
      input_dir_path = optarg;
      printf("input directory path: %s\n", input_dir_path.c_str());
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'c':
      channel_name = optarg;
      break;
    case 'u': {
      char *sep = strchr(optarg, ':');
      if (!sep) {
        fprintf(stderr, "user name and password must be seperated by :\n");
        exit(EXIT_FAILURE);
      }
      *sep = 0;
      user_name = optarg;
      user_pwd = sep + 1;
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rtsp_server_test -t h264 -d h264_frames_dir -p 554 -c "
             "main_stream -u "
             "admin:123456\n");
      printf("rtsp_server_test -t h265 -d h265_frames_dir -p 554 -c "
             "main_stream -u "
             "admin:123456\n\n");
      printf("On PC: ffplay -rtsp_transport tcp -stimeout 2000000 "
             "rtsp://admin:123456@192.168.xxx.xxx/main_stream\n");
      printf("Streame 2: ffplay -rtsp_transport tcp -stimeout 2000000 "
             "rtsp://admin:123456@192.168.xxx.xxx/main_stream_1\n");
      break;
    }
  }
  if ((stream_type == STREAM_TYPE_NONE) || input_dir_path.empty() ||
      channel_name.empty())
    assert(0);
  std::list<std::shared_ptr<easymedia::MediaBuffer>> spspps;
  std::vector<std::shared_ptr<easymedia::MediaBuffer>> buffer_list;
  buffer_list.resize(MAX_FILE_NUM);
  // 1. read all file to buffer list
  int file_index = 0;
  while (file_index <= MAX_FILE_NUM) {
    std::string input_file_path;
    if (stream_type == STREAM_TYPE_H264)
      input_file_path.append(input_dir_path)
          .append("/")
          .append(std::to_string(file_index))
          .append(".h264_frame");
    else
      input_file_path.append(input_dir_path)
          .append("/")
          .append(std::to_string(file_index))
          .append(".h265_frame");
    printf("file : %s\n", input_file_path.c_str());
    FILE *f = fopen(input_file_path.c_str(), "re");
    if (!f) {
      fprintf(stderr, "open %s failed, %m\n", input_file_path.c_str());
      exit(EXIT_FAILURE);
    }
    int len = fseek(f, 0, SEEK_END);
    if (len) {
      fprintf(stderr, "fseek %s to end failed, %m\n", input_file_path.c_str());
      exit(EXIT_FAILURE);
    }
    len = ftell(f);
    assert(len > 0);
    rewind(f);
    auto buffer = easymedia::MediaBuffer::Alloc(len);
    assert(buffer);
    ssize_t read_size = fread(buffer->GetPtr(), 1, len, f);
    assert(read_size == len);
    fclose(f);
    /* 0.h264_frame must be sps pps;
     * 0.h265_frame must be vps sps pps;
     * As live only accept one slice one time, separate vps/sps/pps
     */
    if (file_index == 0) {
      if (stream_type == STREAM_TYPE_H264)
        spspps = easymedia::split_h264_separate(
            (const uint8_t *)buffer->GetPtr(), len, easymedia::gettimeofday());
      else
        spspps = easymedia::split_h265_separate(
            (const uint8_t *)buffer->GetPtr(), len, easymedia::gettimeofday());
      file_index++;
      continue;
    }
    uint8_t *p = (uint8_t *)buffer->GetPtr();
    uint8_t nal_type = 0;
    assert(p[0] == 0 && p[1] == 0);
    if (p[2] == 0 && p[3] == 1) { // 0x00000001
      nal_type = p[4];
    } else if (p[2] == 1) { // 0x000001
      nal_type = p[3];
    } else {
      assert(0 && "not h264 or h265 data");
    }

    if (stream_type == STREAM_TYPE_H264)
      nal_type = nal_type & 0x1f;
    else
      nal_type = (nal_type & 0x7E) >> 1;

    if (nal_type == stream_idr_type)
      buffer->SetUserFlag(easymedia::MediaBuffer::kIntra);
    buffer->SetValidSize(len);
    // buffer->SetUSTimeStamp(easymedia::gettimeofday()); // us
    buffer_list[file_index - 1] = buffer;
    file_index++;
  }
  // 2. create rtsp server
  std::string flow_name = "live555_rtsp_server";
  std::string param;
  // VIDEO_H264 "," AUDIO_PCM
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE,
                      (stream_type == STREAM_TYPE_H264) ? VIDEO_H264
                                                        : VIDEO_H265);
  PARAM_STRING_APPEND(param, KEY_CHANNEL_NAME, channel_name);
  PARAM_STRING_APPEND_TO(param, KEY_PORT_NUM, port);
  if (!user_name.empty() && !user_pwd.empty()) {
    PARAM_STRING_APPEND(param, KEY_USERNAME, user_name);
    PARAM_STRING_APPEND(param, KEY_USERPASSWORD, user_pwd);
  }
  printf("\nparam :\n%s\n", param.c_str());
  auto rtsp_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!rtsp_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  RKMEDIA_LOGI("rtsp server test initial finish\n");
  signal(SIGINT, sigterm_handler);
  // 3. create rtsp server
  param = "";
  std::string channel_name_1 = channel_name + "_1";
  // VIDEO_H264 "," AUDIO_PCM
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE,
                      (stream_type == STREAM_TYPE_H264) ? VIDEO_H264
                                                        : VIDEO_H265);
  PARAM_STRING_APPEND(param, KEY_CHANNEL_NAME, channel_name_1);
  PARAM_STRING_APPEND_TO(param, KEY_PORT_NUM, port);
  if (!user_name.empty() && !user_pwd.empty()) {
    PARAM_STRING_APPEND(param, KEY_USERNAME, user_name);
    PARAM_STRING_APPEND(param, KEY_USERPASSWORD, user_pwd);
  }
  printf("\nparam :\n%s\n", param.c_str());
  auto rtsp_flow_1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!rtsp_flow_1) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  // 4. send data to live555
  int loop = 99999;
  int buffer_idx = 0;
  while (loop > 0 && !quit) {
    if (buffer_idx == 0) {
      // send sps and pps
      for (auto &buf : spspps) {
        buf->SetUSTimeStamp(easymedia::gettimeofday());
        rtsp_flow->SendInput(buf, 0);
        rtsp_flow_1->SendInput(buf, 0);
      }
      buffer_idx++;
    }
    auto &buf = buffer_list[buffer_idx - 1];
    if (!buf) {
      buffer_idx = 0;
      continue;
    }
    buf->SetUSTimeStamp(easymedia::gettimeofday());
    rtsp_flow->SendInput(buf, 0);
    rtsp_flow_1->SendInput(buf, 0);
    buffer_idx++;
    loop++;
    if (buffer_idx >= MAX_FILE_NUM)
      buffer_idx = 0;
    easymedia::msleep(33);
  }
  RKMEDIA_LOGI("rtsp server test reclaiming\n");
  rtsp_flow.reset();
  return 0;
}

#else
static char optstr[] = "?i:w:h:p:c:u:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string input_format;
  int w = 0, h = 0, port = 554;
  std::string channel_name;
  std::string user_name, user_pwd;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input file path: %s\n", input_path.c_str());
      if (!easymedia::string_end_withs(input_path, "nv12")) {
        fprintf(stderr, "input file must be *.nv12");
        exit(EXIT_FAILURE);
      }
      input_format = IMAGE_NV12;
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'c':
      channel_name = optarg;
      break;
    case 'u': {
      char *sep = strchr(optarg, ':');
      if (!sep) {
        fprintf(stderr, "user name and password must be seperated by :\n");
        exit(EXIT_FAILURE);
      }
      *sep = 0;
      user_name = optarg;
      user_pwd = sep + 1;
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rtsp_server_test -i input.nv12 -w 320 -h 240 -p 554 -c "
             "main_stream -u admin:123456\n");
      break;
    }
  }
  if (input_path.empty() || channel_name.empty())
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  easymedia::REFLECTOR(Flow)::DumpFactories();
  int w_align = UPALIGNTO16(w);
  int h_align = UPALIGNTO16(h);
  int fps = 30;
  ImageInfo info;
  info.pix_fmt = PIX_FMT_NV12;
  info.width = w;
  info.height = h;
  info.vir_width = w_align;
  info.vir_height = h_align;
  std::string flow_name;
  std::string param;
  PARAM_STRING_APPEND(param, KEY_PATH, input_path);
  PARAM_STRING_APPEND(param, KEY_OPEN_MODE, "re"); // read and close-on-exec
  PARAM_STRING_APPEND(param, KEY_MEM_TYPE,
                      KEY_MEM_HARDWARE); // we will assign rkmpp as encoder,
                                         // which need hardware buffer
  param += easymedia::to_param_string(info);
  PARAM_STRING_APPEND_TO(param, KEY_FPS, fps); // rhythm reading
  PARAM_STRING_APPEND_TO(param, KEY_LOOP_TIME, 99999);
  printf("\nparam 1:\n%s\n", param.c_str());
  // 1. source
  flow_name = "file_read_flow";
  auto file_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!file_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  // 2. encoder
  flow_name = "video_enc";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE, IMAGE_NV12);
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, VIDEO_H264);
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
  vid_cfg.rc_quality = KEY_BEST;
  vid_cfg.rc_mode = KEY_CBR;
  std::string enc_param;
  enc_param.append(easymedia::to_param_string(enc_config, VIDEO_H264));
  param = easymedia::JoinFlowParam(param, 1, enc_param);
  printf("\nparam 2:\n%s\n", param.c_str());
  auto enc_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!enc_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  // 3. rtsp server
  flow_name = "live555_rtsp_server";
  param = "";
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE,
                      VIDEO_H264); // VIDEO_H264 "," AUDIO_PCM
  PARAM_STRING_APPEND(param, KEY_CHANNEL_NAME, channel_name);
  PARAM_STRING_APPEND_TO(param, KEY_PORT_NUM, port);
  if (!user_name.empty() && !user_pwd.empty()) {
    PARAM_STRING_APPEND(param, KEY_USERNAME, user_name);
    PARAM_STRING_APPEND(param, KEY_USERPASSWORD, user_pwd);
  }
  printf("\nparam 3:\n%s\n", param.c_str());
  auto rtsp_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!rtsp_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  // 4. link above flows
  enc_flow->AddDownFlow(rtsp_flow, 0, 0);
  file_flow->AddDownFlow(
      enc_flow, 0, 0); // the source flow better place the end to add down flow
  RKMEDIA_LOGI("rtsp server test initial finish\n");
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    easymedia::msleep(10);
  }
  RKMEDIA_LOGI("rtsp server test reclaiming\n");
  file_flow->RemoveDownFlow(enc_flow);
  file_flow.reset();
  enc_flow->RemoveDownFlow(rtsp_flow);
  enc_flow.reset();
  rtsp_flow.reset();
  return 0;
}
#endif
