// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "demuxer.h"

#include "buffer.h"
#include "media_type.h"

#include "vorbis/codec.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wunused-variable"
extern "C" {
#include <vorbis/vorbisfile.h>
}
#pragma GCC diagnostic pop

namespace easymedia {

// libvorbisfile has already provided a convenient demux and decode functions
// without call the interfaces of libvorbis and libogg
class OggVorbisDemuxer : public Demuxer {
public:
  OggVorbisDemuxer(const char *param);
  virtual ~OggVorbisDemuxer();
  static const char *GetDemuxName() { return "oggvorbis"; }
  virtual bool IncludeDecoder() final { return true; }
  virtual bool Init(std::shared_ptr<Stream> input,
                    MediaConfig *out_cfg) override;
  virtual char **GetComment();
  virtual std::shared_ptr<MediaBuffer> Read(size_t request_size = 0) override;

private:
  OggVorbis_File vf;
};

OggVorbisDemuxer::OggVorbisDemuxer(const char *param) : Demuxer(param) {
  memset(&vf, 0, sizeof(vf));
}

OggVorbisDemuxer::~OggVorbisDemuxer() { ov_clear(&vf); }

bool OggVorbisDemuxer::Init(std::shared_ptr<Stream> input,
                            MediaConfig *out_cfg) {
  int ret = 0;

  if (!input) {
    // input means the path is a standard file, and open/close by libvorbisfile
    if (path.empty()) {
      RKMEDIA_LOGI("you need pass path=xxx when construct OggVorbisDemuxer\n");
      return false;
    }
    ret = ov_fopen(path.c_str(), &vf);
  } else {
    ov_callbacks callbacks = {
        input->c_operations.read,
        input->c_operations.seek,
        NULL, // as stream open outsides, user should close it outsides too
        input->c_operations.tell,
    };
    ret = ov_open_callbacks(input.get(), &vf, NULL, 0, callbacks);
  }
  if (ret) {
    RKMEDIA_LOGI(
        "input invalid, access trouble or not an Ogg bitstream: ret=%d\n", ret);
    return false;
  }

  vorbis_info *vi = ov_info(&vf, -1);
  AudioConfig &aud_cfg = out_cfg->aud_cfg;
  SampleInfo &sample_info = aud_cfg.sample_info;
  sample_info.fmt = SAMPLE_FMT_S16; // rockchip's audio codec totally only S16LE
  sample_info.channels = vi->channels;
  sample_info.sample_rate = vi->rate;
  sample_info.nb_samples = 0;
  aud_cfg.bit_rate = ov_bitrate(&vf, -1);
  // aud_cfg.bit_rate_nominal = vi->bitrate_nominal;
  // aud_cfg.bit_rate_upper = vi->bitrate_upper;
  // aud_cfg.bit_rate_lower = vi->bitrate_lower;
  out_cfg->type = Type::Audio;

  total_time = ov_time_total(&vf, -1);
  if (total_time == OV_EINVAL)
    total_time = 0.0f; // unknown time length

  return true;
}

char **OggVorbisDemuxer::GetComment() {
  vorbis_comment *vc = ov_comment(&vf, -1);
  return vc ? vc->user_comments : NULL;
}

static int local_free(void *arg) {
  if (arg)
    free(arg);
  return 0;
}

std::shared_ptr<MediaBuffer> OggVorbisDemuxer::Read(size_t request_size) {
  static const size_t buffer_len = 2048 * 4;
  if (request_size == 0)
    request_size = buffer_len;
  void *pcmout = malloc(request_size);
  if (!pcmout)
    return nullptr;
  MediaBuffer mb(pcmout, request_size, -1, pcmout, local_free);
  SampleInfo empty_info;
  memset(&empty_info, 0, sizeof(empty_info));
  std::shared_ptr<SampleBuffer> sb =
      std::make_shared<SampleBuffer>(mb, empty_info);
  if (!sb) {
    LOG_NO_MEMORY();
    free(pcmout);
    return nullptr;
  }
  int current_section = -1;
  long ret =
      ov_read(&vf, (char *)pcmout, request_size, 0, 2, 1, &current_section);
  if (ret > 0) {
    vorbis_info *vi = ov_info(&vf, -1);
    SampleInfo &info = sb->GetSampleInfo();
    info.fmt = SAMPLE_FMT_S16;
    info.channels = vi->channels;
    info.sample_rate = vi->rate;
    info.nb_samples = ret / (vi->channels * 2);
  } else if (ret == 0) {
    sb->SetEOF(true);
  } else if (ret < 0) {
    RKMEDIA_LOGI("ov_read failed: ret=%d\n", (int)ret);
    ret = 0;
  }
  sb->SetValidSize(ret);

  return sb;
}

DEFINE_DEMUXER_FACTORY(OggVorbisDemuxer, Demuxer)
const char *FACTORY(OggVorbisDemuxer)::ExpectedInputDataType() {
  return STREAM_OGG;
}
const char *FACTORY(OggVorbisDemuxer)::OutPutDataType() {
  return AUDIO_PCM_S16;
}

} // namespace easymedia
