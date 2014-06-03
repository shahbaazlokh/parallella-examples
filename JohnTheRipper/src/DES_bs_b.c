/*
 * This file is part of John the Ripper password cracker,
 * Copyright (c) 1996-2001,2003,2010-2013 by Solar Designer
 */

#include "arch.h"
#include "common.h"
#include "DES_bs.h"

#if DES_BS_ASM && defined(_OPENMP) && defined(__GNUC__)
#warning Assembly code and OpenMP are both requested - will provide the former, but not the latter (for DES-based hashes).  This may likely be corrected by enabling SIMD intrinsics with the C compiler (try adding -msse2 to OMPFLAGS).
#endif

#if !DES_BS_ASM

#define vzero (*(vtype *)&DES_bs_all.zero)
#if DES_bs_mt
#define vones (*(vtype *)&DES_bs_all_by_tnum(-1).ones)
#else
#define vones (*(vtype *)&DES_bs_all.ones)
#endif

#define DES_BS_VECTOR_LOOPS 0

#if defined(__ARM_NEON__) && DES_BS_DEPTH == 64
#include <arm_neon.h>

typedef uint32x2_t vtype;

#define vst(dst, ofs, src) \
	vst1_u32((uint32_t *)((DES_bs_vector *)&(dst) + (ofs)), (src))

#define vxorf(a, b) \
	veor_u32((a), (b))

#define vnot(dst, a) \
	(dst) = vmvn_u32((a))
#define vand(dst, a, b) \
	(dst) = vand_u32((a), (b))
#define vor(dst, a, b) \
	(dst) = vorr_u32((a), (b))
#define vandn(dst, a, b) \
	(dst) = vbic_u32((a), (b))
#define vsel(dst, a, b, c) \
	(dst) = vbsl_u32((c), (b), (a))

#if 0
#define vshl1(dst, src) \
	(dst) = vadd_u32((src), (src))
#endif
#define vshl(dst, src, shift) \
	(dst) = vshl_n_u32((src), (shift))
#define vshr(dst, src, shift) \
	(dst) = vshr_n_u32((src), (shift))

#elif defined(__ARM_NEON__) && DES_BS_DEPTH == 128
#include <arm_neon.h>

typedef uint32x4_t vtype;

#define vst(dst, ofs, src) \
	vst1q_u32((uint32_t *)((DES_bs_vector *)&(dst) + (ofs)), (src))

#define vxorf(a, b) \
	veorq_u32((a), (b))

#define vnot(dst, a) \
	(dst) = vmvnq_u32((a))
#define vand(dst, a, b) \
	(dst) = vandq_u32((a), (b))
#define vor(dst, a, b) \
	(dst) = vorrq_u32((a), (b))
#define vandn(dst, a, b) \
	(dst) = vbicq_u32((a), (b))
#define vsel(dst, a, b, c) \
	(dst) = vbslq_u32((c), (b), (a))

#if 0
#define vshl1(dst, src) \
	(dst) = vaddq_u32((src), (src))
#endif
#define vshl(dst, src, shift) \
	(dst) = vshlq_n_u32((src), (shift))
#define vshr(dst, src, shift) \
	(dst) = vshrq_n_u32((src), (shift))

#elif defined(__ARM_NEON__) && \
    ((ARCH_BITS == 64 && DES_BS_DEPTH == 192) || \
    (ARCH_BITS == 32 && DES_BS_DEPTH == 160))
#include <arm_neon.h>

typedef struct {
	uint32x4_t f;
	unsigned ARCH_WORD g;
} vtype;

#define vst(dst, ofs, src) \
	vst1q_u32( \
	    (uint32_t *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g

#define vxor(dst, a, b) \
	(dst).f = veorq_u32((a).f, (b).f); \
	(dst).g = (a).g ^ (b).g

#define vnot(dst, a) \
	(dst).f = vmvnq_u32((a).f); \
	(dst).g = ~(a).g
#define vand(dst, a, b) \
	(dst).f = vandq_u32((a).f, (b).f); \
	(dst).g = (a).g & (b).g
#define vor(dst, a, b) \
	(dst).f = vorrq_u32((a).f, (b).f); \
	(dst).g = (a).g | (b).g
#define vandn(dst, a, b) \
	(dst).f = vbicq_u32((a).f, (b).f); \
	(dst).g = (a).g & ~(b).g
#define vsel(dst, a, b, c) \
	(dst).f = vbslq_u32((c).f, (b).f, (a).f); \
	(dst).g = (((a).g & ~(c).g) ^ ((b).g & (c).g))

#elif defined(__ARM_NEON__) && DES_BS_DEPTH == 256
#include <arm_neon.h>

typedef struct {
	uint32x4_t f, g;
} vtype;

#define vst(dst, ofs, src) \
	vst1q_u32( \
	    (uint32_t *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	vst1q_u32( \
	    (uint32_t *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g, \
	    (src).g)

#define vxor(dst, a, b) \
	(dst).f = veorq_u32((a).f, (b).f); \
	(dst).g = veorq_u32((a).g, (b).g)

#define vnot(dst, a) \
	(dst).f = vmvnq_u32((a).f); \
	(dst).g = vmvnq_u32((a).g)
#define vand(dst, a, b) \
	(dst).f = vandq_u32((a).f, (b).f); \
	(dst).g = vandq_u32((a).g, (b).g)
#define vor(dst, a, b) \
	(dst).f = vorrq_u32((a).f, (b).f); \
	(dst).g = vorrq_u32((a).g, (b).g)
#define vandn(dst, a, b) \
	(dst).f = vbicq_u32((a).f, (b).f); \
	(dst).g = vbicq_u32((a).g, (b).g)
#define vsel(dst, a, b, c) \
	(dst).f = vbslq_u32((c).f, (b).f, (a).f); \
	(dst).g = vbslq_u32((c).g, (b).g, (a).g)

#elif defined(__ALTIVEC__) && DES_BS_DEPTH == 128
#ifdef __linux__
#include <altivec.h>
#endif

typedef vector signed int vtype;

#define vst(dst, ofs, src) \
	vec_st((src), (ofs) * sizeof(DES_bs_vector), (dst))

#define vxorf(a, b) \
	vec_xor((a), (b))

#define vnot(dst, a) \
	(dst) = vec_nor((a), (a))
#define vand(dst, a, b) \
	(dst) = vec_and((a), (b))
#define vor(dst, a, b) \
	(dst) = vec_or((a), (b))
#define vandn(dst, a, b) \
	(dst) = vec_andc((a), (b))
#define vsel(dst, a, b, c) \
	(dst) = vec_sel((a), (b), (vector bool int)(c))

#elif defined(__ALTIVEC__) && \
    ((ARCH_BITS == 64 && DES_BS_DEPTH == 192) || \
    (ARCH_BITS == 32 && DES_BS_DEPTH == 160))
#ifdef __linux__
#include <altivec.h>
#endif

typedef struct {
	vector signed int f;
	unsigned ARCH_WORD g;
} vtype;

#define vst(dst, ofs, src) \
	vec_st((src).f, (ofs) * sizeof(DES_bs_vector), ((vtype *)&(dst))->f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g

#define vxor(dst, a, b) \
	(dst).f = vec_xor((a).f, (b).f); \
	(dst).g = (a).g ^ (b).g

#define vnot(dst, a) \
	(dst).f = vec_nor((a).f, (a).f); \
	(dst).g = ~(a).g
#define vand(dst, a, b) \
	(dst).f = vec_and((a).f, (b).f); \
	(dst).g = (a).g & (b).g
#define vor(dst, a, b) \
	(dst).f = vec_or((a).f, (b).f); \
	(dst).g = (a).g | (b).g
#define vandn(dst, a, b) \
	(dst).f = vec_andc((a).f, (b).f); \
	(dst).g = (a).g & ~(b).g
#define vsel(dst, a, b, c) \
	(dst).f = vec_sel((a).f, (b).f, (vector bool int)(c).f); \
	(dst).g = (((a).g & ~(c).g) ^ ((b).g & (c).g))

#elif defined(__ALTIVEC__) && DES_BS_DEPTH == 256
#ifdef __linux__
#include <altivec.h>
#endif

typedef struct {
	vector signed int f, g;
} vtype;

#define vst(dst, ofs, src) \
	vec_st((src).f, (ofs) * sizeof(DES_bs_vector), ((vtype *)&(dst))->f); \
	vec_st((src).g, (ofs) * sizeof(DES_bs_vector), ((vtype *)&(dst))->g)

#define vxor(dst, a, b) \
	(dst).f = vec_xor((a).f, (b).f); \
	(dst).g = vec_xor((a).g, (b).g)

#define vnot(dst, a) \
	(dst).f = vec_nor((a).f, (a).f); \
	(dst).g = vec_nor((a).g, (a).g)
#define vand(dst, a, b) \
	(dst).f = vec_and((a).f, (b).f); \
	(dst).g = vec_and((a).g, (b).g)
#define vor(dst, a, b) \
	(dst).f = vec_or((a).f, (b).f); \
	(dst).g = vec_or((a).g, (b).g)
#define vandn(dst, a, b) \
	(dst).f = vec_andc((a).f, (b).f); \
	(dst).g = vec_andc((a).g, (b).g)
#define vsel(dst, a, b, c) \
	(dst).f = vec_sel((a).f, (b).f, (vector bool int)(c).f); \
	(dst).g = vec_sel((a).g, (b).g, (vector bool int)(c).g)

#elif defined(__AVX__) && DES_BS_DEPTH == 256 && !defined(DES_BS_NO_AVX256)
#include <immintrin.h>

/* Not __m256i because bitwise ops are "floating-point" with AVX */
typedef __m256 vtype;

#define vst(dst, ofs, src) \
	_mm256_store_ps((float *)((DES_bs_vector *)&(dst) + (ofs)), (src))

#define vxorf(a, b) \
	_mm256_xor_ps((a), (b))

#define vand(dst, a, b) \
	(dst) = _mm256_and_ps((a), (b))
#define vor(dst, a, b) \
	(dst) = _mm256_or_ps((a), (b))
#define vandn(dst, a, b) \
	(dst) = _mm256_andnot_ps((b), (a))

#ifdef __XOP__
/* This could be _mm256_cmov_ps(), but it does not exist (yet?) */
#define vsel(dst, a, b, c) \
	(dst) = __builtin_ia32_vpcmov_v8sf256((b), (a), (c))
#endif

/*
 * We should be able to do 256-bit shifts with one instruction with AVX2, but
 * for plain AVX let's use pairs of 128-bit instructions (and likely incur
 * extra memory stores/loads because the rest of our AVX code is 256-bit). :-(
 */
#define vshl(dst, src, shift) \
	((__m128i *)&(dst))[0] = \
	    _mm_slli_epi64(((__m128i *)&(src))[0], (shift)); \
	((__m128i *)&(dst))[1] = \
	    _mm_slli_epi64(((__m128i *)&(src))[1], (shift))
#define vshr(dst, src, shift) \
	((__m128i *)&(dst))[0] = \
	    _mm_srli_epi64(((__m128i *)&(src))[0], (shift)); \
	((__m128i *)&(dst))[1] = \
	    _mm_srli_epi64(((__m128i *)&(src))[1], (shift))

#elif defined(__AVX__) && DES_BS_DEPTH == 384 && !defined(DES_BS_NO_AVX128)
#include <immintrin.h>
#ifdef __XOP__
#include <x86intrin.h>
#endif

typedef struct {
/* Not __m256i because bitwise ops are "floating-point" with AVX */
	__m256 f;
	__m128i g;
} vtype;

#define vst(dst, ofs, src) \
	_mm256_store_ps( \
	    (float *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	_mm_store_si128(&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g, \
	    (src).g)

#define vxor(dst, a, b) \
	(dst).f = _mm256_xor_ps((a).f, (b).f); \
	(dst).g = _mm_xor_si128((a).g, (b).g)

#define vand(dst, a, b) \
	(dst).f = _mm256_and_ps((a).f, (b).f); \
	(dst).g = _mm_and_si128((a).g, (b).g)
#define vor(dst, a, b) \
	(dst).f = _mm256_or_ps((a).f, (b).f); \
	(dst).g = _mm_or_si128((a).g, (b).g)
#define vandn(dst, a, b) \
	(dst).f = _mm256_andnot_ps((b).f, (a).f); \
	(dst).g = _mm_andnot_si128((b).g, (a).g)

#ifdef __XOP__
/* This could be _mm256_cmov_ps(), but it does not exist (yet?) */
#define vsel(dst, a, b, c) \
	(dst).f = __builtin_ia32_vpcmov_v8sf256((b).f, (a).f, (c).f); \
	(dst).g = _mm_cmov_si128((b).g, (a).g, (c).g)
#endif

#define vshl(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = _mm_slli_epi64((src).g, (shift))
#define vshr(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = _mm_srli_epi64((src).g, (shift))

#elif defined(__AVX__) && DES_BS_DEPTH == 512
#include <immintrin.h>

typedef struct {
/* Not __m256i because bitwise ops are "floating-point" with AVX */
	__m256 f, g;
} vtype;

#define vst(dst, ofs, src) \
	_mm256_store_ps( \
	    (float *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	_mm256_store_ps( \
	    (float *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g, \
	    (src).g)

#define vxor(dst, a, b) \
	(dst).f = _mm256_xor_ps((a).f, (b).f); \
	(dst).g = _mm256_xor_ps((a).g, (b).g)

#define vand(dst, a, b) \
	(dst).f = _mm256_and_ps((a).f, (b).f); \
	(dst).g = _mm256_and_ps((a).g, (b).g)
#define vor(dst, a, b) \
	(dst).f = _mm256_or_ps((a).f, (b).f); \
	(dst).g = _mm256_or_ps((a).g, (b).g)
#define vandn(dst, a, b) \
	(dst).f = _mm256_andnot_ps((b).f, (a).f); \
	(dst).g = _mm256_andnot_ps((b).g, (a).g)

#ifdef __XOP__
/* This could be _mm256_cmov_ps(), but it does not exist (yet?) */
#define vsel(dst, a, b, c) \
	(dst).f = __builtin_ia32_vpcmov_v8sf256((b).f, (a).f, (c).f); \
	(dst).g = __builtin_ia32_vpcmov_v8sf256((b).g, (a).g, (c).g)
#endif

#define vshl(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	((__m128i *)&(dst).g)[0] = \
	    _mm_slli_epi64(((__m128i *)&(src).g)[0], (shift)); \
	((__m128i *)&(dst).g)[1] = \
	    _mm_slli_epi64(((__m128i *)&(src).g)[1], (shift))
#define vshr(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	((__m128i *)&(dst).g)[0] = \
	    _mm_srli_epi64(((__m128i *)&(src).g)[0], (shift)); \
	((__m128i *)&(dst).g)[1] = \
	    _mm_srli_epi64(((__m128i *)&(src).g)[1], (shift))

#elif defined(__AVX__) && defined(__MMX__) && DES_BS_DEPTH == 320 && \
    !defined(DES_BS_NO_MMX)
#include <immintrin.h>
#include <mmintrin.h>

typedef struct {
/* Not __m256i because bitwise ops are "floating-point" with AVX */
	__m256 f;
	__m64 g;
} vtype;

#define vst(dst, ofs, src) \
	_mm256_store_ps( \
	    (float *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g

#define vxor(dst, a, b) \
	(dst).f = _mm256_xor_ps((a).f, (b).f); \
	(dst).g = _mm_xor_si64((a).g, (b).g)

#define vand(dst, a, b) \
	(dst).f = _mm256_and_ps((a).f, (b).f); \
	(dst).g = _mm_and_si64((a).g, (b).g)
#define vor(dst, a, b) \
	(dst).f = _mm256_or_ps((a).f, (b).f); \
	(dst).g = _mm_or_si64((a).g, (b).g)
#define vandn(dst, a, b) \
	(dst).f = _mm256_andnot_ps((b).f, (a).f); \
	(dst).g = _mm_andnot_si64((b).g, (a).g)

#define vshl(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = _mm_slli_si64((src).g, (shift))
#define vshr(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = _mm_srli_si64((src).g, (shift))

#elif defined(__AVX__) && \
    ((ARCH_BITS == 64 && DES_BS_DEPTH == 320) || \
    (ARCH_BITS == 32 && DES_BS_DEPTH == 288))
#include <immintrin.h>
#include <mmintrin.h>

typedef struct {
/* Not __m256i because bitwise ops are "floating-point" with AVX */
	__m256 f;
	unsigned ARCH_WORD g;
} vtype;

#define vst(dst, ofs, src) \
	_mm256_store_ps( \
	    (float *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g

#define vxor(dst, a, b) \
	(dst).f = _mm256_xor_ps((a).f, (b).f); \
	(dst).g = (a).g ^ (b).g

#define vnot(dst, a) \
	(dst).f = _mm256_xor_ps((a).f, vones.f); \
	(dst).g = ~(a).g
#define vand(dst, a, b) \
	(dst).f = _mm256_and_ps((a).f, (b).f); \
	(dst).g = (a).g & (b).g
#define vor(dst, a, b) \
	(dst).f = _mm256_or_ps((a).f, (b).f); \
	(dst).g = (a).g | (b).g
#define vandn(dst, a, b) \
	(dst).f = _mm256_andnot_ps((b).f, (a).f); \
	(dst).g = (a).g & ~(b).g

#define vshl(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = (src).g << (shift)
#define vshr(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = (src).g >> (shift)

#elif defined(__AVX__) && defined(__MMX__) && \
    ((ARCH_BITS == 64 && DES_BS_DEPTH == 384) || \
    (ARCH_BITS == 32 && DES_BS_DEPTH == 352))
#include <immintrin.h>
#include <mmintrin.h>

typedef struct {
/* Not __m256i because bitwise ops are "floating-point" with AVX */
	__m256 f;
	__m64 g;
	unsigned ARCH_WORD h;
} vtype;

#define vst(dst, ofs, src) \
	_mm256_store_ps( \
	    (float *)&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g; \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->h = (src).h

#define vxor(dst, a, b) \
	(dst).f = _mm256_xor_ps((a).f, (b).f); \
	(dst).g = _mm_xor_si64((a).g, (b).g); \
	(dst).h = (a).h ^ (b).h

#define vnot(dst, a) \
	(dst).f = _mm256_xor_ps((a).f, vones.f); \
	(dst).g = _mm_xor_si64((a).g, vones.g); \
	(dst).h = ~(a).h
#define vand(dst, a, b) \
	(dst).f = _mm256_and_ps((a).f, (b).f); \
	(dst).g = _mm_and_si64((a).g, (b).g); \
	(dst).h = (a).h & (b).h
#define vor(dst, a, b) \
	(dst).f = _mm256_or_ps((a).f, (b).f); \
	(dst).g = _mm_or_si64((a).g, (b).g); \
	(dst).h = (a).h | (b).h
#define vandn(dst, a, b) \
	(dst).f = _mm256_andnot_ps((b).f, (a).f); \
	(dst).g = _mm_andnot_si64((b).g, (a).g); \
	(dst).h = (a).h & ~(b).h

#define vshl(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_slli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = _mm_slli_si64((src).g, (shift)); \
	(dst).h = (src).h << (shift)
#define vshr(dst, src, shift) \
	((__m128i *)&(dst).f)[0] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[0], (shift)); \
	((__m128i *)&(dst).f)[1] = \
	    _mm_srli_epi64(((__m128i *)&(src).f)[1], (shift)); \
	(dst).g = _mm_srli_si64((src).g, (shift)); \
	(dst).h = (src).h >> (shift)

#elif defined(__SSE2__) && DES_BS_DEPTH == 128
#ifdef __AVX__
#include <immintrin.h>
#ifdef __XOP__
#include <x86intrin.h>
#endif
#else
#include <emmintrin.h>
#endif

typedef __m128i vtype;

#define vst(dst, ofs, src) \
	_mm_store_si128((vtype *)((DES_bs_vector *)&(dst) + (ofs)), (src))

#define vxorf(a, b) \
	_mm_xor_si128((a), (b))

#define vand(dst, a, b) \
	(dst) = _mm_and_si128((a), (b))
#define vor(dst, a, b) \
	(dst) = _mm_or_si128((a), (b))
#define vandn(dst, a, b) \
	(dst) = _mm_andnot_si128((b), (a))

#ifdef __XOP__
#define vsel(dst, a, b, c) \
	(dst) = _mm_cmov_si128((b), (a), (c))
#else
#define vsel(dst, a, b, c) \
	(dst) = _mm_xor_si128(_mm_andnot_si128((c), (a)), \
	    _mm_and_si128((c), (b)))
#endif

#define vshl1(dst, src) \
	(dst) = _mm_add_epi8((src), (src))
#define vshl(dst, src, shift) \
	(dst) = _mm_slli_epi64((src), (shift))
#define vshr(dst, src, shift) \
	(dst) = _mm_srli_epi64((src), (shift))

#elif defined(__SSE2__) && DES_BS_DEPTH == 256 && defined(DES_BS_NO_MMX)
#ifdef __AVX__
#include <immintrin.h>
#ifdef __XOP__
#include <x86intrin.h>
#endif
#else
#include <emmintrin.h>
#endif

typedef struct {
	__m128i f, g;
} vtype;

#define vst(dst, ofs, src) \
	_mm_store_si128(&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	_mm_store_si128(&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g, \
	    (src).g)

#define vxor(dst, a, b) \
	(dst).f = _mm_xor_si128((a).f, (b).f); \
	(dst).g = _mm_xor_si128((a).g, (b).g)

#define vand(dst, a, b) \
	(dst).f = _mm_and_si128((a).f, (b).f); \
	(dst).g = _mm_and_si128((a).g, (b).g)
#define vor(dst, a, b) \
	(dst).f = _mm_or_si128((a).f, (b).f); \
	(dst).g = _mm_or_si128((a).g, (b).g)
#define vandn(dst, a, b) \
	(dst).f = _mm_andnot_si128((b).f, (a).f); \
	(dst).g = _mm_andnot_si128((b).g, (a).g)

#ifdef __XOP__
#define vsel(dst, a, b, c) \
	(dst).f = _mm_cmov_si128((b).f, (a).f, (c).f); \
	(dst).g = _mm_cmov_si128((b).g, (a).g, (c).g)
#endif

#define vshl1(dst, src) \
	(dst).f = _mm_add_epi8((src).f, (src).f); \
	(dst).g = _mm_add_epi8((src).g, (src).g)
#define vshl(dst, src, shift) \
	(dst).f = _mm_slli_epi64((src).f, (shift)); \
	(dst).g = _mm_slli_epi64((src).g, (shift))
#define vshr(dst, src, shift) \
	(dst).f = _mm_srli_epi64((src).f, (shift)); \
	(dst).g = _mm_srli_epi64((src).g, (shift))

#elif defined(__SSE2__) && defined(__MMX__) && DES_BS_DEPTH == 192 && \
    !defined(DES_BS_NO_MMX)
#include <emmintrin.h>
#include <mmintrin.h>

typedef struct {
	__m128i f;
	__m64 g;
} vtype;

#define vst(dst, ofs, src) \
	_mm_store_si128(&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g

#define vxor(dst, a, b) \
	(dst).f = _mm_xor_si128((a).f, (b).f); \
	(dst).g = _mm_xor_si64((a).g, (b).g)

#define vand(dst, a, b) \
	(dst).f = _mm_and_si128((a).f, (b).f); \
	(dst).g = _mm_and_si64((a).g, (b).g)
#define vor(dst, a, b) \
	(dst).f = _mm_or_si128((a).f, (b).f); \
	(dst).g = _mm_or_si64((a).g, (b).g)
#define vandn(dst, a, b) \
	(dst).f = _mm_andnot_si128((b).f, (a).f); \
	(dst).g = _mm_andnot_si64((b).g, (a).g)

#define vshl1(dst, src) \
	(dst).f = _mm_add_epi8((src).f, (src).f); \
	(dst).g = _mm_add_pi8((src).g, (src).g)
#define vshl(dst, src, shift) \
	(dst).f = _mm_slli_epi64((src).f, (shift)); \
	(dst).g = _mm_slli_si64((src).g, (shift))
#define vshr(dst, src, shift) \
	(dst).f = _mm_srli_epi64((src).f, (shift)); \
	(dst).g = _mm_srli_si64((src).g, (shift))

#elif defined(__SSE2__) && \
    ((ARCH_BITS == 64 && DES_BS_DEPTH == 192) || \
    (ARCH_BITS == 32 && DES_BS_DEPTH == 160))
#include <emmintrin.h>

typedef struct {
	__m128i f;
	unsigned ARCH_WORD g;
} vtype;

#define vst(dst, ofs, src) \
	_mm_store_si128(&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g

#define vxor(dst, a, b) \
	(dst).f = _mm_xor_si128((a).f, (b).f); \
	(dst).g = (a).g ^ (b).g

#define vnot(dst, a) \
	(dst).f = _mm_xor_si128((a).f, vones.f); \
	(dst).g = ~(a).g
#define vand(dst, a, b) \
	(dst).f = _mm_and_si128((a).f, (b).f); \
	(dst).g = (a).g & (b).g
#define vor(dst, a, b) \
	(dst).f = _mm_or_si128((a).f, (b).f); \
	(dst).g = (a).g | (b).g
#define vandn(dst, a, b) \
	(dst).f = _mm_andnot_si128((b).f, (a).f); \
	(dst).g = (a).g & ~(b).g

#define vshl1(dst, src) \
	(dst).f = _mm_add_epi8((src).f, (src).f); \
	(dst).g = (src).g << 1
#define vshl(dst, src, shift) \
	(dst).f = _mm_slli_epi64((src).f, (shift)); \
	(dst).g = (src).g << (shift)
#define vshr(dst, src, shift) \
	(dst).f = _mm_srli_epi64((src).f, (shift)); \
	(dst).g = (src).g >> (shift)

#elif defined(__SSE2__) && defined(__MMX__) && \
    ((ARCH_BITS == 64 && DES_BS_DEPTH == 256) || \
    (ARCH_BITS == 32 && DES_BS_DEPTH == 224))
#include <emmintrin.h>
#include <mmintrin.h>

typedef struct {
	__m128i f;
	__m64 g;
	unsigned ARCH_WORD h;
} vtype;

#define vst(dst, ofs, src) \
	_mm_store_si128(&((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f, \
	    (src).f); \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g; \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->h = (src).h

#define vxor(dst, a, b) \
	(dst).f = _mm_xor_si128((a).f, (b).f); \
	(dst).g = _mm_xor_si64((a).g, (b).g); \
	(dst).h = (a).h ^ (b).h

#define vnot(dst, a) \
	(dst).f = _mm_xor_si128((a).f, vones.f); \
	(dst).g = _mm_xor_si64((a).g, vones.g); \
	(dst).h = ~(a).h
#define vand(dst, a, b) \
	(dst).f = _mm_and_si128((a).f, (b).f); \
	(dst).g = _mm_and_si64((a).g, (b).g); \
	(dst).h = (a).h & (b).h
#define vor(dst, a, b) \
	(dst).f = _mm_or_si128((a).f, (b).f); \
	(dst).g = _mm_or_si64((a).g, (b).g); \
	(dst).h = (a).h | (b).h
#define vandn(dst, a, b) \
	(dst).f = _mm_andnot_si128((b).f, (a).f); \
	(dst).g = _mm_andnot_si64((b).g, (a).g); \
	(dst).h = (a).h & ~(b).h

#define vshl1(dst, src) \
	(dst).f = _mm_add_epi8((src).f, (src).f); \
	(dst).g = _mm_add_pi8((src).g, (src).g); \
	(dst).h = (src).h << 1
#define vshl(dst, src, shift) \
	(dst).f = _mm_slli_epi64((src).f, (shift)); \
	(dst).g = _mm_slli_si64((src).g, (shift)); \
	(dst).h = (src).h << (shift)
#define vshr(dst, src, shift) \
	(dst).f = _mm_srli_epi64((src).f, (shift)); \
	(dst).g = _mm_srli_si64((src).g, (shift)); \
	(dst).h = (src).h >> (shift)

#elif defined(__MMX__) && ARCH_BITS != 64 && DES_BS_DEPTH == 64
#include <mmintrin.h>

typedef __m64 vtype;

#define vxorf(a, b) \
	_mm_xor_si64((a), (b))

#define vand(dst, a, b) \
	(dst) = _mm_and_si64((a), (b))
#define vor(dst, a, b) \
	(dst) = _mm_or_si64((a), (b))
#define vandn(dst, a, b) \
	(dst) = _mm_andnot_si64((b), (a))

#define vshl1(dst, src) \
	(dst) = _mm_add_pi8((src), (src))
#define vshl(dst, src, shift) \
	(dst) = _mm_slli_si64((src), (shift))
#define vshr(dst, src, shift) \
	(dst) = _mm_srli_si64((src), (shift))

#elif defined(__MMX__) && ARCH_BITS == 32 && DES_BS_DEPTH == 96
#include <mmintrin.h>

typedef struct {
	__m64 f;
	unsigned ARCH_WORD g;
} vtype;

#define vst(dst, ofs, src) \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->f = (src).f; \
	((vtype *)((DES_bs_vector *)&(dst) + (ofs)))->g = (src).g

#define vxor(dst, a, b) \
	(dst).f = _mm_xor_si64((a).f, (b).f); \
	(dst).g = (a).g ^ (b).g

#define vnot(dst, a) \
	(dst).f = _mm_xor_si64((a).f, vones.f); \
	(dst).g = ~(a).g
#define vand(dst, a, b) \
	(dst).f = _mm_and_si64((a).f, (b).f); \
	(dst).g = (a).g & (b).g
#define vor(dst, a, b) \
	(dst).f = _mm_or_si64((a).f, (b).f); \
	(dst).g = (a).g | (b).g
#define vandn(dst, a, b) \
	(dst).f = _mm_andnot_si64((b).f, (a).f); \
	(dst).g = (a).g & ~(b).g

#define vshl1(dst, src) \
	(dst).f = _mm_add_pi8((src).f, (src).f); \
	(dst).g = (src).g << 1
#define vshl(dst, src, shift) \
	(dst).f = _mm_slli_si64((src).f, (shift)); \
	(dst).g = (src).g << (shift)
#define vshr(dst, src, shift) \
	(dst).f = _mm_srli_si64((src).f, (shift)); \
	(dst).g = (src).g >> (shift)

#else

#if DES_BS_VECTOR
#undef DES_BS_VECTOR_LOOPS
#define DES_BS_VECTOR_LOOPS 1
#endif

typedef unsigned ARCH_WORD vtype;

#define vxorf(a, b) \
	((a) ^ (b))

#define vnot(dst, a) \
	(dst) = ~(a)
#define vand(dst, a, b) \
	(dst) = (a) & (b)
#define vor(dst, a, b) \
	(dst) = (a) | (b)
#define vandn(dst, a, b) \
	(dst) = (a) & ~(b)
#define vsel(dst, a, b, c) \
	(dst) = (((a) & ~(c)) ^ ((b) & (c)))

#define vshl(dst, src, shift) \
	(dst) = (src) << (shift)
#define vshr(dst, src, shift) \
	(dst) = (src) >> (shift)

/* Assume that 0 always fits in one load immediate instruction */
#undef vzero
#define vzero 0

/* Archs friendly to use of immediate values */
#if defined(__x86_64__) || defined(__i386__)
#undef vones
#define vones (~(vtype)0)
#endif

#endif

#ifndef vst
#define vst(dst, ofs, src) \
	*((vtype *)((DES_bs_vector *)&(dst) + (ofs))) = (src)
#endif

#if !defined(vxor) && defined(vxorf)
#define vxor(dst, a, b) \
	(dst) = vxorf((a), (b))
#endif
#if !defined(vxorf) && defined(vxor)
/*
 * This requires gcc's "Statement Exprs" extension (also supported by a number
 * of other C compilers).
 */
#define vxorf(a, b) \
	({ vtype tmp; vxor(tmp, (a), (b)); tmp; })
#endif

#ifndef vnot
#define vnot(dst, a) \
	vxor((dst), (a), vones)
#endif

#ifndef vshl1
#define vshl1(dst, src) \
	vshl((dst), (src), 1)
#endif

#if !DES_BS_VECTOR_LOOPS && defined(vshl) && defined(vshr)
#define DES_BS_VECTOR_LOOPS_K 0
#define DEPTH_K
#define for_each_depth_k()

#define kvtype vtype
#define kvand vand
#define kvor vor
#define kvshl1 vshl1
#define kvshl vshl
#define kvshr vshr
#else
#if DES_BS_VECTOR
#define DES_BS_VECTOR_LOOPS_K 1
#define DEPTH_K				[depth]
#define for_each_depth_k() \
	for (depth = 0; depth < DES_BS_VECTOR; depth++)
#else
#define DES_BS_VECTOR_LOOPS_K 0
#endif

typedef unsigned ARCH_WORD kvtype;
#define kvand(dst, a, b) \
	(dst) = (a) & (b)
#define kvor(dst, a, b) \
	(dst) = (a) | (b)
#define kvshl1(dst, src) \
	(dst) = (src) << 1
#define kvshl(dst, src, shift) \
	(dst) = (src) << (shift)
#define kvshr(dst, src, shift) \
	(dst) = (src) >> (shift)
#endif

#if !DES_BS_VECTOR || DES_BS_VECTOR_LOOPS_K
#ifdef __x86_64__
#define mask01 0x0101010101010101UL
#elif __i386__
#define mask01 0x01010101UL
#else
#undef mask01
#endif
#ifdef mask01
#define mask02 (mask01 << 1)
#define mask04 (mask01 << 2)
#define mask08 (mask01 << 3)
#define mask10 (mask01 << 4)
#define mask20 (mask01 << 5)
#define mask40 (mask01 << 6)
#define mask80 (mask01 << 7)
#endif
#endif

#ifndef mask01
#define mask01 (*(kvtype *)&DES_bs_all.masks[0])
#define mask02 (*(kvtype *)&DES_bs_all.masks[1])
#define mask04 (*(kvtype *)&DES_bs_all.masks[2])
#define mask08 (*(kvtype *)&DES_bs_all.masks[3])
#define mask10 (*(kvtype *)&DES_bs_all.masks[4])
#define mask20 (*(kvtype *)&DES_bs_all.masks[5])
#define mask40 (*(kvtype *)&DES_bs_all.masks[6])
#define mask80 (*(kvtype *)&DES_bs_all.masks[7])
#endif

#ifdef __i386__
/* register-starved */
#define LOAD_V \
	kvtype v0 = *(kvtype *)&vp[0]; \
	kvtype v4 = *(kvtype *)&vp[4];
#define v1 *(kvtype *)&vp[1]
#define v2 *(kvtype *)&vp[2]
#define v3 *(kvtype *)&vp[3]
#define v5 *(kvtype *)&vp[5]
#define v6 *(kvtype *)&vp[6]
#define v7 *(kvtype *)&vp[7]
#else
#define LOAD_V \
	kvtype v0 = *(kvtype *)&vp[0]; \
	kvtype v1 = *(kvtype *)&vp[1]; \
	kvtype v2 = *(kvtype *)&vp[2]; \
	kvtype v3 = *(kvtype *)&vp[3]; \
	kvtype v4 = *(kvtype *)&vp[4]; \
	kvtype v5 = *(kvtype *)&vp[5]; \
	kvtype v6 = *(kvtype *)&vp[6]; \
	kvtype v7 = *(kvtype *)&vp[7];
#endif

#define kvand_shl1_or(dst, src, mask) \
	kvand(tmp, src, mask); \
	kvshl1(tmp, tmp); \
	kvor(dst, dst, tmp)

#define kvand_shl_or(dst, src, mask, shift) \
	kvand(tmp, src, mask); \
	kvshl(tmp, tmp, shift); \
	kvor(dst, dst, tmp)

#define kvand_shl1(dst, src, mask) \
	kvand(tmp, src, mask); \
	kvshl1(dst, tmp)

#define kvand_or(dst, src, mask) \
	kvand(tmp, src, mask); \
	kvor(dst, dst, tmp)

#define kvand_shr_or(dst, src, mask, shift) \
	kvand(tmp, src, mask); \
	kvshr(tmp, tmp, shift); \
	kvor(dst, dst, tmp)

#define kvand_shr(dst, src, mask, shift) \
	kvand(tmp, src, mask); \
	kvshr(dst, tmp, shift)

#define FINALIZE_NEXT_KEY_BIT_0 { \
	kvtype m = mask01, va, vb, tmp; \
	kvand(va, v0, m); \
	kvand_shl1(vb, v1, m); \
	kvand_shl_or(va, v2, m, 2); \
	kvand_shl_or(vb, v3, m, 3); \
	kvand_shl_or(va, v4, m, 4); \
	kvand_shl_or(vb, v5, m, 5); \
	kvand_shl_or(va, v6, m, 6); \
	kvand_shl_or(vb, v7, m, 7); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#define FINALIZE_NEXT_KEY_BIT_1 { \
	kvtype m = mask02, va, vb, tmp; \
	kvand_shr(va, v0, m, 1); \
	kvand(vb, v1, m); \
	kvand_shl1_or(va, v2, m); \
	kvand_shl_or(vb, v3, m, 2); \
	kvand_shl_or(va, v4, m, 3); \
	kvand_shl_or(vb, v5, m, 4); \
	kvand_shl_or(va, v6, m, 5); \
	kvand_shl_or(vb, v7, m, 6); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#define FINALIZE_NEXT_KEY_BIT_2 { \
	kvtype m = mask04, va, vb, tmp; \
	kvand_shr(va, v0, m, 2); \
	kvand_shr(vb, v1, m, 1); \
	kvand_or(va, v2, m); \
	kvand_shl1_or(vb, v3, m); \
	kvand_shl_or(va, v4, m, 2); \
	kvand_shl_or(vb, v5, m, 3); \
	kvand_shl_or(va, v6, m, 4); \
	kvand_shl_or(vb, v7, m, 5); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#define FINALIZE_NEXT_KEY_BIT_3 { \
	kvtype m = mask08, va, vb, tmp; \
	kvand_shr(va, v0, m, 3); \
	kvand_shr(vb, v1, m, 2); \
	kvand_shr_or(va, v2, m, 1); \
	kvand_or(vb, v3, m); \
	kvand_shl1_or(va, v4, m); \
	kvand_shl_or(vb, v5, m, 2); \
	kvand_shl_or(va, v6, m, 3); \
	kvand_shl_or(vb, v7, m, 4); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#define FINALIZE_NEXT_KEY_BIT_4 { \
	kvtype m = mask10, va, vb, tmp; \
	kvand_shr(va, v0, m, 4); \
	kvand_shr(vb, v1, m, 3); \
	kvand_shr_or(va, v2, m, 2); \
	kvand_shr_or(vb, v3, m, 1); \
	kvand_or(va, v4, m); \
	kvand_shl1_or(vb, v5, m); \
	kvand_shl_or(va, v6, m, 2); \
	kvand_shl_or(vb, v7, m, 3); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#define FINALIZE_NEXT_KEY_BIT_5 { \
	kvtype m = mask20, va, vb, tmp; \
	kvand_shr(va, v0, m, 5); \
	kvand_shr(vb, v1, m, 4); \
	kvand_shr_or(va, v2, m, 3); \
	kvand_shr_or(vb, v3, m, 2); \
	kvand_shr_or(va, v4, m, 1); \
	kvand_or(vb, v5, m); \
	kvand_shl1_or(va, v6, m); \
	kvand_shl_or(vb, v7, m, 2); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#define FINALIZE_NEXT_KEY_BIT_6 { \
	kvtype m = mask40, va, vb, tmp; \
	kvand_shr(va, v0, m, 6); \
	kvand_shr(vb, v1, m, 5); \
	kvand_shr_or(va, v2, m, 4); \
	kvand_shr_or(vb, v3, m, 3); \
	kvand_shr_or(va, v4, m, 2); \
	kvand_shr_or(vb, v5, m, 1); \
	kvand_or(va, v6, m); \
	kvand_shl1_or(vb, v7, m); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#define FINALIZE_NEXT_KEY_BIT_7 { \
	kvtype m = mask80, va, vb, tmp; \
	kvand_shr(va, v0, m, 7); \
	kvand_shr(vb, v1, m, 6); \
	kvand_shr_or(va, v2, m, 5); \
	kvand_shr_or(vb, v3, m, 4); \
	kvand_shr_or(va, v4, m, 3); \
	kvand_shr_or(vb, v5, m, 2); \
	kvand_shr_or(va, v6, m, 1); \
	kvand_or(vb, v7, m); \
	kvor(*(kvtype *)kp, va, vb); \
	kp++; \
}

#if DES_bs_mt
static MAYBE_INLINE void DES_bs_finalize_keys(int t)
#else
static MAYBE_INLINE void DES_bs_finalize_keys(void)
#endif
{
#if DES_BS_VECTOR_LOOPS_K
	int depth;
#endif

	for_each_depth_k() {
		DES_bs_vector *kp = (DES_bs_vector *)&DES_bs_all.K[0] DEPTH_K;
		int ic;
		for (ic = 0; ic < 8; ic++) {
			DES_bs_vector *vp =
			    (DES_bs_vector *)&DES_bs_all.xkeys.v[ic][0] DEPTH_K;
			LOAD_V
			FINALIZE_NEXT_KEY_BIT_0
			FINALIZE_NEXT_KEY_BIT_1
			FINALIZE_NEXT_KEY_BIT_2
			FINALIZE_NEXT_KEY_BIT_3
			FINALIZE_NEXT_KEY_BIT_4
			FINALIZE_NEXT_KEY_BIT_5
			FINALIZE_NEXT_KEY_BIT_6
		}
	}

#if DES_BS_EXPAND
	{
		int index;
		for (index = 0; index < 0x300; index++)
		for_each_depth_k() {
#if DES_BS_VECTOR_LOOPS_K
			DES_bs_all.KS.v[index] DEPTH_K =
			    DES_bs_all.KSp[index] DEPTH_K;
#else
			vst(*(kvtype *)&DES_bs_all.KS.v[index], 0,
			    *(kvtype *)DES_bs_all.KSp[index]);
#endif
		}
	}
#endif
}

#endif

#if DES_bs_mt
MAYBE_INLINE void DES_bs_set_salt_for_thread(int t, unsigned int salt)
#else
void DES_bs_set_salt(ARCH_WORD salt)
#endif
{
	unsigned int new = salt;
	unsigned int old = DES_bs_all.salt;
	int dst;

	DES_bs_all.salt = new;

	for (dst = 0; dst < 24; dst++) {
		if ((new ^ old) & 1) {
			DES_bs_vector *sp1, *sp2;
			int src1 = dst;
			int src2 = dst + 24;
			if (new & 1) {
				src1 = src2;
				src2 = dst;
			}
			sp1 = DES_bs_all.Ens[src1];
			sp2 = DES_bs_all.Ens[src2];
			DES_bs_all.E.E[dst] = (ARCH_WORD *)sp1;
			DES_bs_all.E.E[dst + 24] = (ARCH_WORD *)sp2;
			DES_bs_all.E.E[dst + 48] = (ARCH_WORD *)(sp1 + 32);
			DES_bs_all.E.E[dst + 72] = (ARCH_WORD *)(sp2 + 32);
		}
		new >>= 1;
		old >>= 1;
		if (new == old)
			break;
	}
}

#if !DES_BS_ASM

/* Include the S-boxes here so that the compiler can inline them */
#if DES_BS == 3
#include "sboxes-s.c"
#elif DES_BS == 2
#include "sboxes.c"
#else
#undef andn
#include "nonstd.c"
#endif

#define b				DES_bs_all.B
#define e				DES_bs_all.E.E

#if DES_BS_VECTOR_LOOPS
#define kd				[depth]
#define bd				[depth]
#define ed				[depth]
#define DEPTH				[depth]
#define for_each_depth() \
	for (depth = 0; depth < DES_BS_VECTOR; depth++)
#else
#if DES_BS_EXPAND
#define kd
#else
#define kd				[0]
#endif
#define bd
#define ed				[0]
#define DEPTH
#define for_each_depth()
#endif

#define DES_bs_clear_block_8(i) \
	for_each_depth() { \
		vst(b[i] bd, 0, zero); \
		vst(b[i] bd, 1, zero); \
		vst(b[i] bd, 2, zero); \
		vst(b[i] bd, 3, zero); \
		vst(b[i] bd, 4, zero); \
		vst(b[i] bd, 5, zero); \
		vst(b[i] bd, 6, zero); \
		vst(b[i] bd, 7, zero); \
	}

#define DES_bs_clear_block \
	DES_bs_clear_block_8(0); \
	DES_bs_clear_block_8(8); \
	DES_bs_clear_block_8(16); \
	DES_bs_clear_block_8(24); \
	DES_bs_clear_block_8(32); \
	DES_bs_clear_block_8(40); \
	DES_bs_clear_block_8(48); \
	DES_bs_clear_block_8(56);

#define DES_bs_set_block_8(i, v0, v1, v2, v3, v4, v5, v6, v7) \
	for_each_depth() { \
		vst(b[i] bd, 0, v0); \
		vst(b[i] bd, 1, v1); \
		vst(b[i] bd, 2, v2); \
		vst(b[i] bd, 3, v3); \
		vst(b[i] bd, 4, v4); \
		vst(b[i] bd, 5, v5); \
		vst(b[i] bd, 6, v6); \
		vst(b[i] bd, 7, v7); \
	}

#define x(p) vxorf(*(vtype *)&e[p] ed, *(vtype *)&k[p] kd)
#define y(p, q) vxorf(*(vtype *)&b[p] bd, *(vtype *)&k[q] kd)
#define z(r) ((vtype *)&b[r] bd)

void DES_bs_crypt_25(int keys_count)
{
#if DES_bs_mt
	int t, n = (keys_count + (DES_BS_DEPTH - 1)) / DES_BS_DEPTH;
#endif

#ifdef _OPENMP
#pragma omp parallel for default(none) private(t) shared(n, DES_bs_all_p, keys_count)
#endif
	for_each_t(n) {
#if DES_BS_EXPAND
		DES_bs_vector *k;
#else
		ARCH_WORD **k;
#endif
		int iterations, rounds_and_swapped;
#if DES_BS_VECTOR_LOOPS
		int depth;
#endif

		if (DES_bs_all.keys_changed)
			goto finalize_keys;

body:
#if DES_bs_mt
		DES_bs_set_salt_for_thread(t, DES_bs_all_by_tnum(-1).salt);
#endif

		{
			vtype zero = vzero;
			DES_bs_clear_block
		}

#if DES_BS_EXPAND
		k = DES_bs_all.KS.v;
#else
		k = DES_bs_all.KS.p;
#endif
		rounds_and_swapped = 8;
		iterations = 25;

start:
		for_each_depth()
		s1(x(0), x(1), x(2), x(3), x(4), x(5),
			z(40), z(48), z(54), z(62));
		for_each_depth()
		s2(x(6), x(7), x(8), x(9), x(10), x(11),
			z(44), z(59), z(33), z(49));
		for_each_depth()
		s3(y(7, 12), y(8, 13), y(9, 14),
			y(10, 15), y(11, 16), y(12, 17),
			z(55), z(47), z(61), z(37));
		for_each_depth()
		s4(y(11, 18), y(12, 19), y(13, 20),
			y(14, 21), y(15, 22), y(16, 23),
			z(57), z(51), z(41), z(32));
		for_each_depth()
		s5(x(24), x(25), x(26), x(27), x(28), x(29),
			z(39), z(45), z(56), z(34));
		for_each_depth()
		s6(x(30), x(31), x(32), x(33), x(34), x(35),
			z(35), z(60), z(42), z(50));
		for_each_depth()
		s7(y(23, 36), y(24, 37), y(25, 38),
			y(26, 39), y(27, 40), y(28, 41),
			z(63), z(43), z(53), z(38));
		for_each_depth()
		s8(y(27, 42), y(28, 43), y(29, 44),
			y(30, 45), y(31, 46), y(0, 47),
			z(36), z(58), z(46), z(52));

		if (rounds_and_swapped == 0x100) goto next;

swap:
		for_each_depth()
		s1(x(48), x(49), x(50), x(51), x(52), x(53),
			z(8), z(16), z(22), z(30));
		for_each_depth()
		s2(x(54), x(55), x(56), x(57), x(58), x(59),
			z(12), z(27), z(1), z(17));
		for_each_depth()
		s3(y(39, 60), y(40, 61), y(41, 62),
			y(42, 63), y(43, 64), y(44, 65),
			z(23), z(15), z(29), z(5));
		for_each_depth()
		s4(y(43, 66), y(44, 67), y(45, 68),
			y(46, 69), y(47, 70), y(48, 71),
			z(25), z(19), z(9), z(0));
		for_each_depth()
		s5(x(72), x(73), x(74), x(75), x(76), x(77),
			z(7), z(13), z(24), z(2));
		for_each_depth()
		s6(x(78), x(79), x(80), x(81), x(82), x(83),
			z(3), z(28), z(10), z(18));
		for_each_depth()
		s7(y(55, 84), y(56, 85), y(57, 86),
			y(58, 87), y(59, 88), y(60, 89),
			z(31), z(11), z(21), z(6));
		for_each_depth()
		s8(y(59, 90), y(60, 91), y(61, 92),
			y(62, 93), y(63, 94), y(32, 95),
			z(4), z(26), z(14), z(20));

		k += 96;

		if (--rounds_and_swapped) goto start;
		k -= (0x300 + 48);
		rounds_and_swapped = 0x108;
		if (--iterations) goto swap;
#if DES_bs_mt
		continue;
#else
		return;
#endif

next:
		k -= (0x300 - 48);
		rounds_and_swapped = 8;
		iterations--;
		goto start;

finalize_keys:
		DES_bs_all.keys_changed = 0;
#if DES_bs_mt
		DES_bs_finalize_keys(t);
#else
		DES_bs_finalize_keys();
#endif
		goto body;
	}
}

void DES_bs_crypt(int count, int keys_count)
{
#if DES_bs_mt
	int t, n = (keys_count + (DES_BS_DEPTH - 1)) / DES_BS_DEPTH;
#endif

#ifdef _OPENMP
#pragma omp parallel for default(none) private(t) shared(n, DES_bs_all_p, count, keys_count)
#endif
	for_each_t(n) {
#if DES_BS_EXPAND
		DES_bs_vector *k;
#else
		ARCH_WORD **k;
#endif
		int iterations, rounds_and_swapped;
#if DES_BS_VECTOR_LOOPS
		int depth;
#endif

		if (DES_bs_all.keys_changed)
			goto finalize_keys;

body:
#if DES_bs_mt
		DES_bs_set_salt_for_thread(t, DES_bs_all_by_tnum(-1).salt);
#endif

		{
			vtype zero = vzero;
			DES_bs_clear_block
		}

#if DES_BS_EXPAND
		k = DES_bs_all.KS.v;
#else
		k = DES_bs_all.KS.p;
#endif
		rounds_and_swapped = 8;
		iterations = count;

start:
		for_each_depth()
		s1(x(0), x(1), x(2), x(3), x(4), x(5),
			z(40), z(48), z(54), z(62));
		for_each_depth()
		s2(x(6), x(7), x(8), x(9), x(10), x(11),
			z(44), z(59), z(33), z(49));
		for_each_depth()
		s3(x(12), x(13), x(14), x(15), x(16), x(17),
			z(55), z(47), z(61), z(37));
		for_each_depth()
		s4(x(18), x(19), x(20), x(21), x(22), x(23),
			z(57), z(51), z(41), z(32));
		for_each_depth()
		s5(x(24), x(25), x(26), x(27), x(28), x(29),
			z(39), z(45), z(56), z(34));
		for_each_depth()
		s6(x(30), x(31), x(32), x(33), x(34), x(35),
			z(35), z(60), z(42), z(50));
		for_each_depth()
		s7(x(36), x(37), x(38), x(39), x(40), x(41),
			z(63), z(43), z(53), z(38));
		for_each_depth()
		s8(x(42), x(43), x(44), x(45), x(46), x(47),
			z(36), z(58), z(46), z(52));

		if (rounds_and_swapped == 0x100) goto next;

swap:
		for_each_depth()
		s1(x(48), x(49), x(50), x(51), x(52), x(53),
			z(8), z(16), z(22), z(30));
		for_each_depth()
		s2(x(54), x(55), x(56), x(57), x(58), x(59),
			z(12), z(27), z(1), z(17));
		for_each_depth()
		s3(x(60), x(61), x(62), x(63), x(64), x(65),
			z(23), z(15), z(29), z(5));
		for_each_depth()
		s4(x(66), x(67), x(68), x(69), x(70), x(71),
			z(25), z(19), z(9), z(0));
		for_each_depth()
		s5(x(72), x(73), x(74), x(75), x(76), x(77),
			z(7), z(13), z(24), z(2));
		for_each_depth()
		s6(x(78), x(79), x(80), x(81), x(82), x(83),
			z(3), z(28), z(10), z(18));
		for_each_depth()
		s7(x(84), x(85), x(86), x(87), x(88), x(89),
			z(31), z(11), z(21), z(6));
		for_each_depth()
		s8(x(90), x(91), x(92), x(93), x(94), x(95),
			z(4), z(26), z(14), z(20));

		k += 96;

		if (--rounds_and_swapped) goto start;
		k -= (0x300 + 48);
		rounds_and_swapped = 0x108;
		if (--iterations) goto swap;
#if DES_bs_mt
		continue;
#else
		return;
#endif

next:
		k -= (0x300 - 48);
		rounds_and_swapped = 8;
		if (--iterations) goto start;
#if DES_bs_mt
		continue;
#else
		return;
#endif

finalize_keys:
		DES_bs_all.keys_changed = 0;
#if DES_bs_mt
		DES_bs_finalize_keys(t);
#else
		DES_bs_finalize_keys();
#endif
		goto body;
	}
}

#undef x

#if DES_bs_mt
static MAYBE_INLINE void DES_bs_finalize_keys_LM(int t)
#else
static MAYBE_INLINE void DES_bs_finalize_keys_LM(void)
#endif
{
#if DES_BS_VECTOR_LOOPS_K
	int depth;
#endif

	for_each_depth_k() {
		DES_bs_vector *kp = (DES_bs_vector *)&DES_bs_all.K[0] DEPTH_K;
		int ic;
		for (ic = 0; ic < 7; ic++) {
			DES_bs_vector *vp =
			    (DES_bs_vector *)&DES_bs_all.xkeys.v[ic][0] DEPTH_K;
			LOAD_V
			FINALIZE_NEXT_KEY_BIT_0
			FINALIZE_NEXT_KEY_BIT_1
			FINALIZE_NEXT_KEY_BIT_2
			FINALIZE_NEXT_KEY_BIT_3
			FINALIZE_NEXT_KEY_BIT_4
			FINALIZE_NEXT_KEY_BIT_5
			FINALIZE_NEXT_KEY_BIT_6
			FINALIZE_NEXT_KEY_BIT_7
		}
	}
}

#undef v1
#undef v2
#undef v3
#undef v5
#undef v6
#undef v7

#undef kd
#if DES_BS_VECTOR_LOOPS
#define kd				[depth]
#else
#define kd				[0]
#endif

int DES_bs_crypt_LM(int *pcount, struct db_salt *salt)
{
	int keys_count = *pcount;
#if DES_bs_mt
	int t, n = (keys_count + (DES_BS_DEPTH - 1)) / DES_BS_DEPTH;
#endif

#ifdef _OPENMP
#pragma omp parallel for default(none) private(t) shared(n, DES_bs_all_p, keys_count)
#endif
	for_each_t(n) {
		ARCH_WORD **k;
		int rounds;
#if DES_BS_VECTOR_LOOPS
		int depth;
#endif

		{
			vtype z = vzero, o = vones;
			DES_bs_set_block_8(0, z, z, z, z, z, z, z, z);
			DES_bs_set_block_8(8, o, o, o, z, o, z, z, z);
			DES_bs_set_block_8(16, z, z, z, z, z, z, z, o);
			DES_bs_set_block_8(24, z, z, o, z, z, o, o, o);
			DES_bs_set_block_8(32, z, z, z, o, z, o, o, o);
			DES_bs_set_block_8(40, z, z, z, z, z, o, z, z);
			DES_bs_set_block_8(48, o, o, z, z, z, z, o, z);
			DES_bs_set_block_8(56, o, z, o, z, o, o, o, o);
		}

#if DES_bs_mt
		DES_bs_finalize_keys_LM(t);
#else
		DES_bs_finalize_keys_LM();
#endif

		k = DES_bs_all.KS.p;
		rounds = 8;

		do {
			for_each_depth()
			s1(y(31, 0), y(0, 1), y(1, 2),
				y(2, 3), y(3, 4), y(4, 5),
				z(40), z(48), z(54), z(62));
			for_each_depth()
			s2(y(3, 6), y(4, 7), y(5, 8),
				y(6, 9), y(7, 10), y(8, 11),
				z(44), z(59), z(33), z(49));
			for_each_depth()
			s3(y(7, 12), y(8, 13), y(9, 14),
				y(10, 15), y(11, 16), y(12, 17),
				z(55), z(47), z(61), z(37));
			for_each_depth()
			s4(y(11, 18), y(12, 19), y(13, 20),
				y(14, 21), y(15, 22), y(16, 23),
				z(57), z(51), z(41), z(32));
			for_each_depth()
			s5(y(15, 24), y(16, 25), y(17, 26),
				y(18, 27), y(19, 28), y(20, 29),
				z(39), z(45), z(56), z(34));
			for_each_depth()
			s6(y(19, 30), y(20, 31), y(21, 32),
				y(22, 33), y(23, 34), y(24, 35),
				z(35), z(60), z(42), z(50));
			for_each_depth()
			s7(y(23, 36), y(24, 37), y(25, 38),
				y(26, 39), y(27, 40), y(28, 41),
				z(63), z(43), z(53), z(38));
			for_each_depth()
			s8(y(27, 42), y(28, 43), y(29, 44),
				y(30, 45), y(31, 46), y(0, 47),
				z(36), z(58), z(46), z(52));

			for_each_depth()
			s1(y(63, 48), y(32, 49), y(33, 50),
				y(34, 51), y(35, 52), y(36, 53),
				z(8), z(16), z(22), z(30));
			for_each_depth()
			s2(y(35, 54), y(36, 55), y(37, 56),
				y(38, 57), y(39, 58), y(40, 59),
				z(12), z(27), z(1), z(17));
			for_each_depth()
			s3(y(39, 60), y(40, 61), y(41, 62),
				y(42, 63), y(43, 64), y(44, 65),
				z(23), z(15), z(29), z(5));
			for_each_depth()
			s4(y(43, 66), y(44, 67), y(45, 68),
				y(46, 69), y(47, 70), y(48, 71),
				z(25), z(19), z(9), z(0));
			for_each_depth()
			s5(y(47, 72), y(48, 73), y(49, 74),
				y(50, 75), y(51, 76), y(52, 77),
				z(7), z(13), z(24), z(2));
			for_each_depth()
			s6(y(51, 78), y(52, 79), y(53, 80),
				y(54, 81), y(55, 82), y(56, 83),
				z(3), z(28), z(10), z(18));
			for_each_depth()
			s7(y(55, 84), y(56, 85), y(57, 86),
				y(58, 87), y(59, 88), y(60, 89),
				z(31), z(11), z(21), z(6));
			for_each_depth()
			s8(y(59, 90), y(60, 91), y(61, 92),
				y(62, 93), y(63, 94), y(32, 95),
				z(4), z(26), z(14), z(20));

			k += 96;
		} while (--rounds);
	}

	return keys_count;
}
#endif
