#include "FannFA.h"
#include <assert.h>

FannFA::FannFA()
{
	m_ann = NULL;
	m_numInputs = 0;
	m_numOutputs = 0;
}

FannFA::FannFA(std::shared_ptr<FANN::neural_net> ann)
{
	SetANN(ann);
}

FannFA::~FannFA()
{
}

void FannFA::SetANN(std::shared_ptr<FANN::neural_net> ann)
{
	assert(ann.get() != NULL);
	m_ann = ann;
	m_ann->can_use_avx();
	m_ann->can_use_sse();

	m_numInputs = m_ann->get_num_input();
	m_numOutputs = m_ann->get_num_output();
}

BgReward FannFA::GetReward(const std::vector<float> &input)
{
	assert(m_numInputs + 1 == input.size());
	BgReward res;
	
	float *output = NULL;
	if(m_ann->isAvxOk())
		output = m_ann->run_avx(&input[0]);
	else
	if(m_ann->isSseOk())
		output = m_ann->run_sse(&input[0]);
	else
		output = m_ann->run(&input[0]);

	for(int i = 0; i < std::min(BgReward::NN_SIZE, m_numOutputs); i++)
		res[i] = output[i];
	
	return res;	
}

void FannFA::SetReward(const std::vector<float> &input, const BgReward& reward)
{
	if(m_ann->isAvxOk())
		m_ann->train_avx(&input[0], &reward[0]);
	else
	if(m_ann->isSseOk())
		m_ann->train_sse(&input[0], &reward[0]);
	else
		m_ann->train(&input[0], &reward[0]);
}

void FannFA::createNN(int input, int hidden, int output)
{
	m_ann = std::shared_ptr<FANN::neural_net>(new FANN::neural_net);
	unsigned int layers[3];

	if(hidden)
	{
		layers[0] = input;
		layers[1] = hidden;
		layers[2] = output;
		m_ann->create_standard_array(3, layers);
	}
	else
	{
		layers[0] = input;
		layers[1] = output;
		m_ann->create_standard_array(2, layers);
	}

	m_ann->set_activation_function_hidden(FANN::SIGMOID);
	m_ann->set_activation_function_output(FANN::LINEAR);
	m_ann->set_train_error_function(FANN::ERRORFUNC_LINEAR);
	m_ann->set_training_algorithm(FANN::TRAIN_INCREMENTAL);
	m_ann->set_learning_rate(0.1f);
	//m_ann->set_activation_steepness_hidden(5);
	//m_ann->set_activation_steepness_output(5);
	m_ann->randomize_weights(-0.5f, 0.5f);
	//m_ann->randomize_weights(0, 0);
	m_ann->can_use_sse();
	m_ann->can_use_avx();
	m_ann->print_parameters();

	m_numInputs = input;
	m_numOutputs = output;
}

void FannFA::saveNN(fs::path path, std::string name)
{
	path /= name;
	m_ann->save(path.string().c_str());
}

bool FannFA::loadNN(fs::path path, std::string name)
{
	path /= name;
	m_ann = std::shared_ptr<FANN::neural_net>(new FANN::neural_net);
	if(!m_ann->create_from_file(path.string().c_str()))
		return false;

	m_ann->can_use_sse();
	m_ann->can_use_avx();

	m_numInputs = m_ann->get_num_input();
	m_numOutputs = m_ann->get_num_output();

	return true;
}
