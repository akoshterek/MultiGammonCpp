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
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "fann.h"

/* Create a network from a configuration file.
 */
FANN_EXTERNAL struct fann *FANN_API fann_create_from_file(const char *configuration_file)
{
	struct fann *ann;
	FILE *conf = fopen(configuration_file, "r");

	if(!conf)
	{
		fann_error(NULL, FANN_E_CANT_OPEN_CONFIG_R, configuration_file);
		return NULL;
	}
	ann = fann_create_from_fd(conf, configuration_file);
	fclose(conf);
	return ann;
}

/* Save the network.
 */
FANN_EXTERNAL int FANN_API fann_save(struct fann *ann, const char *configuration_file)
{
	int retval;
	FILE *conf = fopen(configuration_file, "w+");

	if(!conf)
	{
		fann_error((struct fann_error *) ann, FANN_E_CANT_OPEN_CONFIG_W, configuration_file);
		return -1;
	}
	retval = fann_save_internal_fd(ann, conf, configuration_file);
	fclose(conf);
	return retval;
}

/* INTERNAL FUNCTION
   Used to save the network to a file descriptor.
 */
int fann_save_internal_fd(struct fann *ann, FILE * conf, const char *configuration_file)
{
	struct fann_layer *layer_it;
	int calculated_decimal_point = 0;
	struct fann_neuron *neuron_it, *first_neuron;
	fann_type *weights;
	struct fann_neuron **connected_neurons;
	unsigned int i = 0;

#ifndef FIXEDFANN
	/* variabels for use when saving floats as fixed point variabels */
	unsigned int decimal_point = 0;
	unsigned int fixed_multiplier = 0;
	fann_type max_possible_value = 0;
	unsigned int bits_used_for_max = 0;
	fann_type current_max_value = 0;
#endif

	/* save the version information */
	fprintf(conf, FANN_FLO_VERSION "\n");

	/* Save network parameters */
	fprintf(conf, "num_layers=%u\n", ann->last_layer - ann->first_layer);
	fprintf(conf, "learning_rate=%f\n", ann->learning_rate);
	
	fprintf(conf, "learning_momentum=%f\n", ann->learning_momentum);
	fprintf(conf, "training_algorithm=%u\n", ann->training_algorithm);
	fprintf(conf, "train_error_function=%u\n", ann->train_error_function);
	fprintf(conf, "train_stop_function=%u\n", ann->train_stop_function);
	fprintf(conf, "quickprop_decay=%f\n", ann->quickprop_decay);
	fprintf(conf, "quickprop_mu=%f\n", ann->quickprop_mu);
	fprintf(conf, "rprop_increase_factor=%f\n", ann->rprop_increase_factor);
	fprintf(conf, "rprop_decrease_factor=%f\n", ann->rprop_decrease_factor);
	fprintf(conf, "rprop_delta_min=%f\n", ann->rprop_delta_min);
	fprintf(conf, "rprop_delta_max=%f\n", ann->rprop_delta_max);
	fprintf(conf, "rprop_delta_zero=%f\n", ann->rprop_delta_zero);
	fprintf(conf, "bit_fail_limit="FANNPRINTF"\n", ann->bit_fail_limit);
	fprintf(conf, "\n");

	fprintf(conf, "layer_sizes=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		/* the number of neurons in the layers (in the last layer, there is always one too many neurons, because of an unused bias) */
		fprintf(conf, "%u ", layer_it->last_neuron - layer_it->first_neuron);
	}
	fprintf(conf, "\n");

	fprintf(conf, "layer_activation=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		/* the number of neurons in the layers (in the last layer, there is always one too many neurons, because of an unused bias) */
		fprintf(conf, "%u ", layer_it->activation_function);
	}
	fprintf(conf, "\n");

	fprintf(conf, "layer_steepness=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		/* the number of neurons in the layers (in the last layer, there is always one too many neurons, because of an unused bias) */
		fprintf(conf, FANNPRINTF, layer_it->activation_steepness);
		fprintf(conf, " ");
	}
	fprintf(conf, "\n");

#ifndef FIXEDFANN
	/* 2.1 */
	#define SCALE_SAVE( what, where )										\
		fprintf( conf, #what "_" #where "=" );								\
		for( i = 0; i < ann->num_##where##put; i++ )						\
			fprintf( conf, "%f ", ann->what##_##where[ i ] );				\
		fprintf( conf, "\n" );

	//if(!save_as_fixed)
	{
		if(ann->scale_mean_in != NULL)
		{
			fprintf(conf, "scale_included=1\n");
			SCALE_SAVE( scale_mean,			in )
			SCALE_SAVE( scale_deviation,	in )
			SCALE_SAVE( scale_new_min,		in )
			SCALE_SAVE( scale_factor,		in )
		
			SCALE_SAVE( scale_mean,			out )
			SCALE_SAVE( scale_deviation,	out )
			SCALE_SAVE( scale_new_min,		out )
			SCALE_SAVE( scale_factor,		out )
		}
		else
			fprintf(conf, "scale_included=0\n");
	}
#undef SCALE_SAVE
#endif	

	/* 2.0 */
	fprintf(conf, "neurons (num_inputs)=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		/* the neurons */
		for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++)
		{
			fprintf(conf, "(%u) ", neuron_it->last_con - neuron_it->first_con);
		}
	}
	fprintf(conf, "\n");

	connected_neurons = ann->connections;
	weights = ann->weights;
	first_neuron = ann->first_layer->first_neuron;

	/* Now save all the connections.
	 * We only need to save the source and the weight,
	 * since the destination is given by the order.
	 * 
	 * The weight is not saved binary due to differences
	 * in binary definition of floating point numbers.
	 * Especially an iPAQ does not use the same binary
	 * representation as an i386 machine.
	 */
	fprintf(conf, "connections (connected_to_neuron, weight)=");
	for(i = 0; i < ann->total_connections; i++)
	{
		/* save the connection "(source weight) " */
		fprintf(conf, "(%u, " FANNPRINTF ") ", connected_neurons[i] - first_neuron, weights[i]);
	}
	fprintf(conf, "\n");

	return calculated_decimal_point;
}

struct fann *fann_create_from_fd_1_1(FILE * conf, const char *configuration_file);

#define fann_scanf(type, name, val) \
{ \
	if(fscanf(conf, name"="type"\n", val) != 1) \
	{ \
		fann_error(NULL, FANN_E_CANT_READ_CONFIG, name, configuration_file); \
		fann_destroy(ann); \
		return NULL; \
	} \
}

/* INTERNAL FUNCTION
   Create a network from a configuration file descriptor.
 */
struct fann *fann_create_from_fd(FILE * conf, const char *configuration_file)
{
	unsigned int num_layers, layer_size, input_neuron, i, num_connections;
	unsigned int scale_included;
	struct fann_neuron *first_neuron, *neuron_it, *last_neuron, **connected_neurons;
	fann_type *weights;
	struct fann_layer *layer_it;
	struct fann *ann = NULL;

	char *read_version;

	read_version = (char *) calloc(strlen(FANN_CONF_VERSION "\n"), 1);
	if(read_version == NULL)
	{
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}

	fread(read_version, 1, strlen(FANN_CONF_VERSION "\n"), conf);	/* reads version */

	/* compares the version information */
	if(strncmp(read_version, FANN_CONF_VERSION "\n", strlen(FANN_CONF_VERSION "\n")) != 0)
	{
		if(strncmp(read_version, "FANN_FLO_1.1\n", strlen("FANN_FLO_1.1\n")) == 0)
		{
			free(read_version);
			return fann_create_from_fd_1_1(conf, configuration_file);
		}

		/* Maintain compatibility with 2.0 version that doesnt have scale parameters. */
		if(strncmp(read_version, "FANN_FLO_2.0\n", strlen("FANN_FLO_2.0\n")) != 0 &&
		   strncmp(read_version, "FANN_FLO_2.1\n", strlen("FANN_FLO_2.1\n")) != 0)
		{
			free(read_version);
			fann_error(NULL, FANN_E_WRONG_CONFIG_VERSION, configuration_file);

			return NULL;
		}
	}

	free(read_version);

    fann_scanf("%u", "num_layers", &num_layers);

	ann = fann_allocate_structure(num_layers);
	if(ann == NULL)
	{
		return NULL;
	}

    fann_scanf("%f", "learning_rate", &ann->learning_rate);
	fann_scanf("%f", "learning_momentum", &ann->learning_momentum);
	fann_scanf("%u", "training_algorithm", (unsigned int *)&ann->training_algorithm);
	fann_scanf("%u", "train_error_function", (unsigned int *)&ann->train_error_function);
	fann_scanf("%u", "train_stop_function", (unsigned int *)&ann->train_stop_function);
	fann_scanf("%f", "quickprop_decay", &ann->quickprop_decay);
	fann_scanf("%f", "quickprop_mu", &ann->quickprop_mu);
	fann_scanf("%f", "rprop_increase_factor", &ann->rprop_increase_factor);
	fann_scanf("%f", "rprop_decrease_factor", &ann->rprop_decrease_factor);
	fann_scanf("%f", "rprop_delta_min", &ann->rprop_delta_min);
	fann_scanf("%f", "rprop_delta_max", &ann->rprop_delta_max);
	fann_scanf("%f", "rprop_delta_zero", &ann->rprop_delta_zero);

	fann_scanf(FANNSCANF, "bit_fail_limit", &ann->bit_fail_limit);

#ifdef DEBUG
	printf("creating network with %d layers\n", num_layers);
	printf("input\n");
#endif

	fscanf(conf, "layer_sizes=");
	/* determine how many neurons there should be in each layer */
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		if(fscanf(conf, "%u ", &layer_size) != 1)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_CONFIG, "layer_sizes", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		/* we do not allocate room here, but we make sure that
		 * last_neuron - first_neuron is the number of neurons */
		layer_it->first_neuron = NULL;
		layer_it->last_neuron = layer_it->first_neuron + layer_size;
		ann->total_neurons += layer_size;
#ifdef DEBUG
		if(ann->network_type == FANN_NETTYPE_SHORTCUT && layer_it != ann->first_layer)
		{
			printf("  layer       : %d neurons, 0 bias\n", layer_size);
		}
		else
		{
			printf("  layer       : %d neurons, 1 bias\n", layer_size - 1);
		}
#endif
	}

	fscanf(conf, "layer_activation=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		if(fscanf(conf, "%u ", &layer_it->activation_function) != 1)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_CONFIG, "layer_activation", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
	}

	fscanf(conf, "layer_steepness=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		if(fscanf(conf, FANNSCANF, &layer_it->activation_steepness) != 1)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_CONFIG, "layer_steepness", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
	}

	ann->num_input = (unsigned int)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);
	ann->num_output = (unsigned int)(((ann->last_layer - 1)->last_neuron - (ann->last_layer - 1)->first_neuron));
	/* one too many (bias) in the output layer */
	ann->num_output--;

#ifndef FIXEDFANN
#define SCALE_LOAD( what, where )											\
	fscanf( conf, #what "_" #where "=" );									\
	for(i = 0; i < ann->num_##where##put; i++)								\
	{																		\
		if(fscanf( conf, "%f ", (float *)&ann->what##_##where[ i ] ) != 1)  \
		{																	\
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_CONFIG, #what "_" #where, configuration_file); \
			fann_destroy(ann); 												\
			return NULL;													\
		}																	\
	}
	
	if(fscanf(conf, "\nscale_included=%u\n", &scale_included) == 1 && scale_included == 1)
	{
		fann_allocate_scale(ann);
		SCALE_LOAD( scale_mean,			in )
		SCALE_LOAD( scale_deviation,	in )
		SCALE_LOAD( scale_new_min,		in )
		SCALE_LOAD( scale_factor,		in )
	
		SCALE_LOAD( scale_mean,			out )
		SCALE_LOAD( scale_deviation,	out )
		SCALE_LOAD( scale_new_min,		out )
		SCALE_LOAD( scale_factor,		out )
	}
#undef SCALE_LOAD
#endif
	
	/* allocate room for the actual neurons */
	fann_allocate_neurons(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM)
	{
		fann_destroy(ann);
		return NULL;
	}

	last_neuron = (ann->last_layer - 1)->last_neuron;
	fscanf(conf, "neurons (num_inputs)=");
	for(neuron_it = ann->first_layer->first_neuron; neuron_it != last_neuron; neuron_it++)
	{
		if(fscanf(conf, "(%u) ", &num_connections) <= 0)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		neuron_it->first_con = ann->total_connections;
		ann->total_connections += num_connections;
		neuron_it->last_con = ann->total_connections;
	}

	fann_allocate_connections(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM)
	{
		fann_destroy(ann);
		return NULL;
	}

	connected_neurons = ann->connections;
	weights = ann->weights;
	first_neuron = ann->first_layer->first_neuron;

	fscanf(conf, "connections (connected_to_neuron, weight)=");
	for(i = 0; i < ann->total_connections; i++)
	{
		if(fscanf(conf, "(%u, " FANNSCANF ") ", &input_neuron, &weights[i]) != 2)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_CONNECTIONS, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		connected_neurons[i] = first_neuron + input_neuron;
	}

#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}


/* INTERNAL FUNCTION
   Create a network from a configuration file descriptor. (backward compatible read of version 1.1 files)
 */
struct fann *fann_create_from_fd_1_1(FILE * conf, const char *configuration_file)
{
	unsigned int num_layers, layer_size, input_neuron, i, network_type, num_connections;
	unsigned int activation_function_hidden, activation_function_output;
#ifdef FIXEDFANN
	unsigned int decimal_point, multiplier;
#endif
	fann_type activation_steepness_hidden, activation_steepness_output;
	float learning_rate, connection_rate;
	struct fann_neuron *first_neuron, *neuron_it, *last_neuron, **connected_neurons;
	fann_type *weights;
	struct fann_layer *layer_it;
	struct fann *ann;

#ifdef FIXEDFANN
	if(fscanf(conf, "%u\n", &decimal_point) != 1)
	{
		fann_error(NULL, FANN_E_CANT_READ_CONFIG, "decimal_point", configuration_file);
		return NULL;
	}
	multiplier = 1 << decimal_point;
#endif

	if(fscanf(conf, "%u %f %f %u %u %u " FANNSCANF " " FANNSCANF "\n", &num_layers, &learning_rate,
		&connection_rate, &network_type, &activation_function_hidden,
		&activation_function_output, &activation_steepness_hidden,
		&activation_steepness_output) != 8)
	{
		fann_error(NULL, FANN_E_CANT_READ_CONFIG, "parameters", configuration_file);
		return NULL;
	}

	ann = fann_allocate_structure(num_layers);
	if(ann == NULL)
	{
		return NULL;
	}
	ann->learning_rate = learning_rate;

#ifdef DEBUG
	printf("creating network with learning rate %f\n", learning_rate);
	printf("input\n");
#endif

	/* determine how many neurons there should be in each layer */
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++)
	{
		if(fscanf(conf, "%u ", &layer_size) != 1)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		/* we do not allocate room here, but we make sure that
		 * last_neuron - first_neuron is the number of neurons */
		layer_it->first_neuron = NULL;
		layer_it->last_neuron = layer_it->first_neuron + layer_size;
		ann->total_neurons += layer_size;
#ifdef DEBUG
		if(ann->network_type == FANN_NETTYPE_SHORTCUT && layer_it != ann->first_layer)
		{
			printf("  layer       : %d neurons, 0 bias\n", layer_size);
		}
		else
		{
			printf("  layer       : %d neurons, 1 bias\n", layer_size - 1);
		}
#endif
	}

	ann->num_input = (unsigned int)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);
	ann->num_output = (unsigned int)(((ann->last_layer - 1)->last_neuron - (ann->last_layer - 1)->first_neuron));
	/* one too many (bias) in the output layer */
	ann->num_output--;

	/* allocate room for the actual neurons */
	fann_allocate_neurons(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM)
	{
		fann_destroy(ann);
		return NULL;
	}

	last_neuron = (ann->last_layer - 1)->last_neuron;
	for(neuron_it = ann->first_layer->first_neuron; neuron_it != last_neuron; neuron_it++)
	{
		if(fscanf(conf, "%u ", &num_connections) != 1)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		neuron_it->first_con = ann->total_connections;
		ann->total_connections += num_connections;
		neuron_it->last_con = ann->total_connections;
	}

	fann_allocate_connections(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM)
	{
		fann_destroy(ann);
		return NULL;
	}

	connected_neurons = ann->connections;
	weights = ann->weights;
	first_neuron = ann->first_layer->first_neuron;

	for(i = 0; i < ann->total_connections; i++)
	{
		if(fscanf(conf, "(%u " FANNSCANF ") ", &input_neuron, &weights[i]) != 2)
		{
			fann_error((struct fann_error *) ann, FANN_E_CANT_READ_CONNECTIONS, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		connected_neurons[i] = first_neuron + input_neuron;
	}

	fann_set_activation_steepness_hidden(ann, activation_steepness_hidden);
	fann_set_activation_steepness_output(ann, activation_steepness_output);
	fann_set_activation_function_hidden(ann, (enum fann_activationfunc_enum)activation_function_hidden);
	fann_set_activation_function_output(ann, (enum fann_activationfunc_enum)activation_function_output);

#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}
