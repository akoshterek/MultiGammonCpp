#if !defined __BGREWARD_H
#define __BGREWARD_H

#pragma once

#include <algorithm>
#include <vector>
#include "BgCommon.h"

template <class T> T crop(T minVal, T maxVal, T val)
{
	if(minVal > maxVal)
		std::swap(minVal, maxVal);

	return std::min(std::max(val, minVal), maxVal);
}

class BgReward : public std::vector<float>
{
public:
	static const size_t NN_SIZE = NUM_OUTPUTS;

	BgReward(float val = 0)
	{
		resize(NUM_ROLLOUT_OUTPUTS, val);
	}

	BgReward(float* data)
	{
		resize(NUM_ROLLOUT_OUTPUTS, 0);
		for(size_t i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
			(*this)[i] = data[i];
	}

	BgReward(const BgReward& src)
	{
		resize(NUM_ROLLOUT_OUTPUTS, 0);
		for(size_t i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
			(*this)[i] = src[i];
	}

	void reset()
	{
		for(size_t i = 0; i < size(); i++)
			(*this)[i] = 0;
	}

	void set(float val)
	{
		for(size_t i = 0; i < size(); i++)
			(*this)[i] = val;
	}

	BgReward operator+(const BgReward& r) const
	{
		BgReward res;
		for(size_t i = 0; i < size(); i++)
			res[i] = (*this)[i] + r[i];

		return res;
	}
	BgReward operator-(const BgReward& r) const
	{
		BgReward res;
		for(size_t i = 0; i < size(); i++)
			res[i] = (*this)[i] - r[i];

		return res;
	}

	BgReward operator*(const BgReward& r) const
	{
		BgReward res;
		for(int i = 0; i < size(); i++)
			res[i] = (*this)[i] * r[i];

		return res;
	}

	BgReward operator*(const float r) const
	{
		BgReward res;
		for(int i = 0; i < size(); i++)
			res[i] = (*this)[i] * r;

		return res;
	}

	BgReward clamp() const
	{
		const float MIN_VAL = 0.0f;
		const float MAX_VAL = 1.0f;

		BgReward res;
		for(int i = 0; i < size(); i++)
			res[i] = crop(MIN_VAL, MAX_VAL, (*this)[i]);

		return res;
	}

	// Move evaluation
	// let's keep things simple as I don't want to go into cube handling
	// At least for now
	float utility() const
	{
		// calculate money equity
		return (*this)[ OUTPUT_WIN ] * 2.0f - 1.0f +
			( (*this)[ OUTPUT_WINGAMMON ] - (*this)[ OUTPUT_LOSEGAMMON ] ) +
			( (*this)[ OUTPUT_WINBACKGAMMON ] - (*this)[ OUTPUT_LOSEBACKGAMMON ] );
	}

	void invert()
	{
		float r;
    
		(*this)[ OUTPUT_WIN ] = 1.0f - (*this)[ OUTPUT_WIN ];
	
		r = (*this)[ OUTPUT_WINGAMMON ];
		(*this)[ OUTPUT_WINGAMMON ] = (*this)[ OUTPUT_LOSEGAMMON ];
		(*this)[ OUTPUT_LOSEGAMMON ] = r;
	
		r = (*this)[ OUTPUT_WINBACKGAMMON ];
		(*this)[ OUTPUT_WINBACKGAMMON ] = (*this)[ OUTPUT_LOSEBACKGAMMON ];
		(*this)[ OUTPUT_LOSEBACKGAMMON ] = r;

		(*this)[ OUTPUT_EQUITY ] = -(*this)[ OUTPUT_EQUITY ];
	}
};

#endif
