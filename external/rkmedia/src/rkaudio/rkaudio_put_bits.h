#ifndef EASYMEDIA_RKAUDIO_PUT_BITS_H_
#define EASYMEDIA_RKAUDIO_PUT_BITS_H_

#include <stddef.h>
#include <stdint.h>

extern "C" {
#define __STDC_CONSTANT_MACROS
#include "libavutil/avassert.h"
#include "libavutil/intreadwrite.h"
}

namespace easymedia {
typedef struct PutBitContext {
  uint32_t bit_buf;
  int bit_left;
  uint8_t *buf;
  uint8_t *buf_ptr;
  uint8_t *buf_end;
  int size_in_bits;
} PutBitContext;

/**
 * Initialize the PutBitContext s.
 *
 * @param buffer the buffer where to put bits
 * @param buffer_size the size in bytes of buffer
 */
static inline void init_put_bits(PutBitContext *s, uint8_t *buffer,
                                 int buffer_size) {
  if (buffer_size < 0) {
    buffer_size = 0;
    buffer = NULL;
  }

  s->size_in_bits = 8 * buffer_size;
  s->buf = buffer;
  s->buf_end = s->buf + buffer_size;
  s->buf_ptr = s->buf;
  s->bit_left = 32;
  s->bit_buf = 0;
}

/**
 * Write up to 31 bits into a bitstream.
 * Use put_bits32 to write 32 bits.
 */
static inline void put_bits(PutBitContext *s, int n, unsigned int value) {
  unsigned int bit_buf;
  int bit_left;

  av_assert2(n <= 31 && value < (1U << n));

  bit_buf = s->bit_buf;
  bit_left = s->bit_left;

/* XXX: optimize */
#ifdef BITSTREAM_WRITER_LE
  bit_buf |= value << (32 - bit_left);
  if (n >= bit_left) {
    if (3 < s->buf_end - s->buf_ptr) {
      AV_WL32(s->buf_ptr, bit_buf);
      s->buf_ptr += 4;
    } else {
      av_log(NULL, AV_LOG_ERROR, "Internal error, put_bits buffer too small\n");
      av_assert2(0);
    }
    bit_buf = value >> bit_left;
    bit_left += 32;
  }
  bit_left -= n;
#else
  if (n < bit_left) {
    bit_buf = (bit_buf << n) | value;
    bit_left -= n;
  } else {
    bit_buf <<= bit_left;
    bit_buf |= value >> (n - bit_left);
    if (3 < s->buf_end - s->buf_ptr) {
      AV_WB32(s->buf_ptr, bit_buf);
      s->buf_ptr += 4;
    } else {
      av_log(NULL, AV_LOG_ERROR, "Internal error, put_bits buffer too small\n");
      av_assert2(0);
    }
    bit_left += 32 - n;
    bit_buf = value;
  }
#endif

  s->bit_buf = bit_buf;
  s->bit_left = bit_left;
}

/**
 * Pad the end of the output stream with zeros.
 */
static inline void flush_put_bits(PutBitContext *s) {
#ifndef BITSTREAM_WRITER_LE
  if (s->bit_left < 32)
    s->bit_buf <<= s->bit_left;
#endif
  while (s->bit_left < 32) {
    av_assert0(s->buf_ptr < s->buf_end);
#ifdef BITSTREAM_WRITER_LE
    *s->buf_ptr++ = s->bit_buf;
    s->bit_buf >>= 8;
#else
    *s->buf_ptr++ = s->bit_buf >> 24;
    s->bit_buf <<= 8;
#endif
    s->bit_left += 8;
  }
  s->bit_left = 32;
  s->bit_buf = 0;
}

/**
 * @return the total number of bits written to the bitstream.
 */
static inline int put_bits_count(PutBitContext *s) {
  return (s->buf_ptr - s->buf) * 8 + 32 - s->bit_left;
}

} // namespace easymedia
#endif
