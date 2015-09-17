#if !defined __BG_PUBEVAL_H
#define __BG_PUBEVAL_H

#pragma once
#include "BgAgent.h"


class PubevalAgent : public BgAgent
{
public:
	PubevalAgent(fs::path path);
	virtual ~PubevalAgent(void);

	virtual void evaluatePosition(const BgBoard *board, positionclass& pc, BgReward& reward);

private:
	float m_wr[124];
	float m_wc[124];
	float m_x [124]; 

	void loadWeights(fs::path contact, fs::path race);
	void setX(const int pos[28]);
	void preparePos(const BgBoard *board, int pos[28]) const;
	float pubeval(bool race, int pos[28]);
};

#endif
