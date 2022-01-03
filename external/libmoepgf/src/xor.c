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

#include "xor.h"

void
xorr_scalar(uint8_t *region1, const uint8_t *region2, size_t length)
{
	for(; length; region1++, region2++, length--)
		*region1 ^= *region2;
}

void
xorr_gpr32(uint8_t *region1, const uint8_t *region2, size_t length)
{
	uint8_t *end = region1 + length;

	for(; region1 < end; region1+=4, region2+=4)
		*(uint32_t *)region1 ^= *(uint32_t *)region2;
}

void
xorr_gpr64(uint8_t *region1, const uint8_t *region2, size_t length)
{
	uint8_t *end = region1 + length;

	for(; region1 < end; region1+=8, region2+=8)
		*(uint64_t *)region1 ^= *(uint64_t *)region2;
}
