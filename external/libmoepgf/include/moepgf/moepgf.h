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

#ifndef __MOEPGF_H_
#define __MOEPGF_H_

#include <stdint.h>

/*
 * Maximum memory alignemnt used by kernels. Any memory regions supplied to the
 * library must be aligned to this value and the length of those regions must
 * be a multiple of this value.
 */
#define MOEPGF_MAX_ALIGNMENT 32

/*
 * Definitions for hardware SIMD capabilities.
 */
enum MOEPGF_HWCAPS
{
	MOEPGF_HWCAPS_SIMD_NONE		= 0,
	MOEPGF_HWCAPS_SIMD_MMX		= 1,
	MOEPGF_HWCAPS_SIMD_SSE		= 2,
	MOEPGF_HWCAPS_SIMD_SSE2		= 3,
	MOEPGF_HWCAPS_SIMD_SSSE3	= 4,
	MOEPGF_HWCAPS_SIMD_SSE41	= 5,
	MOEPGF_HWCAPS_SIMD_SSE42	= 6,
	MOEPGF_HWCAPS_SIMD_AVX		= 7,
	MOEPGF_HWCAPS_SIMD_AVX2		= 8,
	MOEPGF_HWCAPS_SIMD_NEON		= 9,
	MOEPGF_HWCAPS_SIMD_MSA		= 10,
	MOEPGF_HWCAPS_COUNT		= 11,
};

/*
 * Defines GF parameters. Do not change.
 */
#define MOEPGF2_POLYNOMIAL		3
#define MOEPGF2_EXPONENT		1
#define MOEPGF2_SIZE			(1 << MOEPGF2_EXPONENT)
#define MOEPGF2_MASK			(MOEPGF2_SIZE - 1)

#define MOEPGF4_POLYNOMIAL		7
#define MOEPGF4_EXPONENT		2
#define MOEPGF4_SIZE			(1 << MOEPGF4_EXPONENT)
#define MOEPGF4_MASK			(MOEPGF4_SIZE - 1)

#define MOEPGF16_POLYNOMIAL		19
#define MOEPGF16_EXPONENT		4
#define MOEPGF16_SIZE			(1 << MOEPGF16_EXPONENT)
#define MOEPGF16_MASK			(MOEPGF16_SIZE - 1)

#define MOEPGF256_POLYNOMIAL		285
#define MOEPGF256_EXPONENT		8
#define MOEPGF256_SIZE			(1 << MOEPGF256_EXPONENT)
#define MOEPGF256_MASK			(MOEPGF256_SIZE - 1)

typedef void	(*maddrc_t)	(uint8_t *, const uint8_t *, uint8_t, size_t);
typedef void	(*mulrc_t)	(uint8_t *, uint8_t, size_t);
typedef uint8_t	(*inv_t)	(uint8_t);

/*
 * Used to identify differen GFs.
 */
enum MOEPGF_TYPE {
	MOEPGF2		= 0,
	MOEPGF4		= 1,
	MOEPGF16	= 2,
	MOEPGF256	= 3,
	MOEPGF_COUNT
};

/*
 * Algorithms available ordered by type and required feature set.
 */
enum MOEPGF_ALGORITHM
{
	MOEPGF_SELFTEST		= 0,
	MOEPGF_XOR_SCALAR,
	MOEPGF_XOR_GPR32,
	MOEPGF_XOR_GPR64,
	MOEPGF_XOR_SSE2,
	MOEPGF_XOR_AVX2,
	MOEPGF_XOR_NEON_128,
	MOEPGF_XOR_MSA,
	MOEPGF_LOG_TABLE,
	MOEPGF_FLAT_TABLE,
	MOEPGF_IMUL_SCALAR,
	MOEPGF_IMUL_GPR32,
	MOEPGF_IMUL_GPR64,
	MOEPGF_IMUL_SSE2,
	MOEPGF_IMUL_AVX2,
	MOEPGF_IMUL_NEON_64,
	MOEPGF_IMUL_NEON_128,
	MOEPGF_SHUFFLE_SSSE3,
	MOEPGF_SHUFFLE_AVX2,
	MOEPGF_SHUFFLE_NEON_64,
	MOEPGF_ALGORITHM_BEST,
	MOEPGF_ALGORITHM_COUNT
};

/*
 * Structure representing a moepgf algorithm, including function poitners and
 * informations about the algorithm, required hwcaps and field.
 */
struct moepgf_algorithm {
	maddrc_t		maddrc;
	mulrc_t			mulrc;
	enum MOEPGF_HWCAPS	hwcaps;
	enum MOEPGF_ALGORITHM	type;
	enum MOEPGF_TYPE	field;
};

/*
 * Structure representing a GF, including functions to user-accessible
 * functions maddrc, mulrc, and inv.
 *
 * void maddrc(uint8_t * r1, const uint8_t *r2, uint8_t constant, size_t len)
 * Multiplies region r2 by constant and adds the result to region r1. The result
 * is stored in region r1. Length len specifies the length of the regions.
 *
 * void mulrc(uint8_t * r, uint8_t constant, size_t len)
 * Multiplies region r of length len by constant. The result is stored in
 * region r.
 *
 * uint8_t inv(uint8_t x)
 * Returns the inverse element of x.
 *
 *
 * IMPORTANT: If len is not a multiple of MOEPGF_MAX_ALIGNMENT, SIMD
 * implementations may silently access memory addresses up to the next multiple
 * of MOEPGF_MAX_ALIGNMENT. The rational behind this behavior is to allow for 
 * 1) easy usage since len may be the true length of data to be multiplied
 *    instead of the length of a zero-padded memory region, and
 * 2) efficient processing since no special treatment of the border cases are
 *    needed in the arithmetic kernels.
 *
 * All you have to obey is that one of the following conditions is met:
 * 1) Regions r1 and r2 must be aligned to MOEPGF_MAX_ALIGNMENT and the
 *    allocated memory regions must be a multiple of MOEPGF_MAX_ALIGNMENT.
 * 2) len must be a multiple of MOEPGF_MAX_ALIGNMENT.
 */
struct moepgf {
	enum MOEPGF_TYPE		type;
	enum MOEPGF_HWCAPS		hwcaps;
	char 				name[256];
	uint32_t			exponent;
	uint32_t			mask;
	uint32_t			ppoly;
	int				size;
	maddrc_t			maddrc;
	mulrc_t				mulrc;
	inv_t				inv;
};

/*
 * Translates an enum MOEPGF_ALGORITHM into a string. Not thread-safe.
 */
const char * moepgf_a2name(enum MOEPGF_ALGORITHM a);

/*
 * Checks for SIMD extensions offered by hardware. Return value is a bitmask
 * indicating available MOEPGF_HWCAPS.
 */
uint32_t moepgf_check_available_simd_extensions();

/*
 * Initializes the GF pointed to by gf according to the requested type. SIMD
 * extensions may be explicitly requested by the parameter fset. If fset is set
 * to zero, the fastest implementation available for the current architecture is
 * automatically determined. The function returns 0 on succes and -1 on any
 * error, e.g. the requested SIMD extensions are not available.
 */
int moepgf_init(struct moepgf *gf, enum MOEPGF_TYPE type,
						enum MOEPGF_ALGORITHM atype);

/*
 * Returns an array of all algorithms for the given field. Useful for benchmarks
 * only.
 */
struct moepgf_algorithm ** moepgf_get_algs(enum MOEPGF_TYPE type);

/*
 * Frees an array of algorithms previously initialized by moepgf_get_algs(). 
 * Pointer is unusable after calling this function.
 */
void moepgf_free_algs(struct moepgf_algorithm **algs);

static inline uint8_t
moepgf_rand(uint32_t *s)
{
	const uint32_t a = 214013;
	const uint32_t c = 2531011;

	*s = a * (*s) + c;

	return (uint8_t)(*s >> 16);
}

#endif // __MOEPGF_H_

