// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffer.h"
#include "filter.h"
#include <assert.h>
extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
}
#include "rkaudio_utils.h"

#define DEBUG_FILE 0
#if DEBUG_FILE
#include <fstream>
#endif

namespace easymedia {

class ResampleFilter : public Filter {
public:
  ResampleFilter(const char *param);
  virtual ~ResampleFilter();
  static const char *GetFilterName() { return "rkaudio_resample"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;

private:
  int Resample(std::shared_ptr<SampleBuffer> src, SampleInfo dst_info,
               std::shared_ptr<SampleBuffer> &dst);
  int channels;
  int sample_rate;
  SampleFormat format;
  SwrContext *swr_ctx;

#if DEBUG_FILE
  std::ofstream infile;
  std::ofstream outfile;
#endif
};

ResampleFilter::ResampleFilter(const char *param) : swr_ctx(NULL) {
  std::string s_format;
  std::string s_channels;
  std::string s_sample_rate;
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_SAMPLE_FMT, s_format));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_CHANNELS, s_channels));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_SAMPLE_RATE, s_sample_rate));
  parse_media_param_match(param, params, req_list);
  if (!s_channels.empty())
    channels = std::atoi(s_channels.c_str());
  if (!s_sample_rate.empty())
    sample_rate = std::atoi(s_sample_rate.c_str());
  if (!s_format.empty())
    format = StringToSampleFmt(s_format.c_str());
}

ResampleFilter::~ResampleFilter() {
  if (swr_ctx) {
    swr_free(&swr_ctx);
  }
}

int ResampleFilter::Process(std::shared_ptr<MediaBuffer> input,
                            std::shared_ptr<MediaBuffer> &output) {
  if (!input || input->GetType() != Type::Audio)
    return -EINVAL;
  if (!output)
    return -EINVAL;

  auto src = std::static_pointer_cast<easymedia::SampleBuffer>(input);
  SampleInfo src_info = src->GetSampleInfo();
  if (src_info.fmt == format && src_info.channels == channels &&
      src_info.sample_rate == sample_rate) {
    output = input;
    return 0;
  }

  std::shared_ptr<SampleBuffer> dst;
  SampleInfo dst_info = {format, channels, sample_rate, 0};
  if (!SampleInfoIsValid(dst_info)) {
    RKMEDIA_LOGI("check resample parameter failed\n");
    return -1;
  }

  Resample(src, dst_info, dst);
  output = dst;
  return 0;
}

int ResampleFilter::Resample(std::shared_ptr<SampleBuffer> src,
                             SampleInfo dst_info,
                             std::shared_ptr<SampleBuffer> &dst) {
  SampleInfo src_info = src->GetSampleInfo();
  int ret = 0;
  int64_t src_ch_layout;
  int64_t dst_ch_layout;
  int src_linesize = 0;
  int dst_linesize = 0;

  int src_nb_channels = src_info.channels;
  AVSampleFormat src_sample_fmt = SampleFmtToAVSamFmt(src_info.fmt);
  int src_sample_rate = src_info.sample_rate;
  int src_nb_samples = src_info.nb_samples;
  uint8_t *src_data[src_nb_channels] = {NULL};

  int dst_nb_channels = dst_info.channels;
  AVSampleFormat dst_sample_fmt = SampleFmtToAVSamFmt(dst_info.fmt);
  int dst_sample_rate = dst_info.sample_rate;
  int dst_nb_samples = 0;
  uint8_t *dst_data[dst_nb_channels] = {NULL};

  src_ch_layout = av_get_default_channel_layout(src_nb_channels);
  dst_ch_layout = av_get_default_channel_layout(dst_nb_channels);
  if (src_ch_layout == 0 || dst_ch_layout == 0) {
    RKMEDIA_LOGI("swr_alloc don't support channels %d, %d\n", src_nb_channels,
                 dst_nb_channels);
    return -1;
  }

  if (src_nb_samples <= 0) {
    RKMEDIA_LOGI("src_nb_samples error \n");
    return -1;
  }

  if (!swr_ctx) {
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
      RKMEDIA_LOGI("swr_alloc error \n");
      return -1;
    }
    /* set options */
    av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", src_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", dst_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",
                          (AVSampleFormat)dst_sample_fmt, 0);
    swr_init(swr_ctx);
#if DEBUG_FILE
    static int id = 0;
    id++;
    std::string file_in =
        std::string("/tmp/in") + std::to_string(id) + std::string(".pcm");
    std::string file_out =
        std::string("/tmp/out") + std::to_string(id) + std::string(".pcm");
    infile.open(file_in.c_str(),
                std::ios::out | std::ios::trunc | std::ios::binary);
    outfile.open(file_out.c_str(),
                 std::ios::out | std::ios::trunc | std::ios::binary);
    assert(infile.is_open() && outfile.is_open());
    RKMEDIA_LOGI("Resample：%lld,%d,%d -> %lld,%d,%d \n", src_ch_layout,
                 src_sample_rate, src_sample_fmt, dst_ch_layout,
                 dst_sample_rate, dst_sample_fmt);
#endif
  }

  int64_t inpts =
      av_rescale(src->GetUSTimeStamp(),
                 (int64_t)src_sample_rate * dst_sample_rate, AV_TIME_BASE);
  int64_t outpts = swr_next_pts(swr_ctx, inpts);
  outpts = ROUNDED_DIV(outpts, src_sample_rate);

  dst_nb_samples =
      av_rescale_rnd(swr_get_delay(swr_ctx, src_sample_rate) + src_nb_samples,
                     dst_sample_rate, src_sample_rate, AV_ROUND_UP);
  if (dst_nb_samples <= 0) {
    RKMEDIA_LOGI("av_rescale_rnd error \n");
    return -1;
  }

  int size = av_samples_get_buffer_size(NULL, dst_nb_channels, dst_nb_samples,
                                        dst_sample_fmt, 1);
  if (size < 0)
    return size;

  dst = std::make_shared<easymedia::SampleBuffer>(MediaBuffer::Alloc2(size),
                                                  dst_info);
  if (!dst) {
    RKMEDIA_LOGI("Alloc audio frame buffer failed:%d!\n", size);
    return -1;
  }
  rkaudio_samples_fill_arrays(dst_data, &dst_linesize,
                              (const uint8_t *)dst->GetPtr(), dst_nb_channels,
                              dst_nb_samples, dst_sample_fmt, 1);
  rkaudio_samples_fill_arrays(src_data, &src_linesize,
                              (const uint8_t *)src->GetPtr(), src_nb_channels,
                              src_nb_samples, src_sample_fmt, 1);
  ret = swr_convert(swr_ctx, dst_data, dst_nb_samples,
                    (const uint8_t **)src_data, src_nb_samples);
  if (ret <= 0) {
    RKMEDIA_LOGI("swr_convert error \n");
    return -1;
  }

  int resampled_data_size = av_samples_get_buffer_size(
      &dst_linesize, dst_nb_channels, ret, (AVSampleFormat)dst_sample_fmt, 1);
  if (resampled_data_size <= 0) {
    RKMEDIA_LOGI("av_samples_get_buffer_size error \n");
    return -1;
  }
  dst->SetSamples(ret);
  dst->SetValidSize(resampled_data_size);
  dst->SetUSTimeStamp(av_rescale(outpts, AV_TIME_BASE, dst_sample_rate));

#if DEBUG_FILE
  infile.write((const char *)src->GetPtr(), src->GetValidSize());
  outfile.write((const char *)dst->GetPtr(), dst->GetValidSize());
  RKMEDIA_LOGI("diff pts: %lldus\n",
               dst->GetUSTimeStamp() - src->GetUSTimeStamp());
  RKMEDIA_LOGI("%d -> %d\n", src_nb_samples, ret);
#endif
  return resampled_data_size;
}

DEFINE_COMMON_FILTER_FACTORY(ResampleFilter)
const char *FACTORY(ResampleFilter)::ExpectedInputDataType() {
  return AUDIO_PCM;
}

const char *FACTORY(ResampleFilter)::OutPutDataType() { return AUDIO_PCM; }
} // namespace easymedia
