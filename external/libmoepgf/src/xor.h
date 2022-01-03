/*
 * This file is part of moep80211gf.
 *
 * Copyright (C) 2014   Stephan M. Guenther <moepi@moepi.net>
 * Copyright (C) 2014   Maximilian Riemensberger <riemensberger@tum.de>
 * Copyright (C) 2013   Alexander Kurtz <alexander@kurtz.be>
 *
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <https://www.gnu.org/licenses/>
 *
 */

#ifndef _XOR_H_
#define _XOR_H_

#include <stdint.h>
#include <sys/types.h>

/*
 * These functions should not be called directly. Use the function pointers
 * provided by the galois_field structure instead.
 */
void xorr_scalar(uint8_t *region1, const uint8_t *region2, size_t length);
void xorr_gpr32(uint8_t *region1, const uint8_t *region2, size_t length);
void xorr_gpr64(uint8_t *region1, const uint8_t *region2, size_t length);

#ifdef __x86_64__
void xorr_sse2(uint8_t *region1, const uint8_t *region2, size_t length);
void xorr_avx2(uint8_t *region1, const uint8_t *region2, size_t length);
#endif

#ifdef __arm__
void xorr_neon_64(uint8_t *region1, const uint8_t *region2, size_t length);
void xorr_neon_128(uint8_t *region1, const uint8_t *region2, size_t length);
#endif

#endif // _XOR_H_
