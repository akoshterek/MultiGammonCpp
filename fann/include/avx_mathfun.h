/* SIMD (SSE1+MMX or SSE2) implementation of sin, cos, exp and log

   Inspired by Intel Approximate Math library, and based on the
   corresponding algorithms of the cephes math library

   The default is to use the SSE1 version. If you define FM_USE_SSE2 the
   the SSE2 intrinsics will be used in place of the MMX intrinsics. Do
   not expect any significant performance improvement with SSE2.
*/

/* Copyright (C) 2007  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/

#include <xmmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

#ifndef FM_USE_SSE2
#define FM_USE_SSE2
#endif

/* yes I know, the top of this file is quite ugly */
#ifdef _MSC_VER /* visual c++ */
# define ALIGN32_BEG __declspec(align(32))
# define ALIGN32_END 
#else /* gcc or icc */
# define ALIGN32_BEG
# define ALIGN32_END __attribute__((aligned(32)))
#endif

/* __m256 is ugly to write */
//typedef __m256 v4sf;  // vector of 4 float (sse1)
//But I like __m256 more. AK

#ifdef FM_USE_SSE2
# include <emmintrin.h>
//typedef __m256i v4si; // vector of 4 int (sse2)
#endif

//__inline __m256  __fastcall _mm_castsi128_ps(__m256i n) { return *(__m256 *) &n; } 
//__inline __m256i __fastcall _mm_castps_si128(__m256 n) { return *(__m256i*) &n; }

/* declare some SSE constants -- why can't I figure a better way to do that? */
#define _PS256_CONST(Name, Val)                                            \
  static const ALIGN32_BEG float _ps256_##Name[8] ALIGN32_END = { Val, Val, Val, Val, Val, Val, Val, Val }
#define _PI32256_CONST(Name, Val)                                            \
  static const ALIGN32_BEG int _pi32_256_##Name[8] ALIGN32_END = { Val, Val, Val, Val, Val, Val, Val, Val }
#define _PS256_CONST_TYPE(Name, Type, Val)                                 \
  static const ALIGN32_BEG Type _ps256_##Name[8] ALIGN32_END = { Val, Val, Val, Val, Val, Val, Val, Val }

_PS256_CONST(1  , 1.0f);
_PS256_CONST(0p5, 0.5f);
/* the smallest non denormalized float number */
_PS256_CONST_TYPE(min_norm_pos, int, 0x00800000);
_PS256_CONST_TYPE(mant_mask, int, 0x7f800000);
_PS256_CONST_TYPE(inv_mant_mask, int, ~0x7f800000);

_PS256_CONST_TYPE(sign_mask, int, 0x80000000);
_PS256_CONST_TYPE(inv_sign_mask, int, ~0x80000000);

_PI32256_CONST(1, 1);
_PI32256_CONST(inv1, ~1);
_PI32256_CONST(2, 2);
_PI32256_CONST(4, 4);
_PI32256_CONST(0x7f, 0x7f);


_PS256_CONST(exp_hi,	88.3762626647949f);
_PS256_CONST(exp_lo,	-88.3762626647949f);

_PS256_CONST(cephes_LOG2EF, 1.44269504088896341);
_PS256_CONST(cephes_exp_C1, 0.693359375);
_PS256_CONST(cephes_exp_C2, -2.12194440e-4);

_PS256_CONST(cephes_exp_p0, 1.9875691500E-4);
_PS256_CONST(cephes_exp_p1, 1.3981999507E-3);
_PS256_CONST(cephes_exp_p2, 8.3334519073E-3);
_PS256_CONST(cephes_exp_p3, 4.1665795894E-2);
_PS256_CONST(cephes_exp_p4, 1.6666665459E-1);
_PS256_CONST(cephes_exp_p5, 5.0000001201E-1);

__inline __m256 __fastcall exp256_ps(__m256 x) 
{
  __m256 tmp = _mm256_setzero_ps(), fx;
  __m256i emm0;
  __m256 one = *(__m256*)_ps256_1;

  x = _mm256_min_ps(x, *(__m256*)_ps256_exp_hi);
  x = _mm256_max_ps(x, *(__m256*)_ps256_exp_lo);

  /* express exp(x) as exp(g + n*log(2)) */
  fx = _mm256_mul_ps(x, *(__m256*)_ps256_cephes_LOG2EF);
  fx = _mm256_add_ps(fx, *(__m256*)_ps256_0p5);

  /* how to perform a floorf with SSE: just below */
  emm0 = _mm256_cvttps_epi32(fx);
  tmp  = _mm256_cvtepi32_ps(emm0);

  /* if greater, substract 1 */
  __m256 mask = _mm256_cmp_ps(tmp, fx, _CMP_GT_OS);//_mm_cmpgt_ps(tmp, fx);    
  mask = _mm256_and_ps(mask, one);
  fx = _mm256_sub_ps(tmp, mask);

  tmp = _mm256_mul_ps(fx, *(__m256*)_ps256_cephes_exp_C1);
  __m256 z = _mm256_mul_ps(fx, *(__m256*)_ps256_cephes_exp_C2);
  x = _mm256_sub_ps(x, tmp);
  x = _mm256_sub_ps(x, z);

  z = _mm256_mul_ps(x,x);
  
  __m256 y = *(__m256*)_ps256_cephes_exp_p0;
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *(__m256*)_ps256_cephes_exp_p1);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *(__m256*)_ps256_cephes_exp_p2);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *(__m256*)_ps256_cephes_exp_p3);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *(__m256*)_ps256_cephes_exp_p4);
  y = _mm256_mul_ps(y, x);
  y = _mm256_add_ps(y, *(__m256*)_ps256_cephes_exp_p5);
  y = _mm256_mul_ps(y, z);
  y = _mm256_add_ps(y, x);
  y = _mm256_add_ps(y, one);

  /* build 2^n */
  emm0 = _mm256_cvttps_epi32(fx);
  __m128 emm0lo_ps = _mm256_extractf128_ps(*((__m256 *)&emm0), 0);
  //__m128 emm0lo_ps = _mm256_castps256_ps128(*((__m256 *)&emm0));
  __m128 emm0hi_ps = _mm256_extractf128_ps(*((__m256 *)&emm0), 1);
  __m128i emm0lo = _mm_castps_si128(emm0lo_ps);
  __m128i emm0hi = _mm_castps_si128(emm0hi_ps);
  emm0lo = _mm_add_epi32(emm0lo, *(__m128i*)_pi32_0x7f);
  emm0lo = _mm_slli_epi32(emm0lo, 23);
  emm0hi = _mm_add_epi32(emm0hi, *(__m128i*)_pi32_0x7f);
  emm0hi = _mm_slli_epi32(emm0hi, 23);
  //emm0 = _mm_add_epi32(emm0, *(__m256i*)_pi32_0x7f);
  //emm0 = _mm_slli_epi32(emm0, 23);
  __m128 pow2nlo = _mm_castsi128_ps(emm0lo);
  __m128 pow2nhi = _mm_castsi128_ps(emm0hi);
  __m256 pow2n = _mm256_setzero_ps();
  pow2n = _mm256_insertf128_ps(pow2n, pow2nlo, 0);
  pow2n = _mm256_insertf128_ps(pow2n, pow2nhi, 1);
  y = _mm256_mul_ps(y, pow2n);
  return y;
}
