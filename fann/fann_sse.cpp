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

#include <assert.h>
#include "fann_sse.h"
#include "fann_train.h"
#include <smmintrin.h>

FANN_EXTERNAL bool fann_can_use_sse(struct fann *ann)
{
	unsigned int num_layers, i;
	bool can_use_sse;
	struct fann_layer *current_layer;

	if(ann == NULL)
		return false;

#if (defined FLOATFANN || defined DOUBLEFANN) && defined FANN_USE_SSE
	num_layers = fann_get_num_layers(ann);
	if(num_layers > 1)
	{
		can_use_sse = true;
		for(i = 0; i < num_layers - 1; i++)
		{
			current_layer = ann->first_layer + i;
			if((current_layer->last_neuron - current_layer->first_neuron) % 4 != 0)
			{
				can_use_sse = false;
				break;
			}
		}

		if(!fann_sse_aligned(ann->first_layer) || !fann_sse_aligned(ann->weights)
			|| !fann_sse_aligned(ann->first_layer->sum) || !fann_sse_aligned(ann->first_layer->value))
		{
			can_use_sse = false;
		}
	}
	else
	{
		can_use_sse = false;
	}
#else
	can_use_sse = false;
#endif
	ann->can_use_sse = can_use_sse;
	return can_use_sse;
}

FANN_EXTERNAL void fann_disable_sse(struct fann *ann)
{
	if(ann == NULL)
		return;

	ann->can_use_sse = 0;
}

#if defined FLOATFANN && defined FANN_USE_SSE
static const union PS_vec 
{
	float f[4];
	__m128 ps;
} 
	ssec_0 = {{0, 0, 0, 0}},
	ssec_1 = {{1, 1, 1, 1}}, 
	ssec_2 = {{2, 2, 2, 2}}, 
	ssec_neg2 = {{-2, -2, -2, -2}}, 
	pos_limit = {{150, 150, 150, 150}}, 
	neg_limit = {{-150, -150, -150, -150}};

float __inline __fastcall fann_hadd_ps(const __m128& arg)
{
	__m128 res;
#if FANN_USE_SSE >= 0x41
	//SSE4
	res = _mm_dp_ps(arg, ssec_1.ps, 0xff);
#elif FANN_USE_SSE >= 0x30
	//SSE3
	res = _mm_hadd_ps(arg, arg);
	res = _mm_hadd_ps(arg, arg);
#else
	//SSE1
	__m128 vec0 = _mm_shuffle_ps(arg, arg,_MM_SHUFFLE(2,3,0,1));
	__m128 vec1 = _mm_add_ps(arg, vec0);
	vec0 = _mm_shuffle_ps(vec1,vec1,_MM_SHUFFLE(1,1,3,3));
	res = _mm_add_ps(vec1,vec0);
#endif
	return _mm_cvtss_f32(res);
}

FANN_EXTERNAL fann_type *FANN_API fann_run_sse(struct fann * ann, const fann_type * input)
{
	unsigned int i, num_connections, num_input, num_output;
	fann_type *output;
	fann_type *weights;
	struct fann_layer *layer_it, *last_layer;

	/* store some variabels local for fast access */
	struct fann_neuron *first_neuron = ann->first_layer->first_neuron;

	/* make crash if used improperly */
	assert(ann->can_use_sse);

	/* first set the input */
	num_input = ann->num_input;
	for(i = 0; i != num_input; i++)
	{
		*first_neuron[i].valuePtr = input[i];
	}
	// Set the bias neuron in the input layer
	*(ann->first_layer->last_neuron - 1)->valuePtr = 1;
	last_layer = ann->last_layer;

	//hidden layers
	for(layer_it = ann->first_layer + 1; layer_it != last_layer - 1; layer_it++)
	{
		fann_activationfunc_enum activation_function = layer_it->activation_function;
		fann_type steepness = layer_it->activation_steepness;
		fann_neuron *last_neuron = layer_it->last_neuron;
		fann_neuron *neurons = (layer_it - 1)->first_neuron;

		for(fann_neuron *neuron_it = layer_it->first_neuron; neuron_it < last_neuron; neuron_it++)
		{
			num_connections = neuron_it->last_con - neuron_it->first_con;
			weights = ann->weights + neuron_it->first_con;

			__m128 neuron_sum_v = _mm_setzero_ps(); /* sets it to {0, 0, 0, 0} vector */
			
			for(i = 0; i < num_connections; i += 4)
			{
				__m128 weight_v = _mm_load_ps(weights + i); 
				__m128 value_v = _mm_load_ps(neurons[i].valuePtr);
				neuron_sum_v = _mm_add_ps(neuron_sum_v, _mm_mul_ps(weight_v, value_v));
				//neuron_sum +=
				//	fann_mult(weights[i], *neurons[i].valuePtr) +
				//	fann_mult(weights[i + 1], *neurons[i + 1].valuePtr) +
				//	fann_mult(weights[i + 2], *neurons[i + 2].valuePtr) +
				//	fann_mult(weights[i + 3], *neurons[i + 3].valuePtr);
			}
		
			*neuron_it->sumPtr = fann_hadd_ps(neuron_sum_v);
			//neuron_sum = neuron_sum_v.m128_f32[0] + neuron_sum_v.m128_f32[1]
			//	+ neuron_sum_v.m128_f32[2] + neuron_sum_v.m128_f32[3];
		}

		__m128 steepness_v = _mm_load_ps1(&steepness);
		__m128 max_sum = _mm_div_ps(pos_limit.ps, steepness_v); // 150/steepness
		__m128 min_sum = _mm_div_ps(neg_limit.ps, steepness_v); //-150/steepness
		for(fann_neuron *neuron_it = layer_it->first_neuron; neuron_it < last_neuron; neuron_it += 4)
		{
			__m128 neuron_sum = _mm_mul_ps(steepness_v, _mm_load_ps(neuron_it->sumPtr));
			neuron_sum = _mm_min_ps(max_sum, _mm_max_ps(min_sum, neuron_sum));
			//if(neuron_sum > max_sum)
			//	neuron_sum = max_sum;
			//else if(neuron_sum < min_sum)
			//	neuron_sum = min_sum;

			_mm_store_ps(neuron_it->sumPtr, neuron_sum);
			//*neuron_it->sumPtr = neuron_sum;
			__m128 activationResult;
			fann_activation_switch_ps(activation_function, neuron_sum, activationResult);
			_mm_store_ps(neuron_it->valuePtr, activationResult);
			//fann_activation_switch(activation_function, neuron_sum, *neuron_it->valuePtr);
		}

		//bias
		*(last_neuron-1)->sumPtr = 0;
		*(last_neuron-1)->valuePtr = 1;
	}

	//Output layer
	layer_it = last_layer - 1;
	{
		fann_activationfunc_enum activation_function = layer_it->activation_function;
		fann_type steepness = layer_it->activation_steepness;
		fann_neuron *neurons = (layer_it - 1)->first_neuron;
		fann_neuron *last_neuron = layer_it->last_neuron;
		for(fann_neuron *neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
		{
			fann_type neuron_sum = 0;
			num_connections = neuron_it->last_con - neuron_it->first_con;
			weights = ann->weights + neuron_it->first_con;
			__m128 neuron_sum_v = _mm_setzero_ps(); /* sets it to {0, 0, 0, 0} vector */
			
			for(i = 0; i < num_connections; i += 4)
			{
				__m128 weight_v = _mm_load_ps(weights + i); 
				__m128 value_v = _mm_load_ps(neurons[i].valuePtr);
				neuron_sum_v = _mm_add_ps(neuron_sum_v, _mm_mul_ps(weight_v, value_v));
				//neuron_sum +=
				//	fann_mult(weights[i], *neurons[i].valuePtr) +
				//	fann_mult(weights[i + 1], *neurons[i + 1].valuePtr) +
				//	fann_mult(weights[i + 2], *neurons[i + 2].valuePtr) +
				//	fann_mult(weights[i + 3], *neurons[i + 3].valuePtr);
			}
		
			neuron_sum = fann_hadd_ps(neuron_sum_v);
			//neuron_sum = neuron_sum_v.m128_f32[0] + neuron_sum_v.m128_f32[1]
			//	+ neuron_sum_v.m128_f32[2] + neuron_sum_v.m128_f32[3];

			neuron_sum = steepness * neuron_sum;
			
			fann_type max_sum = 150/steepness;
			if(neuron_sum > max_sum)
				neuron_sum = max_sum;
			else if(neuron_sum < -max_sum)
				neuron_sum = -max_sum;
			
			*neuron_it->sumPtr = neuron_sum;
			fann_activation_switch(activation_function, neuron_sum, *neuron_it->valuePtr);
		}

		//bias
		*(last_neuron-1)->sumPtr = 0;
		*(last_neuron-1)->valuePtr = 1;
	}

	/* set the output */
	output = ann->output;
	num_output = ann->num_output;
	fann_neuron *neurons = (ann->last_layer - 1)->first_neuron;
	for(i = 0; i != num_output; i++)
	{
		output[i] = *neurons[i].valuePtr;
	}
	return ann->output;
}
#elif defined DOUBLEFANN && FANN_USE_SSE >= 0x20
static const union {
	double d[2];
	__m128d pd;
} 
	zeroes = {{0.0f, 0.0f}},
	ones = {{1.0f, 1.0f}}, 
	pos_limit = {{150, 150}}, 
	neg_limit = {{-150, -150}};

__m128d __inline __fastcall fann_hadd_pd(__m128d arg)
{
	__m128d res;
#if FANN_USE_SSE >= 0x30
	//SSE3
	res = _mm_hadd_pd(arg, arg);
#else
	//SSE1
	__m128d vec0 = _mm_shuffle_pd(arg, arg,_MM_SHUFFLE2(0, 1));
	res = _mm_add_pd(arg, vec0);
#endif
	return res;
}

FANN_EXTERNAL fann_type *FANN_API fann_run_sse(struct fann * ann, fann_type * input)
{
	unsigned int i, num_connections, num_input, num_output;
	fann_type *output;
	fann_type *weights;
	struct fann_layer *layer_it, *last_layer;

	/* store some variabels local for fast access */
	struct fann_neuron *first_neuron = ann->first_layer->first_neuron;

	/* make crash if used improperly */
	assert(ann->can_use_sse);

	/* first set the input */
	num_input = ann->num_input;
	for(i = 0; i != num_input; i++)
	{
		*first_neuron[i].valuePtr = input[i];
	}
	/* Set the bias neuron in the input layer */
	*(ann->first_layer->last_neuron - 1)->valuePtr = 1;
	last_layer = ann->last_layer;

	//hidden layers
	for(layer_it = ann->first_layer + 1; layer_it < last_layer - 1; layer_it++)
	{
		fann_activationfunc_enum activation_function = layer_it->activation_function;
		fann_type steepness = layer_it->activation_steepness;
		fann_neuron *last_neuron = layer_it->last_neuron;
		fann_neuron *neurons = (layer_it - 1)->first_neuron;

		for(fann_neuron *neuron_it = layer_it->first_neuron; neuron_it < last_neuron; neuron_it++)
		{
			num_connections = neuron_it->last_con - neuron_it->first_con;
			weights = ann->weights + neuron_it->first_con;
			__m128d neuron_sum_v = _mm_setzero_pd(); /* sets it to {0, 0} vector */

			for(i = 0; i < num_connections; i += 4)
			{
				__m128d weight_v, value_v;
				weight_v = _mm_load_pd(weights + i); 
				value_v = _mm_load_pd(neurons[i].valuePtr);
				neuron_sum_v = _mm_add_pd(neuron_sum_v, _mm_mul_pd(weight_v, value_v));
				int new_i = i + 2;
				weight_v = _mm_load_pd(weights + new_i); 
				value_v = _mm_load_pd(neurons[new_i].valuePtr);
				neuron_sum_v = _mm_add_pd(neuron_sum_v, _mm_mul_pd(weight_v, value_v));
				//neuron_sum +=
				//	fann_mult(weights[i], *neurons[i].valuePtr) +
				//	fann_mult(weights[i + 1], *neurons[i + 1].valuePtr) +
				//	fann_mult(weights[i + 2], *neurons[i + 2].valuePtr) +
				//	fann_mult(weights[i + 3], *neurons[i + 3].valuePtr);
			}
			neuron_sum_v = fann_hadd_pd(neuron_sum_v);
			_mm_store_sd(neuron_it->sumPtr, neuron_sum_v);
			//neuron_sum = neuron_sum_v.d1 + neuron_sum_v.d2;
		}

		__m128d steepness_v = _mm_load1_pd(&steepness);
		__m128d max_sum = _mm_div_pd(pos_limit.pd, steepness_v); // 150/steepness
		__m128d min_sum = _mm_div_pd(neg_limit.pd, steepness_v); //-150/steepness
		for(fann_neuron *neuron_it = layer_it->first_neuron; neuron_it < last_neuron; neuron_it += 4)
		{
			__m128d neuron_sum = _mm_mul_pd(steepness_v, _mm_load_pd(neuron_it->sumPtr));
			neuron_sum = _mm_min_pd(max_sum, _mm_max_pd(min_sum, neuron_sum));
			//if(neuron_sum > max_sum)
			//	neuron_sum = max_sum;
			//else if(neuron_sum < min_sum)
			//	neuron_sum = min_sum;

			_mm_store_pd(neuron_it->sumPtr, neuron_sum);
			//*neuron_it->sumPtr = neuron_sum;
			__m128d activationResult;
			fann_activation_switch_pd(activation_function, neuron_sum, activationResult);
			_mm_store_pd(neuron_it->valuePtr, activationResult);
			//fann_activation_switch(activation_function, neuron_sum, *neuron_it->valuePtr);

			fann_neuron *neuron_it_next = neuron_it+2;
			neuron_sum = _mm_mul_pd(steepness_v, _mm_load_pd(neuron_it_next->sumPtr ));
			neuron_sum = _mm_min_pd(max_sum, _mm_max_pd(min_sum, neuron_sum));
			_mm_store_pd(neuron_it_next->sumPtr, neuron_sum);
			fann_activation_switch_pd(activation_function, neuron_sum, activationResult);
			_mm_store_pd(neuron_it_next->valuePtr, activationResult);
		}

		//bias
		*(last_neuron-1)->sumPtr = 0;
		*(last_neuron-1)->valuePtr = 1;
	}

	//Output layer
	layer_it = last_layer - 1;
	{
		fann_activationfunc_enum activation_function = layer_it->activation_function;
		fann_type steepness = layer_it->activation_steepness;
		fann_neuron *neurons = (layer_it - 1)->first_neuron;
		fann_neuron *last_neuron = layer_it->last_neuron;
		for(fann_neuron *neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
		{
			fann_type neuron_sum = 0;
			num_connections = neuron_it->last_con - neuron_it->first_con;
			weights = ann->weights + neuron_it->first_con;
			__m128d neuron_sum_v = _mm_setzero_pd(); /* sets it to {0, 0} vector */

			for(i = 0; i != num_connections; i += 4)
			{
				__m128d weight_v, value_v;
				weight_v = _mm_load_pd(weights + i); 
				value_v = _mm_load_pd(neurons[i].valuePtr);
				neuron_sum_v = _mm_add_pd(neuron_sum_v, _mm_mul_pd(weight_v, value_v));
				int new_i = i + 2;
				weight_v = _mm_load_pd(weights + new_i); 
				value_v = _mm_load_pd(neurons[new_i].valuePtr);
				neuron_sum_v = _mm_add_pd(neuron_sum_v, _mm_mul_pd(weight_v, value_v));
				//neuron_sum +=
				//	fann_mult(weights[i], *neurons[i].valuePtr) +
				//	fann_mult(weights[i + 1], *neurons[i + 1].valuePtr) +
				//	fann_mult(weights[i + 2], *neurons[i + 2].valuePtr) +
				//	fann_mult(weights[i + 3], *neurons[i + 3].valuePtr);
			}
			neuron_sum_v = fann_hadd_pd(neuron_sum_v);
			_mm_store_sd(neuron_it->sumPtr, neuron_sum_v);
			//neuron_sum = neuron_sum_v.d1 + neuron_sum_v.d2;

			neuron_sum = steepness * neuron_sum;
			
			fann_type max_sum = 150/steepness;
			if(neuron_sum > max_sum)
				neuron_sum = max_sum;
			else if(neuron_sum < -max_sum)
				neuron_sum = -max_sum;
			
			*neuron_it->sumPtr = neuron_sum;
			fann_activation_switch(activation_function, neuron_sum, *neuron_it->valuePtr);
		}

		//bias
		*(last_neuron-1)->sumPtr = 0;
		*(last_neuron-1)->valuePtr = 1;
	}

	/* set the output */
	output = ann->output;
	num_output = ann->num_output;
	fann_neuron *neurons = (ann->last_layer - 1)->first_neuron;
	for(i = 0; i != num_output; i++)
	{
		output[i] = *neurons[i].valuePtr;
	}
	return ann->output;
}
#endif

#if defined FLOATFANN && defined FANN_USE_SSE

static const union 
{
	__int32 i32[4];
	__m128 ps;
} abs_mask = {{0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF}};

static const union 
{
	__int32 i32[4];
	__m128 ps;
} sign_mask = {{0x80000000, 0x80000000, 0x80000000, 0x80000000}};

/*
__inline __m128 tanh_approx_ps(__m128 x)
{
	//http://www.kvraudio.com/forum/viewtopic.php?t=262823&postdays=0&postorder=asc&start=45
	//tanh(x) ~ sign(x) * (1 - 1 / (1 + |x| + x^2 + a * x^4))
	__m128 mask = _mm_cmplt_ps(x, _mm_setzero_ps()); //sign masks
	__m128 signbit = _mm_and_ps(mask, sign_mask.ps); // highest bit is 1 if x is negative/
	__m128 xa = _mm_and_ps (x, abs_mask.ps); // fabs(x) 
	__m128 x2 = _mm_mul_ps(x, x); // x2 = x * x
	__m128 x4 = _mm_mul_ps(x2, x2); // x4 = x2 * x2 
	__m128 poly = _mm_add_ps(_mm_add_ps(xa, x2), x4); // poly = xa + x2 + x4 
	__m128 resa = _mm_div_ps(poly, _mm_add_ps(ones.ps, poly)); // resa = poly / (1 + poly)
	return _mm_or_ps(resa, signbit); // restore sign 
}
*/
__inline __m128 __fastcall fann_sigmoid_real_ps(const __m128& sum)
{
	//   1.0f/(1.0f + exp(-2.0f * sum))
	__m128 inner = _mm_add_ps(ssec_1.ps, exp_ps(_mm_mul_ps(sum, ssec_neg2.ps)));
	return _mm_div_ps(ssec_1.ps, inner);
}

__inline __m128 __fastcall fann_sigmoid_symmetric_real_ps(const __m128& sum)
{
	//   2.0f/(1.0f + exp(-2.0f * sum)) - 1.0f
	__m128 inner = _mm_add_ps(ssec_1.ps, exp_ps(_mm_mul_ps(sum, ssec_neg2.ps)));
	return _mm_sub_ps(_mm_div_ps(ssec_2.ps, inner), ssec_1.ps);
}

__inline __m128 __fastcall fann_linear_derive_ps(fann_type steepness, const __m128& value)
{
	return _mm_load1_ps(&steepness);
}

__inline __m128 __fastcall fann_sigmoid_derive_ps(fann_type steepness, const __m128& value)
{
	//2.0f * steepness * value * (1.0f - value)
	__m128 one_val = _mm_sub_ps(ssec_1.ps, value);
	__m128 step2 = _mm_mul_ps(ssec_2.ps, _mm_load1_ps(&steepness));
	return _mm_mul_ps(step2, _mm_mul_ps(value, one_val));
}

__inline __m128 __fastcall fann_sigmoid_symmetric_derive_ps(fann_type steepness, const __m128& value)
{
	//steepness * (1.0f - (value*value)
	__m128 v2 = _mm_mul_ps(value, value);
	return _mm_mul_ps(_mm_load1_ps(&steepness), _mm_sub_ps(ssec_1.ps, v2));
}

#elif defined DOUBLEFANN && FANN_USE_SSE >= 0x20

static const union 
{
	__int64 i64[2];
	__m128d pd;
} abs_mask = {{0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF}};

static const union 
{
	__int64 i64[2];
	__m128d pd;
} sign_mask = {{0x8000000000000000, 0x8000000000000000}};

__inline __m128d tanh_approx_pd(__m128d x)
{
	/* 
	http://www.kvraudio.com/forum/viewtopic.php?t=262823&postdays=0&postorder=asc&start=45
	tanh(x) ~ sign(x) * (1 - 1 / (1 + |x| + x^2 + a * x^4))
	*/

	__m128d mask = _mm_cmplt_pd(x, _mm_setzero_pd()); /*sign masks*/
	__m128d signbit = _mm_and_pd(mask, sign_mask.pd); /* highest bit is 1 if x is negative */
	__m128d xa = _mm_and_pd (x, abs_mask.pd); /* fabs(x) */
	__m128d x2 = _mm_mul_pd(x, x); /* x2 = x * x */
	__m128d x4 = _mm_mul_pd(x2, x2); /* x4 = x2 * x2 */
	__m128d poly = _mm_add_pd(_mm_add_pd(xa, x2), x4); /* poly = xa + x2 + x4 */
	__m128d resa = _mm_div_pd(poly, _mm_add_pd(ones.pd, poly)); /* resa = poly / (1 + poly) */
	return _mm_or_pd(resa, signbit); /* restore sign */
}
#endif

#if defined FLOATFANN && defined FANN_USE_SSE
fann_type fann_update_MSE(struct fann *ann, fann_activationfunc_enum activation_function, 
						  fann_type neuron_diff);

FANN_EXTERNAL fann_type *FANN_API fann_test_sse(struct fann *ann, fann_type * input,
											fann_type * desired_output)
{
	fann_type neuron_value;
	fann_type *output_begin = fann_run_sse(ann, input);
	fann_type *output_it;
	const fann_type *output_end = output_begin + ann->num_output;
	fann_type neuron_diff;
	struct fann_neuron *output_neuron = (ann->last_layer - 1)->first_neuron;
	fann_activationfunc_enum activation_function = (ann->last_layer - 1)->activation_function;

	/* calculate the error */
	for(output_it = output_begin; output_it != output_end; output_it++)
	{
		neuron_value = *output_it;
		neuron_diff = (*desired_output - neuron_value);
		neuron_diff = fann_update_MSE(ann, activation_function, neuron_diff);
		desired_output++;
		output_neuron++;
		ann->num_MSE++;
	}

	return output_begin;
}

void fann_backpropagate_MSE_sse(struct fann *ann)
{
	fann_type tmp_error;
	struct fann_layer *layer_it;
	struct fann_neuron *neuron_it, *last_neuron;

	fann_type *error_begin = ann->train_errors;
	fann_type *error_prev_layer;
	fann_type *weights;
	const struct fann_neuron *first_neuron = ann->first_layer->first_neuron;
	const struct fann_layer *second_layer = ann->first_layer + 1;
	struct fann_layer *last_layer = ann->last_layer;

	/* go through all the layers, from last to first.
	 * And propagate the error backwards */
	for(layer_it = last_layer - 1; layer_it > second_layer; --layer_it)
	{
		last_neuron = layer_it->last_neuron;

		/* for each connection in this layer, propagate the error backwards */
		error_prev_layer = error_begin + ((layer_it - 1)->first_neuron - first_neuron);

		for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
		{
			tmp_error = error_begin[neuron_it - first_neuron];
			weights = ann->weights + neuron_it->first_con;
			__m128 tmp_error_v = _mm_load1_ps(&tmp_error);
			unsigned int con_count = neuron_it->last_con - neuron_it->first_con;
			for(unsigned int i = 0; i < con_count; i += 4)
			//for(unsigned int i = 0; i < con_count; i ++)
			{
				/*printf("i = %d\n", i);
				 * printf("error_prev_layer[%d] = %f\n", i, error_prev_layer[i]);
				 * printf("weights[%d] = %f\n", i, weights[i]); */
				__m128 error_prev_layer_v = _mm_load_ps(error_prev_layer + i);
				__m128 weights_v = _mm_load_ps(weights + i);
				error_prev_layer_v = _mm_add_ps(error_prev_layer_v, _mm_mul_ps(tmp_error_v, weights_v));
				_mm_store_ps(error_prev_layer + i, error_prev_layer_v);
				//error_prev_layer[i] += tmp_error * weights[i];
			}
		}

		/* then calculate the actual errors in the previous layer */
		error_prev_layer = error_begin + ((layer_it - 1)->first_neuron - first_neuron);
		last_neuron = (layer_it - 1)->last_neuron;
		fann_activationfunc_enum activation_function = (layer_it - 1)->activation_function;
		fann_type activation_steepness = (layer_it - 1)->activation_steepness;

		for(neuron_it = (layer_it - 1)->first_neuron; neuron_it != last_neuron; neuron_it += 4)
		//for(neuron_it = (layer_it - 1)->first_neuron; neuron_it != last_neuron; neuron_it ++)
		{
			__m128 error_prev_layer_v = _mm_load_ps(error_prev_layer);
			__m128 res = fann_activation_derived_ps(activation_function, activation_steepness, 
				neuron_it->valuePtr, neuron_it->sumPtr);
			error_prev_layer_v = _mm_mul_ps(error_prev_layer_v, res);
			_mm_store_ps(error_prev_layer, error_prev_layer_v);
			error_prev_layer += 4;
			//*error_prev_layer *= fann_activation_derived(activation_function, 
			//	activation_steepness, *neuron_it->valuePtr, *neuron_it->sumPtr);
			//error_prev_layer ++;
		}
	}
}

__inline __m128 __fastcall fann_activation_derived_ps(fann_activationfunc_enum activation_function,
								  fann_type steepness, fann_type *value, fann_type *sum)
{
	static const PS_vec sig_l = {{0.01f, 0.01f, 0.01f, 0.01f}};
	static const PS_vec sig_h = {{0.99f, 0.99f, 0.99f, 0.99f}};
	static const PS_vec simsig_l = {{-0.98f, -0.98f, -0.98f, -0.98f}};
	static const PS_vec simsig_h = {{ 0.98f,  0.98f,  0.98f,  0.98f}};

	switch (activation_function)
	{
	case FANN_LINEAR:
	case FANN_LINEAR_PIECE:
	case FANN_LINEAR_PIECE_SYMMETRIC:
		return fann_linear_derive_ps(steepness, _mm_load_ps(value));
	case FANN_SIGMOID:
	case FANN_SIGMOID_STEPWISE:
		{
			__m128 val_v = _mm_load_ps(value);
			val_v = _mm_min_ps(val_v, sig_h.ps);
			val_v = _mm_max_ps(val_v, sig_l.ps);
			return fann_sigmoid_derive_ps(steepness, val_v);
		}
	case FANN_SIGMOID_SYMMETRIC:
	case FANN_SIGMOID_SYMMETRIC_STEPWISE:
		{
			__m128 val_v = _mm_load_ps(value);
			val_v = _mm_min_ps(val_v, simsig_h.ps);
			val_v = _mm_max_ps(val_v, simsig_l.ps);
			return fann_sigmoid_symmetric_derive_ps(steepness, val_v);
		}
		default:
			assert(0);
	}
	return _mm_load_ps(value);
}

//INTERNAL FUNCTION
//Update weights for incremental training
void fann_update_weights_sse(struct fann *ann)
{
	struct fann_neuron *neuron_it, *last_neuron, *prev_neurons;
	struct fann_layer *layer_it;
	unsigned int i;
	unsigned int num_connections;

	/* store some variabels local for fast access */
	const float learning_rate = ann->learning_rate;
    //const float learning_momentum = ann->learning_momentum;  
	struct fann_neuron *first_neuron = ann->first_layer->first_neuron;
	struct fann_layer *first_layer = ann->first_layer;
	const struct fann_layer *last_layer = ann->last_layer;
	fann_type *error_begin = ann->train_errors;

#ifdef DEBUGTRAIN
	printf("\nupdate weights\n");
#endif
	fann_type *deltas_begin = ann->prev_weights_deltas;
	prev_neurons = first_neuron;

	if(ann->learning_momentum != 0)
	{
		const __m128 learning_momentum_v = _mm_load1_ps(&ann->learning_momentum);
		for(layer_it = (first_layer + 1); layer_it != last_layer; layer_it++)
		{
	#ifdef DEBUGTRAIN
			printf("layer[%d]\n", layer_it - first_layer);
	#endif
			last_neuron = layer_it->last_neuron;
			prev_neurons = (layer_it - 1)->first_neuron;
			for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
			{
				fann_type tmp_error = error_begin[neuron_it - first_neuron] * learning_rate;
				__m128 tmp_error_v = _mm_load1_ps(&tmp_error);
				num_connections = neuron_it->last_con - neuron_it->first_con;
				fann_type *weights = ann->weights + neuron_it->first_con;
				fann_type *weights_deltas = deltas_begin + neuron_it->first_con;
				for(i = 0; i != num_connections; i += 4)
				{
					__m128 value_v = _mm_load_ps(prev_neurons[i].valuePtr);
					__m128 weights_deltas_v = _mm_load_ps(weights_deltas + i);

					__m128 delta_w_v = _mm_add_ps(_mm_mul_ps(tmp_error_v, value_v), 
						_mm_mul_ps(learning_momentum_v, weights_deltas_v));
					//fann_type delta_w = tmp_error * *prev_neurons[i].valuePtr + learning_momentum * weights_deltas[i];
					
					__m128 weights_v = _mm_load_ps(weights + i);
					weights_v = _mm_add_ps(weights_v, delta_w_v);
					_mm_store_ps(weights + i, weights_v);
					//weights[i] += delta_w ;

					_mm_store_ps(weights_deltas + i, delta_w_v);
					//weights_deltas[i] = delta_w;
				}
			}
		}
	}
	else
	{
		for(layer_it = (first_layer + 1); layer_it != last_layer; layer_it++)
		{
	#ifdef DEBUGTRAIN
			printf("layer[%d]\n", layer_it - first_layer);
	#endif
			last_neuron = layer_it->last_neuron;
			prev_neurons = (layer_it - 1)->first_neuron;
			for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
			{
				fann_type tmp_error = error_begin[neuron_it - first_neuron] * learning_rate;
				__m128 tmp_error_v = _mm_load1_ps(&tmp_error);
				num_connections = neuron_it->last_con - neuron_it->first_con;
				fann_type *weights = ann->weights + neuron_it->first_con;
				fann_type *weights_deltas = deltas_begin + neuron_it->first_con;
				for(i = 0; i != num_connections; i += 4)
				{
					__m128 value_v = _mm_load_ps(prev_neurons[i].valuePtr);
					__m128 weights_deltas_v = _mm_load_ps(weights_deltas + i);

					__m128 delta_w_v = _mm_mul_ps(tmp_error_v, value_v);
					//fann_type delta_w = tmp_error * *prev_neurons[i].valuePtr;
					
					__m128 weights_v = _mm_load_ps(weights + i);
					weights_v = _mm_add_ps(weights_v, delta_w_v);
					_mm_store_ps(weights + i, weights_v);
					//weights[i] += delta_w ;

					_mm_store_ps(weights_deltas + i, delta_w_v);
					//weights_deltas[i] = delta_w;
				}
			}
		}
	}
}

//Trains the network with the backpropagation algorithm using SSE SIMD instructions.
FANN_EXTERNAL void FANN_API fann_train_sse(struct fann *ann, const fann_type * input, const fann_type * desired_output)
{
	fann_run_sse(ann, input);
	fann_compute_MSE(ann, desired_output);
	fann_backpropagate_MSE_sse(ann);
	fann_update_weights_sse(ann);
}

#endif
