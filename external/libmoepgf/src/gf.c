/*
 * This file is part of moep80211gf.
 *
 * Copyright (C) 2014   Stephan M. Guenther <moepi@moepi.net>
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

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <moepgf/moepgf.h>

#ifdef __arm__
#include "detect_arm_neon.h"
#endif
#ifdef __x86_64__
#include "detect_x86_simd.h"
#endif
#ifdef __mips__
#endif

#include "gf2.h"
#include "gf4.h"
#include "gf16.h"
#include "gf256.h"
#include "xor.h"

const char *gf_names[] =
{
	[MOEPGF_SELFTEST]		= "selftest",
	[MOEPGF_XOR_SCALAR]		= "xor_scalar",
	[MOEPGF_XOR_GPR32]		= "xor_gpr32",
	[MOEPGF_XOR_GPR64]		= "xor_gpr64",
	[MOEPGF_XOR_SSE2]		= "xor_sse2",
	[MOEPGF_XOR_AVX2]		= "xor_avx2",
	[MOEPGF_XOR_NEON_128]		= "xor_neon_128",
	[MOEPGF_XOR_MSA]		= "xor_msa",
	[MOEPGF_LOG_TABLE]		= "log_table",
	[MOEPGF_FLAT_TABLE]		= "flat_table",
	[MOEPGF_IMUL_SCALAR]		= "imul_scalar",
	[MOEPGF_IMUL_GPR32]		= "imul_gpr32",
	[MOEPGF_IMUL_GPR64]		= "imul_gpr64",
	[MOEPGF_IMUL_SSE2]		= "imul_sse2",
	[MOEPGF_IMUL_AVX2]		= "imul_avx2",
	[MOEPGF_IMUL_NEON_64]		= "imul_neon_64",
	[MOEPGF_IMUL_NEON_128]		= "imul_neon_128",
	[MOEPGF_SHUFFLE_SSSE3]		= "shuffle_ssse3",
	[MOEPGF_SHUFFLE_AVX2]		= "shuffle_avx2",
	[MOEPGF_SHUFFLE_NEON_64]	= "shuffle_neon_64"
};

const struct {
	mulrc_t		mulrc;
	maddrc_t	maddrc;
} best_algorithms[MOEPGF_COUNT][MOEPGF_HWCAPS_COUNT] = {
	[MOEPGF2][MOEPGF_HWCAPS_SIMD_NONE]  = {
		.mulrc	= mulrc2,
		.maddrc	= maddrc2_gpr64
	},
#ifdef __x86_64__
	[MOEPGF2][MOEPGF_HWCAPS_SIMD_SSE2]  = {
		.mulrc	= mulrc2,
		.maddrc	= maddrc2_sse2
	},
	[MOEPGF2][MOEPGF_HWCAPS_SIMD_SSSE3] = {
		.mulrc	= mulrc2,
		.maddrc	= maddrc2_sse2
	},
	[MOEPGF2][MOEPGF_HWCAPS_SIMD_AVX2]  = {
		.mulrc	= mulrc2,
		.maddrc	= maddrc2_avx2
	},
#endif
#ifdef __arm__
	[MOEPGF2][MOEPGF_HWCAPS_SIMD_NEON]  = {
		.mulrc	= mulrc2,
		.maddrc	= maddrc2_neon
	},
#endif

	[MOEPGF4][MOEPGF_HWCAPS_SIMD_NONE]  = {
		.mulrc	= mulrc4_imul_gpr64,
		.maddrc	= maddrc4_imul_gpr64
	},
#ifdef __x86_64__
	[MOEPGF4][MOEPGF_HWCAPS_SIMD_SSE2]  = {
		.mulrc	= mulrc4_imul_sse2,
		.maddrc	= maddrc4_imul_sse2
	},
	[MOEPGF4][MOEPGF_HWCAPS_SIMD_SSSE3] = {
		.mulrc	= mulrc4_shuffle_ssse3,
		.maddrc	= maddrc4_shuffle_ssse3
	},
	[MOEPGF4][MOEPGF_HWCAPS_SIMD_AVX2]  = {
		.mulrc	= mulrc4_shuffle_avx2,
		.maddrc	= maddrc4_shuffle_avx2
	},
#endif
#ifdef __arm__
	[MOEPGF4][MOEPGF_HWCAPS_SIMD_NEON]  = {
		.mulrc	= mulrc4_imul_neon_64,
		.maddrc	= maddrc4_imul_neon_128
	},
#endif

	[MOEPGF16][MOEPGF_HWCAPS_SIMD_NONE]  = {
		.mulrc	= mulrc16_imul_gpr64,
		.maddrc	= maddrc16_imul_gpr64
	},
#ifdef __x86_64__
	[MOEPGF16][MOEPGF_HWCAPS_SIMD_SSE2]  = {
		.mulrc	= mulrc16_imul_sse2,
		.maddrc	= maddrc16_imul_sse2
	},
	[MOEPGF16][MOEPGF_HWCAPS_SIMD_SSSE3] = {
		.mulrc	= mulrc16_shuffle_ssse3,
		.maddrc	= maddrc16_shuffle_ssse3
	},
	[MOEPGF16][MOEPGF_HWCAPS_SIMD_AVX2]  = {
		.mulrc	= mulrc16_shuffle_avx2,
		.maddrc	= maddrc16_shuffle_avx2
	},
#endif
#ifdef __arm__
	[MOEPGF16][MOEPGF_HWCAPS_SIMD_NEON]  = {
		.mulrc	= mulrc16_shuffle_neon_64,
		.maddrc	= maddrc16_shuffle_neon_64
	},
#endif

	[MOEPGF256][MOEPGF_HWCAPS_SIMD_NONE]  = {
		.mulrc	= mulrc256_imul_gpr64,
		.maddrc	= maddrc256_imul_gpr64
	},
#ifdef __x86_64__
	[MOEPGF256][MOEPGF_HWCAPS_SIMD_SSE2]  = {
		.mulrc	= mulrc256_imul_sse2,
		.maddrc	= maddrc256_imul_sse2
	},
	[MOEPGF256][MOEPGF_HWCAPS_SIMD_SSSE3] = {
		.mulrc	= mulrc256_shuffle_ssse3,
		.maddrc	= maddrc256_shuffle_ssse3
	},
	[MOEPGF256][MOEPGF_HWCAPS_SIMD_AVX2]  = {
		.mulrc	= mulrc256_shuffle_avx2,
		.maddrc	= maddrc256_shuffle_avx2
	},
#endif
#ifdef __arm__
	[MOEPGF256][MOEPGF_HWCAPS_SIMD_NEON]  = {
		.mulrc	= mulrc256_shuffle_neon_64,
		.maddrc	= maddrc256_shuffle_neon_64
	},
#endif
};


const char *
moepgf_a2name(enum MOEPGF_ALGORITHM a)
{
	if (a > MOEPGF_ALGORITHM_COUNT-2)
		return NULL;

	return gf_names[a];
}

uint32_t
moepgf_check_available_simd_extensions()
{
	uint32_t ret = (1 << MOEPGF_HWCAPS_SIMD_NONE);

#ifdef __x86_64__
	ret |= detect_x86_simd();
#endif

#ifdef __arm__
	ret |= detect_arm_neon();
#endif

	return ret;
}

int
moepgf_init(struct moepgf *gf, enum MOEPGF_TYPE type, enum MOEPGF_ALGORITHM atype)
{
	int ret = 0;
	int hwcaps;

	memset(gf, 0, sizeof(*gf));

	hwcaps = moepgf_check_available_simd_extensions();

	switch (type) {
		gf->hwcaps = (1 << MOEPGF_HWCAPS_SIMD_NONE);
	case MOEPGF2:
		strcpy(gf->name, "MOEPGF2");
		gf->type		= MOEPGF2;
		gf->ppoly		= MOEPGF2_POLYNOMIAL;
		gf->exponent		= MOEPGF2_EXPONENT;
		gf->size		= MOEPGF2_SIZE;
		gf->mask		= MOEPGF2_MASK;
		gf->inv			= inv2;
		break;

	case MOEPGF4:
		strcpy(gf->name, "MOEPGF4");
		gf->type		= MOEPGF4;
		gf->ppoly		= MOEPGF4_POLYNOMIAL;
		gf->exponent		= MOEPGF4_EXPONENT;
		gf->size		= MOEPGF4_SIZE;
		gf->mask		= MOEPGF4_MASK;
		gf->inv			= inv4;
		break;

	case MOEPGF16:
		strcpy(gf->name, "MOEPGF16");
		gf->type		= MOEPGF16;
		gf->ppoly		= MOEPGF16_POLYNOMIAL;
		gf->exponent		= MOEPGF16_EXPONENT;
		gf->size		= MOEPGF16_SIZE;
		gf->mask		= MOEPGF16_MASK;
		gf->inv			= inv16;
		break;

	case MOEPGF256:
		strcpy(gf->name, "MOEPGF256");
		gf->type		= MOEPGF256;
		gf->ppoly		= MOEPGF256_POLYNOMIAL;
		gf->exponent		= MOEPGF256_EXPONENT;
		gf->size		= MOEPGF256_SIZE;
		gf->mask		= MOEPGF256_MASK;
		gf->inv			= inv256;
		break;
	default:
		return -1;
	}

	switch (atype) {
	case MOEPGF_SELFTEST:
		switch (type) {
		case MOEPGF2:
			gf->mulrc = mulrc2;
			gf->maddrc = maddrc2_scalar;
			break;
		case MOEPGF4:
			gf->mulrc = mulrc4_imul_scalar;
			gf->maddrc = maddrc4_imul_scalar;
			break;
		case MOEPGF16:
			gf->mulrc = mulrc16_imul_scalar;
			gf->maddrc = maddrc16_imul_scalar;
			break;
		case MOEPGF256:
			gf->mulrc = mulrc256_pdiv;
			gf->maddrc = maddrc256_pdiv;
			break;
		default:
			return -1;
		}
		break;

	case MOEPGF_ALGORITHM_BEST:
#ifdef __x86_64__
		if (hwcaps & (1 << MOEPGF_HWCAPS_SIMD_AVX2)) {
			gf->hwcaps = (1 << MOEPGF_HWCAPS_SIMD_AVX2);
			gf->mulrc  = best_algorithms[type][MOEPGF_HWCAPS_SIMD_AVX2].mulrc;
			gf->maddrc = best_algorithms[type][MOEPGF_HWCAPS_SIMD_AVX2].maddrc;
		}
		else if (hwcaps & (1 << MOEPGF_HWCAPS_SIMD_SSSE3)) {
			gf->hwcaps = (1 << MOEPGF_HWCAPS_SIMD_SSSE3);
			gf->mulrc  = best_algorithms[type][MOEPGF_HWCAPS_SIMD_SSSE3].mulrc;
			gf->maddrc = best_algorithms[type][MOEPGF_HWCAPS_SIMD_SSSE3].maddrc;
		}
		else if (hwcaps & (1 << MOEPGF_HWCAPS_SIMD_SSE2)) {
			gf->hwcaps = (1 << MOEPGF_HWCAPS_SIMD_SSE2);
			gf->mulrc  = best_algorithms[type][MOEPGF_HWCAPS_SIMD_SSE2].mulrc;
			gf->maddrc = best_algorithms[type][MOEPGF_HWCAPS_SIMD_SSE2].maddrc;
		}
#endif
#ifdef __arm__
		if (hwcaps & (1 << MOEPGF_HWCAPS_SIMD_NEON)) {
			gf->hwcaps = (1 << MOEPGF_HWCAPS_SIMD_NEON);
			gf->mulrc  = best_algorithms[type][MOEPGF_HWCAPS_SIMD_NEON].mulrc;
			gf->maddrc = best_algorithms[type][MOEPGF_HWCAPS_SIMD_NEON].maddrc;
		}
#endif
		else if (hwcaps & (1 << MOEPGF_HWCAPS_SIMD_NONE)) {
			gf->mulrc  = best_algorithms[type][MOEPGF_HWCAPS_SIMD_NONE].mulrc;
			gf->maddrc = best_algorithms[type][MOEPGF_HWCAPS_SIMD_NONE].maddrc;
		}
		else {
			return -1;
		}
		break;

	default:
		return -1;
	}

	return ret;
}

static void
add_algorithm(struct moepgf_algorithm **algs, enum MOEPGF_TYPE gt,
		enum MOEPGF_ALGORITHM at, enum MOEPGF_HWCAPS hwcaps,
		maddrc_t maddrc, mulrc_t mulrc)
{
	struct moepgf_algorithm *alg;

	alg = calloc(1, sizeof(*alg));

	alg->maddrc = maddrc;
	alg->mulrc = mulrc;
	alg->hwcaps = hwcaps;
	alg->type = at;
	alg->field = gt;

	algs[at] = alg;
}

struct moepgf_algorithm **
moepgf_get_algs(enum MOEPGF_TYPE field)
{
	struct moepgf_algorithm **algs;

	if (!(algs = calloc(MOEPGF_ALGORITHM_COUNT,sizeof(*algs))))
		return NULL;

	switch (field) {
	case MOEPGF2:
		add_algorithm(algs, field, MOEPGF_XOR_GPR32,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc2_gpr32, NULL);
		add_algorithm(algs, field, MOEPGF_XOR_GPR64,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc2_gpr64, NULL);
#ifdef __x86_64__
		add_algorithm(algs, field, MOEPGF_XOR_SSE2,
				MOEPGF_HWCAPS_SIMD_SSE2,
				maddrc2_sse2, NULL);
		add_algorithm(algs, field, MOEPGF_XOR_AVX2,
				MOEPGF_HWCAPS_SIMD_AVX2,
				maddrc2_avx2, NULL);
#endif
#ifdef __arm__
		add_algorithm(algs, field, MOEPGF_XOR_NEON_128,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc2_neon, NULL);
#endif
#ifdef __mips__
		add_algorithm(algs, field, MOEPGF_XOR_MSA,
				MOEPGF_HWCAPS_SIMD_MSA,
				maddrc2_msa, NULL);
#endif
		break;
	case MOEPGF4:
		add_algorithm(algs, field, MOEPGF_FLAT_TABLE,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc4_flat_table, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_GPR32,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc4_imul_gpr32, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_GPR64,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc4_imul_gpr64, NULL);
#ifdef __x86_64__
		add_algorithm(algs, field, MOEPGF_IMUL_SSE2,
				MOEPGF_HWCAPS_SIMD_SSE2,
				maddrc4_imul_sse2, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_AVX2,
				MOEPGF_HWCAPS_SIMD_AVX2,
				maddrc4_imul_avx2, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_SSSE3,
				MOEPGF_HWCAPS_SIMD_SSSE3,
				maddrc4_shuffle_ssse3, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_AVX2,
				MOEPGF_HWCAPS_SIMD_AVX2,
				maddrc4_shuffle_avx2, NULL);
#endif
#ifdef __arm__
		add_algorithm(algs, field, MOEPGF_IMUL_NEON_64,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc4_imul_neon_64, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_NEON_128,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc4_imul_neon_128, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_NEON_64,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc4_shuffle_neon_64, NULL);
#endif
		break;
	case MOEPGF16:
		add_algorithm(algs, field, MOEPGF_FLAT_TABLE,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc16_flat_table, NULL);
		add_algorithm(algs, field, MOEPGF_LOG_TABLE,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc16_log_table, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_GPR32,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc16_imul_gpr32, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_GPR64,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc16_imul_gpr64, NULL);
#ifdef __x86_64__
		add_algorithm(algs, field, MOEPGF_IMUL_SSE2,
				MOEPGF_HWCAPS_SIMD_SSE2,
				maddrc16_imul_sse2, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_AVX2,
				MOEPGF_HWCAPS_SIMD_AVX2,
				maddrc16_imul_avx2, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_SSSE3,
				MOEPGF_HWCAPS_SIMD_SSSE3,
				maddrc16_shuffle_ssse3, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_AVX2,
				MOEPGF_HWCAPS_SIMD_AVX2,
				maddrc16_shuffle_avx2, NULL);
#endif
#ifdef __arm__
		add_algorithm(algs, field, MOEPGF_IMUL_NEON_64,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc16_imul_neon_64, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_NEON_128,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc16_imul_neon_128, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_NEON_64,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc16_shuffle_neon_64, NULL);
#endif
		break;
	case MOEPGF256:
		add_algorithm(algs, field, MOEPGF_FLAT_TABLE,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc256_flat_table, NULL);
		add_algorithm(algs, field, MOEPGF_LOG_TABLE,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc256_log_table, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_GPR32,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc256_imul_gpr32, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_GPR64,
				MOEPGF_HWCAPS_SIMD_NONE,
				maddrc256_imul_gpr64, NULL);
#ifdef __x86_64__
		add_algorithm(algs, field, MOEPGF_IMUL_SSE2,
				MOEPGF_HWCAPS_SIMD_SSE2,
				maddrc256_imul_sse2, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_AVX2,
				MOEPGF_HWCAPS_SIMD_AVX2,
				maddrc256_imul_avx2, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_SSSE3,
				MOEPGF_HWCAPS_SIMD_SSSE3,
				maddrc256_shuffle_ssse3, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_AVX2,
				MOEPGF_HWCAPS_SIMD_AVX2,
				maddrc256_shuffle_avx2, NULL);
#endif
#ifdef __arm__
		add_algorithm(algs, field, MOEPGF_IMUL_NEON_64,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc256_imul_neon_64, NULL);
		add_algorithm(algs, field, MOEPGF_IMUL_NEON_128,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc256_imul_neon_128, NULL);
		add_algorithm(algs, field, MOEPGF_SHUFFLE_NEON_64,
				MOEPGF_HWCAPS_SIMD_NEON,
				maddrc256_shuffle_neon_64, NULL);
#endif
		break;

	default:
		// free everything before
		free(algs);
		return NULL;
	}

	return algs;
}

void
moepgf_free_algs(struct moepgf_algorithm **algs)
{
	struct moepgf_algorithm;
	int i;

	for (i=0; i<MOEPGF_ALGORITHM_COUNT; i++)
		free(algs[i]);
	free(algs);
}
