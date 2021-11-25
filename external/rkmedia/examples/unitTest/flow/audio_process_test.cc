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
  SampleFormat output_fmt;
  if (filter_name.compare("AEC") == 0)
    output_fmt = SAMPLE_FMT_S16;
  else
    output_fmt = info.fmt;

  flow_name = "filter";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, filter_name);
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE,
                      SampleFmtToString(info.fmt));
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE,
                      SampleFmtToString(output_fmt));
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

std::shared_ptr<easymedia::Flow> create_file_read_flow(std::string aud_in_path,
                                                       SampleInfo &aud_info) {
  std::string flow_name = "file_read_flow";
  std::string param = "";

  int nb_samples, fps;
  nb_samples = aud_info.nb_samples;
  int read_size = nb_samples * GetSampleSize(aud_info);
  fps = aud_info.sample_rate / nb_samples;

  PARAM_STRING_APPEND(param, KEY_PATH, aud_in_path);
  PARAM_STRING_APPEND(param, KEY_OPEN_MODE, "re"); // read and close-on-exec
  PARAM_STRING_APPEND_TO(param, KEY_MEM_SIZE_PERTIME, read_size);
  PARAM_STRING_APPEND_TO(param, KEY_FPS, fps);
  PARAM_STRING_APPEND_TO(param, KEY_LOOP_TIME, 0);

  auto file_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!file_flow) {
    RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  return file_flow;
}

std::shared_ptr<easymedia::Flow> create_file_write_flow(std::string path) {
  std::string flow_name = "file_write_flow";
  std::string param = "";

  PARAM_STRING_APPEND(param, KEY_PATH, path.c_str());
  PARAM_STRING_APPEND(param, KEY_OPEN_MODE, "w+");

  auto save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!save_flow) {
    RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }
  return save_flow;
}

void usage(char *name) {
  RKMEDIA_LOGI("Usage: %s -i alsa:default -o /tmp/aec_out.pcm -p AEC -r 8000",
               name);
  RKMEDIA_LOGI("Usage: %s -i /tmp/in.pcm -o /tmp/aec_out.pcm -p AEC -r 8000",
               name);
  RKMEDIA_LOGI("NOTICE: process: -p [AEC | ANR]\n");
  RKMEDIA_LOGI("NOTICE: samplerate AEC: -r [8000 | 16000]\n");
  RKMEDIA_LOGI("NOTICE: samplerate ANR: -r [8000 | 16000 | 32000 | 48000]\n");
  exit(EXIT_FAILURE);
}

static char optstr[] = "?i:o:r:p:";

int main(int argc, char **argv) {
  SampleFormat fmt = SAMPLE_FMT_S16;
  int sample_rate = 8000;
  int nb_samples;
  int channels = 2;
  int c;

  std::string aud_in_path = "alsa:default";
  std::string output_path = "/tmp/out.pcm";
  std::string flow_name;
  std::string flow_param;
  std::string sub_param;
  std::string stream_name;
  std::string process = "AEC";

  easymedia::REFLECTOR(Stream)::DumpFactories();
  easymedia::REFLECTOR(Flow)::DumpFactories();

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      aud_in_path = optarg;
      RKMEDIA_LOGI("audio input path: %s\n", aud_in_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      RKMEDIA_LOGI("output file path: %s\n", output_path.c_str());
      break;
    case 'r':
      sample_rate = atoi(optarg);
      break;
    case 'p':
      process = optarg;
      if ((process.compare("AEC") != 0) && (process.compare("ANR") != 0)) {
        RKMEDIA_LOGI("sorry, process %S not supported\n", process.c_str());
        usage(argv[0]);
      }
      RKMEDIA_LOGI("process: %s\n", process.c_str());
      break;
    case '?':
    default:
      usage(argv[0]);
      break;
    }
  }
  if ((process.compare("AEC") == 0) && sample_rate != 8000 &&
      sample_rate != 16000) {
    RKMEDIA_LOGI("sorry, sample_rate %d not supported\n", sample_rate);
    usage(argv[0]);
  }
  if (process.compare("AEC") == 0)
    channels = 2;
  if (process.compare("ANR") == 0)
    channels = 1;

  const int sample_time_ms = 20; // aec only support 16/20ms
  nb_samples = sample_rate * sample_time_ms / 1000;
  SampleInfo sample_info = {fmt, channels, sample_rate, nb_samples};

  // 1. audio source flow
  std::string alsa_device;
  if (aud_in_path.find("alsa:") == 0) {
    alsa_device = aud_in_path.substr(aud_in_path.find(':') + 1);
    assert(!alsa_device.empty());
    RKMEDIA_LOGI("alsa_device: %s\n", alsa_device.c_str());
  }

  std::shared_ptr<easymedia::Flow> audio_source_flow;
  if (!alsa_device.empty()) {
    // if alsa capture source, prefer plane mode
    if (process.compare("AEC") == 0)
      sample_info.fmt = SAMPLE_FMT_S16P;
    audio_source_flow = create_alsa_flow(alsa_device, sample_info, true);
    if (!audio_source_flow) {
      RKMEDIA_LOGI("Create flow alsa_capture_flow failed\n");
      exit(EXIT_FAILURE);
    }
  } else {
    // if file source
    audio_source_flow = create_file_read_flow(aud_in_path, sample_info);
    if (!audio_source_flow) {
      RKMEDIA_LOGI("Create flow create_file_read_flow failed\n");
      exit(EXIT_FAILURE);
    }
  }

  // 2. audio AEC
  std::shared_ptr<easymedia::Flow> filter_flow =
      create_audio_filter_flow(sample_info, process);
  if (!filter_flow) {
    RKMEDIA_LOGI("Create flow audio_filter_flow failed\n");
    exit(EXIT_FAILURE);
  }

  // 3. file output flow
  std::shared_ptr<easymedia::Flow> audio_sink_flow =
      create_file_write_flow(output_path);
  if (!audio_sink_flow) {
    RKMEDIA_LOGI("Create flow create_file_write_flow failed\n");
    exit(EXIT_FAILURE);
  }
  audio_source_flow->AddDownFlow(filter_flow, 0, 0);
  filter_flow->AddDownFlow(audio_sink_flow, 0, 0);

  signal(SIGINT, sigterm_handler);
  while (!quit) {
    easymedia::msleep(100);
  }
  audio_source_flow->RemoveDownFlow(filter_flow);
  filter_flow->RemoveDownFlow(audio_sink_flow);
  audio_source_flow.reset();
  audio_sink_flow.reset();
  filter_flow.reset();
  return 0;
}
