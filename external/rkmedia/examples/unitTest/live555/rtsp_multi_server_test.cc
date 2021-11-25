// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <thread>

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

#define MP3_FROM_FILE 0

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static const struct option rtsp_muli_options[] = {
    {"video1_path", required_argument, NULL, 'v' + 'p' + '1'},
    {"video2_path", required_argument, NULL, 'v' + 'p' + '2'},
    {"audio_path", required_argument, NULL, 'a' + 'p' + '2'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"video_type", required_argument, NULL, 'v'},
    {"audio_type", required_argument, NULL, 'a'},
    {"muxer_format", required_argument, NULL, 'm' + 'f'},
    {"stress_time", required_argument, NULL, 's' + 't'},
    {NULL, 0, NULL, 0},
};

static void print_usage(char *name) {
  printf("On PC:\n");
  printf("Streame 1: ffplay -rtsp_transport tcp -stimeout 2000000 "
         "rtsp://admin:123456@192.168.xxx.xxx/main_stream1\n");
  printf("Streame 2: ffplay -rtsp_transport tcp -stimeout 2000000 "
         "rtsp://admin:123456@192.168.xxx.xxx/main_stream2\n");
  printf("if you have input -t 20000,  will create to stress test\n");
  printf("Streame 3: ffplay -rtsp_transport tcp -stimeout 2000000 "
         "rtsp://admin:123456@192.168.xxx.xxx/main_stream_t\n");
  printf("usage example: \n");
  printf("%s --video_type=H264 --width=2688 --heigh=1520 --audio_type=G711U.\n",
         name);
  printf("#[--video_type]   support list:\n\tH264\n\tH265\n");
  printf("#[--audio_type]   support list:\n\tMP3\n\tG711U\n\tG711A\n");
  printf("#[--muxer_format] supoort list:\n\tmpegts\n\tmpeg\n");
  printf("#[--video1_path]  default is /dev/video0.\n");
  printf("#[--video2_path]  default is /dev/video1.\n");
  printf("#[--audio_path]   default is /dev/video0.\n");
  printf("#[--stress_time]  if set, will start reset strees test.\n");
}

std::shared_ptr<easymedia::Flow> video_enc_flow_1 = nullptr;
std::shared_ptr<easymedia::Flow> video_enc_flow_2 = nullptr;
std::shared_ptr<easymedia::Flow> video_enc_flow_t = nullptr;

static void testStartStreamCallback(easymedia::Flow *f) {
  RKMEDIA_LOGI("%s:%s: force all video_enc send I frame, Flow *f = %p.\n",
               __FILE__, __func__, f);
  auto value = std::make_shared<easymedia::ParameterBuffer>(0);
  if (video_enc_flow_1)
    (video_enc_flow_1)->Control(easymedia::VideoEncoder::kForceIdrFrame, value);
  if (video_enc_flow_2)
    video_enc_flow_2->Control(easymedia::VideoEncoder::kForceIdrFrame, value);
  if (video_enc_flow_t)
    video_enc_flow_t->Control(easymedia::VideoEncoder::kForceIdrFrame, value);
}

static CodecType parseCodec(std::string args) {
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
  if (!args.compare("H264"))
    return CODEC_TYPE_H264;
  if (!args.compare("H265"))
    return CODEC_TYPE_H265;
  if (!args.compare("MJPEG"))
    return CODEC_TYPE_JPEG;
  else
    return CODEC_TYPE_NONE;
}

static std::string CodecToString(CodecType type) {
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
  case CODEC_TYPE_JPEG:
    return IMAGE_JPEG;
  default:
    return "";
  }
}

std::shared_ptr<easymedia::Flow> create_live555_rtsp_server_flow(
    std::string channel_name, std::string media_type,
    unsigned fSamplingFrequency = 0, unsigned fNumChannels = 0,
    unsigned profile = 0, unsigned bitrate = 0) {
  std::shared_ptr<easymedia::Flow> rtsp_flow;

  std::string flow_name;
  std::string flow_param;

  flow_name = "live555_rtsp_server";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, media_type);
  PARAM_STRING_APPEND(flow_param, KEY_CHANNEL_NAME, channel_name);
  PARAM_STRING_APPEND_TO(flow_param, KEY_PORT_NUM, 554);
  PARAM_STRING_APPEND_TO(flow_param, KEY_SAMPLE_RATE, fSamplingFrequency);
  PARAM_STRING_APPEND_TO(flow_param, KEY_CHANNELS, fNumChannels);
  PARAM_STRING_APPEND_TO(flow_param, KEY_PROFILE, profile);
  PARAM_STRING_APPEND_TO(flow_param, KEY_SAMPLE_FMT, bitrate);

  printf("\nRtspFlow:\n%s\n", flow_param.c_str());
  rtsp_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!rtsp_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    // exit(EXIT_FAILURE);
  }
  rtsp_flow->SetPlayVideoHandler(testStartStreamCallback);
  return rtsp_flow;
}
std::shared_ptr<easymedia::Flow>
create_video_enc_flow(std::string video_enc_param, std::string pixel_format,
                      std::string video_enc_type) {
  std::shared_ptr<easymedia::Flow> video_encoder_flow;

  std::string flow_name;
  std::string flow_param;
  std::string enc_param;

  flow_name = "video_enc";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, video_enc_type);
  PARAM_STRING_APPEND_TO(flow_param, KEY_NEED_EXTRA_MERGE, 1);

  flow_param = easymedia::JoinFlowParam(flow_param, 1, video_enc_param);

  printf("\n#VideoEncoder flow param:\n%s\n", flow_param.c_str());
  video_encoder_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_encoder_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    // exit(EXIT_FAILURE);
  }
  return video_encoder_flow;
}
std::shared_ptr<easymedia::Flow>
create_video_read_flow(std::string input_path, std::string pixel_format,
                       int video_width, int video_height) {
  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::shared_ptr<easymedia::Flow> video_read_flow;

  // Reading yuv from camera
  flow_name = "source_stream";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "v4l2_capture_stream");
  // PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, video_fps);
  PARAM_STRING_APPEND(flow_param, KEK_THREAD_SYNC_MODEL, KEY_SYNC);
  PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);
  PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 5);
  stream_param = "";
  PARAM_STRING_APPEND_TO(stream_param, KEY_USE_LIBV4L2, 1);
  PARAM_STRING_APPEND(stream_param, KEY_DEVICE, input_path);
  // PARAM_STRING_APPEND(param, KEY_SUB_DEVICE, sub_input_path);
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
    // exit(EXIT_FAILURE);
  }
  return video_read_flow;
}

std::shared_ptr<easymedia::Flow> create_muxer_flow(std::string audio_enc_param,
                                                   std::string video_enc_param,
                                                   std::string output_type) {
  std::string flow_param = "";
  std::string flow_name = "muxer_flow";
  std::string muxer_param;
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "muxer_flow");
  if (!output_type.empty()) {
    PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, output_type);
    RKMEDIA_LOGI("RKAudio use customIO, output_type = %s.\n",
                 output_type.c_str());
  }
  auto &&param =
      easymedia::JoinFlowParam(flow_param, 2, audio_enc_param, video_enc_param);
  auto muxer_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!muxer_flow) {
    exit(EXIT_FAILURE);
  } else {
    RKMEDIA_LOGI("%s flow ready!\n", flow_name.c_str());
  }
  return muxer_flow;
}

static std::shared_ptr<easymedia::Flow>
create_flow(const std::string &flow_name, const std::string &flow_param,
            const std::string &elem_param) {
  auto &&param = easymedia::JoinFlowParam(flow_param, 1, elem_param);
  auto ret = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!ret)
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
  return ret;
}

static std::shared_ptr<easymedia::Flow>
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

static std::shared_ptr<easymedia::Flow>
create_alsa_flow(std::string aud_in_path, SampleInfo &info) {
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

static std::string get_audio_enc_param(SampleInfo &info, CodecType codec_type,
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

int main(int argc, char **argv) {

  CodecType videoType = CODEC_TYPE_NONE;
  CodecType audioType = CODEC_TYPE_NONE;
  std::string output_type = MUXER_MPEG_TS;
  int video_height = 0;
  int video_width = 0;

  std::string input_format = IMAGE_NV12;
  std::string video0_path = "/dev/video0";
  std::string video1_path = "/dev/video1";
  std::string aud_in_path = "default";
  std::string stream_name0 = "main_stream1";
  std::string stream_name1 = "main_stream2";
  std::string stream_name_t = "main_stream_t";
  int video_fps = 30;

  SampleFormat fmt = SAMPLE_FMT_S16;
  int channels = 1;
  int sample_rate = 8000;
  int nb_samples = 1024;
  int profile = 1;
  // unsigned char bitsPerSample = 16;

  int bitrate = 32000;
  float quality = 1.0; // 0.0 - 1.0

  int stress_sleep_time = 0;
  int c;
  while ((c = getopt_long(argc, argv, "", rtsp_muli_options, NULL)) != -1) {
    switch (c) {
    case 'v':
      videoType = parseCodec(optarg);
      if (videoType == CODEC_TYPE_NONE)
        RKMEDIA_LOGI("videoType error.\n");
      break;
    case 'h':
      video_height = atoi(optarg);
      break;
    case 'w':
      video_width = atoi(optarg);
      break;
    case 'a':
      audioType = parseCodec(optarg);
      if (audioType == CODEC_TYPE_NONE)
        RKMEDIA_LOGI("audioType error.\n");
      break;
    case 's' + 't':
      stress_sleep_time = atoi(optarg);
      break;
    case 'm' + 'f':
      output_type = optarg;
      break;
    case 'v' + 'p' + '1':
      video0_path = optarg;
      break;
    case 'v' + 'p' + '2':
      video1_path = optarg;
      break;
    case 'a' + 'p' + '2':
      aud_in_path = optarg;
      break;
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  // stream 1
  std::shared_ptr<easymedia::Flow> video_read_flow_1;
  std::shared_ptr<easymedia::Flow> video_read_flow_2;

  std::shared_ptr<easymedia::Flow> audio_enc_flow;
  std::shared_ptr<easymedia::Flow> audio_source_flow;

  std::shared_ptr<easymedia::Flow> muxer_flow_2;

  std::shared_ptr<easymedia::Flow> rtsp_flow_1;
  std::shared_ptr<easymedia::Flow> rtsp_flow_2;

  // video enc param
  VideoEncoderCfg vcfg;
  memset(&vcfg, 0, sizeof(vcfg));
  vcfg.type = (char *)CodecToString(videoType).c_str();
  vcfg.fps = video_fps;
  vcfg.max_bps = video_width * video_height * video_fps / 14;
  ImageInfo image_info;
  image_info.pix_fmt = StringToPixFmt(input_format.c_str());
  image_info.width = video_width;
  image_info.height = video_height;
  image_info.vir_width = UPALIGNTO(video_width, 8);
  image_info.vir_height = UPALIGNTO(video_height, 8);
  std::string video_enc_param =
      easymedia::get_video_encoder_config_string(image_info, vcfg);
  std::string audio_enc_param;

  // video
  if (videoType != CODEC_TYPE_NONE) {
    // stream1
    video_read_flow_1 = create_video_read_flow(video0_path, input_format,
                                               video_width, video_height);
    video_enc_flow_1 = create_video_enc_flow(video_enc_param, input_format,
                                             CodecToString(videoType));
    video_read_flow_1->AddDownFlow(video_enc_flow_1, 0, 0);

    // stream2
    video_read_flow_2 = create_video_read_flow(video1_path, input_format,
                                               video_width, video_height);
    video_enc_flow_2 = create_video_enc_flow(video_enc_param, input_format,
                                             CodecToString(videoType));
    video_read_flow_2->AddDownFlow(video_enc_flow_2, 0, 0);
  }

  // audio
  if (audioType != CODEC_TYPE_NONE) {

    if (audioType == CODEC_TYPE_MP3) {
      fmt = SAMPLE_FMT_FLTP;
    } else if (audioType == CODEC_TYPE_G711A || audioType == CODEC_TYPE_G711U) {
      fmt = SAMPLE_FMT_S16;
    } else if (audioType == CODEC_TYPE_MP2) {
      channels = 2;
      sample_rate = 16000;
    } else if (audioType == CODEC_TYPE_G726) {
      fmt = SAMPLE_FMT_S16;
      sample_rate = 8000;
      // bitsPerSample = 2;
    }
    SampleInfo sample_info = {fmt, channels, sample_rate, nb_samples};
    audio_enc_param =
        get_audio_enc_param(sample_info, audioType, bitrate, quality);

    // audio encoder
    audio_enc_flow =
        create_audio_enc_flow(sample_info, audioType, audio_enc_param);
    if (!audio_enc_flow) {
      RKMEDIA_LOGI("Create flow failed\n");
      exit(EXIT_FAILURE);
    }
    // Tuning the nb_samples according to the encoder requirements.
    int read_size = audio_enc_flow->GetInputSize();
    if (read_size > 0) {
      sample_info.nb_samples = read_size / GetSampleSize(sample_info);
      RKMEDIA_LOGI("codec %s : nm_samples fixed to %d\n",
                   CodecToString(audioType).c_str(), sample_info.nb_samples);
    }
    audio_enc_param =
        get_audio_enc_param(sample_info, audioType, bitrate, quality);
    RKMEDIA_LOGI("Audio post enc param: %s\n", audio_enc_param.c_str());

    // 3. alsa capture flow
    audio_source_flow = create_alsa_flow(aud_in_path, sample_info);
    if (!audio_source_flow) {
      RKMEDIA_LOGI("Create flow alsa_capture_flow failed\n");
      exit(EXIT_FAILURE);
    }
    audio_source_flow->AddDownFlow(audio_enc_flow, 0, 0);
  }

  // create muxer flow
  muxer_flow_2 =
      create_muxer_flow(audio_enc_param, video_enc_param, output_type);

  // create rtsp flow
  rtsp_flow_1 = create_live555_rtsp_server_flow(
      stream_name0, CodecToString(audioType) + "," + CodecToString(videoType),
      sample_rate, channels, profile, bitrate);
  if (!rtsp_flow_1) {
    RKMEDIA_LOGI("Create rtsp_stream1_flow failed\n");
    exit(EXIT_FAILURE);
  }
  if (audio_enc_flow) {
    audio_enc_flow->AddDownFlow(rtsp_flow_1, 0, 0);
  }
  if (video_enc_flow_1) {
    video_enc_flow_1->AddDownFlow(rtsp_flow_1, 0, 1);
  }

  rtsp_flow_2 = create_live555_rtsp_server_flow(
      stream_name1, "," + output_type, sample_rate, channels, profile, bitrate);
  if (!rtsp_flow_2) {
    RKMEDIA_LOGI("Create rtsp_stream2_flow failed\n");
    exit(EXIT_FAILURE);
  }

  if (audio_enc_flow) {
    audio_enc_flow->AddDownFlow(muxer_flow_2, 0, 1);
  }
  if (video_enc_flow_2) {
    video_enc_flow_2->AddDownFlow(muxer_flow_2, 0, 0);
  }
  if (false) {
    std::shared_ptr<easymedia::Flow> video_save_flow;
    std::string output_path = "/tmp/out.ts";
    if (!output_type.empty()) {
      std::string flow_name = "file_write_flow";
      std::string flow_param = "";
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
    muxer_flow_2->AddDownFlow(video_save_flow, 0, 0);
  } else
    muxer_flow_2->AddDownFlow(rtsp_flow_2, 0, 1);
  signal(SIGINT, sigterm_handler);

  // stress test
  if (stress_sleep_time > 0) {
    while (!quit) {
      RKMEDIA_LOGI("=========start stress test stress_sleep_time = %d.\n",
                   stress_sleep_time);
      std::shared_ptr<easymedia::Flow> rtsp_flow_t;

      video_enc_flow_t = create_video_enc_flow(video_enc_param, input_format,
                                               CodecToString(videoType));
      if (!video_enc_flow_t) {
        RKMEDIA_LOGI("Create rtsp_stream_t_flow failed\n");
        exit(EXIT_FAILURE);
      }

      rtsp_flow_t = create_live555_rtsp_server_flow(
          stream_name_t,
          CodecToString(videoType) + "," + CodecToString(audioType),
          sample_rate, channels, profile, bitrate);
      if (!rtsp_flow_t) {
        RKMEDIA_LOGI("Create rtsp_stream_t_flow failed\n");
        exit(EXIT_FAILURE);
      }

      if (video_read_flow_1) {
        video_read_flow_1->AddDownFlow(video_enc_flow_t, 0, 0);
      }
      if (audio_enc_flow) {
        audio_enc_flow->AddDownFlow(rtsp_flow_t, 0, 1);
      }

      if (video_enc_flow_t) {
        video_enc_flow_t->AddDownFlow(rtsp_flow_t, 0, 0);
      }
      easymedia::msleep(stress_sleep_time);

      // release
      if (audio_enc_flow) {
        audio_enc_flow->RemoveDownFlow(rtsp_flow_t);
      }

      if (video_enc_flow_t) {
        video_enc_flow_t->RemoveDownFlow(rtsp_flow_t);
      }
      if (video_read_flow_1) {
        video_read_flow_1->RemoveDownFlow(video_enc_flow_t);
      }
      rtsp_flow_t.reset();
      video_enc_flow_t.reset();
    }
  }
  while (!quit)
    easymedia::msleep(100);

  if (videoType != CODEC_TYPE_NONE) {
    // stream 1
    video_enc_flow_1->RemoveDownFlow(rtsp_flow_1);
    video_read_flow_1->RemoveDownFlow(video_enc_flow_1);

    video_enc_flow_1.reset();
    video_read_flow_1.reset();

    // stream 2
    muxer_flow_2->RemoveDownFlow(rtsp_flow_2);
    video_enc_flow_2->RemoveDownFlow(muxer_flow_2);
    video_read_flow_2->RemoveDownFlow(video_enc_flow_1);

    video_read_flow_2.reset();
    video_enc_flow_2.reset();
  }

  if (audioType != CODEC_TYPE_NONE) {

    audio_enc_flow->RemoveDownFlow(rtsp_flow_1);
    audio_enc_flow->RemoveDownFlow(muxer_flow_2);
    muxer_flow_2->RemoveDownFlow(rtsp_flow_2);
    audio_source_flow->RemoveDownFlow(audio_enc_flow);

    audio_enc_flow.reset();
    audio_enc_flow.reset();
  }

  if (rtsp_flow_1)
    rtsp_flow_1.reset();
  if (rtsp_flow_2)
    rtsp_flow_2.reset();
  if (muxer_flow_2)
    muxer_flow_2.reset();
  return 0;
}
