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
#include "easymedia/key_string.h"
#include "easymedia/media_config.h"
#include "easymedia/media_type.h"
#include "easymedia/reflector.h"
#include "easymedia/stream.h"
#include "easymedia/utils.h"

static bool quit = false;

static void sigterm_handler(int sig) {
  RKMEDIA_LOGI("signal %d\n", sig);
  quit = true;
}

std::shared_ptr<easymedia::Flow> create_flow(const std::string &flow_name,
                                             const std::string &flow_param,
                                             const std::string &elem_param) {
  auto &&param = easymedia::JoinFlowParam(flow_param, 1, elem_param);
  auto ret = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!ret)
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
  return ret;
}

std::shared_ptr<easymedia::Flow>
create_alsa_flow(std::string aud_in_path, SampleInfo &info, bool capture) {
  std::string flow_name;
  std::string flow_param;
  std::string sub_param;
  std::string stream_name;

  if (capture) {
    // default sync mode
    flow_name = "source_stream";
    stream_name = "alsa_capture_stream";
  } else {
    flow_name = "output_stream";
    stream_name = "alsa_playback_stream";
    PARAM_STRING_APPEND(flow_param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
    PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);
    PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 5);
  }
  flow_param = "";
  sub_param = "";

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

std::shared_ptr<easymedia::Flow>
create_audio_filter_flow(SampleInfo &info, std::string filter_name) {
  std::string flow_name;
  std::string flow_param;
  std::string sub_param;

  flow_name = "filter";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, filter_name);
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE,
                      SampleFmtToString(info.fmt));
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE,
                      SampleFmtToString(info.fmt));
  sub_param = "";
  PARAM_STRING_APPEND(sub_param, KEY_SAMPLE_FMT, SampleFmtToString(info.fmt));
  PARAM_STRING_APPEND_TO(sub_param, KEY_CHANNELS, info.channels);
  PARAM_STRING_APPEND_TO(sub_param, KEY_SAMPLE_RATE, info.sample_rate);
  PARAM_STRING_APPEND_TO(sub_param, KEY_FRAMES, info.nb_samples);
  flow_param = easymedia::JoinFlowParam(flow_param, 1, sub_param);
  auto flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  return flow;
}

void usage(char *name) {
  RKMEDIA_LOGI(
      "\nUsage: simple mode\t%s -a default -o  default -s 1 -f S16 -r 8000 -c "
      "1\n",
      name);
  RKMEDIA_LOGI(
      "\nUsage: complex mode\t%s -a default -o default -f S16 -r 16000 -c 2 -F "
      "FLTP -R "
      "48000 -C 1\n",
      name);
  RKMEDIA_LOGI(
      "\tNOTICE: format: -f -F [U8 S16 S32 FLT U8P S16P S32P FLTP G711A "
      "G711U]\n");
  RKMEDIA_LOGI("\tNOTICE: channels: -c -C [1 2]\n");
  RKMEDIA_LOGI(
      "\tNOTICE: samplerate: -r -R [8000 16000 24000 32000 441000 48000]\n");
  RKMEDIA_LOGI("\tNOTICE: capture params: -f -c -r\n");
  RKMEDIA_LOGI("\tNOTICE: resample params: -F -C -R\n");
  exit(EXIT_FAILURE);
}

SampleFormat parseFormat(std::string args) {
  if (!args.compare("U8"))
    return SAMPLE_FMT_U8;
  if (!args.compare("S16"))
    return SAMPLE_FMT_S16;
  if (!args.compare("S32"))
    return SAMPLE_FMT_S32;
  if (!args.compare("FLT"))
    return SAMPLE_FMT_FLT;
  if (!args.compare("U8P"))
    return SAMPLE_FMT_U8P;
  if (!args.compare("S16P"))
    return SAMPLE_FMT_S16P;
  if (!args.compare("S32P"))
    return SAMPLE_FMT_S32P;
  if (!args.compare("FLTP"))
    return SAMPLE_FMT_FLTP;
  if (!args.compare("G711A"))
    return SAMPLE_FMT_G711A;
  if (!args.compare("G711U"))
    return SAMPLE_FMT_G711U;
  else
    return SAMPLE_FMT_NONE;
}

static char optstr[] = "?a:o:s:f:r:c:F:R:C:";

int main(int argc, char **argv) {
  SampleFormat fmt = SAMPLE_FMT_S16;
  int channels = 1;
  int sample_rate = 8000;
  int nb_samples;
  SampleFormat res_fmt = SAMPLE_FMT_S16;
  int res_channels = 1;
  int res_sample_rate = 8000;
  int simple_mode = 0;
  int c;
  std::string aud_in_path = "default";
  std::string output_path = "default";
  std::string flow_name;
  std::string flow_param;
  std::string sub_param;
  std::string stream_name;

  easymedia::REFLECTOR(Stream)::DumpFactories();
  easymedia::REFLECTOR(Flow)::DumpFactories();

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'a':
      aud_in_path = optarg;
      RKMEDIA_LOGI("audio device path: %s\n", aud_in_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      RKMEDIA_LOGI("output device path: %s\n", output_path.c_str());
      break;
    case 'f':
      fmt = parseFormat(optarg);
      if (fmt == SAMPLE_FMT_NONE)
        usage(argv[0]);
      break;
    case 'r':
      sample_rate = atoi(optarg);
      if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 24000 &&
          sample_rate != 32000 && sample_rate != 44100 &&
          sample_rate != 48000) {
        RKMEDIA_LOGI("sorry, sample_rate %d not supported\n", sample_rate);
        usage(argv[0]);
      }
      break;
    case 'c':
      channels = atoi(optarg);
      if (channels < 1 || channels > 2)
        usage(argv[0]);
      break;
    case 's':
      simple_mode = atoi(optarg);
      break;
    case 'F':
      res_fmt = parseFormat(optarg);
      if (res_fmt == SAMPLE_FMT_NONE)
        usage(argv[0]);
      break;
    case 'R':
      res_sample_rate = atoi(optarg);
      if (res_sample_rate != 8000 && res_sample_rate != 16000 &&
          res_sample_rate != 24000 && res_sample_rate != 32000 &&
          res_sample_rate != 44100 && res_sample_rate != 48000) {
        RKMEDIA_LOGI("sorry, sample_rate %d not supported\n", res_sample_rate);
        usage(argv[0]);
      }
      break;
    case 'C':
      res_channels = atoi(optarg);
      if (res_channels < 1 || res_channels > 2)
        usage(argv[0]);
      break;
    case '?':
    default:
      usage(argv[0]);
      break;
    }
  }
  if (simple_mode) {
    const int sample_time_ms = 20; // anr only support 10/16/20ms
    nb_samples = sample_rate * sample_time_ms / 1000;
    SampleInfo sample_info = {fmt, channels, sample_rate, nb_samples};

    RKMEDIA_LOGI("Loop in simple mode: capture -> playback\n");

    // 1. alsa capture flow
    std::shared_ptr<easymedia::Flow> audio_source_flow =
        create_alsa_flow(aud_in_path, sample_info, true);
    if (!audio_source_flow) {
      RKMEDIA_LOGI("Create flow alsa_capture_flow failed\n");
      exit(EXIT_FAILURE);
    }

    // 2. alsa playback flow
    std::shared_ptr<easymedia::Flow> audio_sink_flow =
        create_alsa_flow(output_path, sample_info, false);
    if (!audio_sink_flow) {
      RKMEDIA_LOGI("Create flow alsa_capture_flow failed\n");
      exit(EXIT_FAILURE);
    }
    audio_source_flow->AddDownFlow(audio_sink_flow, 0, 0);

    signal(SIGINT, sigterm_handler);
    while (!quit) {
      easymedia::msleep(100);
    }
    audio_source_flow->RemoveDownFlow(audio_sink_flow);
    audio_source_flow.reset();
    audio_sink_flow.reset();
    return 0;
  }
  int res_nb_samples;
  const int sample_time_ms = 20; // anr only support 10/16/20ms
  nb_samples = sample_rate * sample_time_ms / 1000;
  res_nb_samples = res_sample_rate * sample_time_ms / 1000;
  SampleInfo sample_info = {fmt, channels, sample_rate, nb_samples};
  SampleInfo res_sample_info = {res_fmt, res_channels, res_sample_rate,
                                res_nb_samples};

  RKMEDIA_LOGI(
      "Loop in complex mode: capture -> anr -> resample -> resample -> fifo -> "
      "playback\n");

  // 1. alsa capture flow
  std::shared_ptr<easymedia::Flow> audio_source_flow =
      create_alsa_flow(aud_in_path, sample_info, true);
  if (!audio_source_flow) {
    RKMEDIA_LOGI("Create flow alsa_capture_flow failed\n");
    exit(EXIT_FAILURE);
  }
  int volume = 70;
  audio_source_flow->Control(easymedia::S_ALSA_VOLUME, &volume);

  // 2. audio ANR
  std::shared_ptr<easymedia::Flow> anr_flow =
      create_audio_filter_flow(sample_info, "ANR");
  if (!anr_flow) {
    RKMEDIA_LOGI("Create flow audio_resample_flow failed\n");
    exit(EXIT_FAILURE);
  }
  // 3. alsa resample
  std::shared_ptr<easymedia::Flow> audio_resample_flow =
      create_audio_filter_flow(res_sample_info, "rkaudio_resample");
  if (!audio_resample_flow) {
    RKMEDIA_LOGI("Create flow audio_resample_flow failed\n");
    exit(EXIT_FAILURE);
  }

  // 4. alsa resample back
  std::shared_ptr<easymedia::Flow> audio_resample_back_flow =
      create_audio_filter_flow(sample_info, "rkaudio_resample");
  if (!audio_resample_back_flow) {
    RKMEDIA_LOGI("Create flow audio_resample_flow failed\n");
    exit(EXIT_FAILURE);
  }

  // 5. audio fifo to fixed output samples
  sample_info.nb_samples *= 2;
  std::shared_ptr<easymedia::Flow> audio_fifo_flow =
      create_audio_filter_flow(sample_info, "rkaudio_audio_fifo");
  if (!audio_fifo_flow) {
    RKMEDIA_LOGI("Create flow audio_fifo_flow failed\n");
    exit(EXIT_FAILURE);
  }

  // 6. alsa playback flow
  std::shared_ptr<easymedia::Flow> audio_sink_flow =
      create_alsa_flow(output_path, sample_info, false);
  if (!audio_sink_flow) {
    RKMEDIA_LOGI("Create flow alsa_capture_flow failed\n");
    exit(EXIT_FAILURE);
  }
  volume = 60;
  audio_sink_flow->Control(easymedia::S_ALSA_VOLUME, &volume);

  audio_fifo_flow->AddDownFlow(audio_sink_flow, 0, 0);
  audio_resample_back_flow->AddDownFlow(audio_fifo_flow, 0, 0);
  audio_resample_flow->AddDownFlow(audio_resample_back_flow, 0, 0);
  anr_flow->AddDownFlow(audio_resample_flow, 0, 0);
  audio_source_flow->AddDownFlow(anr_flow, 0, 0);

  signal(SIGINT, sigterm_handler);

  while (!quit) {
    easymedia::msleep(100);
    printf("Switch ANR ON/OFF: 0|1\n");
    char value = getchar();
    if (value == '0' || value == '1') {
      int i_value = value - '0';
      anr_flow->Control(easymedia::S_ANR_ON, &i_value);
    }
  }
  audio_fifo_flow->RemoveDownFlow(audio_sink_flow);
  audio_resample_back_flow->RemoveDownFlow(audio_fifo_flow);
  audio_resample_flow->RemoveDownFlow(audio_resample_back_flow);
  anr_flow->RemoveDownFlow(audio_resample_flow);
  audio_source_flow->RemoveDownFlow(anr_flow);
  audio_resample_back_flow.reset();
  audio_resample_flow.reset();
  anr_flow.reset();
  audio_source_flow.reset();
  audio_fifo_flow.reset();
  audio_sink_flow.reset();
  return 0;
}
