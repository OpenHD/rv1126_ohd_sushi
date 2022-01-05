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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <moepgf/moepgf.h>

#include "coding_buffer.h"

/* Size of the array containing a predetermined sequence of pseudo random
 * values. Must be a power of two or bad things will happen. */
#define RVAL_COUNT (1 << 14)
static uint8_t _rval[RVAL_COUNT];

#ifdef __MACH__
#include <mach/mach_time.h>

#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)
#define CLOCK_MONOTONIC 0

static double orwl_timebase = 0.0;
static uint64_t orwl_timestart = 0;

void clock_gettime(void *clk_id, struct timespec *t) {
	// be more careful in a multithreaded environement
	double diff;
	mach_timebase_info_data_t tb;

	// maintain signature of clock_gettime() but make gcc happy
	clk_id = NULL;

	if (!orwl_timestart) {
		mach_timebase_info(&tb);
		orwl_timebase = tb.numer;
		orwl_timebase /= tb.denom;
		orwl_timestart = mach_absolute_time();
	}
	diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
	t->tv_sec = diff * ORWL_NANO;
	t->tv_nsec = diff - (t->tv_sec * ORWL_GIGA);
}
#endif

#define timespecclear(tvp)	((tvp)->tv_sec = (tvp)->tv_nsec = 0)
#define timespecisset(tvp)	((tvp)->tv_sec || (tvp)->tv_nsec)
#define timespeccmp(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	    ((tvp)->tv_nsec cmp (uvp)->tv_nsec) :			\
	    ((tvp)->tv_sec cmp (uvp)->tv_sec))
#define timespecadd(vvp, uvp)						\
	({								\
		(vvp)->tv_sec += (uvp)->tv_sec;				\
		(vvp)->tv_nsec += (uvp)->tv_nsec;			\
		if ((vvp)->tv_nsec >= 1000000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_nsec -= 1000000000;			\
		}							\
	})
#define timespecsub(vvp, uvp)						\
	({								\
		(vvp)->tv_sec -= (uvp)->tv_sec;				\
		(vvp)->tv_nsec -= (uvp)->tv_nsec;			\
		if ((vvp)->tv_nsec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_nsec += 1000000000;			\
		}							\
	})
#define timespecmset(vvp, msec)						\
	({								\
		(vvp)->tv_sec = msec/1000;				\
		(vvp)->tv_nsec = msec-((vvp)->tv_sec*1000);		\
		(vvp)->tv_nsec *= 1000000;				\
	})
#define timespeccpy(vvp, uvp)						\
	({								\
		(vvp)->tv_sec = (uvp)->tv_sec;				\
		(vvp)->tv_nsec = (uvp)->tv_nsec;			\
	})

#define u8_to_float(x)							\
	({								\
		(float)(x)/255.0;               			\
	})
#define float_to_u8(x)							\
	({								\
		(u8)((x)*255.0);                			\
	})

typedef void (*madd_t)(uint8_t *, const uint8_t *, uint8_t, size_t);

struct args {
	int count;
	int maxsize;
	int random;
	int repeat;
	int threads;
} args;

struct thread_args {
	madd_t	madd;
	uint8_t	mask;
	double	gbps;
	int	length;
	int	rep;
	int	random;
	int	count;
};

struct thread_info {
	int 			tid;
	pthread_t 		thread;
	struct thread_args 	args;
};

static void
fill_random(struct coding_buffer *cb)
{
	int i,j;

	for (i=0; i<cb->scount; i++) {
		for (j=0; j<cb->ssize; j++)
			cb->slot[i][j] = rand();
	}
}

static void
init_test_buffers(uint8_t *test1, uint8_t *test2, uint8_t *test3, int size)
{
	int i;
	for (i=0; i<size; i++) {
		test1[i] = rand();
		test2[i] = test1[i];
		test3[i] = rand();
	}
}

static void
selftest()
{
	int i,j,k,fset;
	int tlen = (1 << 15);
	uint8_t	*test1, *test2, *test3;
	struct moepgf_algorithm **algs;
	struct moepgf gf;

	fset = moepgf_check_available_simd_extensions();
	fprintf(stderr, "CPU SIMD extensions detected: \n");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_MMX))
		fprintf(stderr, "MMX ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_SSE))
		fprintf(stderr, "SSE ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_SSE2))
		fprintf(stderr, "SSE2 ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_SSSE3))
		fprintf(stderr, "SSSE3 ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_SSE41))
		fprintf(stderr, "SSE41 ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_SSE42))
		fprintf(stderr, "SSE42 ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_AVX))
		fprintf(stderr, "AVX ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_AVX2))
		fprintf(stderr, "AVX2 ");
	if (fset & (1 << MOEPGF_HWCAPS_SIMD_NEON))
		fprintf(stderr, "NEON ");
	fprintf(stderr, "\n\n");

	if (posix_memalign((void *)&test1, 32, tlen))
		exit(-1);
	if (posix_memalign((void *)&test2, 32, tlen))
		exit(-1);
	if (posix_memalign((void *)&test3, 32, tlen))
		exit(-1);

	for (i=0; i<4; i++) {
		moepgf_init(&gf, i, MOEPGF_SELFTEST);
		algs = moepgf_get_algs(gf.type);
		fprintf(stderr, "%s:\n", gf.name);

		for (j=0; j<MOEPGF_ALGORITHM_COUNT; j++) {
			if (!algs[j])
				continue;

			fprintf(stderr, "- selftest (%s)    ",
						moepgf_a2name(algs[j]->type));
			if (!(fset & (1 << algs[j]->hwcaps))) {
				fprintf(stderr, "\tNecessary SIMD "
						"instructions not supported\n");
				continue;
			}

			for (k=gf.size-1; k>=0; k--) {
				init_test_buffers(test1, test2, test3, tlen);

				gf.maddrc(test1, test3, k, tlen);
				algs[j]->maddrc(test2, test3, k, tlen);

				if (memcmp(test1, test2, tlen)){
					fprintf(stderr,"FAIL: results differ, c = %d\n", k);
				}
			}
			fprintf(stderr, "\tPASS\n");
		}
		fprintf(stderr, "\n");
		moepgf_free_algs(algs);
	}

	free(test1);
	free(test2);
	free(test3);
}

struct thread_state {
	unsigned int 	rseed;
	int		pos;
};

static void
encode_random(madd_t madd, int mask, uint8_t *dst, struct coding_buffer *cb,
						struct thread_state *state)
{
	int i,c;

	for (i=0; i<cb->scount; i++) {
		c = moepgf_rand(&state->rseed) & mask;
		madd(dst, cb->slot[i], c, cb->ssize);
	}
}

static void
encode_permutation(madd_t madd, int mask, uint8_t *dst, struct coding_buffer *cb,
						struct thread_state *state)
{
	(void) state;
	int i,c;

	for (i=0; i<cb->scount; i++, state->pos++) {
		c = _rval[state->pos & (RVAL_COUNT-1)] & mask;
		madd(dst, cb->slot[i], c, cb->ssize);
	}
}

static void *
encode_thread(void *args)
{
	struct thread_args *ta = args;
	struct coding_buffer cb;
	struct timespec start, end;
	struct thread_state state;
	uint8_t *frame;
	int i;
	void (*encode)(madd_t, int, uint8_t *, struct coding_buffer *,
						struct thread_state *state);

	memset(&state, 0, sizeof(state));
	clock_gettime(CLOCK_MONOTONIC, &start);
	state.rseed = (unsigned int)start.tv_nsec;
	encode = encode_random;

	if (ta->random == 0) {
		encode = encode_permutation;
	}

	if (posix_memalign((void *)&frame, 32, ta->length))
		exit(-1);
			
	if (cb_init(&cb, ta->count, ta->length, 32))
		exit(-1);
				
	fill_random(&cb);

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i=0; i<ta->rep; i++)
		encode(ta->madd, ta->mask, frame, &cb, &state);
	clock_gettime(CLOCK_MONOTONIC, &end);
				
	timespecsub(&end, &start);
				
	ta->gbps = (double)ta->rep/((double)end.tv_sec
			+ (double)end.tv_nsec*1e-9);
	ta->gbps *= ta->length*8.0*1e-9;

	cb_free(&cb);
	free(frame);

	return NULL;
}

static void
benchmark(struct args *args)
{
	int i,j,l,m,rep,fset;
	uint32_t s;
	struct moepgf_algorithm **algs;
	struct moepgf gf;
	struct thread_info *tinfo;
	double gbps;

	tinfo = malloc(args->threads * sizeof(*tinfo));
	memset(tinfo, 0, args->threads * sizeof(*tinfo));

	fset = moepgf_check_available_simd_extensions();

    fprintf(stderr,"Test was unrolled2\n");

            fprintf(stderr,
		"\nEncoding benchmark: "
		"Encoding throughput in Gbps (1e9 bits per sec)\n"
		"maxsize=%d, count=%d, repetitions=%d, "
		"threads=%d\n\n",	args->maxsize, args->count, args->repeat, 
		args->threads);
		
	s = rand();
	for (i=0; i<RVAL_COUNT; i++)
		_rval[i] = moepgf_rand(&s) & 0xff;

	for (i=0; i<4; i++) {
		moepgf_init(&gf, i, 0);
		algs = moepgf_get_algs(gf.type);

		fprintf(stderr, "%s\n", gf.name);
		fprintf(stderr, "size \t");

		for (j=0; j<MOEPGF_ALGORITHM_COUNT; j++) {
			if (!algs[j])
				continue;
			fprintf(stderr, "%s \t", moepgf_a2name(algs[j]->type));
		}
		fprintf(stderr, "\n");

		for (l=128, rep=args->repeat; l<=args->maxsize; l*=2, rep/=2) {
			fprintf(stderr, "%d\t", l);
			for (j=0; j<MOEPGF_ALGORITHM_COUNT; j++) {
				if (!algs[j])
					continue;

				if (!(fset & (1 << algs[j]->hwcaps))) {
					fprintf(stderr, "n/a      \t");
					continue;
				}

				if (rep < 256) {
					fprintf(stderr, "rep too small\t");
					continue;
				}

				for (m=0; m<args->threads; m++) {
					tinfo[m].args.madd = algs[j]->maddrc;
					tinfo[m].args.mask = gf.mask;
					tinfo[m].args.length = l;
					tinfo[m].args.rep = rep;
					tinfo[m].args.random = args->random;
					tinfo[m].args.count = args->count;
				}
				
				for (m=0; m<args->threads; m++) {
					tinfo[m].tid = 
						pthread_create(
							&tinfo[m].thread, NULL, 
							encode_thread,
							(void *)&tinfo[m].args);
				}
				for (m=0; m<args->threads; m++)
					pthread_join(tinfo[m].thread, NULL);

				gbps = 0;
				for (m=0; m<args->threads; m++)
					gbps += tinfo[m].args.gbps;

				fprintf(stderr, "%.6f \t", gbps);
			}
			fprintf(stderr, "\n");
		}
		fprintf(stderr, "\n");
		moepgf_free_algs(algs);
	}

	free(tinfo);
}

static void
print_help(const char *name)
{
	fprintf(stdout, "Usage: %s [-m maxsize] [-c count] [-r repeat] "\
			"[-t threads] [-d]\n\n", name);
	fprintf(stdout, "    -m maxsize   Maximum packet size [Byte]\n");
	fprintf(stdout, "    -c count     Number of packets per generation\n");
	fprintf(stdout, "    -r repeat    Number of repetitions per setting\n");
	fprintf(stdout, "    -t threads   Number of threads to use\n");
	fprintf(stdout, "    -d           Deterministic permutation of coefficients\n");
	fprintf(stdout, "\n");
}

int
main(int argc, char **argv)
{
	int opt;

	args.count = 16;
	args.maxsize = 1024*1024*8;
	args.repeat = 1024*1024;
	args.random = 1;
	args.threads = 1;

	while (-1 != (opt = getopt(argc, argv, "m:c:r:t:dh"))) {
		switch (opt) {
		case 'm':
			args.maxsize = atoi(optarg);
			if (args.maxsize < 128) {
				fprintf(stderr, "minimum maxsize is 128 Byte\n\n");
				exit(-1);
			}
			break;
		case 'c':
			args.count = atoi(optarg);
			if (args.count < 1) {
				fprintf(stderr, "minimum count is 1\n\n");
				exit(-1);
			}
			break;
		case 'r':
			args.repeat = atoi(optarg);
			if (args.repeat < 1024) {
				fprintf(stderr, "minimum repeat is 1024\n\n");
				exit(-1);
			}
			break;
		case 'd':
			args.random = 0;
			break;
		case 't':
			args.threads = atoi(optarg);
			if (args.threads < 1) {
				fprintf(stderr, "invalid thread cound\n\n");
				exit(-1);
			}
			break;
		case 'h':
			print_help(argv[0]);
			exit(0);
		default:
			fprintf(stderr, "unknown option %c\n\n", (char)opt);
			print_help(argv[0]);
			exit(-1);
		}
	}

	selftest();
	benchmark(&args);

	return 0;
}
