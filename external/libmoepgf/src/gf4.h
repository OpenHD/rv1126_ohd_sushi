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

#ifndef _MOEPGF4_H_
#define _MOEPGF4_H_

#include <stdint.h>

uint8_t inv4(uint8_t element);

void maddrc4_flat_table(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_imul_scalar(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_imul_gpr32(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_imul_gpr64(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);

void mulrc4_imul_scalar(uint8_t *region, uint8_t constant, size_t length);
void mulrc4_imul_gpr32(uint8_t *region, uint8_t constant, size_t length);
void mulrc4_imul_gpr64(uint8_t *region, uint8_t constant, size_t length);

#ifdef __x86_64__
void mulrc4_imul_sse2(uint8_t *region, uint8_t constant, size_t length);
void mulrc4_imul_avx2(uint8_t *region, uint8_t constant, size_t length);
void mulrc4_shuffle_ssse3(uint8_t *region, uint8_t constant, size_t length);
void mulrc4_shuffle_avx2(uint8_t *region, uint8_t constant, size_t length);

void maddrc4_imul_sse2(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_imul_avx2(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_shuffle_ssse3(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_shuffle_avx2(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
#endif

#ifdef __arm__
void maddrc4_shuffle_neon_64(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_imul_neon_64(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc4_imul_neon_128(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void mulrc4_imul_neon_64(uint8_t *region, uint8_t constant, size_t length);
#endif

#endif
