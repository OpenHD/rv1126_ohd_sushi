// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "encoder.h"

#include <assert.h>
#include <deque>

extern "C" {
#include <vorbis/vorbisenc.h>
}

#include "ogg_utils.h"

#include "buffer.h"

namespace easymedia {

// A encoder which call the libvorbisenc interface.
class VorbisEncoder : public AudioEncoder {
public:
  VorbisEncoder(const char *param);
  virtual ~VorbisEncoder();
  static const char *GetCodecName() { return "libvorbisenc"; }
  virtual bool Init() override { return true; }
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
  vorbis_info vi;    /* struct that stores all the static vorbis bitstream
                        settings */
  vorbis_comment vc; /* struct that stores all the user comments */

  vorbis_dsp_state vd;
  vorbis_block vb;

  std::deque<std::shared_ptr<MediaBuffer>> cached_ogg_packets;
  static const int MAX_CACHED_SIZE = 8;
  static const uint32_t gBufferFlag = MediaBuffer::kBuildinLibvorbisenc;
};

VorbisEncoder::VorbisEncoder(const char *param _UNUSED) {
  codec_type = CODEC_TYPE_VORBIS;
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);
  memset(&vd, 0, sizeof(vd));
  memset(&vb, 0, sizeof(vb));
}

VorbisEncoder::~VorbisEncoder() {
  vorbis_block_clear(&vb);
  vorbis_dsp_clear(&vd);
  vorbis_comment_clear(&vc);
  vorbis_info_clear(&vi);
}

static int __ogg_packet_free(void *p) {
  return ogg_packet_free((ogg_packet *)p);
}

bool VorbisEncoder::InitConfig(const MediaConfig &cfg) {
  if (!vi.codec_setup)
    return false;
  const AudioConfig &ac = cfg.aud_cfg;
  const SampleInfo &si = ac.sample_info;
  // set vbr
  int ret =
      vorbis_encode_init_vbr(&vi, si.channels, si.sample_rate, ac.quality);
  if (ret) {
    RKMEDIA_LOGI("vorbis_encode_init_vbr failed, ret = %d\n", ret);
    return false;
  }
  vorbis_comment_add_tag(&vc, "Encoder", GetCodecName());
  ret = vorbis_analysis_init(&vd, &vi);
  if (ret) {
    RKMEDIA_LOGI("vorbis_analysis_init failed, ret = %d\n", ret);
    return false;
  }
  ret = vorbis_block_init(&vd, &vb);
  if (ret) {
    RKMEDIA_LOGI("vorbis_block_init failed, ret = %d\n", ret);
    return false;
  }
  ogg_packet header;
  ogg_packet header_comm;
  ogg_packet header_code;
  ret =
      vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
  if (ret) {
    RKMEDIA_LOGI("vorbis_analysis_headerout failed, ret = %d\n", ret);
    return false;
  }
  std::list<ogg_packet> ogg_packets;
  ogg_packets.push_back(header);
  ogg_packets.push_back(header_comm);
  ogg_packets.push_back(header_code);
  auto extradata = PackOggPackets(ogg_packets);
  if (extradata) {
    extradata->SetUserFlag(gBufferFlag);
    SetExtraData(extradata);
  } else {
    RKMEDIA_LOGI("PackOggPackets failed\n");
    return false;
  }
  return AudioEncoder::InitConfig(cfg);
}

int VorbisEncoder::SendInput(const std::shared_ptr<MediaBuffer> &input) {
  assert(input->GetType() == Type::Audio);
  auto sample_buffer = std::static_pointer_cast<SampleBuffer>(input);
  SampleInfo &info = sample_buffer->GetSampleInfo();
  if (info.channels != 2 || info.fmt != SAMPLE_FMT_S16) {
    RKMEDIA_LOGI("libvorbisenc only support s16 with 2ch\n");
    return -1;
  }

  int ret;
  int sample_num = sample_buffer->GetSamples();
  if (sample_num == 0) {
    if ((ret = vorbis_analysis_wrote(&vd, 0)) < 0) {
      RKMEDIA_LOGI("vorbis_analysis_wrote 0 failed, ret = %d\n", ret);
      return -1;
    }
  } else {
    if (cached_ogg_packets.size() > MAX_CACHED_SIZE) {
      RKMEDIA_LOGI("cached packets must be page out first\n");
      return -1;
    }
    int i;
    signed char *readbuffer = (signed char *)sample_buffer->GetPtr();
    float **buffer = vorbis_analysis_buffer(&vd, sample_num);
    /* uninterleave samples */
    for (i = 0; i < sample_num; i++) {
      buffer[0][i] =
          ((readbuffer[i * 4 + 1] << 8) | (0x00ff & (int)readbuffer[i * 4])) /
          32768.f;
      buffer[1][i] = ((readbuffer[i * 4 + 3] << 8) |
                      (0x00ff & (int)readbuffer[i * 4 + 2])) /
                     32768.f;
    }

    /* tell the library how much we actually submitted */
    if ((ret = vorbis_analysis_wrote(&vd, i)) < 0) {
      RKMEDIA_LOGI("vorbis_analysis_wrote %d failed, ret = %d\n", i, ret);
      return -1;
    }
  }
  ogg_packet op;
  while ((ret = vorbis_analysis_blockout(&vd, &vb) == 1)) {
    if ((ret = vorbis_analysis(&vb, NULL)) < 0)
      break;
    if ((ret = vorbis_bitrate_addblock(&vb)) < 0)
      break;
    while ((ret = vorbis_bitrate_flushpacket(&vd, &op)) == 1) {
      ogg_packet *new_packet = ogg_packet_clone(op);
      if (!new_packet) {
        errno = ENOMEM;
        return -1;
      }
      std::shared_ptr<MediaBuffer> buffer = std::make_shared<MediaBuffer>(
          new_packet->packet, new_packet->bytes, -1, 0, -1, new_packet,
          __ogg_packet_free);
      if (!buffer) {
        errno = ENOMEM;
        return -1;
      }
      buffer->SetValidSize(op.bytes);
      buffer->SetUSTimeStamp(op.granulepos);
      buffer->SetEOF(op.e_o_s);
      buffer->SetUserFlag(gBufferFlag);
      cached_ogg_packets.push_back(buffer);
    }
    if (ret < 0) {
      RKMEDIA_LOGI("vorbis_bitrate_flushpacket failed\n");
      break;
    }
  }

  if (ret < 0) {
    RKMEDIA_LOGI("error getting available ogg packets, ret = %d\n", ret);
    return -1;
  }

  return 0;
}

// output: userdata is the ogg_packet while ptr is the ogg_packet.packet.
std::shared_ptr<MediaBuffer> VorbisEncoder::FetchOutput() {
  if (cached_ogg_packets.size() == 0)
    return nullptr;
  auto buffer = cached_ogg_packets.front();
  cached_ogg_packets.pop_front();
  return buffer;
}

DEFINE_AUDIO_ENCODER_FACTORY(VorbisEncoder)
const char *FACTORY(VorbisEncoder)::ExpectedInputDataType() {
  return AUDIO_PCM_S16;
}
const char *FACTORY(VorbisEncoder)::OutPutDataType() { return AUDIO_VORBIS; }

} // namespace easymedia
