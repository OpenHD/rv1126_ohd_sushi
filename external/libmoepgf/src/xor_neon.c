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

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "gf2.h"
#include "gf4.h"
#include "gf16.h"
#include "gf256.h"

void
xorr_neon_64(uint8_t *region1, const uint8_t *region2, size_t length)
{
	uint8_t *end;
	register uint64x1_t in, out;

	for (end=region1+length; region1<end; region1+=8, region2+=8) {
		in  = vld1_u64((void *)region2);
		out = vld1_u64((void *)region1);
		out = veor_u64(in, out);
		vst1_u64((void *)region1, out);
	}
}

void
xorr_neon_128(uint8_t *region1, const uint8_t *region2, size_t length)
{
	uint8_t *end;
	register uint64x2_t in, out;

	for (end=region1+length; region1<end; region1+=16, region2+=16) {
		in  = vld1q_u64((void *)region2);
		out = vld1q_u64((void *)region1);
		out = veorq_u64(in, out);
		vst1q_u64((void *)region1, out);
	}
}

