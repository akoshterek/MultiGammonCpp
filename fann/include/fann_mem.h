/*
  Fast Artificial Neural Network Library (fann)
  Copyright (C) 2003 Steffen Nissen (lukesky@diku.dk)
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Memory routines for allocating 16 bytes aligned memory (SSE requirement)
  Copyright (C) 2011 Alex Koshterek
*/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __fann_mem_h__
#define __fann_mem_h__ 

#include <malloc.h>

#if _MSC_VER > 1000
	#include <stddef.h>
	#define FANN_RESTRICT __declspec(restrict)
	#define FANN_NOALIAS __declspec(noalias) 
#else
	#include <stdint.h>
	#define FANN_RESTRICT 
	#define FANN_NOALIAS 
#endif /* _MSC_VER */

#if defined FANN_USE_SSE
	#if defined  FANN_USE_AVX 
		#define FANN_SSEALIGN_SIZE		32
	#else
		#define FANN_SSEALIGN_SIZE		16
	#endif //FANN_USE_AVX

	#ifdef _MSC_VER
		#define FANN_SSE_ALIGN(D) __declspec(align(FANN_SSEALIGN_SIZE)) D
	#else
		#define FANN_SSE_ALIGN(D) D __attribute__ ((aligned(FANN_SSEALIGN_SIZE)))
	#endif /* _MSC_VER */

	#define fann_sse_aligned(ptr) (!((intptr_t)(ptr) % FANN_SSEALIGN_SIZE))
#else
	#define FANN_SSE_ALIGN(D) D
	#define fann_sse_aligned(ptr) 1
#endif /* FANN_USE_SSE */

FANN_RESTRICT void *fann_malloc(size_t size);
FANN_RESTRICT void *fann_calloc(size_t num, size_t size);
FANN_NOALIAS void fann_free(void *memblock);

#endif  /* __fann_mem_h__ */
