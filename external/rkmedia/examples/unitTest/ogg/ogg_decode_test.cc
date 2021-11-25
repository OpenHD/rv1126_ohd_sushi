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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "demuxer.h"
#include "utils.h"

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
      printf("input file path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("output path: %s\n", output_path.c_str());
      break;
    case '?':
    default:
      printf("usage example: \n");
      printf("ogg_decode_test -i input.ogg -o output.pcm\n"
             "ogg_decode_test -i input.ogg -o alsa:default\n");
      break;
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);

  int output_file_fd = -1;
  std::string alsa_device;

  if (output_path.find("alsa:") == 0) {
    alsa_device = output_path.substr(output_path.find(':') + 1);
    assert(!alsa_device.empty());
  } else {
    unlink(output_path.c_str());
    output_file_fd =
        open(output_path.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC, 0664);
    assert(output_file_fd >= 0);
  }

  RKMEDIA_LOGI("alsa_device: %s, output_file_fd: %d\n", alsa_device.c_str(),
               output_file_fd);

  easymedia::REFLECTOR(Demuxer)::DumpFactories();

  // here we use fixed demuxer "oggvorbis"
  std::string codec_name("oggvorbis");
  std::string param;
  PARAM_STRING_APPEND(param, KEY_PATH, input_path);
  auto ogg_demuxer = easymedia::REFLECTOR(Demuxer)::Create<easymedia::Demuxer>(
      codec_name.c_str(), param.c_str());

  if (!ogg_demuxer) {
    fprintf(stderr, "Create demuxer %s failed\n", codec_name.c_str());
    exit(EXIT_FAILURE);
  }

  // On rockchip powerful platform, ogg audio demuxer and decoder seems
  // unnecessary to be separated.
  if (!ogg_demuxer->IncludeDecoder()) {
    RKMEDIA_LOGI("ogg TODO: separate ogg demuxer and ogg decoder\n");
    exit(EXIT_FAILURE);
  }

  MediaConfig pcm_config;
  // input is file, set the stream nullptr
  if (!ogg_demuxer->Init(nullptr, &pcm_config)) {
    fprintf(stderr, "Init demuxer %s failed\n", codec_name.c_str());
    exit(EXIT_FAILURE);
  }
  char **comment = ogg_demuxer->GetComment();
  if (comment) {
    if (*comment)
      printf("ogg comment: \n");
    while (*comment) {
      printf("\t%s\n", *comment);
      ++comment;
    }
  }
  AudioConfig &aud_cfg = pcm_config.aud_cfg;
  SampleInfo &sample_info = aud_cfg.sample_info;
  printf("channel num : %d , sample rate %d, average bit rate: %d\n",
         sample_info.channels, sample_info.sample_rate, aud_cfg.bit_rate);

  std::shared_ptr<easymedia::Stream> out_stream;
  if (!alsa_device.empty()) {
    easymedia::REFLECTOR(Stream)::DumpFactories();

    std::string stream_name("alsa_playback_stream");
    std::string fmt_str = SampleFmtToString(sample_info.fmt);
    std::string rule;
    PARAM_STRING_APPEND(rule, KEY_INPUTDATATYPE, fmt_str);
    if (!easymedia::REFLECTOR(Stream)::IsMatch(stream_name.c_str(),
                                               rule.c_str())) {
      fprintf(stderr, "unsupport data type\n");
      exit(EXIT_FAILURE);
    }
    std::string params;
    PARAM_STRING_APPEND(params, KEY_DEVICE, alsa_device);
    PARAM_STRING_APPEND(params, KEY_SAMPLE_FMT, fmt_str);
    PARAM_STRING_APPEND_TO(params, KEY_CHANNELS, sample_info.channels);
    PARAM_STRING_APPEND_TO(params, KEY_SAMPLE_RATE, sample_info.sample_rate);
    RKMEDIA_LOGI("params:\n%s\n", params.c_str());
    out_stream = easymedia::REFLECTOR(Stream)::Create<easymedia::Stream>(
        stream_name.c_str(), params.c_str());
    if (!out_stream) {
      fprintf(stderr, "Create stream %s failed\n", stream_name.c_str());
      exit(EXIT_FAILURE);
    }
  }

  while (1) {
    auto buffer = ogg_demuxer->Read();
    if (!buffer)
      exit(EXIT_FAILURE); // fatal error
    if (buffer->IsEOF())
      break;
    assert(buffer->GetSampleFormat() != SAMPLE_FMT_NONE);
    std::shared_ptr<easymedia::SampleBuffer> sample_buffer =
        std::static_pointer_cast<easymedia::SampleBuffer>(buffer);
    SampleInfo &info = sample_buffer->GetSampleInfo();
    if ((info.channels != sample_info.channels) ||
        (info.sample_rate != sample_info.sample_rate)) {
      // hmm, config changed, maybe should write to another file
      fprintf(stderr, "initial config is %d %dch, but this buffer is %d %dch\n",
              sample_info.sample_rate, sample_info.channels, info.sample_rate,
              info.channels);
      continue;
    }
    if (out_stream)
      out_stream->Write(
          buffer->GetPtr(), sample_buffer->GetSampleSize(),
          sample_buffer->GetSamples()); // TODO: check the ret value
    if (output_file_fd >= 0) {
      ssize_t count =
          write(output_file_fd, buffer->GetPtr(), buffer->GetValidSize());
      if (count < 0) {
        RKMEDIA_LOGI("write file failed!\n");
      }
    }
  }

  out_stream.reset();
  if (output_file_fd >= 0)
    close(output_file_fd);

  return 0;
}
