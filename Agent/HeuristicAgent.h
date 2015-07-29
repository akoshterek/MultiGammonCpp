#if !defined __BG_HEURISTIC_H
#define __BG_HEURISTIC_H

#pragma once
#include "BgAgent.h"

class HeuristicAgent :	public BgAgent
{
public:
	HeuristicAgent(fs::path path);
	virtual ~HeuristicAgent(void);

	//virtual void evaluatePosition(const BgBoard *board, positionclass& pc, 
	//	const bgvariation bgv, BgReward& reward);

	virtual void evalRace(const BgBoard *board, BgReward& reward);
	virtual void evalCrashed(const BgBoard *board, BgReward& reward) {evalRace(board, reward);}
	virtual void evalContact(const BgBoard *board, BgReward& reward) {evalRace(board, reward);}
};

#endif
