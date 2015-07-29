#include "HeuristicAgent.h"
#include "BgBoard.h"

/**
 * Copyright: Francois Rivest (University of Montreal) / Marc G. Bellemare (McGill University)
 */

HeuristicAgent::HeuristicAgent(fs::path path)
	: BgAgent(path)
{
	m_fullName = "Heuristic";
}

HeuristicAgent::~HeuristicAgent(void)
{
}

void HeuristicAgent::evalRace(const BgBoard *board, BgReward& reward)
{
	reward.reset();
	float value = 0;
    const char *points = board->anBoard[BgBoard::SELF];

	char total = 0;
	for(int i = 0; i < 25; i++)
		total += points[i];
	char atHome = 15 - total;

    // 1/15th of a point per man home
    value += atHome / 15.0f;

    // -1/5th of a point per man on the bar
    value -= points[BgBoard::BAR] / 5.0f;

    for (int i = 0; i < 24; i++)
    {
		// -1/10th of a point for each blot
		// +1/20th for contiguous points
		if (points[i] == 1) 
			value -= 0.10f;
		else 
		if (i > 0 && points[i] >= 2 && points[i-1] >= 2) 
			value += 0.05;

		int dist = 25 - i;

		// value based on closeness to home
		value += (12.5f - dist) * (float)points[i] / 225.0f;
    }

	reward[OUTPUT_WIN] = value;
}
/*
void HeuristicAgent::evaluatePosition(const BgBoard *board, positionclass& pc, 
		const bgvariation bgv, BgReward& reward)
{
	if(pc == CLASS_OVER)
	{
		evalOver(board, bgv, reward);
	}
	else
	{
		evalRace(board, reward);
		pc = CLASS_CONTACT;
	}
}
*/