// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include "buffer.h"
#include "decoder.h"
#include "flow.h"
#include "media_type.h"
#include "sound.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_ADEC

namespace easymedia {
static bool decode(Flow *f, MediaBufferVector &input_vector);

class AudioDecoderFlow : public Flow {
public:
  AudioDecoderFlow(const char *param);
  virtual ~AudioDecoderFlow() {
    AutoPrintLine apl(__func__);
    StopAllThread();
  }
  static const char *GetFlowName() { return "audio_dec"; }
  int GetInputSize() override;

private:
  std::shared_ptr<AudioDecoder> dec;
  int input_size;

  friend bool decode(Flow *f, MediaBufferVector &input_vector);
};

bool decode(Flow *f, MediaBufferVector &input_vector) {
  AudioDecoderFlow *af = (AudioDecoderFlow *)f;
  std::shared_ptr<AudioDecoder> dec = af->dec;
  std::shared_ptr<MediaBuffer> &src = input_vector[0];
  std::shared_ptr<MediaBuffer> dst;
  bool result = true;
  if (!src)
    return false;
  if (!src->IsValid()) {
    dec->SendInput(src);
    dst = dec->FetchOutput();
    if (dst->IsEOF()) {
      RKMEDIA_LOGI("decode, first EOF.\n");
      return true;
    }
  }
  while (src->GetValidSize() > 0) {
    dec->SendInput(src);
    while (1) {
      dst = dec->FetchOutput();
      if (dst == nullptr) {
        break;
      }
      if (dst->IsEOF()) {
        RKMEDIA_LOGI("decode, second EOF.\n");
        break;
      }
      af->SetOutput(dst, 0);
    }
  }
  return result;
}

AudioDecoderFlow::AudioDecoderFlow(const char *param) {
  printf("AudioDecoderFlow.\n");
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;
  if (!ParseWrapFlowParams(param, params, separate_list)) {
    SetError(-EINVAL);
    return;
  }
  std::string &codec_name = params[KEY_NAME];
  if (codec_name.empty()) {
    RKMEDIA_LOGI("missing codec name\n");
    SetError(-EINVAL);
    return;
  }
  const char *ccodec_name = codec_name.c_str();

  const std::string &dec_param_str = separate_list.back();
  printf("dec_param_str = %s.\n", dec_param_str.c_str());
  std::map<std::string, std::string> dec_params;

  if (!parse_media_param_map(dec_param_str.c_str(), dec_params)) {
    SetError(-EINVAL);
    return;
  }

  MediaConfig mc;
  std::string value = "";
  AudioConfig &aud_cfg = mc.aud_cfg;
  ParseSampleInfoFromMap(dec_params, aud_cfg.sample_info);
  mc.type = Type::Audio;
  auto decoder = REFLECTOR(Decoder)::Create<AudioDecoder>(
      ccodec_name, dec_param_str.c_str());
  if (!decoder) {
    RKMEDIA_LOGI("Fail to create audio decoder %s<%s>\n", ccodec_name,
                 dec_param_str.c_str());
    SetError(-EINVAL);
    return;
  }

  if (!decoder->InitConfig(mc)) {
    RKMEDIA_LOGI("Fail to init config, %s\n", ccodec_name);
    SetError(-EINVAL);
    return;
  }

  dec = decoder;
  input_size = dec->GetNbSamples() * GetSampleSize(mc.aud_cfg.sample_info);

  SlotMap sm;
  sm.input_slots.push_back(0);
  sm.output_slots.push_back(0);

  sm.process = decode;
  sm.thread_model = Model::ASYNCCOMMON;
  sm.mode_when_full = InputMode::BLOCKING;
  sm.input_maxcachenum.push_back(10);
  if (!InstallSlotMap(sm, "AudioDecoderFlow", 40)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap for AudioDecoderFlow\n");
    SetError(-EINVAL);
    return;
  }

  SetFlowTag("AudioDecoderFlow");
}

int AudioDecoderFlow::GetInputSize() { return input_size; }

DEFINE_FLOW_FACTORY(AudioDecoderFlow, Flow)
const char *FACTORY(AudioDecoderFlow)::ExpectedInputDataType() { return ""; }
const char *FACTORY(AudioDecoderFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
