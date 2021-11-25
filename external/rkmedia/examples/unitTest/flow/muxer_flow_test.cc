// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "assert.h"
#include "signal.h"
#include "stdint.h"
#include "stdio.h"
#include "unistd.h"

#include <iostream>
#include <memory>
#include <string>

#include "easymedia/buffer.h"
#include "easymedia/control.h"
#include "easymedia/flow.h"
#include "easymedia/image.h"
#include "easymedia/key_string.h"
#include "easymedia/media_config.h"
#include "easymedia/media_type.h"
#include "easymedia/reflector.h"
#include "easymedia/stream.h"
#include "easymedia/utils.h"

std::string CodecToString(CodecType type) {
  switch (type) {
  case CODEC_TYPE_MP3:
    return AUDIO_MP3;
  case CODEC_TYPE_MP2:
    return AUDIO_MP2;
  case CODEC_TYPE_VORBIS:
    return AUDIO_VORBIS;
  case CODEC_TYPE_G711A:
    return AUDIO_G711A;
  case CODEC_TYPE_G711U:
    return AUDIO_G711U;
  case CODEC_TYPE_G726:
    return AUDIO_G726;
  case CODEC_TYPE_H264:
    return VIDEO_H264;
  case CODEC_TYPE_H265:
    return VIDEO_H265;
  default:
    return "";
  }
}
std::shared_ptr<easymedia::Flow> create_flow(const std::string &flow_name,
                                             const std::string &flow_param,
                                             const std::string &elem_param);

static bool quit = false;

static void sigterm_handler(int sig) {
  RKMEDIA_LOGI("signal %d\n", sig);
  quit = true;
}

std::shared_ptr<easymedia::Flow> create_alsa_flow(std::string aud_in_path,
                                                  SampleInfo &info) {
  std::string flow_name;
  std::string flow_param;
  std::string sub_param;
  std::string stream_name;

  flow_name = "source_stream";
  flow_param = "";
  sub_param = "";
  stream_name = "alsa_capture_stream";

  PARAM_STRING_APPEND(flow_param, KEY_NAME, stream_name);
  PARAM_STRING_APPEND(sub_param, KEY_DEVICE, aud_in_path);
  PARAM_STRING_APPEND(sub_param, KEY_SAMPLE_FMT, SampleFmtToString(info.fmt));
  PARAM_STRING_APPEND_TO(sub_param, KEY_CHANNELS, info.channels);
  PARAM_STRING_APPEND_TO(sub_param, KEY_FRAMES, info.nb_samples);
  PARAM_STRING_APPEND_TO(sub_param, KEY_SAMPLE_RATE, info.sample_rate);

  auto audio_source_flow = create_flow(flow_name, flow_param, sub_param);
  if (!audio_source_flow) {
    printf("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  } else {
    printf("%s flow ready!\n", flow_name.c_str());
  }
  return audio_source_flow;
}

std::string get_audio_enc_param(SampleInfo &info, CodecType codec_type,
                                int bit_rate, float quality) {
  std::string audio_enc_param;
  MediaConfig audio_enc_config;
  auto &ac = audio_enc_config.aud_cfg;
  ac.sample_info = info;
  ac.bit_rate = bit_rate;
  ac.quality = quality;
  audio_enc_config.type = Type::Audio;

  audio_enc_config.aud_cfg.codec_type = codec_type;
  audio_enc_param.append(
      easymedia::to_param_string(audio_enc_config, CodecToString(codec_type)));
  return audio_enc_param;
}

std::shared_ptr<easymedia::Flow>
create_audio_enc_flow(SampleInfo &info, CodecType codec_type,
                      std::string audio_enc_param) {
  std::string flow_name;
  std::string flow_param;

  flow_name = "audio_enc";
  flow_param = "";
  if (codec_type == CODEC_TYPE_VORBIS)
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "libvorbisenc");
  else
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkaudio_aud");

  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE,
                      CodecToString(codec_type));
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE,
                      SampleFmtToString(info.fmt));

  flow_param = easymedia::JoinFlowParam(flow_param, 1, audio_enc_param);
  auto audio_enc_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!audio_enc_flow) {
    RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
  } else {
    RKMEDIA_LOGI("%s flow ready!\n", flow_name.c_str());
  }
  return audio_enc_flow;
}

CodecType parseCodec(std::string args) {
  if (!args.compare("MP3"))
    return CODEC_TYPE_MP3;
  if (!args.compare("MP2"))
    return CODEC_TYPE_MP2;
  if (!args.compare("VORBIS"))
    return CODEC_TYPE_VORBIS;
  if (!args.compare("G711A"))
    return CODEC_TYPE_G711A;
  if (!args.compare("G711U"))
    return CODEC_TYPE_G711U;
  if (!args.compare("G726"))
    return CODEC_TYPE_G726;
  else
    return CODEC_TYPE_NONE;
}

void usage(char *name) {
  RKMEDIA_LOGI(
      "\nUsage: \t%s -a default -w 1280 -h 720 -c MP3 -v /dev/video0 -o "
      "/tmp/out.mp4\n",
      name);
  RKMEDIA_LOGI(
      "\n(customIO): \t%s -a default -w 1280 -h 720 -c MP3 -v /dev/video0 "
      "-t mpeg -o /tmp/out.mpeg\n",
      name);
  RKMEDIA_LOGI("\tNOTICE: audio codec : -c [MP3 MP2 G711A G711U G726]\n");
  exit(EXIT_FAILURE);
}

static char optstr[] = "?a:v:o:c:w:h:t:";

int main(int argc, char **argv) {
  int in_w = 1280;
  int in_h = 720;
  int fps = 30;
  int w_virtual = UPALIGNTO(in_w, 8);
  int h_virtual = UPALIGNTO(in_h, 8);

  PixelFormat enc_in_fmt = PIX_FMT_NV12;

  SampleFormat fmt = SAMPLE_FMT_S16;
  int channels = 1;
  int sample_rate = 8000;
  int nb_samples = 1024;
  CodecType codec = CODEC_TYPE_MP3;
  CodecType video_codec = CODEC_TYPE_H264;
  int bitrate = 32000;
  float quality = 1.0; // 0.0 - 1.0

  std::string vid_in_path = "/dev/video0";
  std::string aud_in_path = "default";
  std::string output_path;
  std::string input_format = IMAGE_NV12;
  std::string flow_name;
  std::string flow_param;
  std::string sub_param;
  std::string video_enc_param;
  std::string audio_enc_param;
  std::string muxer_param;
  std::string stream_name;
  std::string output_type;
  int c;

  easymedia::REFLECTOR(Stream)::DumpFactories();
  easymedia::REFLECTOR(Flow)::DumpFactories();

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'h':
      in_h = atoi(optarg);
      h_virtual = UPALIGNTO(in_h, 8);
      break;
    case 'w':
      in_w = atoi(optarg);
      w_virtual = UPALIGNTO(in_w, 8);
      break;
    case 'a':
      aud_in_path = optarg;
      RKMEDIA_LOGI("audio device path: %s\n", aud_in_path.c_str());
      break;
    case 'v':
      vid_in_path = optarg;
      RKMEDIA_LOGI("video device path: %s\n", vid_in_path.c_str());
      break;
    case 't':
      output_type = optarg;
      RKMEDIA_LOGI("use customIO, output type is %s.\n", output_type.c_str());
      break;
    case 'o':
      output_path = optarg;
      RKMEDIA_LOGI("output file path: %s\n", output_path.c_str());
      break;
    case 'c':
      codec = parseCodec(optarg);
      if (codec == CODEC_TYPE_NONE)
        usage(argv[0]);
      break;
      break;
    case '?':
    default:
      usage(argv[0]);
      break;
    }
  }

  ImageInfo info = {enc_in_fmt, in_w, in_h, w_virtual, h_virtual};

  if (vid_in_path.empty() || (output_path.empty() && output_type.empty()))
    exit(EXIT_FAILURE);

  if (vid_in_path.empty() || aud_in_path.empty()) {
    RKMEDIA_LOGI("use default video device and audio device!\n");
  }

  // param fixed
  if (codec == CODEC_TYPE_MP3) {
    fmt = SAMPLE_FMT_FLTP;
  } else if (codec == CODEC_TYPE_VORBIS) {
    channels = 2;
  } else if (codec == CODEC_TYPE_MP2) {
    sample_rate = 16000;
  }

  flow_name = "source_stream";
  stream_name = "v4l2_capture_stream";

  PARAM_STRING_APPEND(flow_param, KEY_NAME, stream_name);

  PARAM_STRING_APPEND_TO(sub_param, KEY_USE_LIBV4L2, 1);
  PARAM_STRING_APPEND(sub_param, KEY_DEVICE, vid_in_path);
  PARAM_STRING_APPEND(sub_param, KEY_V4L2_CAP_TYPE,
                      KEY_V4L2_C_TYPE(VIDEO_CAPTURE));
  PARAM_STRING_APPEND(sub_param, KEY_V4L2_MEM_TYPE,
                      KEY_V4L2_M_TYPE(MEMORY_DMABUF));
  PARAM_STRING_APPEND_TO(sub_param, KEY_FRAMES, 8);
  PARAM_STRING_APPEND(sub_param, KEY_OUTPUTDATATYPE, input_format);
  PARAM_STRING_APPEND_TO(sub_param, KEY_BUFFER_WIDTH, in_w);
  PARAM_STRING_APPEND_TO(sub_param, KEY_BUFFER_HEIGHT, in_h);
  auto video_source_flow = create_flow(flow_name, flow_param, sub_param);

  if (!video_source_flow) {
    printf("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  } else {
    printf("%s flow ready!\n", flow_name.c_str());
  }

  flow_param = "";
  flow_name = "video_enc";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, input_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE,
                      CodecToString(video_codec));
  PARAM_STRING_APPEND_TO(flow_param, KEY_NEED_EXTRA_MERGE, 1);

  MediaConfig video_enc_config;
  memset(&video_enc_config, 0, sizeof(video_enc_config));
  VideoConfig &vid_cfg = video_enc_config.vid_cfg;
  ImageConfig &img_cfg = vid_cfg.image_cfg;
  img_cfg.image_info = info;
  vid_cfg.qp_init = 24;
  vid_cfg.qp_step = 4;
  vid_cfg.qp_min = 12;
  vid_cfg.qp_max = 48;
  vid_cfg.bit_rate = in_w * in_h * 7;
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

  video_enc_param.append(
      easymedia::to_param_string(video_enc_config, CodecToString(video_codec)));
  auto video_enc_flow = create_flow(flow_name, flow_param, video_enc_param);

  if (!video_enc_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  } else {
    RKMEDIA_LOGI("%s flow ready!\n", flow_name.c_str());
  }

  SampleInfo sample_info = {fmt, channels, sample_rate, nb_samples};
  audio_enc_param = get_audio_enc_param(sample_info, codec, bitrate, quality);
  RKMEDIA_LOGI("Audio pre enc param: %s\n", audio_enc_param.c_str());

  // 1. audio encoder
  std::shared_ptr<easymedia::Flow> audio_enc_flow =
      create_audio_enc_flow(sample_info, codec, audio_enc_param);
  if (!audio_enc_flow) {
    RKMEDIA_LOGI("Create flow failed\n");
    exit(EXIT_FAILURE);
  }

  // 2. Tuning the nb_samples according to the encoder requirements.
  int read_size = audio_enc_flow->GetInputSize();
  if (read_size > 0) {
    sample_info.nb_samples = read_size / GetSampleSize(sample_info);
    RKMEDIA_LOGI("codec %s : nm_samples fixed to %d\n",
                 CodecToString(codec).c_str(), sample_info.nb_samples);
  }
  audio_enc_param = get_audio_enc_param(sample_info, codec, bitrate, quality);
  RKMEDIA_LOGI("Audio post enc param: %s\n", audio_enc_param.c_str());

  // 3. alsa capture flow
  std::shared_ptr<easymedia::Flow> audio_source_flow =
      create_alsa_flow(aud_in_path, sample_info);
  if (!audio_source_flow) {
    RKMEDIA_LOGI("Create flow alsa_capture_flow failed\n");
    exit(EXIT_FAILURE);
  }
  // 4. set alsa capture volume to max
  int volume;
  audio_source_flow->Control(easymedia::G_ALSA_VOLUME, &volume);
  RKMEDIA_LOGI("Get capture volume %d\n", volume);
  volume = 100;
  audio_source_flow->Control(easymedia::S_ALSA_VOLUME, &volume);

  flow_param = "";
  flow_name = "muxer_flow";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "muxer_flow");
  if (!output_type.empty()) {
    PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, output_type);
    RKMEDIA_LOGI("RKAudio use customIO.\n");
  } else if (!output_path.empty()) {
    PARAM_STRING_APPEND_TO(flow_param, KEY_FILE_DURATION, 30);
    PARAM_STRING_APPEND_TO(flow_param, KEY_FILE_TIME, 1);
    PARAM_STRING_APPEND_TO(flow_param, KEY_FILE_INDEX, 18);
    // PARAM_STRING_APPEND(flow_param, KEY_FILE_PREFIX, "/tmp/RKMEDIA");
    PARAM_STRING_APPEND(flow_param, KEY_PATH, output_path);
  }

  video_enc_config.vid_cfg.image_cfg.codec_type = video_codec;
  muxer_param.append(
      easymedia::to_param_string(video_enc_config, CodecToString(video_codec)));

  auto &&param =
      easymedia::JoinFlowParam(flow_param, 2, audio_enc_param, muxer_param);
  auto muxer_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!muxer_flow) {
    exit(EXIT_FAILURE);
  } else {
    RKMEDIA_LOGI("%s flow ready!\n", flow_name.c_str());
  }

  std::shared_ptr<easymedia::Flow> video_save_flow;
  if (!output_type.empty()) {
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
  }

  video_enc_flow->AddDownFlow(muxer_flow, 0, 0);
  audio_enc_flow->AddDownFlow(muxer_flow, 0, 1);
  video_source_flow->AddDownFlow(video_enc_flow, 0, 0);
  audio_source_flow->AddDownFlow(audio_enc_flow, 0, 0);
  if (!output_type.empty()) {
    muxer_flow->AddDownFlow(video_save_flow, 0, 0);
  }

  signal(SIGINT, sigterm_handler);

  while (!quit) {
    easymedia::msleep(100);
  }

  video_source_flow->RemoveDownFlow(video_enc_flow);
  audio_source_flow->RemoveDownFlow(audio_enc_flow);
  video_enc_flow->RemoveDownFlow(muxer_flow);
  audio_enc_flow->RemoveDownFlow(muxer_flow);

  audio_source_flow.reset();
  video_source_flow.reset();
  video_enc_flow.reset();
  audio_enc_flow.reset();
  muxer_flow.reset();

  return 0;
}

std::shared_ptr<easymedia::Flow> create_flow(const std::string &flow_name,
                                             const std::string &flow_param,
                                             const std::string &elem_param) {
  auto &&param = easymedia::JoinFlowParam(flow_param, 1, elem_param);
  printf("\n#%s flow param:\n%s\n", flow_name.c_str(), param.c_str());
  auto ret = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!ret)
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
  return ret;
}
