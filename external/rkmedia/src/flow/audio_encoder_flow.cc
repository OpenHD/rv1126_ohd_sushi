// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include "buffer.h"
#include "encoder.h"
#include "flow.h"
#include "media_type.h"
#include "sound.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_AENC

namespace easymedia {

static bool encode(Flow *f, MediaBufferVector &input_vector);

class AudioEncoderFlow : public Flow {
public:
  AudioEncoderFlow(const char *param);
  virtual ~AudioEncoderFlow() {
    AutoPrintLine apl(__func__);
    StopAllThread();
  }
  static const char *GetFlowName() { return "audio_enc"; }
  int GetInputSize() override;
  int Control(unsigned long int request, ...) override;

private:
  std::shared_ptr<AudioEncoder> enc;
  int input_size;
  bool is_mute;

  friend bool encode(Flow *f, MediaBufferVector &input_vector);
};

bool encode(Flow *f, MediaBufferVector &input_vector) {
  AudioEncoderFlow *af = (AudioEncoderFlow *)f;
  std::shared_ptr<AudioEncoder> enc = af->enc;
  std::shared_ptr<MediaBuffer> &src = input_vector[0];
  std::shared_ptr<MediaBuffer> dst;
  std::shared_ptr<MediaBuffer> mute_mb;
  bool result = true;
  bool feed_null = false;
  size_t limit_size = af->input_size;

  if (!src)
    return false;

  if (af->is_mute) {
    mute_mb = MediaBuffer::Clone(*src);
    if (mute_mb) {
      memset((char *)mute_mb->GetPtr(), 0, mute_mb->GetValidSize());
      src = mute_mb;
    }
  }

  if (limit_size && (src->GetValidSize() > limit_size))
    RKMEDIA_LOGW("AudioEncFlow: buffer(%d) is bigger than expected(%d)\n",
                 (int)src->GetValidSize(), (int)limit_size);
  else if (src->GetValidSize() < limit_size) {
    // last frame?
    if (src->GetValidSize() <= 0)
      src->SetValidSize(0); // as null frame.
    else if (src->GetSize() >= limit_size) {
      char *null_ptr = (char *)src->GetPtr() + src->GetValidSize();
      memset(null_ptr, 0, limit_size - src->GetValidSize());
      src->SetValidSize(limit_size);
      feed_null = true;
    } else if (src->GetSize() == src->GetValidSize()) {
      // case MP3 it is ok
    } else {
      src->SetValidSize(0); // as null frame.
    }
  }

  // last frame.
  if ((src->GetValidSize() == limit_size) && src->IsEOF())
    feed_null = true;

  int ret = enc->SendInput(src);
  if (ret < 0) {
    fprintf(stderr, "[Audio]: frame encode failed, ret=%d\n", ret);
    // return -1;
  }

  if (feed_null) {
    auto null_buffer = MediaBuffer::Alloc(1, MediaBuffer::MemType::MEM_COMMON);
    if (!null_buffer)
      LOG_NO_MEMORY();
    else
      enc->SendInput(null_buffer);
  }

  while (ret >= 0) {
    dst = enc->FetchOutput();
    if (!dst) {
      if (errno != EAGAIN) {
        fprintf(stderr, "[Audio]: frame fetch failed, ret=%d\n", errno);
        result = false;
      }
      break;
    }
    size_t out_len = dst->GetValidSize();
    if (out_len == 0)
      break;
    RKMEDIA_LOGD("[Audio]: frame encoded, out %d bytes\n\n", (int)out_len);
    result = af->SetOutput(dst, 0);
    if (!result)
      break;
  }

  return result;
}

AudioEncoderFlow::AudioEncoderFlow(const char *param) {
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
  // check input/output type
  std::string &&rule = gen_datatype_rule(params);
  if (rule.empty()) {
    SetError(-EINVAL);
    return;
  }

  if (!REFLECTOR(Encoder)::IsMatch(ccodec_name, rule.c_str())) {
    RKMEDIA_LOGI("Unsupport for audio encoder %s : [%s]\n", ccodec_name,
                 rule.c_str());
    SetError(-EINVAL);
    return;
  }

  const std::string &enc_param_str = separate_list.back();
  std::map<std::string, std::string> enc_params;
  if (!parse_media_param_map(enc_param_str.c_str(), enc_params)) {
    SetError(-EINVAL);
    return;
  }
  MediaConfig mc;
  if (!ParseMediaConfigFromMap(enc_params, mc)) {
    SetError(-EINVAL);
    return;
  }

  is_mute = false;

  auto encoder = REFLECTOR(Encoder)::Create<AudioEncoder>(
      ccodec_name, enc_param_str.c_str());
  if (!encoder) {
    RKMEDIA_LOGI("Fail to create audio encoder %s<%s>\n", ccodec_name,
                 enc_param_str.c_str());
    SetError(-EINVAL);
    return;
  }

  if (!encoder->InitConfig(mc)) {
    RKMEDIA_LOGI("Fail to init config, %s\n", ccodec_name);
    SetError(-EINVAL);
    return;
  }

  enc = encoder;
  input_size = enc->GetNbSamples() * GetSampleSize(mc.aud_cfg.sample_info);

  SlotMap sm;
  sm.input_slots.push_back(0);
  sm.output_slots.push_back(0);

  sm.process = encode;
  sm.thread_model = Model::ASYNCCOMMON;
  sm.mode_when_full = InputMode::DROPFRONT;
  sm.input_maxcachenum.push_back(3);
  if (!InstallSlotMap(sm, "AudioEncoderFlow", 40)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap for AudioEncoderFlow\n");
    SetError(-EINVAL);
    return;
  }

  SetFlowTag("AudioEncoderFlow");
}

int AudioEncoderFlow::GetInputSize() { return input_size; }

int AudioEncoderFlow::Control(unsigned long int request, ...) {
  int ret = 0;
  va_list vl;
  va_start(vl, request);

  switch (request) {
  case S_SET_MUTE: {
    int mute = va_arg(vl, int);
    is_mute = mute ? true : false;
    RKMEDIA_LOGI("%s mute\n", is_mute ? "enable" : "disable");
  } break;
  default:
    ret = -1;
    break;
  }

  va_end(vl);
  return ret;
}

DEFINE_FLOW_FACTORY(AudioEncoderFlow, Flow)
// type depends on encoder
const char *FACTORY(AudioEncoderFlow)::ExpectedInputDataType() { return ""; }
const char *FACTORY(AudioEncoderFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
