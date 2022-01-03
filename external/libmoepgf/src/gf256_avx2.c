/*
 * This file is part of moep80211gf.
 *
 * Copyright (C) 2014   Stephan M. Guenther <moepi@moepi.net>
 * Copyright (C) 2014   Maximilian Riemensberger <riemensberger@tum.de>
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

#include <immintrin.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <moepgf/moepgf.h>

#include "gf256.h"
#include "xor.h"

#if MOEPGF256_POLYNOMIAL == 285
#include "gf256tables285.h"
#else
#error "Invalid prime polynomial or tables not available."
#endif

static const uint8_t pt[MOEPGF256_SIZE][MOEPGF256_EXPONENT] = MOEPGF256_POLYNOMIAL_DIV_TABLE;
static const uint8_t tl[MOEPGF256_SIZE][16] = MOEPGF256_SHUFFLE_LOW_TABLE;
static const uint8_t th[MOEPGF256_SIZE][16] = MOEPGF256_SHUFFLE_HIGH_TABLE;

void
maddrc256_shuffle_avx2(uint8_t *region1, const uint8_t *region2,
					uint8_t constant, size_t length)
{
	uint8_t *end;
	register __m256i t1, t2, m1, m2, in1, in2, out, l, h;
	register __m128i bc;

	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_avx2(region1, region2, length);
		return;
	}

	bc = _mm_load_si128((void *)tl[constant]);
	t1 = __builtin_ia32_vbroadcastsi256(bc);
	bc = _mm_load_si128((void *)th[constant]);
	t2 = __builtin_ia32_vbroadcastsi256(bc);
	m1 = _mm256_set1_epi8(0x0f);
	m2 = _mm256_set1_epi8(0xf0);

	for (end=region1+length; region1<end; region1+=32, region2+=32) {
		in2 = _mm256_load_si256((void *)region2);
		in1 = _mm256_load_si256((void *)region1);
		l = _mm256_and_si256(in2, m1);
		l = _mm256_shuffle_epi8(t1, l);
		h = _mm256_and_si256(in2, m2);
		h = _mm256_srli_epi64(h, 4);
		h = _mm256_shuffle_epi8(t2, h);
		out = _mm256_xor_si256(h, l);
		out = _mm256_xor_si256(out, in1);
		_mm256_store_si256((void *)region1, out);
	}
}

void
maddrc256_imul_avx2(uint8_t *region1, const uint8_t *region2,
					uint8_t constant, size_t length)
{
	uint8_t *end;
	register __m256i ri[8], mi[8], sp[8], reg1, reg2;
	const uint8_t *p = pt[constant];
	
	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_avx2(region1, region2, length);
		return;
	}
	
	mi[0] = _mm256_set1_epi8(0x01);
	mi[1] = _mm256_set1_epi8(0x02);
	mi[2] = _mm256_set1_epi8(0x04);
	mi[3] = _mm256_set1_epi8(0x08);
	mi[4] = _mm256_set1_epi8(0x10);
	mi[5] = _mm256_set1_epi8(0x20);
	mi[6] = _mm256_set1_epi8(0x40);
	mi[7] = _mm256_set1_epi8(0x80);

	sp[0] = _mm256_set1_epi16(p[0]);
	sp[1] = _mm256_set1_epi16(p[1]);
	sp[2] = _mm256_set1_epi16(p[2]);
	sp[3] = _mm256_set1_epi16(p[3]);
	sp[4] = _mm256_set1_epi16(p[4]);
	sp[5] = _mm256_set1_epi16(p[5]);
	sp[6] = _mm256_set1_epi16(p[6]);
	sp[7] = _mm256_set1_epi16(p[7]);

	for (end=region1+length; region1<end; region1+=32, region2+=32) {
		reg1 = _mm256_load_si256((void *)region1);
		reg2 = _mm256_load_si256((void *)region2);

		ri[0] = _mm256_and_si256(reg2, mi[0]);
		ri[1] = _mm256_and_si256(reg2, mi[1]);
		ri[2] = _mm256_and_si256(reg2, mi[2]);
		ri[3] = _mm256_and_si256(reg2, mi[3]);
		ri[4] = _mm256_and_si256(reg2, mi[4]);
		ri[5] = _mm256_and_si256(reg2, mi[5]);
		ri[6] = _mm256_and_si256(reg2, mi[6]);
		ri[7] = _mm256_and_si256(reg2, mi[7]);

		ri[1] = _mm256_srli_epi16(ri[1], 1);
		ri[2] = _mm256_srli_epi16(ri[2], 2);
		ri[3] = _mm256_srli_epi16(ri[3], 3);
		ri[4] = _mm256_srli_epi16(ri[4], 4);
		ri[5] = _mm256_srli_epi16(ri[5], 5);
		ri[6] = _mm256_srli_epi16(ri[6], 6);
		ri[7] = _mm256_srli_epi16(ri[7], 7);

		ri[0] = _mm256_mullo_epi16(ri[0], sp[0]);
		ri[1] = _mm256_mullo_epi16(ri[1], sp[1]);
		ri[2] = _mm256_mullo_epi16(ri[2], sp[2]);
		ri[3] = _mm256_mullo_epi16(ri[3], sp[3]);
		ri[4] = _mm256_mullo_epi16(ri[4], sp[4]);
		ri[5] = _mm256_mullo_epi16(ri[5], sp[5]);
		ri[6] = _mm256_mullo_epi16(ri[6], sp[6]);
		ri[7] = _mm256_mullo_epi16(ri[7], sp[7]);

		ri[0] = _mm256_xor_si256(ri[0], ri[1]);
		ri[2] = _mm256_xor_si256(ri[2], ri[3]);
		ri[4] = _mm256_xor_si256(ri[4], ri[5]);
		ri[6] = _mm256_xor_si256(ri[6], ri[7]);
		ri[0] = _mm256_xor_si256(ri[0], ri[2]);
		ri[4] = _mm256_xor_si256(ri[4], ri[6]);
		ri[0] = _mm256_xor_si256(ri[0], ri[4]);
		ri[0] = _mm256_xor_si256(ri[0], reg1);

		_mm256_store_si256((void *)region1, ri[0]);
	}
}

void
mulrc256_shuffle_avx2(uint8_t *region, uint8_t constant, size_t length)
{
	uint8_t *end;
	register __m256i t1, t2, m1, m2, in, out, l, h;
	register __m128i bc;

	if (constant == 0) {
		memset(region, 0, length);
		return;
	}

	if (constant == 1)
		return;

	bc = _mm_load_si128((void *)tl[constant]);
	t1 = __builtin_ia32_vbroadcastsi256(bc);
	bc = _mm_load_si128((void *)th[constant]);
	t2 = __builtin_ia32_vbroadcastsi256(bc);
	m1 = _mm256_set1_epi8(0x0f);
	m2 = _mm256_set1_epi8(0xf0);

	for (end=region+length; region<end; region+=32) {
		in = _mm256_load_si256((void *)region);
		l = _mm256_and_si256(in, m1);
		l = _mm256_shuffle_epi8(t1, l);
		h = _mm256_and_si256(in, m2);
		h = _mm256_srli_epi64(h, 4);
		h = _mm256_shuffle_epi8(t2, h);
		out = _mm256_xor_si256(h, l);
		_mm256_store_si256((void *)region, out);
	}
}

