/*
  Fast Artificial Neural Network Library (fann)
  Copyright (C) 2003 Steffen Nissen (lukesky@diku.dk)
  Copyright (C) 2011 Alex Koshterek (koshterek@gmail.com)

  This library is fann_free software; you can redistribute it and/or
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "fann.h"
#if defined FANN_USE_SSE
//#include <mmintrin.h>
#endif

FANN_EXTERNAL struct fann *FANN_API fann_create_standard(unsigned int num_layers, ...)
{
	struct fann *ann;
	va_list layer_sizes;
	int i;
	unsigned int *layers = (unsigned int *) fann_calloc(num_layers, sizeof(unsigned int));

	if(layers == NULL)
	{
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}

	va_start(layer_sizes, num_layers);
	for(i = 0; i < (int) num_layers; i++)
	{
		layers[i] = va_arg(layer_sizes, unsigned int);
	}
	va_end(layer_sizes);

	ann = fann_create_standard_array(num_layers, layers);

	fann_free(layers);

	return ann;
}

FANN_EXTERNAL struct fann *FANN_API fann_create_standard_array(unsigned int num_layers, 
															   const unsigned int *layers)
{
	struct fann_layer *layer_it, *last_layer, *prev_layer;
	struct fann *ann;
	struct fann_neuron *neuron_it, *last_neuron;
#ifdef DEBUG
	unsigned int prev_layer_size;
#endif
	unsigned int num_neurons_in, num_neurons_out, i;
	unsigned int min_connections, max_connections, num_connections;
	unsigned int connections_per_neuron, allocated_connections;
	unsigned int tmp_con;

	/* seed random */
#ifndef FANN_NO_SEED
	fann_seed_rand();
#endif

	/* allocate the general structure */
	ann = fann_allocate_structure(num_layers);
	if(ann == NULL)
	{
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}

	/* determine how many neurons there should be in each layer */
	i = 0;
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		/* we do not allocate room here, but we make sure that
		 * last_neuron - first_neuron is the number of neurons */
		layer_it->first_neuron = NULL;
		layer_it->last_neuron = layer_it->first_neuron + layers[i++] + 1;	/* +1 for bias */
		ann->total_neurons += (unsigned int)(layer_it->last_neuron - layer_it->first_neuron);
	}

	ann->num_output = (unsigned int)((ann->last_layer - 1)->last_neuron - (ann->last_layer - 1)->first_neuron - 1);
	ann->num_input = (unsigned int)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);

	/* allocate room for the actual neurons */
	fann_allocate_neurons(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM)
	{
		fann_destroy(ann);
		return NULL;
	}

#ifdef DEBUG
	printf("creating network with connection rate %f\n", connection_rate);
	printf("input\n");
	printf("  layer       : %d neurons, 1 bias\n",
		   ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);
#endif

	num_neurons_in = ann->num_input;
	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++)
	{
		layer_it->activation_function = FANN_SIGMOID_SYMMETRIC;
		layer_it->activation_steepness = 0.5;
		
		num_neurons_out = (unsigned int)(layer_it->last_neuron - layer_it->first_neuron - 1);
		/*�if all neurons in each layer should be connected to at least one neuron
		 * in the previous layer, and one neuron in the next layer.
		 * and the bias node should be connected to the all neurons in the next layer.
		 * Then this is the minimum amount of neurons */
		min_connections = fann_max(num_neurons_in, num_neurons_out) + num_neurons_out;
		max_connections = num_neurons_in * num_neurons_out;	/* not calculating bias */
		num_connections = fann_max(min_connections, max_connections + num_neurons_out);
		connections_per_neuron = num_connections / num_neurons_out;
		allocated_connections = 0;
		/* Now split out the connections on the different neurons */
		for(i = 0; i != num_neurons_out; i++)
		{
			layer_it->first_neuron[i].first_con = ann->total_connections + allocated_connections;
			allocated_connections += connections_per_neuron;
			layer_it->first_neuron[i].last_con = ann->total_connections + allocated_connections;

			if(allocated_connections < (num_connections * (i + 1)) / num_neurons_out)
			{
				layer_it->first_neuron[i].last_con++;
				allocated_connections++;
			}
		}

		/* bias neuron also gets stuff */
		layer_it->first_neuron[i].first_con = ann->total_connections + allocated_connections;
		layer_it->first_neuron[i].last_con = ann->total_connections + allocated_connections;

		ann->total_connections += num_connections;

		/* used in the next run of the loop */
		num_neurons_in = num_neurons_out;
	}

	fann_allocate_connections(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM)
	{
		fann_destroy(ann);
		return NULL;
	}

#ifdef DEBUG
	prev_layer_size = ann->num_input + 1;
#endif
	prev_layer = ann->first_layer;
	last_layer = ann->last_layer;
	for(layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++)
	{
		last_neuron = layer_it->last_neuron - 1;
		for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
		{
			tmp_con = neuron_it->last_con - 1;
			for(i = neuron_it->first_con; i != tmp_con; i++)
			{
				ann->weights[i] = (fann_type) fann_random_weight();
				/* these connections are still initialized for fully connected networks, to allow
				 * operations to work, that are not optimized for fully connected networks.
				 */
				ann->connections[i] = prev_layer->first_neuron + (i - neuron_it->first_con);
			}

			/* bias weight */
			ann->weights[tmp_con] = (fann_type) fann_random_bias_weight();
			ann->connections[tmp_con] = prev_layer->first_neuron + (tmp_con - neuron_it->first_con);
		}
#ifdef DEBUG
		prev_layer_size = layer_it->last_neuron - layer_it->first_neuron;
#endif
		prev_layer = layer_it;
#ifdef DEBUG
		printf("  layer       : %d neurons, 1 bias\n", prev_layer_size - 1);
#endif
	}

#ifdef DEBUG
	printf("output\n");
#endif

	return ann; 
}

FANN_EXTERNAL fann_type *FANN_API fann_run(struct fann * ann, const fann_type * input)
{
	struct fann_neuron *neuron_it, *last_neuron, *neurons;
	unsigned int i, num_connections, num_input, num_output;
	fann_type neuron_sum, *output;
	fann_type *weights;
	struct fann_layer *layer_it, *last_layer;
	unsigned int activation_function;
	fann_type steepness;

	/* store some variabels local for fast access */
	struct fann_neuron *first_neuron = ann->first_layer->first_neuron;
	fann_type max_sum;	

	/* first set the input */
	num_input = ann->num_input;
	for(i = 0; i != num_input; i++)
	{
		*first_neuron[i].valuePtr = input[i];
	}
	/* Set the bias neuron in the input layer */
	*(ann->first_layer->last_neuron - 1)->valuePtr = 1;

	last_layer = ann->last_layer;
	for(layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++)
	{
		activation_function = layer_it->activation_function;
		steepness = layer_it->activation_steepness;

		last_neuron = layer_it->last_neuron;
		for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
		{
			if(neuron_it->first_con == neuron_it->last_con)
			{
				/* bias neurons */
				*neuron_it->valuePtr = 1;
				continue;
			}

			neuron_sum = 0;
			num_connections = neuron_it->last_con - neuron_it->first_con;
			weights = ann->weights + neuron_it->first_con;

			neurons = (layer_it - 1)->first_neuron;

			/* unrolled loop start */
			i = num_connections & 3;	/* same as modulo 4 */
			switch (i)
			{
				case 3:
					neuron_sum += fann_mult(weights[2], *neurons[2].valuePtr);
				case 2:
					neuron_sum += fann_mult(weights[1], *neurons[1].valuePtr);
				case 1:
					neuron_sum += fann_mult(weights[0], *neurons[0].valuePtr);
				case 0:
					break;
			}
			for(; i != num_connections; i += 4)
			{
				neuron_sum +=
					fann_mult(weights[i], *neurons[i].valuePtr) +
					fann_mult(weights[i + 1], *neurons[i + 1].valuePtr) +
					fann_mult(weights[i + 2], *neurons[i + 2].valuePtr) +
					fann_mult(weights[i + 3], *neurons[i + 3].valuePtr);
			}
			/* unrolled loop end */

			/*
			 * for(i = 0;i != num_connections; i++){
			 * printf("%f += %f*%f, ", neuron_sum, weights[i], neurons[i].value);
			 * neuron_sum += fann_mult(weights[i], neurons[i].value);
			 * }
			 */

			neuron_sum = fann_mult(steepness, neuron_sum);
			
			max_sum = 150/steepness;
			if(neuron_sum > max_sum)
				neuron_sum = max_sum;
			else if(neuron_sum < -max_sum)
				neuron_sum = -max_sum;
			
			*neuron_it->sumPtr = neuron_sum;

			fann_activation_switch(activation_function, neuron_sum, *neuron_it->valuePtr);
		}
	}

	/* set the output */
	output = ann->output;
	num_output = ann->num_output;
	neurons = (ann->last_layer - 1)->first_neuron;
	for(i = 0; i != num_output; i++)
	{
		output[i] = *neurons[i].valuePtr;
	}
	return ann->output;
}

FANN_EXTERNAL void FANN_API fann_destroy(struct fann *ann)
{
	if(ann == NULL)
		return;
	fann_safe_free(ann->weights);
	fann_safe_free(ann->connections);
	fann_safe_free(ann->first_layer->first_neuron);
	//AK
	fann_safe_free(ann->first_layer->sum);
	fann_safe_free(ann->first_layer->value);
	//
	fann_safe_free(ann->first_layer);
	fann_safe_free(ann->output);
	fann_safe_free(ann->train_errors);
	fann_safe_free(ann->train_slopes);
	fann_safe_free(ann->prev_train_slopes);
	fann_safe_free(ann->prev_steps);
	fann_safe_free(ann->prev_weights_deltas);
	fann_safe_free(ann->errstr);
	
	fann_safe_free( ann->scale_mean_in );
	fann_safe_free( ann->scale_deviation_in );
	fann_safe_free( ann->scale_new_min_in );
	fann_safe_free( ann->scale_factor_in );

	fann_safe_free( ann->scale_mean_out );
	fann_safe_free( ann->scale_deviation_out );
	fann_safe_free( ann->scale_new_min_out );
	fann_safe_free( ann->scale_factor_out );
	
	fann_safe_free(ann);
}

FANN_EXTERNAL void FANN_API fann_randomize_weights(struct fann *ann, fann_type min_weight,
												   fann_type max_weight)
{
	fann_type *last_weight;
	fann_type *weights = ann->weights;

	last_weight = weights + ann->total_connections;
	for(; weights != last_weight; weights++)
	{
		*weights = (fann_type) (fann_rand(min_weight, max_weight));
	}

#ifndef FIXEDFANN
	if(ann->prev_train_slopes != NULL)
	{
		fann_clear_train_arrays(ann);
	}
#endif
}

FANN_EXTERNAL void FANN_API fann_print_connections(struct fann *ann)
{
	struct fann_layer *layer_it;
	struct fann_neuron *neuron_it;
	unsigned int i;
	int value;
	char *neurons;
	unsigned int num_neurons = fann_get_total_neurons(ann) - fann_get_num_output(ann);

	neurons = (char *) fann_malloc(num_neurons + 1);
	if(neurons == NULL)
	{
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}
	neurons[num_neurons] = 0;

	printf("Layer / Neuron ");
	for(i = 0; i < num_neurons; i++)
	{
		printf("%d", i % 10);
	}
	printf("\n");

	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++)
	{
		for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++)
		{

			memset(neurons, (int) '.', num_neurons);
			for(i = neuron_it->first_con; i < neuron_it->last_con; i++)
			{
				if(ann->weights[i] < 0)
				{
#ifdef FIXEDFANN
					value = (int) ((ann->weights[i] / (double) ann->multiplier) - 0.5);
#else
					value = (int) ((ann->weights[i]) - 0.5);
#endif
					if(value < -25)
						value = -25;
					neurons[ann->connections[i] - ann->first_layer->first_neuron] = (char)('a' - value);
				}
				else
				{
#ifdef FIXEDFANN
					value = (int) ((ann->weights[i] / (double) ann->multiplier) + 0.5);
#else
					value = (int) ((ann->weights[i]) + 0.5);
#endif
					if(value > 25)
						value = 25;
					neurons[ann->connections[i] - ann->first_layer->first_neuron] = (char)('A' + value);
				}
			}
			printf("L %3d / N %4d %s\n", layer_it - ann->first_layer,
				   neuron_it - ann->first_layer->first_neuron, neurons);
		}
	}

	fann_free(neurons);
}

/* Initialize the weights using Widrow + Nguyen's algorithm.
*/
FANN_EXTERNAL void FANN_API fann_init_weights(struct fann *ann, struct fann_train_data *train_data)
{
	fann_type smallest_inp, largest_inp;
	unsigned int dat = 0, elem, num_connect, num_hidden_neurons;
	struct fann_layer *layer_it;
	struct fann_neuron *neuron_it, *last_neuron, *bias_neuron;

#ifdef FIXEDFANN
	unsigned int multiplier = ann->multiplier;
#endif
	float scale_factor;

	for(smallest_inp = largest_inp = train_data->input[0][0]; dat < train_data->num_data; dat++)
	{
		for(elem = 0; elem < train_data->num_input; elem++)
		{
			if(train_data->input[dat][elem] < smallest_inp)
				smallest_inp = train_data->input[dat][elem];
			if(train_data->input[dat][elem] > largest_inp)
				largest_inp = train_data->input[dat][elem];
		}
	}

	num_hidden_neurons =
		ann->total_neurons - (ann->num_input + ann->num_output +
							  (unsigned int)(ann->last_layer - ann->first_layer));
	scale_factor =
		(float) (pow
				 ((double) (0.7f * (double) num_hidden_neurons),
				  (double) (1.0f / (double) ann->num_input)) / (double) (largest_inp -
																		 smallest_inp));

#ifdef DEBUG
	printf("Initializing weights with scale factor %f\n", scale_factor);
#endif
	bias_neuron = ann->first_layer->last_neuron - 1;
	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++)
	{
		last_neuron = layer_it->last_neuron;
		bias_neuron = (layer_it - 1)->last_neuron - 1;

		for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++)
		{
			for(num_connect = neuron_it->first_con; num_connect < neuron_it->last_con;
				num_connect++)
			{
				if(bias_neuron == ann->connections[num_connect])
				{
					ann->weights[num_connect] = (fann_type) fann_rand(-scale_factor, scale_factor);
				}
				else
				{
					ann->weights[num_connect] = (fann_type) fann_rand(0, scale_factor);
				}
			}
		}
	}

	if(ann->prev_train_slopes != NULL)
	{
		fann_clear_train_arrays(ann);
	}
}

FANN_EXTERNAL void FANN_API fann_print_parameters(struct fann *ann)
{
	struct fann_layer *layer_it;

	printf("Input layer                          :%4d neurons, 1 bias\n", ann->num_input);
	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer - 1; layer_it++)
	{
			printf("  Hidden layer                       :%4d neurons, 1 bias\n",
				   layer_it->last_neuron - layer_it->first_neuron - 1);
	}
	printf("Output layer                         :%4d neurons\n", ann->num_output);
	printf("Total neurons and biases             :%4d\n", fann_get_total_neurons(ann));
	printf("Total connections                    :%4d\n", ann->total_connections);
	printf("Training algorithm                   :   %s\n", FANN_TRAIN_NAMES[ann->training_algorithm]);
	printf("Training error function              :   %s\n", FANN_ERRORFUNC_NAMES[ann->train_error_function]);
	printf("Training stop function               :   %s\n", FANN_STOPFUNC_NAMES[ann->train_stop_function]);
	printf("Bit fail limit                       :%8.3f\n", ann->bit_fail_limit);
	printf("Learning rate                        :%8.3f\n", ann->learning_rate);
	printf("Learning momentum                    :%8.3f\n", ann->learning_momentum);
	printf("Quickprop decay                      :%11.6f\n", ann->quickprop_decay);
	printf("Quickprop mu                         :%8.3f\n", ann->quickprop_mu);
	printf("RPROP increase factor                :%8.3f\n", ann->rprop_increase_factor);
	printf("RPROP decrease factor                :%8.3f\n", ann->rprop_decrease_factor);
	printf("RPROP delta min                      :%8.3f\n", ann->rprop_delta_min);
	printf("RPROP delta max                      :%8.3f\n", ann->rprop_delta_max);
	
	/* TODO: dump scale parameters */
}

FANN_GET(unsigned int, num_input)
FANN_GET(unsigned int, num_output)

FANN_EXTERNAL unsigned int FANN_API fann_get_total_neurons(struct fann *ann)
{
	/* -1, because there is always an unused bias neuron in the last layer */
	return ann->total_neurons - 1;
}

FANN_GET(unsigned int, total_connections)

FANN_EXTERNAL unsigned int FANN_API fann_get_num_layers(struct fann *ann)
{
    return (unsigned int)(ann->last_layer - ann->first_layer);
}

FANN_EXTERNAL void FANN_API fann_get_layer_array(struct fann *ann, unsigned int *layers)
{
    struct fann_layer *layer_it;

    for (layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) 
	{
        unsigned int count = (unsigned int)(layer_it->last_neuron - layer_it->first_neuron);
        /* Remove the bias from the count of neurons. */
        --count;
        *layers++ = count;
    }
}

FANN_EXTERNAL void FANN_API fann_get_bias_array(struct fann *ann, unsigned int *bias)
{
    struct fann_layer *layer_it;

    for (layer_it = ann->first_layer; layer_it != ann->last_layer; ++layer_it, ++bias) 
	{
        /* Report one bias in each layer except the last */
        if (layer_it != ann->last_layer-1)
            *bias = 1;
        else
            *bias = 0;
    }
}

FANN_EXTERNAL void FANN_API fann_get_connection_array(struct fann *ann, struct fann_connection *connections)
{
    struct fann_neuron *first_neuron;
    struct fann_layer *layer_it;
    struct fann_neuron *neuron_it;
    unsigned int index;
    unsigned int source_index;
    unsigned int destination_index;

    first_neuron = ann->first_layer->first_neuron;

    source_index = 0;
    destination_index = 0;
    
    /* The following assumes that the last unused bias has no connections */

    /* for each layer */
    for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++){
        /* for each neuron */
        for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++){
            /* for each connection */
            for (index = neuron_it->first_con; index < neuron_it->last_con; index++){
                /* Assign the source, destination and weight */
                connections->from_neuron = (unsigned int)(ann->connections[source_index] - first_neuron);
                connections->to_neuron = destination_index;
                connections->weight = ann->weights[source_index];

                connections++;
                source_index++;
            }
            destination_index++;
        }
    }
}

FANN_EXTERNAL void FANN_API fann_set_weight_array(struct fann *ann,
    struct fann_connection *connections, unsigned int num_connections)
{
    unsigned int index;

    for (index = 0; index < num_connections; index++) {
        fann_set_weight(ann, connections[index].from_neuron,
            connections[index].to_neuron, connections[index].weight);
    }
}

FANN_EXTERNAL void FANN_API fann_set_weight(struct fann *ann,
    unsigned int from_neuron, unsigned int to_neuron, fann_type weight)
{
    struct fann_neuron *first_neuron;
    struct fann_layer *layer_it;
    struct fann_neuron *neuron_it;
    unsigned int index;
    unsigned int source_index;
    unsigned int destination_index;

    first_neuron = ann->first_layer->first_neuron;

    source_index = 0;
    destination_index = 0;

    /* Find the connection, simple brute force search through the network
       for one or more connections that match to minimize datastructure dependencies.
       Nothing is done if the connection does not already exist in the network. */

    /* for each layer */
    for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++){
        /* for each neuron */
        for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++){
            /* for each connection */
            for (index = neuron_it->first_con; index < neuron_it->last_con; index++){
                /* If the source and destination neurons match, assign the weight */
                if (((int)from_neuron == ann->connections[source_index] - first_neuron) &&
                    (to_neuron == destination_index))
                {
                    ann->weights[source_index] = weight;
                }
                source_index++;
            }
            destination_index++;
        }
    }
}

FANN_GET_SET(void *, user_data)

/* INTERNAL FUNCTION
   Allocates the main structure and sets some default values.
 */
struct fann *fann_allocate_structure(unsigned int num_layers)
{
	struct fann *ann;

	if(num_layers < 2)
	{
#ifdef DEBUG
		printf("less than 2 layers - ABORTING.\n");
#endif
		return NULL;
	}

	/* allocate and initialize the main network structure */
	ann = (struct fann *) fann_malloc(sizeof(struct fann));
	if(ann == NULL)
	{
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}

	ann->errno_f = FANN_E_NO_ERROR;
	ann->error_log = fann_default_error_log;
	ann->errstr = NULL;
	ann->learning_rate = 0.7f;
	ann->learning_momentum = 0.0;
	ann->total_neurons = 0;
	ann->total_connections = 0;
	ann->num_input = 0;
	ann->num_output = 0;
	ann->train_errors = NULL;
	ann->train_slopes = NULL;
	ann->prev_steps = NULL;
	ann->prev_train_slopes = NULL;
	ann->prev_weights_deltas = NULL;
	ann->training_algorithm = FANN_TRAIN_RPROP;
	ann->num_MSE = 0;
	ann->MSE_value = 0;
	ann->num_bit_fail = 0;
	ann->bit_fail_limit = (fann_type)0.35;
	ann->train_error_function = FANN_ERRORFUNC_TANH;
	ann->train_stop_function = FANN_STOPFUNC_MSE;
	ann->callback = NULL;
    ann->user_data = NULL; /* User is responsible for deallocation */
	ann->weights = NULL;
	ann->connections = NULL;
	ann->output = NULL;
#ifndef FIXEDFANN
	ann->scale_mean_in = NULL;
	ann->scale_deviation_in = NULL;
	ann->scale_new_min_in = NULL;
	ann->scale_factor_in = NULL;
	ann->scale_mean_out = NULL;
	ann->scale_deviation_out = NULL;
	ann->scale_new_min_out = NULL;
	ann->scale_factor_out = NULL;
#endif	
	
	/* Variables for use with with Quickprop training (reasonable defaults) */
	ann->quickprop_decay = (float) -0.0001;
	ann->quickprop_mu = 1.75;

	/* Variables for use with with RPROP training (reasonable defaults) */
	ann->rprop_increase_factor = 1.2f;
	ann->rprop_decrease_factor = 0.5f;
	ann->rprop_delta_min = 0.0f;
	ann->rprop_delta_max = 50.0f;
	ann->rprop_delta_zero = 0.1f;

	ann->can_use_sse = false;
	ann->can_use_avx = false;
	
	fann_init_error_data((struct fann_error *) ann);

	/* allocate room for the layers */
	ann->first_layer = (struct fann_layer *) fann_calloc(num_layers, sizeof(struct fann_layer));
	if(ann->first_layer == NULL)
	{
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_free(ann);
		return NULL;
	}

	ann->last_layer = ann->first_layer + num_layers;
	return ann;
}

/* INTERNAL FUNCTION
   Allocates room for the scaling parameters.
 */
int fann_allocate_scale(struct fann *ann)
{
	/* todo this should only be allocated when needed */
#ifndef FIXEDFANN
	unsigned int i = 0;
#define SCALE_ALLOCATE( what, where, default_value )		    			\
		ann->what##_##where = (fann_type *)fann_calloc(								\
			ann->num_##where##put,											\
			sizeof( float )													\
			);																\
		if( ann->what##_##where == NULL )									\
		{																	\
			fann_error( NULL, FANN_E_CANT_ALLOCATE_MEM );					\
			fann_destroy( ann );                            				\
			return 1;														\
		}																	\
		for( i = 0; i < ann->num_##where##put; i++ )						\
			ann->what##_##where[ i ] = ( default_value );

	SCALE_ALLOCATE( scale_mean,		in,		0.0 )
	SCALE_ALLOCATE( scale_deviation,	in,		1.0 )
	SCALE_ALLOCATE( scale_new_min,	in,		-1.0 )
	SCALE_ALLOCATE( scale_factor,		in,		1.0 )

	SCALE_ALLOCATE( scale_mean,		out,	0.0 )
	SCALE_ALLOCATE( scale_deviation,	out,	1.0 )
	SCALE_ALLOCATE( scale_new_min,	out,	-1.0 )
	SCALE_ALLOCATE( scale_factor,		out,	1.0 )
#undef SCALE_ALLOCATE
#endif	
	return 0;
}

/* INTERNAL FUNCTION
   Allocates room for the neurons.
 */
void fann_allocate_neurons(struct fann *ann)
{
	struct fann_layer *layer_it;
	struct fann_neuron *neurons;
	fann_type *sumPtr = NULL;
	fann_type *valuePtr = NULL;
	unsigned int num_neurons_so_far = 0;
	unsigned int num_neurons = 0;
	unsigned int i;

	/* all the neurons is allocated in one long array (fann_calloc clears mem) */
	neurons = (struct fann_neuron *) fann_calloc(ann->total_neurons, sizeof(struct fann_neuron));

	if(neurons == NULL)
	{
		fann_error((struct fann_error *) ann, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}

	// extra --- AK
	sumPtr = (fann_type *)fann_calloc(ann->total_neurons, sizeof(fann_type));
	valuePtr = (fann_type *)fann_calloc(ann->total_neurons, sizeof(fann_type));

	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		num_neurons = (unsigned int)(layer_it->last_neuron - layer_it->first_neuron);
		layer_it->first_neuron = neurons + num_neurons_so_far;
		layer_it->last_neuron = layer_it->first_neuron + num_neurons;

		//AK
		layer_it->sum = sumPtr + num_neurons_so_far;
		layer_it->value = valuePtr + num_neurons_so_far;

		for(i = 0; i < num_neurons; i++)
		{
			layer_it->first_neuron[i].sumPtr = layer_it->sum + i;
			layer_it->first_neuron[i].valuePtr = layer_it->value + i;
			//layer_it->first_neuron[i].activation_functionPtr = &layer_it->activation_function;
			//layer_it->first_neuron[i].activation_steepnessPtr = &layer_it->activation_steepness;
		}

		num_neurons_so_far += num_neurons;
	}

	ann->output = (fann_type *) fann_calloc(num_neurons, sizeof(fann_type));
	if(ann->output == NULL)
	{
		fann_error((struct fann_error *) ann, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}

	ann->train_errors = (fann_type *) fann_calloc(ann->total_neurons, sizeof(fann_type));
	if(ann->train_errors == NULL)
	{
		fann_error((struct fann_error *) ann, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}
}

/* INTERNAL FUNCTION
   Allocate room for the connections.
 */
void fann_allocate_connections(struct fann *ann)
{
	ann->weights = (fann_type *) fann_calloc(ann->total_connections, sizeof(fann_type));
	if(ann->weights == NULL)
	{
		fann_error((struct fann_error *) ann, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}

	/* TODO make special cases for all places where the connections
	 * is used, so that it is not needed for fully connected networks.
	 */
	ann->connections =
		(struct fann_neuron **) fann_calloc(ann->total_connections,
									   sizeof(struct fann_neuron *));
	if(ann->connections == NULL)
	{
		fann_error((struct fann_error *) ann, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}

	if(ann->prev_weights_deltas == NULL)
	{
		ann->prev_weights_deltas =
			(fann_type *) fann_calloc(ann->total_connections, sizeof(fann_type));
		if(ann->prev_weights_deltas == NULL)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}		
	}
}


/* INTERNAL FUNCTION
   Seed the random function.
 */
void fann_seed_rand()
{
#ifndef _WIN32
	FILE *fp = fopen("/dev/urandom", "r");
	unsigned int foo;
	struct timeval t;

	if(!fp)
	{
		gettimeofday(&t, NULL);
		foo = t.tv_usec;
#ifdef DEBUG
		printf("unable to open /dev/urandom\n");
#endif
	}
	else
	{
		fread(&foo, sizeof(foo), 1, fp);
		fclose(fp);
	}
	srand(foo);
#else
	/* COMPAT_TIME REPLACEMENT */
	srand(GetTickCount());
#endif
}

