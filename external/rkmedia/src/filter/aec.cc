// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffer.h"
#include "filter.h"
#include <assert.h>
extern "C" {
#include <RKAP_3A.h>
}

#define DEBUG_FILE 0
#if DEBUG_FILE
#include <fstream>
#endif

namespace easymedia {

class AECFilter : public Filter {
public:
  AECFilter(const char *param);
  virtual ~AECFilter();
  static const char *GetFilterName() { return "AEC"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;

private:
  int AEC(std::shared_ptr<SampleBuffer> src, SampleInfo dst_info,
          std::shared_ptr<SampleBuffer> &dst);
  int channels;
  int sample_rate;
  SampleFormat format;
  int nb_samples;
  std::string param_path;
  RKAP_Handle aec_handle;
  short *prebuf;
#if DEBUG_FILE
  std::ofstream infile;
  std::ofstream outfile;
#endif
};

AECFilter::AECFilter(const char *param) : aec_handle(nullptr), prebuf(nullptr) {
  std::string s_format;
  std::string s_channels;
  std::string s_sample_rate;
  std::string s_nb_samples;
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_SAMPLE_FMT, s_format));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_CHANNELS, s_channels));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_SAMPLE_RATE, s_sample_rate));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_FRAMES, s_nb_samples));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_PATH, param_path));
  parse_media_param_match(param, params, req_list);
  if (!s_channels.empty())
    channels = std::atoi(s_channels.c_str());
  if (!s_sample_rate.empty())
    sample_rate = std::atoi(s_sample_rate.c_str());
  if (!s_format.empty())
    format = StringToSampleFmt(s_format.c_str());
  if (!s_nb_samples.empty())
    nb_samples = std::atoi(s_nb_samples.c_str());

  /* support 8/16K and 2 channels only, channel 0: mic, channel 1:refs */
  assert(format == SAMPLE_FMT_S16P || format == SAMPLE_FMT_S16);
  assert(sample_rate == 8000 || sample_rate == 16000);
  assert(channels == 2);
  int frame_time = nb_samples * 1000 / sample_rate;
  RKMEDIA_LOGI("AEC: frame time %d\n", frame_time);
  assert(frame_time == 16 || frame_time == 20);
  assert(nb_samples > 0);
  if (param_path.empty()) {
    if (sample_rate == 8000)
      param_path = "/usr/share/rkap_3a/para/8k/RKAP_3A_Para.bin";
    else
      param_path = "/usr/share/rkap_3a/para/16k/RKAP_3A_Para.bin";
  }

  RKAP_AEC_State state;
  state.swSampleRate = sample_rate; // 8k|16k
  state.swFrameLen = nb_samples;    // 16ms|20ms
  state.pathPara = param_path.c_str();

  RKMEDIA_LOGI("AEC: param file = %s\n", param_path.c_str());

  aec_handle = RKAP_3A_Init(&state, AEC_TX_TYPE);
  assert(aec_handle);

  if (format == SAMPLE_FMT_S16)
    prebuf = (short *)malloc(channels * 2 * nb_samples);
#if DEBUG_FILE
  static int id = 0;
  id++;
  std::string file_in =
      std::string("/tmp/aec_in") + std::to_string(id) + std::string(".pcm");
  std::string file_out =
      std::string("/tmp/aec_out") + std::to_string(id) + std::string(".pcm");
  infile.open(file_in.c_str(),
              std::ios::out | std::ios::trunc | std::ios::binary);
  outfile.open(file_out.c_str(),
               std::ios::out | std::ios::trunc | std::ios::binary);
  assert(infile.is_open() && outfile.is_open());
#endif
}

AECFilter::~AECFilter() {
  RKAP_3A_Destroy(aec_handle);
  if (prebuf)
    free(prebuf);
#if DEBUG_FILE
  infile.close();
  outfile.close();
#endif
}

int AECFilter::Process(std::shared_ptr<MediaBuffer> input,
                       std::shared_ptr<MediaBuffer> &output) {
  if (!input)
    return -EINVAL;
  if (!output)
    return -EINVAL;

  SampleInfo dst_info = {SAMPLE_FMT_S16, 1, sample_rate, nb_samples};
  int dst_size = GetSampleSize(dst_info) * nb_samples;
  auto dst = std::make_shared<easymedia::SampleBuffer>(
      MediaBuffer::Alloc2(dst_size), dst_info);
  assert(dst);
  assert(input->GetValidSize() == (dst_size * 2));

  short *sigin;
  short *sigref;
  short *sigout;
  if (format == SAMPLE_FMT_S16) {
    sigin = prebuf;
    sigref = prebuf + nb_samples;
    sigout = (short *)dst->GetPtr();
    for (int i = 0; i < nb_samples; i++) {
      sigin[i] = *((short *)input->GetPtr() + i * 2);
      sigref[i] = *((short *)input->GetPtr() + i * 2 + 1);
    }
  } else { // AUDIO_PCM_S16P
    sigin = (short *)input->GetPtr();
    sigref = (short *)input->GetPtr() + nb_samples,
    sigout = (short *)dst->GetPtr();
  }
  RKAP_3A_Process(aec_handle, sigin, sigref, sigout);

  dst->SetSamples(nb_samples);
  dst->SetUSTimeStamp(input->GetUSTimeStamp());
  output = dst;
#if DEBUG_FILE
  infile.write((const char *)input->GetPtr(), input->GetValidSize());
  outfile.write((const char *)dst->GetPtr(), dst->GetValidSize());
#endif
  return 0;
}

#define AEC_PCM                                                                \
  TYPENEAR(AUDIO_PCM_S16)                                                      \
  TYPENEAR(AUDIO_PCM_S16P)

DEFINE_COMMON_FILTER_FACTORY(AECFilter)
const char *FACTORY(AECFilter)::ExpectedInputDataType() { return AEC_PCM; }

const char *FACTORY(AECFilter)::OutPutDataType() { return nullptr; }
} // namespace easymedia
