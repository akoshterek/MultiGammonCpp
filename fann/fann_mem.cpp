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

#include <memory.h>

#if defined FANN_USE_SSE
#include <emmintrin.h> 
#endif

#include "fann_mem.h"


FANN_RESTRICT void *fann_malloc(size_t size)
{
#if defined FANN_USE_SSE
	return _mm_malloc(size, FANN_SSEALIGN_SIZE);
#else
	return malloc(size);
#endif
}

FANN_RESTRICT void *fann_calloc(size_t num, size_t size)
{
#if defined FANN_USE_SSE
	void *ptr = _mm_malloc(num * size, FANN_SSEALIGN_SIZE);
	if(ptr) 
		memset(ptr, 0, num * size);
	fann_sse_aligned(ptr);
	return ptr;
#else
	return calloc(num, size);
#endif
}

FANN_NOALIAS void fann_free(void *memblock)
{
#if defined FANN_USE_SSE
	_mm_free(memblock);
#else
	free(memblock);
#endif
}
