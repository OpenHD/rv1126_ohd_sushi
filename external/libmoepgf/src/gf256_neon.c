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

#include <arm_neon.h>

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
maddrc256_shuffle_neon_64(uint8_t *region1, const uint8_t *region2,
					uint8_t constant, size_t length)
{
	uint8_t *end;
	register uint8x8x2_t t1, t2;
	register uint8x8_t m1, m2, in1, in2, out, l, h;

	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_neon_128(region1, region2, length);
		return;
	}

	t1 = vld2_u8((void *)tl[constant]);
	t2 = vld2_u8((void *)th[constant]);
	m1 = vdup_n_u8(0x0f);
	m2 = vdup_n_u8(0xf0);

	for (end=region1+length; region1<end; region1+=8, region2+=8) {
		in2 = vld1_u8((void *)region2);
		in1 = vld1_u8((void *)region1);
		l = vand_u8(in2, m1);
		l = vtbl2_u8(t1, l);
		h = vand_u8(in2, m2);
		h = vshr_n_u8(h, 4);
		h = vtbl2_u8(t2, h);
		out = veor_u8(h, l);
		out = veor_u8(out, in1);
		vst1_u8(region1, out);
	}
}

void
maddrc256_imul_neon_64(uint8_t *region1, const uint8_t *region2,
					uint8_t constant, size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	register uint8x8_t mi[8], sp[8], ri[8], reg1, reg2;

	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_neon_64(region1, region2, length);
		return;
	}

	mi[0] = vdup_n_u8(0x01);
	mi[1] = vdup_n_u8(0x02);
	mi[2] = vdup_n_u8(0x04);
	mi[3] = vdup_n_u8(0x08);
	mi[4] = vdup_n_u8(0x10);
	mi[5] = vdup_n_u8(0x20);
	mi[6] = vdup_n_u8(0x40);
	mi[7] = vdup_n_u8(0x80);

	sp[0] = vdup_n_u8(p[0]);
	sp[1] = vdup_n_u8(p[1]);
	sp[2] = vdup_n_u8(p[2]);
	sp[3] = vdup_n_u8(p[3]);
	sp[4] = vdup_n_u8(p[4]);
	sp[5] = vdup_n_u8(p[5]);
	sp[6] = vdup_n_u8(p[6]);
	sp[7] = vdup_n_u8(p[7]);

	for (end=region1+length; region1<end; region1+=8, region2+=8) {
		reg2 = vld1_u8((void *)region2);
		reg1 = vld1_u8((void *)region1);

		ri[0] = vand_u8(reg2, mi[0]);
		ri[1] = vand_u8(reg2, mi[1]);
		ri[2] = vand_u8(reg2, mi[2]);
		ri[3] = vand_u8(reg2, mi[3]);
		ri[4] = vand_u8(reg2, mi[4]);
		ri[5] = vand_u8(reg2, mi[5]);
		ri[6] = vand_u8(reg2, mi[6]);
		ri[7] = vand_u8(reg2, mi[7]);

		ri[1] = vshr_n_u8(ri[1], 1);
		ri[2] = vshr_n_u8(ri[2], 2);
		ri[3] = vshr_n_u8(ri[3], 3);
		ri[4] = vshr_n_u8(ri[4], 4);
		ri[5] = vshr_n_u8(ri[5], 5);
		ri[6] = vshr_n_u8(ri[6], 6);
		ri[7] = vshr_n_u8(ri[7], 7);

		ri[0] = vmul_u8(ri[0], sp[0]);
		ri[1] = vmul_u8(ri[1], sp[1]);
		ri[2] = vmul_u8(ri[2], sp[2]);
		ri[3] = vmul_u8(ri[3], sp[3]);
		ri[4] = vmul_u8(ri[4], sp[4]);
		ri[5] = vmul_u8(ri[5], sp[5]);
		ri[6] = vmul_u8(ri[6], sp[6]);
		ri[7] = vmul_u8(ri[7], sp[7]);

		ri[0] = veor_u8(ri[0], ri[1]);
		ri[2] = veor_u8(ri[2], ri[3]);
		ri[4] = veor_u8(ri[4], ri[5]);
		ri[6] = veor_u8(ri[6], ri[7]);
		ri[0] = veor_u8(ri[0], ri[2]);
		ri[4] = veor_u8(ri[4], ri[6]);
		ri[0] = veor_u8(ri[0], ri[4]);
		ri[0] = veor_u8(ri[0], reg1);

		vst1_u8(region1, ri[0]);
	}
}

void
maddrc256_imul_neon_128(uint8_t *region1, const uint8_t *region2,
					uint8_t constant, size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	register uint8x16_t mi[8], sp[8], ri[8], reg1, reg2;

	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_neon_128(region1, region2, length);
		return;
	}
	
	mi[0] = vdupq_n_u8(0x01);
	mi[1] = vdupq_n_u8(0x02);
	mi[2] = vdupq_n_u8(0x04);
	mi[3] = vdupq_n_u8(0x08);
	mi[4] = vdupq_n_u8(0x10);
	mi[5] = vdupq_n_u8(0x20);
	mi[6] = vdupq_n_u8(0x40);
	mi[7] = vdupq_n_u8(0x80);

	sp[0] = vdupq_n_u8(p[0]);
	sp[1] = vdupq_n_u8(p[1]);
	sp[2] = vdupq_n_u8(p[2]);
	sp[3] = vdupq_n_u8(p[3]);
	sp[4] = vdupq_n_u8(p[4]);
	sp[5] = vdupq_n_u8(p[5]);
	sp[6] = vdupq_n_u8(p[6]);
	sp[7] = vdupq_n_u8(p[7]);

	for (end=region1+length; region1<end; region1+=16, region2+=16) {
		reg2 = vld1q_u8((void *)region2);
		reg1 = vld1q_u8((void *)region1);

		ri[0] = vandq_u8(reg2, mi[0]);
		ri[1] = vandq_u8(reg2, mi[1]);
		ri[2] = vandq_u8(reg2, mi[2]);
		ri[3] = vandq_u8(reg2, mi[3]);
		ri[4] = vandq_u8(reg2, mi[4]);
		ri[5] = vandq_u8(reg2, mi[5]);
		ri[6] = vandq_u8(reg2, mi[6]);
		ri[7] = vandq_u8(reg2, mi[7]);

		ri[1] = vshrq_n_u8(ri[1], 1);
		ri[2] = vshrq_n_u8(ri[2], 2);
		ri[3] = vshrq_n_u8(ri[3], 3);
		ri[4] = vshrq_n_u8(ri[4], 4);
		ri[5] = vshrq_n_u8(ri[5], 5);
		ri[6] = vshrq_n_u8(ri[6], 6);
		ri[7] = vshrq_n_u8(ri[7], 7);

		ri[0] = vmulq_u8(ri[0], sp[0]);
		ri[1] = vmulq_u8(ri[1], sp[1]);
		ri[2] = vmulq_u8(ri[2], sp[2]);
		ri[3] = vmulq_u8(ri[3], sp[3]);
		ri[4] = vmulq_u8(ri[4], sp[4]);
		ri[5] = vmulq_u8(ri[5], sp[5]);
		ri[6] = vmulq_u8(ri[6], sp[6]);
		ri[7] = vmulq_u8(ri[7], sp[7]);

		ri[0] = veorq_u8(ri[0], ri[1]);
		ri[2] = veorq_u8(ri[2], ri[3]);
		ri[4] = veorq_u8(ri[4], ri[5]);
		ri[6] = veorq_u8(ri[6], ri[7]);
		ri[0] = veorq_u8(ri[0], ri[2]);
		ri[4] = veorq_u8(ri[4], ri[6]);
		ri[0] = veorq_u8(ri[0], ri[4]);
		ri[0] = veorq_u8(ri[0], reg1);

		vst1q_u8(region1, ri[0]);
	}
}

void
mulrc256_shuffle_neon_64(uint8_t *region, uint8_t constant, size_t length)
{
	uint8_t *end;
	register uint8x8x2_t t1, t2;
	register uint8x8_t m1, m2, in, out, l, h;

	if (constant == 0) {
		memset(region, 0, length);
		return;
	}

	if (constant == 1)
		return;

	t1 = vld2_u8((void *)tl[constant]);
	t2 = vld2_u8((void *)th[constant]);
	m1 = vdup_n_u8(0x0f);
	m2 = vdup_n_u8(0xf0);

	for (end=region+length; region<end; region+=8) {
		in = vld1_u8((void *)region);
		l = vand_u8(in, m1);
		l = vtbl2_u8(t1, l);
		h = vand_u8(in, m2);
		h = vshr_n_u8(h, 4);
		h = vtbl2_u8(t2, h);
		out = veor_u8(h, l);
		vst1_u8((void *)region, out);
	}
}

