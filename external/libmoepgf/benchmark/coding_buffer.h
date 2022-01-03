/*
 * Copyright 2014	Stephan M. Guenther <moepi@moepi.net>
 * 			Maximilian Riemensberger <riemensberger@tum.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * See COPYING for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CODING_BUFFER_H_
#define __CODING_BUFFER_H_

#include <stdint.h>

struct coding_buffer {
	int scount;
	int ssize;
	uint8_t *pcb;
	uint8_t **slot;
};

int	cb_init(struct coding_buffer *cb, int scount, int ssize, int alignment);
void	cb_free(struct coding_buffer *cb);

#endif //__CODING_BUFFER_H_
