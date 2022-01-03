
AM_CFLAGS = -O2 -Wall

AM_CPPFLAGS = -I$(top_srcdir)/include


lib_LTLIBRARIES = libmoepgf.la

libmoepgf_la_SOURCES  = src/gf.c
libmoepgf_la_SOURCES += src/gf2.c
libmoepgf_la_SOURCES += src/gf2.h
libmoepgf_la_SOURCES += src/gf4.c
libmoepgf_la_SOURCES += src/gf4.h
libmoepgf_la_SOURCES += src/gf4tables7.h
libmoepgf_la_SOURCES += src/gf16.c
libmoepgf_la_SOURCES += src/gf16.h
libmoepgf_la_SOURCES += src/gf16tables19.h
libmoepgf_la_SOURCES += src/gf256.c
libmoepgf_la_SOURCES += src/gf256.h
libmoepgf_la_SOURCES += src/gf256tables285.h
libmoepgf_la_SOURCES += src/xor.c
libmoepgf_la_SOURCES += src/xor.h
if ARCH_X86_64
libmoepgf_la_SOURCES += src/detect_x86_simd.c
libmoepgf_la_SOURCES += src/detect_x86_simd.h
endif
if ARCH_ARM
libmoepgf_la_SOURCES += src/detect_arm_neon.h
endif

libmoepgf_la_CFLAGS = $(AM_CFLAGS) $(LIB_CFLAGS)

libmoepgf_la_LDFLAGS = -version-info 0:0:0

libmoepgf_la_includedir = $(includedir)/moepgf

libmoepgf_la_include_HEADERS  = include/moepgf/moepgf.h

libmoepgf_la_LIBADD =
if ARCH_X86_64
libmoepgf_la_LIBADD += libmoepgf_sse2.la libmoepgf_ssse3.la libmoepgf_avx2.la
endif
if ARCH_ARM
libmoepgf_la_LIBADD += libmoepgf_neon.la
endif
if ARCH_MIPS
endif


noinst_LTLIBRARIES =

if ARCH_X86_64
noinst_LTLIBRARIES += libmoepgf_sse2.la libmoepgf_ssse3.la libmoepgf_avx2.la


libmoepgf_sse2_la_SOURCES  = src/gf4_sse2.c
libmoepgf_sse2_la_SOURCES += src/gf16_sse2.c
libmoepgf_sse2_la_SOURCES += src/gf256_sse2.c
libmoepgf_sse2_la_SOURCES += src/xor_sse2.c

libmoepgf_sse2_la_CFLAGS = $(libmoepgf_la_CFLAGS) $(SSE2_CFLAGS)


libmoepgf_ssse3_la_SOURCES  = src/gf4_ssse3.c
libmoepgf_ssse3_la_SOURCES += src/gf16_ssse3.c
libmoepgf_ssse3_la_SOURCES += src/gf256_ssse3.c

libmoepgf_ssse3_la_CFLAGS = $(libmoepgf_la_CFLAGS) $(SSSE3_CFLAGS)


libmoepgf_avx2_la_SOURCES  = src/gf4_avx2.c
libmoepgf_avx2_la_SOURCES += src/gf16_avx2.c
libmoepgf_avx2_la_SOURCES += src/gf256_avx2.c
libmoepgf_avx2_la_SOURCES += src/xor_avx2.c

libmoepgf_avx2_la_CFLAGS = $(libmoepgf_la_CFLAGS) $(AVX2_CFLAGS)
endif

if ARCH_ARM
noinst_LTLIBRARIES += libmoepgf_neon.la


libmoepgf_neon_la_SOURCES  = src/detect_arm_neon.c
libmoepgf_neon_la_SOURCES += src/gf4_neon.c
libmoepgf_neon_la_SOURCES += src/gf16_neon.c
libmoepgf_neon_la_SOURCES += src/gf256_neon.c
libmoepgf_neon_la_SOURCES += src/xor_neon.c

libmoepgf_neon_la_CFLAGS = $(libmoepgf_la_CFLAGS) $(NEON_CFLAGS)
endif
if ARCH_MIPS
endif


bin_PROGRAMS = moepgfbench

moepgfbench_SOURCES  = benchmark/benchmark.c
moepgfbench_SOURCES += benchmark/coding_buffer.c
moepgfbench_SOURCES += benchmark/coding_buffer.h

moepgfbench_LDADD = libmoepgf.la


TESTS = moepgfbench

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmoepgf.pc
