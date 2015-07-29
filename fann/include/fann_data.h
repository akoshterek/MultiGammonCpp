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

#ifndef __fann_data_h__
#define __fann_data_h__

#include <stdio.h>

/* Section: FANN Datatypes

   The two main datatypes used in the fann library is <struct fann>, 
   which represents an artificial neural network, and <struct fann_train_data>,
   which represent training data.
 */


/* Type: fann_type
   fann_type is the type used for the weights, inputs and outputs of the neural network.
   
	fann_type is defined as a:
	float - if you include fann.h or floatfann.h
	double - if you include doublefann.h
	int - if you include fixedfann.h (please be aware that fixed point usage is 
			only to be used during execution, and not during training).
*/

/* Enum: fann_train_enum
	The Training algorithms used when training on <struct fann_train_data> with functions like
	<fann_train_on_data> or <fann_train_on_file>. The incremental training looks alters the weights
	after each time it is presented an input pattern, while batch only alters the weights once after
	it has been presented to all the patterns.

	FANN_TRAIN_INCREMENTAL -  Standard backpropagation algorithm, where the weights are 
		updated after each training pattern. This means that the weights are updated many 
		times during a single epoch. For this reason some problems, will train very fast with 
		this algorithm, while other more advanced problems will not train very well.
	FANN_TRAIN_BATCH -  Standard backpropagation algorithm, where the weights are updated after 
		calculating the mean square error for the whole training set. This means that the weights 
		are only updated once during a epoch. For this reason some problems, will train slower with 
		this algorithm. But since the mean square error is calculated more correctly than in 
		incremental training, some problems will reach a better solutions with this algorithm.
	FANN_TRAIN_RPROP - A more advanced batch training algorithm which achieves good results 
		for many problems. The RPROP training algorithm is adaptive, and does therefore not 
		use the learning_rate. Some other parameters can however be set to change the way the 
		RPROP algorithm works, but it is only recommended for users with insight in how the RPROP 
		training algorithm works. The RPROP training algorithm is described by 
		[Riedmiller and Braun, 1993], but the actual learning algorithm used here is the 
		iRPROP- training algorithm which is described by [Igel and Husken, 2000] which 
    	is an variety of the standard RPROP training algorithm.
	FANN_TRAIN_QUICKPROP - A more advanced batch training algorithm which achieves good results 
		for many problems. The quickprop training algorithm uses the learning_rate parameter 
		along with other more advanced parameters, but it is only recommended to change these 
		advanced parameters, for users with insight in how the quickprop training algorithm works.
		The quickprop training algorithm is described by [Fahlman, 1988].
	
	See also:
		<fann_set_training_algorithm>, <fann_get_training_algorithm>
*/
enum fann_train_enum
{
	FANN_TRAIN_INCREMENTAL = 0,
	FANN_TRAIN_BATCH,
	FANN_TRAIN_RPROP,
	FANN_TRAIN_QUICKPROP
};

/* Constant: FANN_TRAIN_NAMES
   
   Constant array consisting of the names for the training algorithms, so that the name of an
   training function can be received by:
   (code)
   char *name = FANN_TRAIN_NAMES[train_function];
   (end)

   See Also:
      <fann_train_enum>
*/
static char const *const FANN_TRAIN_NAMES[] = {
	"FANN_TRAIN_INCREMENTAL",
	"FANN_TRAIN_BATCH",
	"FANN_TRAIN_RPROP",
	"FANN_TRAIN_QUICKPROP"
};

/* Enums: fann_activationfunc_enum
   
	The activation functions used for the neurons during training. The activation functions
	can either be defined for a group of neurons by <fann_set_activation_function_hidden> and
	<fann_set_activation_function_output> or it can be defined for a single neuron by <fann_set_activation_function>.

	The steepness of an activation function is defined in the same way by 
	<fann_set_activation_steepness_hidden>, <fann_set_activation_steepness_output> and <fann_set_activation_steepness>.
   
   The functions are described with functions where:
   * x is the input to the activation function,
   * y is the output,
   * s is the steepness and
   * d is the derivation.

   FANN_LINEAR - Linear activation function. 
     * span: -inf < y < inf
	 * y = x*s, d = 1*s
	 * Can NOT be used in fixed point.

   FANN_THRESHOLD - Threshold activation function.
	 * x < 0 -> y = 0, x >= 0 -> y = 1
	 * Can NOT be used during training.

   FANN_THRESHOLD_SYMMETRIC - Threshold activation function.
	 * x < 0 -> y = 0, x >= 0 -> y = 1
	 * Can NOT be used during training.

   FANN_SIGMOID - Sigmoid activation function.
	 * One of the most used activation functions.
	 * span: 0 < y < 1
	 * y = 1/(1 + exp(-2*s*x))
	 * d = 2*s*y*(1 - y)

   FANN_SIGMOID_STEPWISE - Stepwise linear approximation to sigmoid.
	 * Faster than sigmoid but a bit less precise.

   FANN_SIGMOID_SYMMETRIC - Symmetric sigmoid activation function, aka. tanh.
	 * One of the most used activation functions.
	 * span: -1 < y < 1
	 * y = tanh(s*x) = 2/(1 + exp(-2*s*x)) - 1
	 * d = s*(1-(y*y))

   FANN_SIGMOID_SYMMETRIC - Stepwise linear approximation to symmetric sigmoid.
	 * Faster than symmetric sigmoid but a bit less precise.

   FANN_GAUSSIAN - Gaussian activation function.
	 * 0 when x = -inf, 1 when x = 0 and 0 when x = inf
	 * span: 0 < y < 1
	 * y = exp(-x*s*x*s)
	 * d = -2*x*s*y*s

   FANN_GAUSSIAN_SYMMETRIC - Symmetric gaussian activation function.
	 * -1 when x = -inf, 1 when x = 0 and 0 when x = inf
	 * span: -1 < y < 1
	 * y = exp(-x*s*x*s)*2-1
	 * d = -2*x*s*(y+1)*s
	 
   FANN_ELLIOT - Fast (sigmoid like) activation function defined by David Elliott
	 * span: 0 < y < 1
	 * y = ((x*s) / 2) / (1 + |x*s|) + 0.5
	 * d = s*1/(2*(1+|x*s|)*(1+|x*s|))
	 
   FANN_ELLIOT_SYMMETRIC - Fast (symmetric sigmoid like) activation function defined by David Elliott
	 * span: -1 < y < 1   
	 * y = (x*s) / (1 + |x*s|)
	 * d = s*1/((1+|x*s|)*(1+|x*s|))

	FANN_LINEAR_PIECE - Bounded linear activation function.
	 * span: 0 <= y <= 1
	 * y = x*s, d = 1*s
	 
	FANN_LINEAR_PIECE_SYMMETRIC - Bounded linear activation function.
	 * span: -1 <= y <= 1
	 * y = x*s, d = 1*s
	
	FANN_SIN_SYMMETRIC - Periodical sinus activation function.
	 * span: -1 <= y <= 1
	 * y = sin(x*s)
	 * d = s*cos(x*s)
	 
	FANN_COS_SYMMETRIC - Periodical cosinus activation function.
	 * span: -1 <= y <= 1
	 * y = cos(x*s)
	 * d = s*-sin(x*s)
	 
	FANN_SIN - Periodical sinus activation function.
	 * span: 0 <= y <= 1
	 * y = sin(x*s)/2+0.5
	 * d = s*cos(x*s)/2
	 
	FANN_COS - Periodical cosinus activation function.
	 * span: 0 <= y <= 1
	 * y = cos(x*s)/2+0.5
	 * d = s*-sin(x*s)/2
	 
	See also:
   	<fann_set_activation_function_layer>, <fann_set_activation_function_hidden>,
   	<fann_set_activation_function_output>, <fann_set_activation_steepness>,
    <fann_set_activation_function>
*/
enum fann_activationfunc_enum
{
	FANN_LINEAR,
	FANN_THRESHOLD,
	FANN_THRESHOLD_SYMMETRIC,
	FANN_SIGMOID,
	FANN_SIGMOID_STEPWISE,
	FANN_SIGMOID_SYMMETRIC,
	FANN_SIGMOID_SYMMETRIC_STEPWISE,
	FANN_GAUSSIAN,
	FANN_GAUSSIAN_SYMMETRIC,
	/* Stepwise linear approximation to gaussian.
	 * Faster than gaussian but a bit less precise.
	 * NOT implemented yet.
	 */
	FANN_GAUSSIAN_STEPWISE,
	FANN_ELLIOT,
	FANN_ELLIOT_SYMMETRIC,
	FANN_LINEAR_PIECE,
	FANN_LINEAR_PIECE_SYMMETRIC,
	FANN_SIN_SYMMETRIC,
	FANN_COS_SYMMETRIC,
	FANN_SIN,
	FANN_COS
};

/* Constant: FANN_ACTIVATIONFUNC_NAMES
   
   Constant array consisting of the names for the activation function, so that the name of an
   activation function can be received by:
   (code)
   char *name = FANN_ACTIVATIONFUNC_NAMES[activation_function];
   (end)

   See Also:
      <fann_activationfunc_enum>
*/
static char const *const FANN_ACTIVATIONFUNC_NAMES[] = {
	"FANN_TANH_APPROX", 
	"FANN_LINEAR",
	"FANN_THRESHOLD",
	"FANN_THRESHOLD_SYMMETRIC",
	"FANN_SIGMOID",
	"FANN_SIGMOID_STEPWISE",
	"FANN_SIGMOID_SYMMETRIC",
	"FANN_SIGMOID_SYMMETRIC_STEPWISE",
	"FANN_GAUSSIAN",
	"FANN_GAUSSIAN_SYMMETRIC",
	"FANN_GAUSSIAN_STEPWISE",
	"FANN_ELLIOT",
	"FANN_ELLIOT_SYMMETRIC",
	"FANN_LINEAR_PIECE",
	"FANN_LINEAR_PIECE_SYMMETRIC",
	"FANN_SIN_SYMMETRIC",
	"FANN_COS_SYMMETRIC",
	"FANN_SIN",
	"FANN_COS"
};

/* Enum: fann_errorfunc_enum
	Error function used during training.
	
	FANN_ERRORFUNC_LINEAR - Standard linear error function.
	FANN_ERRORFUNC_TANH - Tanh error function, usually better 
		but can require a lower learning rate. This error function agressively targets outputs that
		differ much from the desired, while not targetting outputs that only differ a little that much.
		This activation function is not recommended for cascade training and incremental training.

	See also:
		<fann_set_train_error_function>, <fann_get_train_error_function>
*/
enum fann_errorfunc_enum
{
	FANN_ERRORFUNC_LINEAR = 0,
	FANN_ERRORFUNC_TANH
};

/* Constant: FANN_ERRORFUNC_NAMES
   
   Constant array consisting of the names for the training error functions, so that the name of an
   error function can be received by:
   (code)
   char *name = FANN_ERRORFUNC_NAMES[error_function];
   (end)

   See Also:
      <fann_errorfunc_enum>
*/
static char const *const FANN_ERRORFUNC_NAMES[] = {
	"FANN_ERRORFUNC_LINEAR",
	"FANN_ERRORFUNC_TANH"
};

/* Enum: fann_stopfunc_enum
	Stop criteria used during training.

	FANN_STOPFUNC_MSE - Stop criteria is Mean Square Error (MSE) value.
	FANN_STOPFUNC_BIT - Stop criteria is number of bits that fail. The number of bits; means the
		number of output neurons which differ more than the bit fail limit 
		(see <fann_get_bit_fail_limit>, <fann_set_bit_fail_limit>). 
		The bits are counted in all of the training data, so this number can be higher than
		the number of training data.

	See also:
		<fann_set_train_stop_function>, <fann_get_train_stop_function>
*/
enum fann_stopfunc_enum
{
	FANN_STOPFUNC_MSE = 0,
	FANN_STOPFUNC_BIT
};

/* Constant: FANN_STOPFUNC_NAMES
   
   Constant array consisting of the names for the training stop functions, so that the name of a
   stop function can be received by:
   (code)
   char *name = FANN_STOPFUNC_NAMES[stop_function];
   (end)

   See Also:
      <fann_stopfunc_enum>
*/
static char const *const FANN_STOPFUNC_NAMES[] = {
	"FANN_STOPFUNC_MSE",
	"FANN_STOPFUNC_BIT"
};

/* forward declarations for use with the callback */
struct fann;
struct fann_train_data;
/* Type: fann_callback_type
   This callback function can be called during training when using <fann_train_on_data>, 
   <fann_train_on_file> or <fann_cascadetrain_on_data>.
	
	>typedef int (FANN_API * fann_callback_type) (struct fann *ann, struct fann_train_data *train, 
	>											  unsigned int max_epochs, 
	>                                             unsigned int epochs_between_reports, 
	>                                             float desired_error, unsigned int epochs);
	
	The callback can be set by using <fann_set_callback> and is very usefull for doing custom 
	things during training. It is recommended to use this function when implementing custom 
	training procedures, or when visualizing the training in a GUI etc. The parameters which the
	callback function takes is the parameters given to the <fann_train_on_data>, plus an epochs
	parameter which tells how many epochs the training have taken so far.
	
	The callback function should return an integer, if the callback function returns -1, the training
	will terminate.
	
	Example of a callback function:
		>int FANN_API test_callback(struct fann *ann, struct fann_train_data *train,
		>				            unsigned int max_epochs, unsigned int epochs_between_reports, 
		>				            float desired_error, unsigned int epochs)
		>{
		>	printf("Epochs     %8d. MSE: %.5f. Desired-MSE: %.5f\n", epochs, fann_get_MSE(ann), desired_error);
		>	return 0;
		>}
	
	See also:
		<fann_set_callback>, <fann_train_on_data>
 */ 
FANN_EXTERNAL typedef int (FANN_API * fann_callback_type) (struct fann *ann, struct fann_train_data *train, 
														   unsigned int max_epochs, 
														   unsigned int epochs_between_reports, 
														   float desired_error, unsigned int epochs);


/* ----- Data structures -----
 * No data within these structures should be altered directly by the user.
 */

struct fann_neuron
{
	/* Index to the first and last connection
	 * (actually the last is a past end index)
	 */
	unsigned int first_con;
	unsigned int last_con;
	/* The sum of the inputs multiplied with the weights */
	//fann_type sum;
	fann_type *sumPtr;
	/* The value of the activation function applied to the sum */
	//fann_type value;
	fann_type *valuePtr;

	/* Pointer to the steepness of the activation function */
	//fann_type *activation_steepnessPtr;
	/* Pointer to Used to choose which activation function to use */
	//enum fann_activationfunc_enum *activation_functionPtr;

#ifdef __GNUC__
} __attribute__ ((packed));
#else
};
#endif

/* A single layer in the neural network.
 */
struct fann_layer
{
	/* A pointer to the first neuron in the layer 
	 * When allocated, all the neurons in all the layers are actually
	 * in one long array, this is because we wan't to easily clear all
	 * the neurons at once.
	 */
	struct fann_neuron *first_neuron;

	/* A pointer to the neuron past the last neuron in the layer */
	/* the number of neurons is last_neuron - first_neuron */
	struct fann_neuron *last_neuron;

	/* A pointer to the sum of the inputs multiplied with the weights 
	 * One long array for all
	 */
	fann_type *sum;
	/* A pointer to the value of the activation function applied to the sum 
	 * One long array for all
	 */
	fann_type *value;

	/* The steepness of the activation function */
	fann_type activation_steepness;
	/* Used to choose which activation function to use */
	enum fann_activationfunc_enum activation_function;
};

/* Struct: struct fann_error
   
	Structure used to store error-related information, both
	<struct fann> and <struct fann_train_data> can be casted to this type.
	
	See also:
		<fann_set_error_log>, <fann_get_errno>
*/
struct fann_error
{
	enum fann_errno_enum errno_f;
	FILE *error_log;
	char *errstr;
};


/* 	Struct: struct fann
	The fast artificial neural network(fann) structure.

	Data within this structure should never be accessed directly, but only by using the
	*fann_get_...* and *fann_set_...* functions.

	The fann structure is created using one of the *fann_create_...* functions and each of
	the functions which operates on the structure takes *struct fann * ann* as the first parameter.

	See also:
		<fann_create_standard>, <fann_destroy>
 */
struct fann
{
	/* The type of error that last occured. */
	enum fann_errno_enum errno_f;

	/* Where to log error messages. */
	FILE *error_log;

	/* A string representation of the last error. */
	char *errstr;

	/* the learning rate of the network */
	float learning_rate;

	/* The learning momentum used for backpropagation algorithm. */
	float learning_momentum;

	/* pointer to the first layer (input layer) in an array af all the layers,
	 * including the input and outputlayers 
	 */
	struct fann_layer *first_layer;

	/* pointer to the layer past the last layer in an array af all the layers,
	 * including the input and outputlayers 
	 */
	struct fann_layer *last_layer;

	/* Total number of neurons.
	 * very usefull, because the actual neurons are allocated in one long array
	 */
	unsigned int total_neurons;

	/* Number of input neurons (not calculating bias) */
	unsigned int num_input;

	/* Number of output neurons (not calculating bias) */
	unsigned int num_output;

	/* The weight array */
	fann_type *weights;

	/* The connection array */
	struct fann_neuron **connections;

	/* Used to contain the errors used during training
	 * Is allocated during first training session,
	 * which means that if we do not train, it is never allocated.
	 */
	fann_type *train_errors;

	/* Training algorithm used when calling fann_train_on_..
	 */
	enum fann_train_enum training_algorithm;


	/* Total number of connections.
	 * very usefull, because the actual connections
	 * are allocated in one long array
	 */
	unsigned int total_connections;

	/* used to store outputs in */
	fann_type *output;

	/* the number of data used to calculate the mean square error.
	 */
	unsigned int num_MSE;

	/* the total error value.
	 * the real mean square error is MSE_value/num_MSE
	 */
	float MSE_value;

	/* The number of outputs which would fail (only valid for classification problems)
	 */
	unsigned int num_bit_fail;

	/* The maximum difference between the actual output and the expected output 
	 * which is accepted when counting the bit fails.
	 * This difference is multiplied by two when dealing with symmetric activation functions,
	 * so that symmetric and not symmetric activation functions can use the same limit.
	 */
	fann_type bit_fail_limit;

	/* The error function used during training. (default FANN_ERRORFUNC_TANH)
	 */
	enum fann_errorfunc_enum train_error_function;
	
	/* The stop function used during training. (default FANN_STOPFUNC_MSE)
	*/
	enum fann_stopfunc_enum train_stop_function;

	/* The callback function used during training. (default NULL)
	*/
	fann_callback_type callback;

    /* A pointer to user defined data. (default NULL)
    */
    void *user_data;

	/* Variables for use with Quickprop training */

	/* Decay is used to make the weights not go so high */
	float quickprop_decay;

	/* Mu is a factor used to increase and decrease the stepsize */
	float quickprop_mu;

	/* Variables for use with with RPROP training */

	/* Tells how much the stepsize should increase during learning */
	float rprop_increase_factor;

	/* Tells how much the stepsize should decrease during learning */
	float rprop_decrease_factor;

	/* The minimum stepsize */
	float rprop_delta_min;

	/* The maximum stepsize */
	float rprop_delta_max;

	/* The initial stepsize */
	float rprop_delta_zero;
        
	/* Used to contain the slope errors used during batch training
	 * Is allocated during first training session,
	 * which means that if we do not train, it is never allocated.
	 */
	fann_type *train_slopes;

	/* The previous step taken by the quickprop/rprop procedures.
	 * Not allocated if not used.
	 */
	fann_type *prev_steps;

	/* The slope values used by the quickprop/rprop procedures.
	 * Not allocated if not used.
	 */
	fann_type *prev_train_slopes;
        
	/* The last delta applied to a connection weight.
	 * This is used for the momentum term in the backpropagation algorithm.
	 * Not allocated if not used.	 
	 */
	fann_type *prev_weights_deltas;
	
	/* Arithmetic mean used to remove steady component in input data.  */
	fann_type *scale_mean_in;

	/* Standart deviation used to normalize input data (mostly to [-1;1]). */
	fann_type *scale_deviation_in;

	/* User-defined new minimum for input data.
	 * Resulting data values may be less than user-defined minimum. 
	 */
	fann_type *scale_new_min_in;

	/* Used to scale data to user-defined new maximum for input data.
	 * Resulting data values may be greater than user-defined maximum. 
	 */
	fann_type *scale_factor_in;
	
	/* Arithmetic mean used to remove steady component in output data.  */
	fann_type *scale_mean_out;

	/* Standart deviation used to normalize output data (mostly to [-1;1]). */
	fann_type *scale_deviation_out;

	/* User-defined new minimum for output data.
	 * Resulting data values may be less than user-defined minimum. 
	 */
	fann_type *scale_new_min_out;

	/* Used to scale data to user-defined new maximum for output data.
	 * Resulting data values may be greater than user-defined maximum. 
	 */
	fann_type *scale_factor_out;

	/* Automatically set to non-zero when ANN can be processed with SSE SIMD instructions 
	   Set it to zero if you don't want to use SSE here

       See also:
           <fann_can_use_sse>, <fann_disable_sse>
	 */
	bool can_use_sse;

	/* Automatically set to non-zero when ANN can be processed with AVX256 SIMD instructions 
	   Set it to zero if you don't want to use AVX here

       See also:
           <fann_can_use_avx>, <fann_disable_avx>
	 */
	bool can_use_avx;
};

/* Type: fann_connection

    Describes a connection between two neurons and its weight

    from_neuron - Unique number used to identify source neuron
    to_neuron - Unique number used to identify destination neuron
    weight - The numerical value of the weight

    See Also:
        <fann_get_connection_array>, <fann_set_weight_array>

   This structure appears in FANN >= 2.1.0
*/
struct fann_connection
{
    /* Unique number used to identify source neuron */
    unsigned int from_neuron;
    /* Unique number used to identify destination neuron */
    unsigned int to_neuron;
    /* The numerical value of the weight */
    fann_type weight;
};

#endif
