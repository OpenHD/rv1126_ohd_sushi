// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include "buffer.h"
#include "encoder.h"
#include "media_type.h"
#include "rkaudio_utils.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_AENC

namespace easymedia {

// A encoder which call the rkaudio interface.
class RKAUDIOAudioEncoder : public AudioEncoder {
public:
  RKAUDIOAudioEncoder(const char *param);
  virtual ~RKAUDIOAudioEncoder();
  static const char *GetCodecName() { return "rkaudio_aud"; }
  virtual bool Init() override;
  virtual bool InitConfig(const MediaConfig &cfg) override;
  virtual int Process(const std::shared_ptr<MediaBuffer> &,
                      std::shared_ptr<MediaBuffer> &,
                      std::shared_ptr<MediaBuffer>) override {
    errno = ENOSYS;
    return -1;
  }
  virtual int GetNbSamples() override;
  // rkcodec_send_frame()/rkcodec_receive_packet()
  virtual int SendInput(const std::shared_ptr<MediaBuffer> &input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

private:
  AVCodec *rk_codec;
  AVCodecContext *avctx;
  AVFrame *frame;
  enum AVSampleFormat input_fmt;
  std::string output_data_type;
  std::string rkaudio_codec_name;
};

RKAUDIOAudioEncoder::RKAUDIOAudioEncoder(const char *param)
    : rk_codec(nullptr), avctx(nullptr), input_fmt(AV_SAMPLE_FMT_NONE) {
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_OUTPUTDATATYPE, output_data_type));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_NAME, rkaudio_codec_name));
  parse_media_param_match(param, params, req_list);
}

RKAUDIOAudioEncoder::~RKAUDIOAudioEncoder() {
  if (frame) {
    av_frame_free(&frame);
  }
  if (avctx) {
    rkcodec_free_context(&avctx);
  }
}

bool RKAUDIOAudioEncoder::Init() {
  if (output_data_type.empty()) {
    RKMEDIA_LOGI("missing %s\n", KEY_OUTPUTDATATYPE);
    return false;
  }
  codec_type = StringToCodecType(output_data_type.c_str());
  if (!rkaudio_codec_name.empty()) {
    rk_codec = rkcodec_find_encoder_by_name(rkaudio_codec_name.c_str());
  } else {
    AVCodecID id = CodecTypeToAVCodecID(codec_type);
    rk_codec = rkcodec_find_encoder(id);
  }
  if (!rk_codec) {
    RKMEDIA_LOGI(
        "Fail to find rkaudio codec, request codec name=%s, or format=%s\n",
        rkaudio_codec_name.c_str(), output_data_type.c_str());
    return false;
  }
  avctx = rkcodec_alloc_context3(rk_codec);
  if (!avctx) {
    RKMEDIA_LOGI("Fail to rkcodec_alloc_context3\n");
    return false;
  }
  RKMEDIA_LOGI("av codec name=%s\n",
               rk_codec->long_name ? rk_codec->long_name : rk_codec->name);
  return true;
}

static bool check_sample_fmt(const AVCodec *codec,
                             enum AVSampleFormat sample_fmt) {
  const enum AVSampleFormat *p = codec->sample_fmts;
  if (!p)
    return true;
  while (*p != AV_SAMPLE_FMT_NONE) {
    if (*p++ == sample_fmt)
      return true;
  }
  RKMEDIA_LOGI("av codec_id [0x%08x] does not support av sample fmt [%d]\n",
               codec->id, sample_fmt);
  return false;
}

static bool check_sample_rate(const AVCodec *codec, int sample_rate) {
  const int *p = codec->supported_samplerates;
  if (!p)
    return true;
  while (*p != 0) {
    if (*p++ == sample_rate)
      return true;
  }
  RKMEDIA_LOGI("av codec_id [0x%08x] does not support sample_rate [%d]\n",
               codec->id, sample_rate);
  return false;
}

static bool check_channel_layout(const AVCodec *codec,
                                 uint64_t channel_layout) {
  const uint64_t *p = codec->channel_layouts;
  if (!p)
    return true;
  while (*p != 0) {
    if (*p++ == channel_layout)
      return true;
  }
  RKMEDIA_LOGI(
      "av codec_id [0x%08x] does not support audio channel_layout [%d]\n",
      codec->id, (int)channel_layout);
  return false;
}

bool RKAUDIOAudioEncoder::InitConfig(const MediaConfig &cfg) {
  const AudioConfig &ac = cfg.aud_cfg;
  input_fmt = avctx->sample_fmt = SampleFmtToAVSamFmt(ac.sample_info.fmt);
  if (!check_sample_fmt(rk_codec, avctx->sample_fmt))
    return false;
  avctx->bit_rate = ac.bit_rate;
  avctx->sample_rate = ac.sample_info.sample_rate;
  if (!check_sample_rate(rk_codec, avctx->sample_rate))
    return false;
  avctx->channels = ac.sample_info.channels;
  avctx->channel_layout = av_get_default_channel_layout(avctx->channels);
  if (!check_channel_layout(rk_codec, avctx->channel_layout))
    return false;
  // avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  int av_ret = rkcodec_open2(avctx, rk_codec, NULL);
  if (av_ret < 0) {
    PrintAVError(av_ret, "Fail to rkcodec_open2", rk_codec->long_name);
    return false;
  }
  auto mc = cfg;
  mc.type = Type::Audio;
  mc.aud_cfg.codec_type = codec_type;
  if (!(avctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
    mc.aud_cfg.sample_info.nb_samples = avctx->frame_size;

  frame = rkaudio_frame_alloc();
  if (!frame) {
    fprintf(stderr, "Could not allocate audio frame\n");
    return false;
  }
  auto &info = mc.aud_cfg.sample_info;
  frame->nb_samples = info.nb_samples;
  frame->channels = info.channels;
  frame->channel_layout = av_get_default_channel_layout(info.channels);
  frame->format = input_fmt;

  return AudioEncoder::InitConfig(mc);
}

int RKAUDIOAudioEncoder::GetNbSamples() {
  return avctx ? avctx->frame_size : 0;
}

int RKAUDIOAudioEncoder::SendInput(const std::shared_ptr<MediaBuffer> &input) {
  int ret = 0;
  if (input && input->IsValid()) {
    if ((input->GetType() != Type::Audio)) {
      RKMEDIA_LOGE("AENC: input buffer not Audio type.\n");
      return 0;
    }
    auto in = std::static_pointer_cast<SampleBuffer>(input);
    if ((rk_codec->id == AV_CODEC_ID_MP3) &&
        (in->GetSampleFormat() != SAMPLE_FMT_FLTP)) {
      SampleInfo sampleinfo;
      sampleinfo.fmt = SAMPLE_FMT_FLTP;
      sampleinfo.channels = avctx->channels;
      sampleinfo.nb_samples = avctx->frame_size;
      // from S16 to FLT
      int buffer_size = avctx->channels *
                        av_get_bytes_per_sample(avctx->sample_fmt) *
                        avctx->frame_size;
      auto buffer = std::make_shared<easymedia::SampleBuffer>(
          MediaBuffer::Alloc2(buffer_size), sampleinfo);
      uint8_t *po = (uint8_t *)buffer->GetPtr();
      uint8_t *pi = (uint8_t *)in->GetPtr();
      int is = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
      int os = av_get_bytes_per_sample(avctx->sample_fmt);

      conv_AV_SAMPLE_FMT_S16_to_AV_SAMPLE_FMT_FLT(po, pi, is, os,
                                                  po + buffer_size);

      if (avctx->channels > 1) {
        // from FLT to FLTP
        auto fltp_buf = std::make_shared<easymedia::SampleBuffer>(
            MediaBuffer::Alloc2(buffer_size), sampleinfo);
        conv_package_to_planar((uint8_t *)fltp_buf->GetPtr(),
                               (uint8_t *)buffer->GetPtr(), sampleinfo);
        buffer = fltp_buf;
      }
      buffer->SetSamples(avctx->frame_size);
      buffer->SetUSTimeStamp(in->GetUSTimeStamp());
      buffer->SetValidSize(buffer_size);
      in = buffer;
    }
    if (in->GetSamples() > 0) {
      frame->nb_samples =
          in->GetValidSize() /
          (avctx->channels * av_get_bytes_per_sample(avctx->sample_fmt));
      ret = rkcodec_fill_audio_frame(frame, avctx->channels, avctx->sample_fmt,
                                     (const uint8_t *)in->GetPtr(),
                                     in->GetValidSize(), 0);
      if (ret < 0) {
        PrintAVError(ret, "Fail to fill audio frame\n", rk_codec->long_name);
        return -1;
      }
      frame->pts = in->GetUSTimeStamp();
    }
    ret = rkcodec_send_frame(avctx, frame);
  } else {
    ret = rkcodec_send_frame(avctx, nullptr);
  }
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN))
      return -EAGAIN;
    PrintAVError(ret, "Fail to send frame to encoder", rk_codec->long_name);
    return -1;
  }
  return 0;
}

static int __rkaudio_packet_free(void *arg) {
  auto pkt = (AVPacket *)arg;
  av_packet_free(&pkt);
  return 0;
}

std::shared_ptr<MediaBuffer> RKAUDIOAudioEncoder::FetchOutput() {
  auto pkt = av_packet_alloc();
  if (!pkt)
    return nullptr;
  std::shared_ptr<MediaBuffer> buffer = std::make_shared<MediaBuffer>(
      pkt->data, 0, -1, 0, -1, pkt, __rkaudio_packet_free);

  int ret = rkcodec_receive_packet(avctx, pkt);
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN)) {
      errno = EAGAIN;
      return nullptr;
    } else if (ret == AVERROR_EOF) {
      buffer->SetEOF(true);
      return buffer;
    }
    errno = EFAULT;
    PrintAVError(ret, "Fail to receiver from encoder", rk_codec->long_name);
    return nullptr;
  }
  buffer->SetPtr(pkt->data);
  buffer->SetValidSize(pkt->size);
  buffer->SetUSTimeStamp(pkt->pts);
  buffer->SetType(Type::Audio);
  return buffer;
}

DEFINE_AUDIO_ENCODER_FACTORY(RKAUDIOAudioEncoder)
const char *FACTORY(RKAUDIOAudioEncoder)::ExpectedInputDataType() {
  return AUDIO_PCM;
}
const char *FACTORY(RKAUDIOAudioEncoder)::OutPutDataType() {
  return TYPE_ANYTHING;
}

} // namespace easymedia
