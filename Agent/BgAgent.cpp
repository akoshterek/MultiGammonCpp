#include "BgAgent.h"

BgAgent::BgAgent(fs::path path)
{
	m_learnMode = false;
	m_supportsSanityCheck = false;
	m_fullName = "";
	m_playedGames = 0;
	m_curBoard = NULL;
	m_isFixed = true;
	m_needsInvertedEval = false;

	m_path = path /= "agents";
}

void BgAgent::evalOver(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalOver(board, reward, m_bgv);
}

void BgAgent::evalHypergammon1(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalHypergammon1(board, reward);
}

void BgAgent::evalHypergammon2(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalHypergammon2(board, reward);
}

void BgAgent::evalHypergammon3(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalHypergammon3(board, reward);
}

void BgAgent::evalBearoff2(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalBearoff2(board, reward);
}

void BgAgent::evalBearoffTS(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalBearoffTS(board, reward);
}

void BgAgent::evalBearoff1(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalBearoff1(board, reward);
}

void BgAgent::evalBearoffOS(const BgBoard *board, BgReward& reward)
{
	BgEval::Instance()->EvalBearoffOS(board, reward);
}

void BgAgent::evaluatePosition(const BgBoard *board, positionclass& pc, BgReward& reward)
{
	switch(pc)
	{
	case CLASS_OVER:
		evalOver(board, reward);
		break;
	case CLASS_HYPERGAMMON1:
		evalHypergammon1(board, reward);
		break;
	case CLASS_HYPERGAMMON2:
		evalHypergammon2(board, reward);
		break;
	case CLASS_HYPERGAMMON3:
		evalHypergammon3(board, reward);
		break;
	case CLASS_BEAROFF2:
		evalBearoff2(board, reward);
		break;
	case CLASS_BEAROFF_TS:
		evalBearoffTS(board, reward);
		break;
	case CLASS_BEAROFF1:
		evalBearoff1(board, reward);
		break;
	case CLASS_BEAROFF_OS:
		evalBearoffOS(board, reward);
		break;
	case CLASS_RACE:
		evalRace(board, reward);
		break;
	case CLASS_CRASHED:
		evalCrashed(board, reward);
		break;
	case CLASS_CONTACT:
		evalContact(board, reward);
		break;
	default:
		throw std::exception("Unknown class. How did we get here?");
	}
}
