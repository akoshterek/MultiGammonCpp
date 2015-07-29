#ifndef _FUNCTIONAPPROXIMATOR_H_
#define _FUNCTIONAPPROXIMATOR_H_
#pragma once

#include <vector>
#include "BgReward.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

enum ApproxType {atFann, atMBP};

class FunctionApproximator
{
public:
	virtual BgReward GetReward(const std::vector<float> &input) = 0;
	virtual void SetReward(const std::vector<float> &input, const BgReward& reward) = 0;
	
	virtual void AddToReward(const std::vector<float> &input, const BgReward& deltaReward)
	{
		SetReward(input, (GetReward(input) + deltaReward).clamp());
	}
	
	virtual ~FunctionApproximator(){};

	virtual void createNN(int input, int hidden, int output) = 0;
	virtual void saveNN(fs::path path, std::string name) = 0;
	virtual bool loadNN(fs::path path, std::string name) = 0;
};

#endif
