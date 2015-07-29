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
#include "fann_avx.h"
#include "fann_train.h"
#include <intrin.h>
#include <xmmintrin.h>

FANN_EXTERNAL bool fann_can_use_avx(struct fann *ann)
{
	bool can_use_avx;

	if(ann == NULL)
		return false;

#if (defined FLOATFANN || defined DOUBLEFANN) && defined FANN_USE_AVX
	unsigned int num_layers = fann_get_num_layers(ann);
	if(num_layers > 1)
	{
		can_use_avx = true;
		for(unsigned int i = 0; i < num_layers - 1; i++)
		{
			struct fann_layer *current_layer = ann->first_layer + i;
			if((current_layer->last_neuron - current_layer->first_neuron) % 8 != 0)
			{
				can_use_avx = false;
				break;
			}
		}

		if(!fann_sse_aligned(ann->first_layer) || !fann_sse_aligned(ann->weights)
			|| !fann_sse_aligned(ann->first_layer->sum) || !fann_sse_aligned(ann->first_layer->value))
		{
			can_use_avx = false;
		}
	}
	else
	{
		can_use_avx = false;
	}
#else
	can_use_avx = false;
#endif
	ann->can_use_avx = can_use_avx;
	return can_use_avx;
}

FANN_EXTERNAL void fann_disable_avx(struct fann *ann)
{
	if(ann == NULL)
		return;

	ann->can_use_avx = 0;
}

#if defined FLOATFANN && defined FANN_USE_AVX
static const union PS256_vec 
{
	float f[8];
	__m256 ps;
} 
	ssec256_0 = {{0, 0, 0, 0, 0, 0, 0, 0}},
	ssec256_1 = {{1, 1, 1, 1, 1, 1, 1, 1}}, 
	ssec256_2 = {{2, 2, 2, 2, 2, 2, 2, 2}}, 
	ssec256_neg2 = {{-2, -2, -2, -2, -2, -2, -2, -2}}, 
	pos_limit256 = {{150, 150, 150, 150, 150, 150, 150, 150}}, 
	neg_limit256 = {{-150, -150, -150, -150, -150, -150, -150, -150}};

float __inline __fastcall fann_hadd256_ps(const __m256& arg)
{
	__m256 temp = _mm256_add_ps(arg, _mm256_permute_ps(arg, 0xee));
	temp = _mm256_add_ps(temp, _mm256_movehdup_ps(temp));
	return _mm_cvtss_f32(_mm_add_ss(_mm256_castps256_ps128(temp), _mm256_extractf128_ps(temp,1)));
	//return _mm_cvtss_f32(_mm_add_ss(_mm256_extractf128_ps(temp, 0), _mm256_extractf128_ps(temp,1)));
}

FANN_EXTERNAL fann_type *FANN_API fann_run_avx(struct fann * ann, const fann_type * input)
{
	unsigned int i, num_connections, num_input, num_output;
	fann_type *output;
	fann_type *weights;
	struct fann_layer *layer_it, *last_layer;

	/* store some variabels local for fast access */
	struct fann_neuron *first_neuron = ann->first_layer->first_neuron;

	/* make crash if used improperly */
	assert(ann->can_use_avx);

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

			__m256 neuron_sum_v = _mm256_setzero_ps(); /* sets it to {0, 0, 0, 0} vector */
			
			for(i = 0; i < num_connections; i += 8)
			{
				__m256 weight_v = _mm256_load_ps(weights + i); 
				__m256 value_v = _mm256_load_ps(neurons[i].valuePtr);
				neuron_sum_v = _mm256_add_ps(neuron_sum_v, _mm256_mul_ps(weight_v, value_v));
				//neuron_sum +=
				//	fann_mult(weights[i], *neurons[i].valuePtr) +
				//	fann_mult(weights[i + 1], *neurons[i + 1].valuePtr) +
				//	fann_mult(weights[i + 2], *neurons[i + 2].valuePtr) +
				//	fann_mult(weights[i + 3], *neurons[i + 3].valuePtr);
			}
		
			*neuron_it->sumPtr = fann_hadd256_ps(neuron_sum_v);
			//neuron_sum = neuron_sum_v.m128_f32[0] + neuron_sum_v.m128_f32[1]
			//	+ neuron_sum_v.m128_f32[2] + neuron_sum_v.m128_f32[3];
		}

		__m256 steepness_v = _mm256_set1_ps(steepness);
		__m256 max_sum = _mm256_div_ps(pos_limit256.ps, steepness_v); // 150/steepness
		__m256 min_sum = _mm256_div_ps(neg_limit256.ps, steepness_v); //-150/steepness
		for(fann_neuron *neuron_it = layer_it->first_neuron; neuron_it < last_neuron; neuron_it += 8)
		{
			__m256 neuron_sum = _mm256_mul_ps(steepness_v, _mm256_load_ps(neuron_it->sumPtr));
			neuron_sum = _mm256_min_ps(max_sum, _mm256_max_ps(min_sum, neuron_sum));
			//if(neuron_sum > max_sum)
			//	neuron_sum = max_sum;
			//else if(neuron_sum < min_sum)
			//	neuron_sum = min_sum;

			_mm256_store_ps(neuron_it->sumPtr, neuron_sum);
			//_mm256_stream_ps(neuron_it->sumPtr, neuron_sum);
			//*neuron_it->sumPtr = neuron_sum;
			__m256 activationResult;
			fann256_activation_switch_ps(activation_function, neuron_sum, activationResult);
			_mm256_store_ps(neuron_it->valuePtr, activationResult);
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
			__m256 neuron_sum_v = _mm256_setzero_ps(); /* sets it to {0, 0, 0, 0} vector */
			
			for(i = 0; i < num_connections; i += 8)
			{
				__m256 weight_v = _mm256_load_ps(weights + i); 
				__m256 value_v = _mm256_load_ps(neurons[i].valuePtr);
				neuron_sum_v = _mm256_add_ps(neuron_sum_v, _mm256_mul_ps(weight_v, value_v));
				//neuron_sum +=
				//	fann_mult(weights[i], *neurons[i].valuePtr) +
				//	fann_mult(weights[i + 1], *neurons[i + 1].valuePtr) +
				//	fann_mult(weights[i + 2], *neurons[i + 2].valuePtr) +
				//	fann_mult(weights[i + 3], *neurons[i + 3].valuePtr);
			}
		
			neuron_sum = fann_hadd256_ps(neuron_sum_v);
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

static const union PS_vec 
{
	float f[4];
	__m128 ps;
} 
	ssec_0 = {{0, 0, 0, 0}},
	ssec_1 = {{1, 1, 1, 1}}, 
	ssec_2 = {{2, 2, 2, 2}}, 
	ssec_neg2 = {{-2, -2, -2, -2}};

__inline __m256 __fastcall fann256_sigmoid_real_ps(const __m256& sum)
{
	//   1.0f/(1.0f + exp(-2.0f * sum))
	__m256 inner = _mm256_add_ps(ssec256_1.ps, exp256_ps(_mm256_mul_ps(sum, ssec256_neg2.ps)));
	return _mm256_div_ps(ssec256_1.ps, inner);
}

__inline __m256 __fastcall fann256_sigmoid_symmetric_real_ps(const __m256& sum)
{
	//   2.0f/(1.0f + exp(-2.0f * sum)) - 1.0f
	__m256 inner = _mm256_add_ps(ssec256_1.ps, exp256_ps(_mm256_mul_ps(sum, ssec256_neg2.ps)));
	return _mm256_sub_ps(_mm256_div_ps(ssec256_2.ps, inner), ssec256_1.ps);
}

__inline __m256 __fastcall fann256_linear_derive_ps(fann_type steepness, const __m256& value)
{
	return _mm256_set1_ps(steepness);
}

__inline __m256 __fastcall fann256_sigmoid_derive_ps(fann_type steepness, const __m256& value)
{
	//2.0f * steepness * value * (1.0f - value)
	__m256 one_val = _mm256_sub_ps(ssec256_1.ps, value);
	__m256 step2 = _mm256_mul_ps(ssec256_2.ps, _mm256_set1_ps(steepness));
	return _mm256_mul_ps(step2, _mm256_mul_ps(value, one_val));
}

__inline __m256 __fastcall fann256_sigmoid_symmetric_derive_ps(fann_type steepness, const __m256& value)
{
	//steepness * (1.0f - (value*value)
	__m256 v2 = _mm256_mul_ps(value, value);
	return _mm256_mul_ps(_mm256_set1_ps(steepness), _mm256_sub_ps(ssec256_1.ps, v2));
}

fann_type fann_update_MSE(struct fann *ann, fann_activationfunc_enum activation_function, 
						  fann_type neuron_diff);

FANN_EXTERNAL fann_type *FANN_API fann_test_avx(struct fann *ann, fann_type * input,
											fann_type * desired_output)
{
	fann_type neuron_value;
	fann_type *output_begin = fann_run_avx(ann, input);
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

void fann_backpropagate_MSE_avx(struct fann *ann)
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
			__m256 tmp_error_v = _mm256_set1_ps(tmp_error);
			unsigned int con_count = neuron_it->last_con - neuron_it->first_con;
			for(unsigned int i = 0; i < con_count; i += 8)
			//for(unsigned int i = 0; i < con_count; i ++)
			{
				/*printf("i = %d\n", i);
				 * printf("error_prev_layer[%d] = %f\n", i, error_prev_layer[i]);
				 * printf("weights[%d] = %f\n", i, weights[i]); */
				__m256 error_prev_layer_v = _mm256_load_ps(error_prev_layer + i);
				__m256 weights_v = _mm256_load_ps(weights + i);
				error_prev_layer_v = _mm256_add_ps(error_prev_layer_v, _mm256_mul_ps(tmp_error_v, weights_v));
				_mm256_store_ps(error_prev_layer + i, error_prev_layer_v);
				//error_prev_layer[i] += tmp_error * weights[i];
			}
		}

		/* then calculate the actual errors in the previous layer */
		error_prev_layer = error_begin + ((layer_it - 1)->first_neuron - first_neuron);
		last_neuron = (layer_it - 1)->last_neuron;
		fann_activationfunc_enum activation_function = (layer_it - 1)->activation_function;
		fann_type activation_steepness = (layer_it - 1)->activation_steepness;

		for(neuron_it = (layer_it - 1)->first_neuron; neuron_it != last_neuron; neuron_it += 8)
		//for(neuron_it = (layer_it - 1)->first_neuron; neuron_it != last_neuron; neuron_it ++)
		{
			__m256 error_prev_layer_v = _mm256_load_ps(error_prev_layer);
			__m256 res = fann256_activation_derived_ps(activation_function, activation_steepness, 
				neuron_it->valuePtr, neuron_it->sumPtr);
			error_prev_layer_v = _mm256_mul_ps(error_prev_layer_v, res);
			_mm256_store_ps(error_prev_layer, error_prev_layer_v);
			error_prev_layer += 8;
			//*error_prev_layer *= fann_activation_derived(activation_function, 
			//	activation_steepness, *neuron_it->valuePtr, *neuron_it->sumPtr);
			//error_prev_layer ++;
		}
	}
}

__inline __m256 __fastcall fann256_activation_derived_ps(fann_activationfunc_enum activation_function,
								  fann_type steepness, fann_type *value, fann_type *sum)
{
	static const PS256_vec sig_l = {{0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f}};
	static const PS256_vec sig_h = {{0.99f, 0.99f, 0.99f, 0.99f, 0.99f, 0.99f, 0.99f, 0.99f}};
	static const PS256_vec simsig_l = {{-0.98f, -0.98f, -0.98f, -0.98f, -0.98f, -0.98f, -0.98f, -0.98f}};
	static const PS256_vec simsig_h = {{ 0.98f,  0.98f,  0.98f,  0.98f,  0.98f,  0.98f,  0.98f,  0.98f}};

	switch (activation_function)
	{
	case FANN_LINEAR:
	case FANN_LINEAR_PIECE:
	case FANN_LINEAR_PIECE_SYMMETRIC:
		return fann256_linear_derive_ps(steepness, _mm256_load_ps(value));
	case FANN_SIGMOID:
	case FANN_SIGMOID_STEPWISE:
		{
			__m256 val_v = _mm256_load_ps(value);
			val_v = _mm256_min_ps(val_v, sig_h.ps);
			val_v = _mm256_max_ps(val_v, sig_l.ps);
			return fann256_sigmoid_derive_ps(steepness, val_v);
		}
	case FANN_SIGMOID_SYMMETRIC:
	case FANN_SIGMOID_SYMMETRIC_STEPWISE:
		{
			__m256 val_v = _mm256_load_ps(value);
			val_v = _mm256_min_ps(val_v, simsig_h.ps);
			val_v = _mm256_max_ps(val_v, simsig_l.ps);
			return fann256_sigmoid_symmetric_derive_ps(steepness, val_v);
		}
		default:
			assert(0);
	}
	return _mm256_load_ps(value);
}

//INTERNAL FUNCTION
//Update weights for incremental training
void fann_update_weights_avx(struct fann *ann)
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
		const __m256 learning_momentum_v = _mm256_set1_ps(ann->learning_momentum);
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
				__m256 tmp_error_v = _mm256_broadcast_ss(&tmp_error);
				num_connections = neuron_it->last_con - neuron_it->first_con;
				fann_type *weights = ann->weights + neuron_it->first_con;
				fann_type *weights_deltas = deltas_begin + neuron_it->first_con;
				for(i = 0; i != num_connections; i += 8)
				{
					__m256 value_v = _mm256_load_ps(prev_neurons[i].valuePtr);
					__m256 weights_deltas_v = _mm256_load_ps(weights_deltas + i);

					__m256 delta_w_v = _mm256_add_ps(_mm256_mul_ps(tmp_error_v, value_v), 
						_mm256_mul_ps(learning_momentum_v, weights_deltas_v));
					//fann_type delta_w = tmp_error * *prev_neurons[i].valuePtr + learning_momentum * weights_deltas[i];
					
					__m256 weights_v = _mm256_load_ps(weights + i);
					weights_v = _mm256_add_ps(weights_v, delta_w_v);
					_mm256_store_ps(weights + i, weights_v);
					//weights[i] += delta_w ;

					_mm256_store_ps(weights_deltas + i, delta_w_v);
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
				__m256 tmp_error_v = _mm256_broadcast_ss(&tmp_error);
				num_connections = neuron_it->last_con - neuron_it->first_con;
				fann_type *weights = ann->weights + neuron_it->first_con;
				fann_type *weights_deltas = deltas_begin + neuron_it->first_con;
				for(i = 0; i != num_connections; i += 8)
				{
					__m256 value_v = _mm256_load_ps(prev_neurons[i].valuePtr);
					__m256 weights_deltas_v = _mm256_load_ps(weights_deltas + i);

					__m256 delta_w_v = _mm256_mul_ps(tmp_error_v, value_v);
					//fann_type delta_w = tmp_error * *prev_neurons[i].valuePtr;
					
					__m256 weights_v = _mm256_load_ps(weights + i);
					weights_v = _mm256_add_ps(weights_v, delta_w_v);
					_mm256_store_ps(weights + i, weights_v);
					//weights[i] += delta_w ;

					_mm256_store_ps(weights_deltas + i, delta_w_v);
					//weights_deltas[i] = delta_w;
				}
			}
		}
	}
}

//Trains the network with the backpropagation algorithm using SSE SIMD instructions.
FANN_EXTERNAL void FANN_API fann_train_avx(struct fann *ann, const fann_type * input, const fann_type * desired_output)
{
	fann_run_avx(ann, input);
	fann_compute_MSE(ann, desired_output);
	fann_backpropagate_MSE_avx(ann);
	fann_update_weights_avx(ann);
}

#endif
