// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RKAUDIO_VID_DECODER_
#define __RKAUDIO_VID_DECODER_
#include "decoder.h"
#include "rkaudio_utils.h"
namespace easymedia {
class RKAudioDecoder : public VideoDecoder {
public:
  RKAudioDecoder(const char *param);
  virtual ~RKAudioDecoder();
  static const char *GetCodecName() { return "rkaudio_vid"; }

  virtual bool Init() override;
  virtual int Process(const std::shared_ptr<MediaBuffer> &input,
                      std::shared_ptr<MediaBuffer> &output,
                      std::shared_ptr<MediaBuffer> extra_output) override;
  virtual int SendInput(const std::shared_ptr<MediaBuffer> &input) override;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() override;

private:
  int need_split;
  AVCodecID codec_id;
  bool support_sync;
  bool support_async;
  AVPacket *pkt;
  AVCodec *codec;
  AVCodecContext *rkaudio_context;
  AVCodecParserContext *parser;
};
} // namespace easymedia
#endif // #ifndef __RKAUDIO_VID_DECODER_
