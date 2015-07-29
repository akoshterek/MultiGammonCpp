#ifndef _FANNFA_H_
#define _FANNFA_H_

#include <memory>
#include "FunctionApproximator.h"
#include "fann.h"
#include "fann_cpp.h"

class FannFA : public FunctionApproximator
{
public:
	FannFA();
	FannFA(std::shared_ptr<FANN::neural_net> m_ann);
	virtual ~FannFA();
	
	std::shared_ptr<FANN::neural_net> GetANN() const {return m_ann;}
	void SetANN(std::shared_ptr<FANN::neural_net> m_ann);
	
	virtual void SetReward(const std::vector<float> &input, const BgReward& reward);
	virtual BgReward GetReward(const std::vector<float> &input);

	virtual void createNN(int input, int hidden, int output);
	virtual void saveNN(fs::path path, std::string name);
	virtual bool loadNN(fs::path path, std::string name);

protected:
	std::shared_ptr<FANN::neural_net> m_ann;
	BgReward InternalGetReward(const std::vector<float> &input);
	size_t m_numInputs, m_numOutputs;
};

#endif
