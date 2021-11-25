// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include "buffer.h"
#include "decoder.h"
#include "media_type.h"
#include "rkaudio_utils.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_ADEC

namespace easymedia {

// A decoder which call the rkaudio interface.
class RKAUDIOAudioDecoder : public AudioDecoder {
public:
  RKAUDIOAudioDecoder(const char *param);
  virtual ~RKAUDIOAudioDecoder();
  static const char *GetCodecName() { return "rkaudio_aud"; }
  virtual bool Init() override;
  virtual bool InitConfig(const MediaConfig &cfg) override;
  virtual int Process(const std::shared_ptr<MediaBuffer> &,
                      std::shared_ptr<MediaBuffer> &,
                      std::shared_ptr<MediaBuffer>) override {
    errno = ENOSYS;
    return -1;
  }
  virtual int SendInput(const std::shared_ptr<MediaBuffer> &input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

private:
  AVCodec *rk_codec;
  AVCodecContext *avctx;
  AVCodecParserContext *parser;
  AVPacket *avpkt;
  enum AVSampleFormat output_fmt;
  std::string input_data_type;
  std::string rkaudio_codec_name;
  bool need_parser;
};

RKAUDIOAudioDecoder::RKAUDIOAudioDecoder(const char *param)
    : rk_codec(nullptr), avctx(nullptr), parser(nullptr),
      output_fmt(AV_SAMPLE_FMT_NONE), need_parser(true) {
  printf("%s:%d.\n", __func__, __LINE__);
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_INPUTDATATYPE, input_data_type));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_NAME, rkaudio_codec_name));
  parse_media_param_match(param, params, req_list);
}

RKAUDIOAudioDecoder::~RKAUDIOAudioDecoder() {
  if (avpkt) {
    av_packet_free(&avpkt);
  }
  if (avctx) {
    rkcodec_free_context(&avctx);
  }
  if (parser) {
    av_parser_close(parser);
  }
}

bool RKAUDIOAudioDecoder::Init() {
  if (input_data_type.empty()) {
    RKMEDIA_LOGI("missing %s\n", KEY_INPUTDATATYPE);
    return false;
  }
  codec_type = StringToCodecType(input_data_type.c_str());
  if (codec_type == CODEC_TYPE_G711A || codec_type == CODEC_TYPE_G711U)
    need_parser = false;
  if (!rkaudio_codec_name.empty()) {
    rk_codec = rkcodec_find_decoder_by_name(rkaudio_codec_name.c_str());
  } else {
    AVCodecID id = CodecTypeToAVCodecID(codec_type);
    rk_codec = rkcodec_find_decoder(id);
  }
  if (!rk_codec) {
    RKMEDIA_LOGI(
        "Fail to find rkaudio codec, request codec name=%s, or format=%s\n",
        rkaudio_codec_name.c_str(), input_data_type.c_str());
    return false;
  }

  if (need_parser) {
    parser = av_parser_init(rk_codec->id);
    if (!parser) {
      RKMEDIA_LOGI("Parser not found\n");
      return false;
    }
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

bool RKAUDIOAudioDecoder::InitConfig(const MediaConfig &cfg) {
  if (!need_parser) {
    avctx->channels = cfg.aud_cfg.sample_info.channels;
    avctx->sample_rate = cfg.aud_cfg.sample_info.sample_rate;
  }
  int av_ret = rkcodec_open2(avctx, rk_codec, NULL);
  RKMEDIA_LOGI("InitConfig channels = %d, sample_rate = %d. sample_fmt = %d.\n",
               avctx->channels, avctx->sample_rate, avctx->sample_fmt);
  if (av_ret < 0) {
    PrintAVError(av_ret, "Fail to rkcodec_open2", rk_codec->long_name);
    return false;
  }

  if (avctx->channels > NUM_DATA_POINTERS) {
    fprintf(stderr, "Data channels: %d is too large\n", avctx->channels);
    return false;
  }

  auto mc = cfg;
  mc.type = Type::Audio;
  mc.aud_cfg.codec_type = codec_type;

  avpkt = av_packet_alloc();
  if (!avpkt) {
    fprintf(stderr, "Could not allocate audio pkt\n");
    return false;
  }

  return AudioDecoder::InitConfig(mc);
}

int RKAUDIOAudioDecoder::SendInput(const std::shared_ptr<MediaBuffer> &input) {
  int ret = -1;
  if (input->IsValid()) {
    assert(input && input->GetType() == Type::Audio);
    uint8_t *data = (uint8_t *)input->GetPtr();
    uint64_t data_size = input->GetValidSize();
    if (data_size > 0) {
      if (need_parser) {
        ret = rkaudio_parser_parse2(parser, avctx, &avpkt->data, &avpkt->size,
                                    data, data_size, AV_NOPTS_VALUE,
                                    AV_NOPTS_VALUE, 0);
        if (ret < 0) {
          fprintf(stderr, "Error while parsing\n");
          return -1;
        }
        data += ret;
        data_size -= ret;
        input->SetValidSize(data_size);
        input->SetPtr(data);
      } else {
        avpkt->data = data;
        avpkt->size = data_size;
        input->SetValidSize(0);
        input->SetPtr(data);
      }

      if (avpkt->size) {
        ret = rkcodec_send_packet(avctx, avpkt);
        if (ret < 0) {
          RKMEDIA_LOGI("Audio data lost!, ret = %d.\n", ret);
        }
      }
    }
  } else {
    RKMEDIA_LOGI(
        " will use rkcodec_send_packet send nullptr to fflush decoder.\n");
    ret = rkcodec_send_packet(avctx, nullptr);
  }
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN))
      return -EAGAIN;
    PrintAVError(ret, "Error submitting the packet to the decoder",
                 rk_codec->long_name);
    return -1;
  }
  return 0;
}

static int __rkaudio_frame_free(void *arg) {
  auto frame = (AVFrame *)arg;
  av_frame_free(&frame);
  return 0;
}

std::shared_ptr<MediaBuffer> RKAUDIOAudioDecoder::FetchOutput() {
  int ret = -1;
  auto frame = rkaudio_frame_alloc();
  if (!frame)
    return nullptr;
  std::shared_ptr<MediaBuffer> buffer = std::make_shared<MediaBuffer>(
      frame->data, 0, -1, 0, -1, frame, __rkaudio_frame_free);
  ret = rkcodec_receive_frame(avctx, frame);

  if (ret < 0) {
    if (ret == AVERROR(EAGAIN)) {
      errno = EAGAIN;
      return nullptr;
    } else if (ret == AVERROR_EOF) {
      buffer->SetEOF(true);
      return buffer;
    }
    errno = EFAULT;
    PrintAVError(ret, "Fail to receive frame from decoder",
                 rk_codec->long_name);
    return nullptr;
  }

  auto data_size = rkaudio_get_bytes_per_sample(avctx->sample_fmt);
  if (data_size < 0) {
    /* This should not occur, checking just for paranoia */
    fprintf(stderr, "Failed to calculate data size\n");
    return nullptr;
  }
  RKMEDIA_LOGD("decode [%d]-[%d]-[%d]-[%d]\n", ret, frame->nb_samples,
               data_size, avctx->channels);
  buffer->SetPtr(frame->data[0]);
  buffer->SetValidSize(data_size * frame->nb_samples * avctx->channels);
  buffer->SetUSTimeStamp(frame->pts);
  buffer->SetType(Type::Audio);

  // put the planar pointers into array
  for (int ch = 0; ch < avctx->channels; ch++) {
    buffer->SetPtrArray(frame->data[ch], ch);
  }

  if (rk_codec->id == AV_CODEC_ID_MP3) {
    // from FLTP to FLT
    int buffer_size;

    if (avctx->channels > 1) {
      SampleInfo sampleinfo;
      sampleinfo.fmt = SAMPLE_FMT_FLT;
      sampleinfo.channels = avctx->channels;
      sampleinfo.nb_samples = avctx->frame_size;
      std::shared_ptr<MediaBuffer> buffer_flt = std::make_shared<MediaBuffer>(
          MediaBuffer::Alloc2(data_size * frame->nb_samples * avctx->channels));

      buffer_size =
          sampleinfo.channels *
          rkaudio_get_bytes_per_sample(SampleFmtToAVSamFmt(sampleinfo.fmt)) *
          sampleinfo.nb_samples;
      conv_planar_to_package((uint8_t *)buffer_flt->GetPtr(),
                             (uint8_t **)buffer->GetPtrArrayBase(), sampleinfo);
      buffer_flt->SetValidSize(buffer_size);
      buffer_flt->SetUSTimeStamp(buffer->GetUSTimeStamp());
      buffer_flt->SetType(Type::Audio);

      buffer = buffer_flt;
    }

    // from FLT to S16
    buffer_size = avctx->channels *
                  rkaudio_get_bytes_per_sample(AV_SAMPLE_FMT_S16) *
                  avctx->frame_size;

    std::shared_ptr<MediaBuffer> buffer_s16 =
        std::make_shared<MediaBuffer>(MediaBuffer::Alloc2(buffer_size));
    uint8_t *po = (uint8_t *)buffer_s16->GetPtr();
    uint8_t *pi = (uint8_t *)buffer->GetPtr();

    int os = rkaudio_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    int is = rkaudio_get_bytes_per_sample(avctx->sample_fmt);
    conv_AV_SAMPLE_FMT_FLT_to_AV_SAMPLE_FMT_S16(po, pi, is, os,
                                                po + buffer_size);
    buffer_s16->SetValidSize(buffer_size);
    buffer_s16->SetUSTimeStamp(buffer->GetUSTimeStamp());
    buffer_s16->SetType(Type::Audio);

    buffer = buffer_s16;
  }
  return buffer;
}

DEFINE_AUDIO_DECODER_FACTORY(RKAUDIOAudioDecoder)
const char *FACTORY(RKAUDIOAudioDecoder)::ExpectedInputDataType() {
  return AUDIO_PCM;
}
const char *FACTORY(RKAUDIOAudioDecoder)::OutPutDataType() {
  return TYPE_ANYTHING;
}

} // namespace easymedia
