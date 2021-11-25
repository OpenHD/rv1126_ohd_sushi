// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffer.h"
#include "filter.h"
#include <assert.h>
extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavutil/audio_fifo.h>
}
#include "rkaudio_utils.h"

#define DEBUG_FILE 0
#if DEBUG_FILE
#include <fstream>
#endif

namespace easymedia {
/**
 * Add converted input audio samples to the FIFO buffer for later processing.
 * @param fifo                    Buffer to add the samples to
 * @param new_samples             Samples to be added. The dimensions are
 * channel (for multi-channel audio), sample.
 * @param frame_size              Number of samples to be converted
 * @return Error code (0 if successful)
 */
static int add_samples_to_fifo(AVAudioFifo *fifo, uint8_t **new_samples,
                               const int frame_size) {
  int error;

  /* Make the FIFO as large as it needs to be to hold both,
   * the old and the new samples. */
  if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) +
                                               frame_size)) < 0) {
    fprintf(stderr, "Could not reallocate FIFO\n");
    return error;
  }

  /* Store the new samples in the FIFO buffer. */
  if (av_audio_fifo_write(fifo, (void **)new_samples, frame_size) <
      frame_size) {
    fprintf(stderr, "Could not write data to FIFO\n");
    return AVERROR_EXIT;
  }
  return 0;
};

class AudioFifo : public Filter {
public:
  AudioFifo(const char *param);
  virtual ~AudioFifo();
  static const char *GetFilterName() { return "rkaudio_audio_fifo"; }
  virtual int SendInput(std::shared_ptr<MediaBuffer> input _UNUSED);
  virtual std::shared_ptr<MediaBuffer> FetchOutput();

private:
  int channels;
  int sample_rate;
  SampleFormat format;
  int nb_samples;
  AVAudioFifo *fifo;
  int64_t in_timestamp; // timebase 1/AV_TIME_BASE
  int64_t in_pts;       // timebase 1/samplerate
  int64_t out_pts;      // timebase 1/samplerate
  int finished;

#if DEBUG_FILE
  std::ofstream infile;
  std::ofstream outfile;
#endif
};

AudioFifo::AudioFifo(const char *param)
    : fifo(NULL), in_timestamp(0), in_pts(0), out_pts(0), finished(0) {
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
  parse_media_param_match(param, params, req_list);
  if (!s_channels.empty())
    channels = std::atoi(s_channels.c_str());
  if (!s_sample_rate.empty())
    sample_rate = std::atoi(s_sample_rate.c_str());
  if (!s_format.empty())
    format = StringToSampleFmt(s_format.c_str());
  if (!s_nb_samples.empty())
    nb_samples = std::atoi(s_nb_samples.c_str());

  SampleInfo info = {format, channels, sample_rate, nb_samples};
  int ret = SampleInfoIsValid(info);
  if (!ret) {
    RKMEDIA_LOGI("%s: sample info not valid\n", __func__);
  }
  assert(ret == 0);
  assert(nb_samples > 0);

  /* Create the FIFO buffer based on the specified sample format. */
  fifo = av_audio_fifo_alloc(SampleFmtToAVSamFmt(format), channels, 1);
  assert(fifo);

#if DEBUG_FILE
  static int id = 0;
  id++;
  std::string file_in =
      std::string("/tmp/fifo_in") + std::to_string(id) + std::string(".pcm");
  std::string file_out =
      std::string("/tmp/fifo_out") + std::to_string(id) + std::string(".pcm");
  infile.open(file_in.c_str(),
              std::ios::out | std::ios::trunc | std::ios::binary);
  outfile.open(file_out.c_str(),
               std::ios::out | std::ios::trunc | std::ios::binary);
  assert(infile.is_open() && outfile.is_open());
#endif
}

AudioFifo::~AudioFifo() {
  if (fifo)
    av_audio_fifo_free(fifo);

#if DEBUG_FILE
  infile.close();
  outfile.close();
#endif
}

int AudioFifo::SendInput(std::shared_ptr<MediaBuffer> input _UNUSED) {
  int src_linesize = 0;
  uint8_t *src_data[channels] = {NULL};

  if (!input || input->GetType() != Type::Audio || !input->IsValid())
    return -EINVAL;

  auto in = std::static_pointer_cast<easymedia::SampleBuffer>(input);
  SampleInfo src_info = in->GetSampleInfo();
  if (src_info.fmt != format && src_info.channels != channels &&
      src_info.sample_rate != sample_rate) {
    RKMEDIA_LOGI("check sample info failed\n");
    return -1;
  }

  in_timestamp = in->GetUSTimeStamp();
  in_pts += in->GetSamples();

  rkaudio_samples_fill_arrays(src_data, &src_linesize,
                              (const uint8_t *)in->GetPtr(), channels,
                              in->GetSamples(), SampleFmtToAVSamFmt(format), 1);
  if (add_samples_to_fifo(fifo, src_data, in->GetSamples())) {
    RKMEDIA_LOGI("add samples to fifo failed\n");
    return -1;
  }

#if DEBUG_FILE
  infile.write((const char *)in->GetPtr(), in->GetValidSize());
#endif
  return 0;
}

std::shared_ptr<MediaBuffer> AudioFifo::FetchOutput() {
  int linesize = 0;
  uint8_t *dst_data[channels] = {NULL};

  if (av_audio_fifo_size(fifo) >= nb_samples ||
      (finished && av_audio_fifo_size(fifo) > 0)) {

    const int dst_nb_samples = FFMIN(av_audio_fifo_size(fifo), nb_samples);
    int size = av_samples_get_buffer_size(NULL, channels, dst_nb_samples,
                                          SampleFmtToAVSamFmt(format), 1);
    assert(size > 0);

    SampleInfo dst_info = {format, channels, sample_rate, dst_nb_samples};
    auto dst = std::make_shared<easymedia::SampleBuffer>(
        MediaBuffer::Alloc2(size), dst_info);
    assert(dst);

    rkaudio_samples_fill_arrays(dst_data, &linesize,
                                (const uint8_t *)dst->GetPtr(), channels,
                                dst_nb_samples, SampleFmtToAVSamFmt(format), 1);

    if (av_audio_fifo_read(fifo, (void **)dst_data, dst_nb_samples) <
        dst_nb_samples) {
      RKMEDIA_LOGI("Could not read data from FIFO\n");
      return NULL;
    }
    out_pts += dst_nb_samples;

    dst->SetSamples(dst_nb_samples);
    dst->SetValidSize(size);
    dst->SetUSTimeStamp(
        in_timestamp + av_rescale(out_pts - in_pts, AV_TIME_BASE, sample_rate));

#if DEBUG_FILE
    outfile.write((const char *)dst->GetPtr(), dst->GetValidSize());
    RKMEDIA_LOGI("diff pts: %lldus\n", dst->GetUSTimeStamp() - in_timestamp);
#endif
    return dst;
  }
  return NULL;
}

DEFINE_COMMON_FILTER_FACTORY(AudioFifo)
const char *FACTORY(AudioFifo)::ExpectedInputDataType() { return AUDIO_PCM; }

const char *FACTORY(AudioFifo)::OutPutDataType() { return AUDIO_PCM; }
} // namespace easymedia
