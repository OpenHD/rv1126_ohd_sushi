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

#include <signal.h>
#include <setjmp.h>

#include <moepgf/moepgf.h>

static sigjmp_buf jmpbuf;

static void
sigill_intrinsic_handler(int sig)
{
	siglongjmp(jmpbuf, 1);
}

uint32_t
detect_arm_neon()
{
	struct sigaction new_action, old_action;
	uint32_t hwcaps = 0;

	new_action.sa_handler = sigill_intrinsic_handler;
	new_action.sa_flags = SA_RESTART;
	sigemptyset(&new_action.sa_mask);
	sigaction(SIGILL, &new_action, &old_action);

	if (!sigsetjmp(jmpbuf, 1)) {
		asm volatile (
			"vand.u8 d0, d1, d0\n"
		);
		hwcaps |= (1 << MOEPGF_HWCAPS_SIMD_NEON);
	}

	sigaction(SIGILL, &old_action, NULL);

	return hwcaps;
}

