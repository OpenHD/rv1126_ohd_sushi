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

static const uint8_t inverses[MOEPGF4_SIZE] = MOEPGF4_INV_TABLE;
static const uint8_t pt[MOEPGF4_SIZE][MOEPGF16_EXPONENT] = MOEPGF4_POLYNOMIAL_DIV_TABLE;
static const uint8_t multab[MOEPGF4_SIZE][256] = MOEPGF4_LOOKUP_TABLE;

inline uint8_t
inv4(uint8_t element)
{
	return inverses[element];
}

inline void
maddrc4_imul_scalar(uint8_t* region1, const uint8_t* region2,
					uint8_t constant, size_t length)
{
	const uint8_t *p = pt[constant];
	uint8_t r[4];

	if (constant == 0)
		return;

	if (constant == 1) {
		xorr_scalar(region1, region2, length);
		return;
	}

	for (; length; region1++, region2++, length--) {
		r[0] = ((*region2 & 0x55) >> 0) * p[0];
		r[1] = ((*region2 & 0xaa) >> 1) * p[1];
		*region1 ^= r[0] ^ r[1];
	}
}

inline void
maddrc4_imul_gpr32(uint8_t *region1, const uint8_t *region2, uint8_t constant, 
								size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	uint32_t r64[4];

	if (constant == 0)
	       return;

        if (constant == 1) {
	       xorr_gpr32(region1, region2, length);
	       return;
        }

	for (end=region1+length; region1<end; region1+=4, region2+=4) {
		r64[0] = ((*(uint32_t *)region2 & 0x55555555)>>0)*p[0];
		r64[1] = ((*(uint32_t *)region2 & 0xaaaaaaaa)>>1)*p[1];
		*((uint32_t *)region1) ^= r64[0] ^ r64[1];
	}
}

inline void
maddrc4_imul_gpr64(uint8_t *region1, const uint8_t *region2, uint8_t constant, 
								size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	uint64_t r64[4];

	if (constant == 0)
	       return;

        if (constant == 1) {
	       xorr_gpr64(region1, region2, length);
	       return;
        }

	for (end=region1+length; region1<end; region1+=8, region2+=8) {
		r64[0] = ((*(uint64_t *)region2 & 0x5555555555555555)>>0)*p[0];
		r64[1] = ((*(uint64_t *)region2 & 0xaaaaaaaaaaaaaaaa)>>1)*p[1];
		*((uint64_t *)region1) ^= r64[0] ^ r64[1];
	}
}

inline void
maddrc4_flat_table(uint8_t *region1, const uint8_t *region2, uint8_t constant,
								size_t length)
{
	if (constant == 0)
	       return;

        if (constant == 1) {
		xorr_scalar(region1, region2, length);
	       return;
        }

	for (; length; region1++, region2++, length--) {
		*region1 ^= multab[constant][*region2];
	}
}

void
mulrc4_imul_scalar(uint8_t *region, uint8_t constant, size_t length)
{
	const uint8_t *p = pt[constant];
	uint8_t r[4];

	if (constant == 0) {
		memset(region, 0, length);
		return;
	}

	if (constant == 1)
		return;

	for (; length; region++, length--) {
		r[0] = ((*region & 0x55) >> 0) * p[0];
		r[1] = ((*region & 0xaa) >> 1) * p[1];
		*region = r[0] ^ r[1];
	}
}

void
mulrc4_imul_gpr32(uint8_t *region, uint8_t constant, size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	uint64_t r64[2];

	if (constant == 0) {
		memset(region, 0, length);
		return;
	}

	if (constant == 1)
		return;
	
	for (end=region+length; region<end; region+=4, length-=4) {
		r64[0] = ((*(uint32_t *)region & 0x55555555)>>0)*p[0];
		r64[1] = ((*(uint32_t *)region & 0xaaaaaaaa)>>1)*p[1];
		*((uint32_t *)region) = r64[0] ^ r64[1];
	}
}

void
mulrc4_imul_gpr64(uint8_t *region, uint8_t constant, size_t length)
{
	uint8_t *end;
	const uint8_t *p = pt[constant];
	uint64_t r64[2];

	if (constant == 0) {
		memset(region, 0, length);
		return;
	}

	if (constant == 1)
		return;
	
	for (end=region+length; region<end; region+=8) {
		r64[0] = ((*(uint64_t *)region & 0x5555555555555555)>>0)*p[0];
		r64[1] = ((*(uint64_t *)region & 0xaaaaaaaaaaaaaaaa)>>1)*p[1];
		*((uint64_t *)region) = r64[0] ^ r64[1];
	}
}

