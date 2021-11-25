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
  RKMEDIA_LOGI("signal %d\n", sig);
  quit = true;
}

static char optstr[] = "?i:o:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      RKMEDIA_LOGI("input file path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      RKMEDIA_LOGI("output file path: %s\n", output_path.c_str());
      break;
    case '?':
    default:
      RKMEDIA_LOGI("usage example: \n");
      RKMEDIA_LOGI("\t%s -i input.pcm -o output.mp2\n"
                   "\tNOTICE: input.pcm: 2Channels, 44100, S16_LE\n",
                   argv[0]);
      break;
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);

  easymedia::REFLECTOR(Flow)::DumpFactories();
  std::string flow_name;
  std::string param;

  // 1. audio encoder
  flow_name = "audio_enc";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "rkaudio_aud");
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, AUDIO_MP2);
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE, AUDIO_PCM_S16);
  MediaConfig enc_config;
  // s16 2ch stereo, 1024 nb_samples
  SampleInfo aud_info = {SAMPLE_FMT_S16, 2, 44100, 1024};
  auto &ac = enc_config.aud_cfg;
  ac.sample_info = aud_info;
  ac.bit_rate = 64000; // 64kbps
  enc_config.type = Type::Audio;

  std::string enc_param;
  enc_param.append(easymedia::to_param_string(enc_config, AUDIO_MP2));
  param = easymedia::JoinFlowParam(param, 1, enc_param);
  RKMEDIA_LOGI("AudioEncoder flow param:\n%s\n", param.c_str());
  auto enc_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!enc_flow) {
    RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  // 2. file reader
  flow_name = "file_read_flow";
  param = "";

  /* Configure the data block size according to
   * the encoder requirements.
   */
  int nb_samples, fps;
  int read_size = enc_flow->GetInputSize();
  if (read_size > 0) {
    nb_samples = read_size / GetSampleSize(aud_info);
    fps = aud_info.sample_rate / nb_samples;
    RKMEDIA_LOGI("The data block size(%d Byte) is specified by the codec.\n",
                 read_size);
  } else {
    nb_samples = aud_info.nb_samples;
    read_size = nb_samples * GetSampleSize(aud_info);
    fps = aud_info.sample_rate / nb_samples;
    RKMEDIA_LOGI("The data block size(%d Byte) is specified by the user.\n",
                 read_size);
  }

  PARAM_STRING_APPEND(param, KEY_PATH, input_path);
  PARAM_STRING_APPEND(param, KEY_OPEN_MODE, "re"); // read and close-on-exec
  PARAM_STRING_APPEND_TO(param, KEY_MEM_SIZE_PERTIME, read_size);
  PARAM_STRING_APPEND_TO(param, KEY_FPS, fps);
  PARAM_STRING_APPEND_TO(param, KEY_LOOP_TIME, 0);
  RKMEDIA_LOGI("ReadFile flow param:\n%s\n", param.c_str());

  auto file_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!file_flow) {
    RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  // 3. file writer
  flow_name = "file_write_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_PATH, output_path.c_str());
  PARAM_STRING_APPEND(param, KEY_OPEN_MODE, "w+");
  RKMEDIA_LOGI("FileWrite flow param:\n%s\n", param.c_str());
  auto save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  if (!save_flow) {
    RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  // 4. link above flows
  enc_flow->AddDownFlow(save_flow, 0, 0);
  file_flow->AddDownFlow(
      enc_flow, 0, 0); // the source flow better place the end to add down flow
  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

  signal(SIGINT, sigterm_handler);

  while (!quit) {
    easymedia::msleep(100);
  }

  RKMEDIA_LOGI("%s reclaiming\n", argv[0]);
  file_flow->RemoveDownFlow(enc_flow);
  enc_flow->RemoveDownFlow(save_flow);
  file_flow.reset();
  enc_flow.reset();
  save_flow.reset();
  return 0;
}
