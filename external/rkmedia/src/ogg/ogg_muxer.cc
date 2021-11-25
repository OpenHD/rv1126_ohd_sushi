// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "muxer.h"

#include <assert.h>

#include <ogg/ogg.h>

#include "buffer.h"
#include "media_type.h"
#include "ogg_utils.h"

namespace easymedia {

class OggMuxer : public Muxer {
public:
  OggMuxer(const char *param);
  virtual ~OggMuxer();
  static const char *GetMuxName() { return "liboggmuxer"; }

  virtual bool Init() override { return true; }
  virtual bool
  NewMuxerStream(const MediaConfig &mc,
                 const std::shared_ptr<MediaBuffer> &enc_extra_data,
                 int &stream_no) override;
  virtual std::shared_ptr<MediaBuffer> WriteHeader(int stream_no);
  virtual std::shared_ptr<MediaBuffer>
  Write(std::shared_ptr<MediaBuffer> orig_data, int stream_no) override;

private:
  std::map<int, ogg_stream_state> streams;
  int stream_number;
};

OggMuxer::OggMuxer(const char *param) : Muxer(param), stream_number(0) {}

OggMuxer::~OggMuxer() { assert(streams.empty()); }

bool OggMuxer::NewMuxerStream(
    const MediaConfig &mc _UNUSED,
    const std::shared_ptr<MediaBuffer> &enc_extra_data, int &stream_no) {
  int ret;
  ogg_stream_state os;
  int stream_index = stream_number + 1;
  stream_no = -1;
  if ((ret = ogg_stream_init(&os, stream_index))) {
    RKMEDIA_LOGI("ogg_stream_init failed, ret = %d", ret);
    return false;
  }

  if (enc_extra_data) {
    int r = 0;
    if (!(enc_extra_data->GetUserFlag() & MediaBuffer::kBuildinLibvorbisenc)) {
      RKMEDIA_LOGI("buildin muxer[liboggmuxer] only accept data from encoder "
                   "[libvorbisenc]\n");
      return false;
    }
    void *extradata = enc_extra_data->GetPtr();
    size_t extradatasize = enc_extra_data->GetValidSize();
    std::list<ogg_packet> ogg_packets;
    if (UnpackOggData(extradata, extradatasize, ogg_packets)) {
      for (auto &p : ogg_packets) {
        r = ogg_stream_packetin(&os, &p);
        if (r)
          break;
      }
    } else {
      r = -1;
    }
    if (r) {
      RKMEDIA_LOGI("ogg get header failed, ret = %d\n", ret);
      ogg_stream_clear(&os);
      streams.erase(stream_no);
      return false;
    }
  }

  stream_number++;
  stream_no = stream_index;
  streams[stream_no] = os;

  return true;
}

// return the data length
static size_t _write_ogg_page(const ogg_page &og,
                              std::list<std::pair<void *, size_t>> &pages,
                              std::shared_ptr<Stream> &out) {
  if (out) {
    int wlen = out->Write(og.header, 1, og.header_len);
    if (wlen != og.header_len)
      RKMEDIA_LOGI("write_ogg_page failed, %m\n");
    wlen = out->Write(og.body, 1, og.body_len);
    if (wlen != og.body_len)
      RKMEDIA_LOGI("write_ogg_page failed, %m\n");
  }
  size_t len = (og.header_len + og.body_len);
  void *buffer = malloc(len);
  if (!buffer) {
    errno = ENOMEM;
    return 0;
  }
  memcpy(buffer, og.header, og.header_len);
  memcpy(((unsigned char *)buffer) + og.header_len, og.body, og.body_len);
  pages.push_back(std::pair<void *, size_t>(buffer, len));
  return len;
}

std::shared_ptr<MediaBuffer>
_gather_data(const std::list<std::pair<void *, size_t>> &pages,
             size_t total_len) {
  if (total_len == 0)
    return nullptr;
  unsigned char *total_buffer;
  auto ret = MediaBuffer::Alloc(total_len);
  if (ret) {
    ret->SetValidSize(total_len);
  } else {
    errno = ENOMEM;
    return nullptr;
  }
  total_buffer = (unsigned char *)ret->GetPtr();
  for (auto &p : pages) {
    memcpy(total_buffer, p.first, p.second);
    total_buffer += p.second;
  }
  return ret;
}

std::shared_ptr<MediaBuffer> OggMuxer::WriteHeader(int stream_no) {
  auto s = streams.find(stream_no);
  if (s == streams.end()) {
    RKMEDIA_LOGI("Invalid stream no : %d\n", stream_no);
    return nullptr;
  }
  ogg_stream_state &os = s->second;
  std::shared_ptr<MediaBuffer> ret;
  size_t total_len = 0;
  std::list<std::pair<void *, size_t>> pages;
  while (true) {
    ogg_page og;
    int result = ogg_stream_flush(&os, &og);
    if (result == 0)
      break;
    size_t len = _write_ogg_page(og, pages, io_output);
    if (len == 0)
      goto out;
    total_len += len;
  }
  ret = _gather_data(pages, total_len);

out:
  for (auto &p : pages)
    free(p.first);
  return ret;
}

std::shared_ptr<MediaBuffer>
OggMuxer::Write(std::shared_ptr<MediaBuffer> orig_data, int stream_no) {
  assert(orig_data->GetUserFlag() & MediaBuffer::kBuildinLibvorbisenc);
  auto s = streams.find(stream_no);
  if (s == streams.end()) {
    RKMEDIA_LOGI("Invalid stream no : %d\n", stream_no);
    return nullptr;
  }
  ogg_stream_state &os = s->second;
  ogg_packet op;
  memset(&op, 0, sizeof(op));
  assert(orig_data);
  op.packet = (unsigned char *)orig_data->GetPtr();
  assert(op.packet);
  op.bytes = orig_data->GetValidSize();
  op.b_o_s = 0;
  op.e_o_s = orig_data->IsEOF() ? 1 : 0;
  op.granulepos = orig_data->GetUSTimeStamp();
  op.packetno = 0;

  int result = ogg_stream_packetin(&os, &op);
  if (result) {
    RKMEDIA_LOGI("ogg_stream_packetin failed, ret = %d\n", result);
    return nullptr;
  }

  bool eos = false;
  std::shared_ptr<MediaBuffer> ret;
  size_t total_len = 0;
  std::list<std::pair<void *, size_t>> pages;
  while (!eos) {
    ogg_page og;
    result = ogg_stream_pageout(&os, &og);
    if (result == 0)
      break;
    size_t len = _write_ogg_page(og, pages, io_output);
    if (len == 0)
      goto out;
    total_len += len;
    if (ogg_page_eos(&og))
      eos = true;
  }
  ret = _gather_data(pages, total_len);
  if (eos) {
    ret->SetEOF(true);
    ogg_stream_clear(&os);
    streams.erase(stream_no);
  }

out:
  for (auto &p : pages)
    free(p.first);
  return ret;
}

DEFINE_COMMON_MUXER_FACTORY(OggMuxer)
const char *FACTORY(OggMuxer)::ExpectedInputDataType() { return AUDIO_VORBIS; }
const char *FACTORY(OggMuxer)::OutPutDataType() { return STREAM_OGG; }

} // namespace easymedia
