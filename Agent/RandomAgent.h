#if !defined __BG_RANDOMAGENT_H
#define __BG_RANDOMAGENT_H

#pragma once
#include "BgAgent.h"

class RandomAgent :	public BgAgent
{
public:
	RandomAgent(fs::path path);
	virtual ~RandomAgent(void);

	virtual void evalRace(const BgBoard *board, BgReward& reward);
	virtual void evalCrashed(const BgBoard *board, BgReward& reward) {evalRace(board, reward);}
	virtual void evalContact(const BgBoard *board, BgReward& reward) {evalRace(board, reward);}
};

#endif
