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

#include <arm_neon.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <moepgf/moepgf.h>

#include "gf4.h"
#include "xor.h"

#if MOEPGF4_POLYNOMIAL == 7
#include "gf4tables7.h"
#else
#error "Invalid prime polynomial or tables not available."
#endif

static const uint8_t pt[MOEPGF4_SIZE][MOEPGF4_EXPONENT] = MOEPGF4_POLYNOMIAL_DIV_TABLE;
static const uint8_t tl[MOEPGF4_SIZE][16] = MOEPGF4_SHUFFLE_LOW_TABLE;
static const uint8_t th[MOEPGF4_SIZE][16] = MOEPGF4_SHUFFLE_HIGH_TABLE;

void
maddrc4_shuffle_neon_64(uint8_t* region1, const uint8_t* region2,
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
maddrc4_imul_neon_64(uint8_t *region1, const uint8_t *region2, uint8_t constant,
								size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	register uint8x8_t reg1, reg2, ri[2], sp[2], mi[2];

	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_neon_64(region1, region2, length);
		return;
	}

	mi[0] = vdup_n_u8(0x55);
	mi[1] = vdup_n_u8(0xaa);
	sp[0] = vdup_n_u8(p[0]);
	sp[1] = vdup_n_u8(p[1]);

	for (end=region1+length; region1<end; region1+=8, region2+=8) {
		reg2 = vld1_u8((void *)region2);
		reg1 = vld1_u8((void *)region1);
		ri[0] = vand_u8(reg2, mi[0]);
		ri[1] = vand_u8(reg2, mi[1]);
		ri[1] = vshr_n_u8(ri[1], 1);
		ri[0] = vmul_u8(ri[0], sp[0]);
		ri[1] = vmul_u8(ri[1], sp[1]);
		ri[0] = veor_u8(ri[0], ri[1]);
		ri[0] = veor_u8(ri[0], reg1);
		vst1_u8((void *)region1, ri[0]);
	}
}

void
maddrc4_imul_neon_128(uint8_t *region1, const uint8_t *region2, uint8_t constant,
								size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	register uint8x16_t reg1, reg2, ri[2], sp[2], mi[2];

	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_neon_128(region1, region2, length);
		return;
	}

	mi[0] = vdupq_n_u8(0x55);
	mi[1] = vdupq_n_u8(0xaa);
	sp[0] = vdupq_n_u8(p[0]);
	sp[1] = vdupq_n_u8(p[1]);

	for (end=region1+length; region1<end; region1+=16, region2+=16) {
		reg2 = vld1q_u8((void *)region2);
		reg1 = vld1q_u8((void *)region1);
		ri[0] = vandq_u8(reg2, mi[0]);
		ri[1] = vandq_u8(reg2, mi[1]);
		ri[1] = vshrq_n_u8(ri[1], 1);
		ri[0] = vmulq_u8(ri[0], sp[0]);
		ri[1] = vmulq_u8(ri[1], sp[1]);
		ri[0] = veorq_u8(ri[0], ri[1]);
		ri[0] = veorq_u8(ri[0], reg1);
		vst1q_u8((void *)region1, ri[0]);
	}
}

void
mulrc4_imul_neon_64(uint8_t *region, uint8_t constant, size_t length)
{
	uint8_t *end;
	register uint8x8_t reg, ri[2], sp[2], mi[2];
	const uint8_t *p = pt[constant];

	if (constant == 0) {
		memset(region, 0, length);
		return;
	}

	if (constant == 1)
		return;
	
	mi[0] = vdup_n_u8(0x55);
	mi[1] = vdup_n_u8(0xaa);
	sp[0] = vdup_n_u8(p[0]);
	sp[1] = vdup_n_u8(p[1]);

	for (end=region+length; region<end; region+=8) {
		reg = vld1_u8((void *)region);
		ri[0] = vand_u8(reg, mi[0]);
		ri[1] = vand_u8(reg, mi[1]);
		ri[1] = vshr_n_u8(ri[1], 1);
		ri[0] = vmul_u8(ri[0], sp[0]);
		ri[1] = vmul_u8(ri[1], sp[1]);
		ri[0] = veor_u8(ri[0], ri[1]);
		vst1_u8((void *)region, ri[0]);
	}
}

