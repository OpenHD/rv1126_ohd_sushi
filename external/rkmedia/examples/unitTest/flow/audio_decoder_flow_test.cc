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

#include "easymedia/buffer.h"
#include "easymedia/encoder.h"
#include "easymedia/flow.h"
#include "easymedia/image.h"
#include "easymedia/key_string.h"
#include "easymedia/media_config.h"
#include "easymedia/media_type.h"
#include "easymedia/message.h"
#include "easymedia/stream.h"
#include "easymedia/utils.h"

SampleInfo g_sampleInfo;

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
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
  default:
    return "";
  }
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
  else
    return CODEC_TYPE_NONE;
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
    PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_BLOCKING);
    PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 1);
  }

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
create_audio_decoder_flow(CodecType codec_type) {
  std::string flow_name;
  std::string flow_param;

  std::string dec_param;

  flow_name = "audio_dec";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkaudio_aud");

  dec_param = "";
  PARAM_STRING_APPEND(dec_param, KEY_INPUTDATATYPE,
                      CodecTypeToString(codec_type));
  PARAM_STRING_APPEND_TO(dec_param, KEY_CHANNELS, g_sampleInfo.channels);
  PARAM_STRING_APPEND_TO(dec_param, KEY_SAMPLE_RATE, g_sampleInfo.sample_rate);

  flow_param = easymedia::JoinFlowParam(flow_param, 1, dec_param);
  printf("\n#audioDecoder flow param:\n%s\n", flow_param.c_str());
  auto audio_decoder_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!audio_decoder_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  return audio_decoder_flow;
}

std::shared_ptr<easymedia::Flow> create_file_reader_flow(std::string path) {
  // Reading yuv from file.
  std::string flow_name;
  std::string flow_param;
  flow_name = "file_read_flow";
  flow_param = "";

  PARAM_STRING_APPEND(flow_param, KEY_PATH, path.c_str());
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "rb"); // read and close-on-exec
  PARAM_STRING_APPEND_TO(flow_param, KEY_MEM_SIZE_PERTIME, 1152);
  PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, 6000);
  PARAM_STRING_APPEND_TO(flow_param, KEY_LOOP_TIME, 0);
  RKMEDIA_LOGI("#FileRead flow param:\n%s\n", flow_param.c_str());

  auto audio_read_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!audio_read_flow) {
    RKMEDIA_LOGI("Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  return audio_read_flow;
}

std::shared_ptr<easymedia::Flow> create_file_writer_flow(std::string path) {
  std::string flow_name;
  std::string flow_param;
  flow_name = "file_write_flow";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_PATH, path.c_str());
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "w+"); // read and close-on-exec
  printf("\n#FileWrite:\n%s\n", flow_param.c_str());
  auto audio_save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!audio_save_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  return audio_save_flow;
}

static std::shared_ptr<easymedia::Flow>
create_audio_enc_flow(SampleInfo &info, CodecType codec_type,
                      std::string audio_enc_param) {
  std::string flow_name;
  std::string flow_param;

  flow_name = "audio_enc";
  flow_param = "";
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

static void file_decoder_alsa(CodecType codec_type, std::string in_file_path,
                              std::string out_alsa_path) {
  std::shared_ptr<easymedia::Flow> file_read;
  std::shared_ptr<easymedia::Flow> audio_decoder;
  std::shared_ptr<easymedia::Flow> alsa_output;

  file_read = create_file_reader_flow(in_file_path);
  audio_decoder = create_audio_decoder_flow(codec_type);

  alsa_output = create_alsa_flow(out_alsa_path, g_sampleInfo, false);

  file_read->AddDownFlow(audio_decoder, 0, 0);
  audio_decoder->AddDownFlow(alsa_output, 0, 0);

  signal(SIGINT, sigterm_handler);
  RKMEDIA_LOGI("%s initial finish\n", __func__);
  while (!quit) {
    easymedia::msleep(100);
  }
  RKMEDIA_LOGI("%s deinitial finish\n", __func__);
}

static void file_decoder_file(CodecType codec_type, const char *in_file_path,
                              const char *out_file_path) {
  std::shared_ptr<easymedia::Flow> file_read;
  std::shared_ptr<easymedia::Flow> audio_decoder;
  std::shared_ptr<easymedia::Flow> file_write;
  file_read = create_file_reader_flow(in_file_path);
  audio_decoder = create_audio_decoder_flow(codec_type);
  file_write = create_file_writer_flow(out_file_path);
  file_read->AddDownFlow(audio_decoder, 0, 0);
  audio_decoder->AddDownFlow(file_write, 0, 0);

  signal(SIGINT, sigterm_handler);
  RKMEDIA_LOGI("%s initial finish\n", __func__);
  while (!quit) {
    easymedia::msleep(100);
  }
  RKMEDIA_LOGI("%s deinitial finish\n", __func__);
}

static void alsa_encoder_decoder_file(CodecType codec_type,
                                      std::string alsa_path,
                                      std::string out_file_path) {
  std::shared_ptr<easymedia::Flow> alsa_read;
  std::shared_ptr<easymedia::Flow> audio_encoder;
  std::shared_ptr<easymedia::Flow> audio_decoder;
  std::shared_ptr<easymedia::Flow> file_write;

  int bitrate = 32000;
  float quality = 1.0; // 0.0 - 1.0

  // 1. read from alsa
  alsa_read = create_alsa_flow(alsa_path, g_sampleInfo, true);

  // 2. create audio encodec
  std::string audio_enc_param;
  audio_enc_param =
      get_audio_enc_param(g_sampleInfo, codec_type, bitrate, quality);
  audio_encoder =
      create_audio_enc_flow(g_sampleInfo, codec_type, audio_enc_param);

  // 3. create audio decoder
  audio_decoder = create_audio_decoder_flow(codec_type);

  // 4. save to file
  file_write = create_file_writer_flow(out_file_path);

  alsa_read->AddDownFlow(audio_encoder, 0, 0);
  audio_encoder->AddDownFlow(audio_decoder, 0, 0);
  audio_decoder->AddDownFlow(file_write, 0, 0);

  signal(SIGINT, sigterm_handler);
  RKMEDIA_LOGI("%s initial finish\n", __func__);
  while (!quit) {
    easymedia::msleep(100);
  }
  RKMEDIA_LOGI("%s deinitial finish\n", __func__);
}

static void file_alsa(std::string in_file_path, std::string out_alsa_path) {
  std::shared_ptr<easymedia::Flow> file_read;
  std::shared_ptr<easymedia::Flow> alsa_output;

  file_read = create_file_reader_flow(in_file_path);
  alsa_output = create_alsa_flow(out_alsa_path, g_sampleInfo, false);
  file_read->AddDownFlow(alsa_output, 0, 0);

  signal(SIGINT, sigterm_handler);
  RKMEDIA_LOGI("%s initial finish\n", __func__);
  while (!quit) {
    easymedia::msleep(100);
  }
  RKMEDIA_LOGI("%s deinitial finish\n", __func__);
}
static void audio_decoder_usage() {
  printf("\n\n/Usage:./audio_decoder_test <index> <codecType> [inFilePath] "
         "[outFilePath]/\n");
  printf("\tindex and its function list below\n");
  printf("\t0:  inFile --> Audio Decoder --> outFile\n");
  printf("\t1:  inFile --> Audio Decoder --> ALSA\n");
  printf("\t2:  ALSA --> Audio Encoder --> Audio Decoder --> outFile\n");
  printf("\t3:  inFile --> ALSA.\n");
  printf("\n");
  printf("\tcodecType list:\n");
  printf("\tMP3 MP2 G711A G711U G726\n");
}

int main(int argc, char **argv) {
  int index = 0;
  if (argc != 3 && argc != 4 && argc != 5) {
    audio_decoder_usage();
    return -1;
  }
  index = std::atoi(argv[1]);
  CodecType codec_type = CODEC_TYPE_NONE;
  std::string file_in_path = "/userdata/out.mp2";
  std::string file_out_path = "/userdata/out.pcm";
  std::string alsa_in_path = "default:CARD=rockchiprk809co";
  std::string alsa_out_path = "default:CARD=rockchiprk809co";

  if (argv[2] != nullptr) {
    codec_type = parseCodec(argv[2]);
  }
  if (argv[3] != nullptr) {
    file_in_path = argv[3];
    alsa_in_path = argv[3];
  }

  if (argv[4] != nullptr) {
    alsa_out_path = argv[4];
    file_out_path = argv[4];
  }

  g_sampleInfo.fmt = SAMPLE_FMT_S16;
  g_sampleInfo.nb_samples = 1152; // MP2
  g_sampleInfo.channels = 2;
  g_sampleInfo.sample_rate = 44100;
  if (codec_type == CODEC_TYPE_MP3) {
    g_sampleInfo.fmt = SAMPLE_FMT_FLTP;
    g_sampleInfo.nb_samples = 1024; // MP3
  }

  switch (index) {
  case 0:
    file_decoder_file(codec_type, file_in_path.c_str(), file_out_path.c_str());
    break;
  case 1:
    file_decoder_alsa(codec_type, file_in_path, alsa_out_path);
    break;
  case 2:
    alsa_encoder_decoder_file(codec_type, file_in_path, file_out_path);
    break;
  case 3:
    file_alsa(file_in_path, alsa_out_path);
    break;
  default:
    break;
  }
  return 0;
}
