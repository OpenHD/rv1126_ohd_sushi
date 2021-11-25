// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_OGG_UTILS_H_
#define EASYMEDIA_OGG_UTILS_H_

extern "C" {
#include <ogg/ogg.h>
}

#include <list>

#include "buffer.h"

namespace easymedia {

std::shared_ptr<MediaBuffer>
PackOggPackets(const std::list<ogg_packet> &ogg_packets);
bool UnpackOggData(void *in_buffer, size_t in_size,
                   std::list<ogg_packet> &ogg_packets);

ogg_packet *ogg_packet_clone(const ogg_packet &orig);
int ogg_packet_free(ogg_packet *p);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_OGG_UTILS_H_
