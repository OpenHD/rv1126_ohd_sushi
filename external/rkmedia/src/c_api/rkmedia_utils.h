// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_UTILS_
#define __RKMEDIA_UTILS_

std::string ImageTypeToString(IMAGE_TYPE_E type);
IMAGE_TYPE_E StringToImageType(std::string type);
std::string CodecToString(CODEC_TYPE_E type);
std::string SampleFormatToString(SAMPLE_FORMAT_E type);
char *ModIdToString(MOD_ID_E mod_id);
#endif // #ifndef __RKMEDIA_UTILS_
