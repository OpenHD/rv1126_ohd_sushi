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

#ifndef _MOEPGF256_H_
#define _MOEPGF256_H_

#include <stdint.h>

uint8_t inv256(uint8_t element);

void maddrc256_pdiv(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_log_table(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_flat_table(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_imul_gpr32(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_imul_gpr64(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);

void mulrc256_pdiv(uint8_t *region, uint8_t constant, size_t length);
void mulrc256_imul_gpr32(uint8_t *region, uint8_t constant, size_t length);
void mulrc256_imul_gpr64(uint8_t *region, uint8_t constant, size_t length);

#ifdef __x86_64__
void maddrc256_imul_sse2(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_imul_avx2(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_shuffle_ssse3(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_shuffle_avx2(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);

void mulrc256_imul_sse2(uint8_t *region, uint8_t constant, size_t length);
void mulrc256_shuffle_ssse3(uint8_t *region, uint8_t constant, size_t length);
void mulrc256_shuffle_avx2(uint8_t *region, uint8_t constant, size_t length);
#endif

#ifdef __arm__
void maddrc256_shuffle_neon_64(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_imul_neon_64(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);
void maddrc256_imul_neon_128(uint8_t *region1, const uint8_t *region2, uint8_t constant, size_t length);

void mulrc256_shuffle_neon_64(uint8_t *region, uint8_t constant, size_t length);
#endif

#endif
