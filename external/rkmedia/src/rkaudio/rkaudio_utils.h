// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_RKAUDIO_UTILS_H_
#define EASYMEDIA_RKAUDIO_UTILS_H_

extern "C" {
#define __STDC_CONSTANT_MACROS
}

#include "image.h"
#include "media_type.h"
#include "sound.h"

namespace easymedia {

enum AVPixelFormat PixFmtToAVPixFmt(PixelFormat fmt);
PixelFormat AVPixFmtToPixFmt(AVPixelFormat fmt);

enum AVCodecID SampleFmtToAVCodecID(SampleFormat fmt);
enum AVSampleFormat SampleFmtToAVSamFmt(SampleFormat sfmt);

enum AVCodecID CodecTypeToAVCodecID(CodecType fmt);

void PrintAVError(int err, const char *log, const char *mark);
void conv_AV_SAMPLE_FMT_FLT_to_AV_SAMPLE_FMT_S16(uint8_t *po, const uint8_t *pi,
                                                 int is, int os, uint8_t *end);
void conv_AV_SAMPLE_FMT_S16_to_AV_SAMPLE_FMT_FLT(uint8_t *po, const uint8_t *pi,
                                                 int is, int os, uint8_t *end);
void conv_planar_to_package(uint8_t *po, uint8_t **pi, SampleInfo sampleInfo);
void conv_package_to_planar(uint8_t *po, uint8_t *pi, SampleInfo sampleInfo);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_RKAUDIO_UTILS_H_
