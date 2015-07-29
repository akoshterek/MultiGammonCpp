/*
Fast Artificial Neural Network Library (fann)
Copyright (C) 2003 Steffen Nissen (lukesky@diku.dk)
Copyright (C) 2011 Alex Koshterek (koshterek@gmail.com)

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
*/  

#ifndef __fann_sse_h__
#define __fann_sse_h__
#include "fann.h"

#if (defined FLOATFANN || defined DOUBLEFANN) && defined FANN_USE_SSE
/* Function: fann_run_sse
	Will run input through the neural network, returning an array of outputs, the number of which being 
	equal to the number of neurons in the output layer.
	Does the same as fann_run but uses SSE instructions

	See also:
		<fann_run>

	This function appears in FANN >= 2.1.0beta.
*/ 
FANN_EXTERNAL fann_type * FANN_API fann_run_sse(struct fann *ann, const fann_type * input);

/* Function: fann_test_sse
   Test with a set of inputs, and a set of desired outputs.
   This operation updates the mean square error, but does not
   change the network in any way.
   
   See also:
   		<fann_test_data>, <fann_train_sse>
   
   This function appears in FANN >= 1.0.0.
*/ 
FANN_EXTERNAL fann_type * FANN_API fann_test_sse(struct fann *ann, fann_type * input, 
												 fann_type * desired_output);

void fann_backpropagate_MSE_sse(struct fann *ann);

__inline __m128 __fastcall fann_activation_derived_ps(fann_activationfunc_enum activation_function,
								  fann_type steepness, fann_type *value, fann_type *sum);

void fann_update_weights_sse(struct fann *ann);

#endif

/*  Function: fann_can_use_sse
	Sets to non-zero ann->can_use_sse when ANN can be processed with SSE SIMD instructions. 
	To use SSE FANN must use floats, network must be fully connected (shortcuts should be ok too),
	and number of neurons in input and hidden layers including bias neurons must be multiply of 4.
	Also pointers to processing data must be aligned to 16 bytes boundary.

	Parameters:
		ann - A previously created neural network structure of type <struct fann> pointer.

    Returns:
        Nonzero if ANN can be processed with SSE SIMD instructions

   This function appears in FANN >= 2.1.0beta
*/
FANN_EXTERNAL bool fann_can_use_sse(struct fann *ann);

/*  Function: fann_disable_sse
	Disables SSE using by resetting ann->can_use_sse flag

	Parameters:
		ann - A previously created neural network structure of type <struct fann> pointer.

   This function appears in FANN >= 2.1.0beta
*/
FANN_EXTERNAL void fann_disable_sse(struct fann *ann);


#endif	/* __fann_sse_h__ */

