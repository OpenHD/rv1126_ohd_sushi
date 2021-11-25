// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_RKAUDIO_VIDEO_ENCODER_H
#define EASYMEDIA_RKAUDIO_VIDEO_ENCODER_H

#include "encoder.h"
#include "rkaudio_utils.h"
namespace easymedia {

class RKAudioVideoEncoder : public VideoEncoder {
public:
  RKAudioVideoEncoder(const char *param);
  virtual ~RKAudioVideoEncoder();
  static const char *GetCodecName() { return "rkaudio_vid"; }

  virtual bool InitConfig(const MediaConfig &cfg) override;
  virtual bool Init() override;
  virtual int Process(const std::shared_ptr<MediaBuffer> &input,
                      std::shared_ptr<MediaBuffer> &output,
                      std::shared_ptr<MediaBuffer> extra_output) override;
  virtual int SendInput(const std::shared_ptr<MediaBuffer> &input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

protected:
  bool CheckConfigChange(std::pair<uint32_t, std::shared_ptr<ParameterBuffer>>);
  int EncodeControl(int cmd, void *param);

private:
  enum AVCodecID CodecId_;
  AVCodec *Codec_;
  AVCodecContext *Context_;
  AVFrame *frame;
  AVPacket *pkt;
  std::string OutputType_;
  std::string CodecName_;
};

} // namespace easymedia

#endif // EASYMEDIA_RKAUDIO_VIDEO_ENCODER_H
