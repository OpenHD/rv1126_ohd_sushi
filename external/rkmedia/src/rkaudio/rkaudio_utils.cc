// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "rkaudio_utils.h"

namespace easymedia {

enum AVPixelFormat PixFmtToAVPixFmt(PixelFormat fmt) {
  static const struct PixFmtAVPFEntry {
    PixelFormat fmt;
    enum AVPixelFormat av_fmt;
  } pix_fmt_av_pixfmt_map[] = {
      {PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P},
      {PIX_FMT_NV12, AV_PIX_FMT_NV12},
      {PIX_FMT_NV21, AV_PIX_FMT_NV21},
      {PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P16LE},
      {PIX_FMT_NV16, AV_PIX_FMT_NV16},
      {PIX_FMT_NV61, AV_PIX_FMT_NONE},
      {PIX_FMT_YUYV422, AV_PIX_FMT_YUYV422},
      {PIX_FMT_UYVY422, AV_PIX_FMT_UYVY422},
      {PIX_FMT_RGB332, AV_PIX_FMT_RGB8},
      {PIX_FMT_RGB565, AV_PIX_FMT_RGB565LE},
      {PIX_FMT_BGR565, AV_PIX_FMT_BGR565LE},
      {PIX_FMT_RGB888, AV_PIX_FMT_BGR24},
      {PIX_FMT_BGR888, AV_PIX_FMT_RGB24},
      {PIX_FMT_ARGB8888, AV_PIX_FMT_BGRA},
      {PIX_FMT_ABGR8888, AV_PIX_FMT_RGBA},
  };
  FIND_ENTRY_TARGET(fmt, pix_fmt_av_pixfmt_map, fmt, av_fmt)
  return AV_PIX_FMT_NONE;
}

PixelFormat AVPixFmtToPixFmt(enum AVPixelFormat av_fmt) {
  static const struct AVPFPixFmtEntry {
    enum AVPixelFormat av_fmt;
    PixelFormat fmt;
  } av_pixfmt_pix_fmt_map[] = {
      {AV_PIX_FMT_YUV420P, PIX_FMT_YUV420P},
      {AV_PIX_FMT_NV12, PIX_FMT_NV12},
      {AV_PIX_FMT_NV21, PIX_FMT_NV21},
      {AV_PIX_FMT_YUV422P16LE, PIX_FMT_YUV422P},
      {AV_PIX_FMT_NV16, PIX_FMT_NV16},
      {AV_PIX_FMT_NONE, PIX_FMT_NV61},
      {AV_PIX_FMT_YUYV422, PIX_FMT_YUYV422},
      {AV_PIX_FMT_UYVY422, PIX_FMT_UYVY422},
      {AV_PIX_FMT_RGB8, PIX_FMT_RGB332},
      {AV_PIX_FMT_RGB565LE, PIX_FMT_RGB565},
      {AV_PIX_FMT_BGR565LE, PIX_FMT_BGR565},
      {AV_PIX_FMT_BGR24, PIX_FMT_RGB888},
      {AV_PIX_FMT_RGB24, PIX_FMT_BGR888},
      {AV_PIX_FMT_BGRA, PIX_FMT_ARGB8888},
      {AV_PIX_FMT_RGBA, PIX_FMT_ABGR8888},
  };
  FIND_ENTRY_TARGET(av_fmt, av_pixfmt_pix_fmt_map, av_fmt, fmt)
  return PIX_FMT_NB;
}

enum AVCodecID SampleFmtToAVCodecID(SampleFormat fmt) {
  static const struct SampleFmtAVCodecIDEntry {
    SampleFormat fmt;
    enum AVCodecID rk_codecid;
  } sample_fmt_rk_codecid_map[] = {
      {SAMPLE_FMT_U8, AV_CODEC_ID_PCM_U8},
      {SAMPLE_FMT_S16, AV_CODEC_ID_PCM_S16LE},
      {SAMPLE_FMT_S32, AV_CODEC_ID_PCM_S32LE},
  };
  FIND_ENTRY_TARGET(fmt, sample_fmt_rk_codecid_map, fmt, rk_codecid)
  return AV_CODEC_ID_NONE;
}

enum AVCodecID CodecTypeToAVCodecID(CodecType type) {
  static const struct SampleFmtAVCodecIDEntry {
    CodecType type;
    enum AVCodecID rk_codecid;
  } codec_type_rk_codecid_map[] = {
      {CODEC_TYPE_MP3, AV_CODEC_ID_MP3},
      {CODEC_TYPE_MP2, AV_CODEC_ID_MP2},
      {CODEC_TYPE_VORBIS, AV_CODEC_ID_VORBIS},
      {CODEC_TYPE_G711A, AV_CODEC_ID_PCM_ALAW},
      {CODEC_TYPE_G711U, AV_CODEC_ID_PCM_MULAW},
      {CODEC_TYPE_G726, AV_CODEC_ID_ADPCM_G726},
      {CODEC_TYPE_H264, AV_CODEC_ID_H264},
      {CODEC_TYPE_H265, AV_CODEC_ID_H265},
      {CODEC_TYPE_JPEG, AV_CODEC_ID_MJPEG},
  };
  FIND_ENTRY_TARGET(type, codec_type_rk_codecid_map, type, rk_codecid)
  return AV_CODEC_ID_NONE;
}

enum AVSampleFormat SampleFmtToAVSamFmt(SampleFormat sfmt) {
  static const struct SampleFmtAVSFEntry {
    SampleFormat fmt;
    enum AVSampleFormat av_sfmt;
  } sample_fmt_av_sfmt_map[] = {
      {SAMPLE_FMT_U8, AV_SAMPLE_FMT_U8},
      {SAMPLE_FMT_S16, AV_SAMPLE_FMT_S16},
      {SAMPLE_FMT_S32, AV_SAMPLE_FMT_S32},
      {SAMPLE_FMT_FLT, AV_SAMPLE_FMT_FLT},
      {SAMPLE_FMT_U8P, AV_SAMPLE_FMT_U8P},
      {SAMPLE_FMT_S16P, AV_SAMPLE_FMT_S16P},
      {SAMPLE_FMT_S32P, AV_SAMPLE_FMT_S32P},
      {SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_FLTP},
  };
  FIND_ENTRY_TARGET(sfmt, sample_fmt_av_sfmt_map, fmt, av_sfmt)
  return AV_SAMPLE_FMT_NONE;
}

void PrintAVError(int err, const char *log, const char *mark) {
  char str[AV_ERROR_MAX_STRING_SIZE] = {0};
  av_strerror(err, str, sizeof(str));
  if (mark)
    RKMEDIA_LOGI("%s '%s': %s\n", log, mark, str);
  else
    RKMEDIA_LOGI("%s: %s\n", log, str);
}

#define CONV_FUNC_NAME(dst_fmt, src_fmt) conv_##src_fmt##_to_##dst_fmt

// FIXME rounding ?
#define CONV_FUNC(ofmt, otype, ifmt, expr)                                     \
  void CONV_FUNC_NAME(ofmt, ifmt)(uint8_t * po, const uint8_t *pi, int is,     \
                                  int os, uint8_t *end) {                      \
    uint8_t *end2 = end - 3 * os;                                              \
    while (po < end2) {                                                        \
      *(otype *)po = expr;                                                     \
      pi += is;                                                                \
      po += os;                                                                \
      *(otype *)po = expr;                                                     \
      pi += is;                                                                \
      po += os;                                                                \
      *(otype *)po = expr;                                                     \
      pi += is;                                                                \
      po += os;                                                                \
      *(otype *)po = expr;                                                     \
      pi += is;                                                                \
      po += os;                                                                \
    }                                                                          \
    while (po < end) {                                                         \
      *(otype *)po = expr;                                                     \
      pi += is;                                                                \
      po += os;                                                                \
    }                                                                          \
  }

CONV_FUNC(AV_SAMPLE_FMT_S16, int16_t, AV_SAMPLE_FMT_FLT,
          av_clip_int16(lrintf(*(const float *)pi *(1 << 15))))
CONV_FUNC(AV_SAMPLE_FMT_FLT, float, AV_SAMPLE_FMT_S16,
          *(const int16_t *)pi *(1.0f / (1 << 15)))

void conv_planar_to_package(uint8_t *po, uint8_t **pi, SampleInfo sampleInfo) {
  int sample_size =
      av_get_bytes_per_sample(SampleFmtToAVSamFmt(sampleInfo.fmt));
  for (int i = 0; i < sampleInfo.nb_samples; i++) {
    for (int j = 0; j < sampleInfo.channels; j++) {
      memcpy(po, pi[j] + sample_size * i, sample_size);
      po += sample_size;
    }
  }
}

void conv_package_to_planar(uint8_t *po, uint8_t *pi, SampleInfo sampleInfo) {
  int sample_size =
      av_get_bytes_per_sample(SampleFmtToAVSamFmt(sampleInfo.fmt));
  for (int i = 0; i < sampleInfo.nb_samples; i++) {
    for (int j = 0; j < sampleInfo.channels; j++) {
      memcpy(po + (i + sampleInfo.nb_samples * j) * sample_size, pi,
             sample_size);
      pi += sample_size;
    }
  }
}

} // namespace easymedia
