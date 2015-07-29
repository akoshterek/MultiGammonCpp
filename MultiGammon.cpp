// MultiGammon.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#ifdef _DEBUG
		#include <vld.h>
	#endif
#endif

#include <stdlib.h>
#include <math.h>
#include <intrin.h>
//#include <omp.h>

#include "fann.h"
#include "fann_cpp.h"

#include "BgDispatcher.h"
void perfTest()
{
	float FANN_SSE_ALIGN(x[]) = {1, 1, 1, 1, 1, 1, 1, 1};
	float FANN_SSE_ALIGN(y[]) = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};

	srand(GetTickCount());
	const int bound = 1 * 8 * 255 * 4 * 1000;
	fann_type *ar1 = new fann_type[bound];
	
//#pragma omp  parallel for
	for(int i = 0; i < bound; i++)
		ar1[i] = fann_type(rand()) / RAND_MAX;
	DWORD t1, t2;

	FANN::neural_net fann;
	unsigned int layers[] = {255, 127, 5};
	fann.create_standard_array(3, layers);
	fann.set_activation_function_hidden(FANN::SIGMOID_SYMMETRIC);
	fann.set_activation_function_output(FANN::LINEAR);
	fann.set_train_error_function(FANN::ERRORFUNC_LINEAR);
	fann.set_training_algorithm(FANN::TRAIN_INCREMENTAL);
	//fann.print_parameters();
	fann.can_use_sse();
	fann.can_use_avx();

	float desired[5] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
	_mm256_zeroall();
	t1 = GetTickCount();
//#pragma omp  parallel for
	for(int i = 0; i < bound; i+=255)
	{
		//fann.run_avx(ar1+i);
		//fann.run_sse(ar1+i);	
		//fann.run(ar1+i);
		//fann.train(ar1+i, desired);
		//fann.train_sse(ar1+i, desired);
		//fann.train_avx(ar1+i, desired);
	}
	t2 = GetTickCount();
	printf("Absolute FANN float %d\n", t2-t1); //target is < 900
	unsigned __int64 r1, r2;
	
	r1 = __rdtsc();
	//fann.run_sse(ar1);
	//double ddd1 = tanh_approx(0.1);
	r2 = __rdtsc();
	printf("Absolute FANN float for single run %I64d cpu ticks\n", r2-r1); //~150,000

	delete [] ar1;
}

fann_type testFunction(fann_type x, fann_type y)
{
	return (x*x - y*y) / 2;
}

void runTest()
{
	srand(GetTickCount());
	const int dataSize = 101 * 101;
	fann_type dataset[dataSize][7];
	fann_type desired[dataSize];
	
	FANN::neural_net fann;
	unsigned int layers[] = {7, 15, 1};
	fann.create_standard_array(3, layers);
	fann.set_activation_function_hidden(FANN::SIGMOID_SYMMETRIC);
	fann.set_activation_function_output(FANN::LINEAR);
	fann.set_train_error_function(FANN::ERRORFUNC_LINEAR);
	fann.set_training_algorithm(FANN::TRAIN_INCREMENTAL);
	fann.set_learning_rate(0.1f);
	fann.set_activation_steepness_hidden(1);
	fann.set_activation_steepness_output(1);
	fann.randomize_weights(-0.5f, 0.5f);
	fann.print_parameters();
	fann.can_use_sse();
	fann.can_use_avx();

	for(int i = 0; i < 101; i++)
	{
		for(int j = 0; j < 101; j++)
		{
			fann_type x = -1.0f + 0.02f * i;
			fann_type y = -1.0f + 0.02f * j;
			fann_type z = testFunction(x, y);
			int index = i * 101 + j;
			dataset[index][0] = x;
			dataset[index][1] = y;
			for(int k = 2; k < 7; k++)
				dataset[index][k] = 0;

			desired[index] = z;
		}
	}

	printf("\n\n");
	fann_type test[7] = {0};

	fann_type mse = 1;
	int iteration = 0;
	while(mse > 1e-4)
	{
		iteration++;
		for(int j = 0; j < dataSize; j++)
		{
			//fann.train_sse(dataset[j], desired + j);
		}

		fann_type x = 0.71;//-1.0f + 2 * (float)rand() / RAND_MAX;
		fann_type y = 0.71;//-1.0f + 2 * (float)rand() / RAND_MAX;
		fann_type z = testFunction(x, y);
		test[0] = x;
		test[1] = y;
		fann_type res1, res2;
		res1 = *fann.run(test);
		res2 = *fann.run(test);
		mse = (res1 - z) * (res1 - z);

		printf("%d\t(%4.2f, %4.2f) = %f\t%f\t%f\tmse=%f\n", iteration, x, y, z, res1, res2, mse);
	}

	//for(int i = 0; i < 10; i++)
	{
	//	fann_type x = 0.71;//-1.0f + 2 * (float)rand() / RAND_MAX;
	//	fann_type y = 0.71;//-1.0f + 2 * (float)rand() / RAND_MAX;
	//	fann_type z = testFunction(x, y);
	//	test[0] = x;
	//	test[1] = y;
	//	fann_type res1, res2;
	//	res1 = *fann.run(test);
	//	res2 = *fann.run_sse(test);
	//	printf("(%4.2f, %4.2f) = %f\t%f\t%f\n", x, y, z, res1, res2);
	}

	fann.save("paraboloid.ann");
	//fann.print_connections();
	printf("\n\n");

	FANN::neural_net fann2;
	fann2.create_from_file("paraboloid.ann");
	{
		fann_type x = 0.71;//-1.0f + 2 * (float)rand() / RAND_MAX;
		fann_type y = 0.71;//-1.0f + 2 * (float)rand() / RAND_MAX;
		fann_type z = testFunction(x, y);
		test[0] = x;
		test[1] = y;
		fann_type res1, res2;
		res1 = *fann.run(test);
		res2 = *fann2.run(test);
		printf("(%4.2f, %4.2f) = %f\t%f ?? %f\n", x, y, z, res1, res2);
	}
	fann2.print_parameters();
	printf("finished\n");
}

int main(int argc, char **argv)
{
	BgDispatcher *dispatcher = new BgDispatcher();
	if(dispatcher->init(argc, argv))
	{
		dispatcher->run();
	}

	//perfTest();
	//runTest();

	delete dispatcher;
	BgEval::Destroy();

	getchar();
	return 0;
}
