// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ogg_utils.h"

#include <assert.h>
#include <string.h>

#include "utils.h"

namespace easymedia {

std::shared_ptr<MediaBuffer>
PackOggPackets(const std::list<ogg_packet> &ogg_packets) {
  size_t total_size = 0;
  for (const ogg_packet &packet : ogg_packets)
    total_size += (sizeof(ogg_packet) + packet.bytes);
  if (total_size == 0)
    return nullptr;
  auto out = MediaBuffer::Alloc(total_size);
  if (!out) {
    LOG_NO_MEMORY();
    return nullptr;
  }

  unsigned char *buffer = (unsigned char *)out->GetPtr();
  for (ogg_packet packet : ogg_packets) {
    unsigned char *packet_data = packet.packet;
    packet.packet = buffer + sizeof(packet);
    memcpy(buffer, &packet, sizeof(packet));
    buffer += sizeof(packet);
    memcpy(buffer, packet_data, packet.bytes);
    buffer += packet.bytes;
  }
  out->SetValidSize(total_size);
  return out;
}

bool UnpackOggData(void *in_buffer, size_t in_size,
                   std::list<ogg_packet> &ogg_packets) {
  if (!in_buffer || in_size < sizeof(ogg_packet))
    return false;
  unsigned char *buffer = (unsigned char *)in_buffer;
  size_t offset = 0;
  while (offset < in_size) {
    ogg_packet packet = *((ogg_packet *)(buffer + offset));
    assert(packet.packet == buffer + offset + sizeof(ogg_packet));
    offset += (sizeof(ogg_packet) + packet.bytes);
    ogg_packets.push_back(std::move(packet));
  }
  return true;
}

ogg_packet *ogg_packet_clone(const ogg_packet &orig) {
  if (orig.bytes <= 0)
    return nullptr;
  ogg_packet *new_packet = (ogg_packet *)malloc(sizeof(ogg_packet));
  if (!new_packet)
    return nullptr;
  *new_packet = orig;
  void *buffer = malloc(orig.bytes);
  if (!buffer) {
    free(new_packet);
    return nullptr;
  }
  memcpy(buffer, orig.packet, orig.bytes);
  new_packet->packet = (unsigned char *)buffer;

  return new_packet;
}

int ogg_packet_free(ogg_packet *p) {
  if (!p)
    return -1;
  if (p->packet)
    free(p->packet);
  free(p);
  return 0;
}

} // namespace easymedia
