#include "RandomAgent.h"


RandomAgent::RandomAgent(fs::path path)
	: BgAgent(path)
{
	m_fullName = "Random";
}


RandomAgent::~RandomAgent(void)
{
}

void RandomAgent::evalRace(const BgBoard *board, BgReward& reward)
{
	reward.reset();
	reward[OUTPUT_WIN] = float(rand()) / RAND_MAX;
}
