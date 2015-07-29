#include "FlexAgent.h"
#include "BgBoard.h"

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
//-A Raw-Tesauro92 -A Raw-Tesauro89  -A Raw-Sutton -A Raw-Gnu -B Heuristic -T 1000000 -G 1000 -P 10000
FlexAgent::FlexAgent(fs::path path, std::shared_ptr<InputRepresentation> representation, std::string name)
	: BgAgent(path), m_representation(representation) 
{
	m_fullName = name;
	m_path /= name;
	m_isFixed = false;

	m_supportsSanityCheck = true;

	m_alpha = 0.2f;
	m_alphaAnnealFactor = 1;
	m_gamma = 1;
 	m_lambda = 0.7f;
	m_step = 0;

	m_heuristic.reset(new HeuristicAgent(m_path));
}

FlexAgent::~FlexAgent(void)
{
}

BgAgent *FlexAgent::clone()
{
	FlexAgent *a = new FlexAgent(BgEval::Instance()->getBasePath(), m_representation, m_fullName);
	
	a->m_supportsSanityCheck = m_supportsSanityCheck;
	a->m_alpha = m_alpha;
	a->m_alphaAnnealFactor = m_alphaAnnealFactor;
	a->m_gamma = m_gamma;
	a->m_lambda = m_lambda;
	a->m_learnMode = m_learnMode;

	a->m_nnContact = m_nnContact;
	a->m_nnCrashed = m_nnCrashed;
	a->m_nnRace = m_nnRace;

	return a;
}

void FlexAgent::createContactFA(int input, int hidden, int output, ApproxType annType)
{
	if(annType == atFann)
	{
		fs::create_directories(m_path);
		m_nnContact = std::shared_ptr<FunctionApproximator>(new FannFA());
		m_nnContact->createNN(input, hidden, output);
		m_nnContact->saveNN(m_path, "contact.fann");
	}
	else
	{
		assert(0);
	}
}

void FlexAgent::createRaceFA(int input, int hidden, int output, ApproxType annType)
{
	if(annType == atFann)
	{
		fs::create_directories(m_path);
		m_nnRace = std::shared_ptr<FunctionApproximator>(new FannFA());
		m_nnRace->createNN(input, hidden, output);
		m_nnRace->saveNN(m_path, "race.fann");
	}
	else
	{
		assert(0);
	}
}

void FlexAgent::createCrashedFA(int input, int hidden, int output, ApproxType annType)
{
	if(annType == atFann)
	{
		fs::create_directories(m_path);
		m_nnCrashed = std::shared_ptr<FunctionApproximator>(new FannFA());
		m_nnCrashed->createNN(input, hidden, output);
		m_nnCrashed->saveNN(m_path, "crashed.fann");
	}
	else
	{
		assert(0);
	}
}

bool FlexAgent::loadNN(ApproxType annType)
{
	if(annType == atFann)
	{
		m_nnContact = std::shared_ptr<FunctionApproximator>(new FannFA());
		if(!m_nnContact->loadNN(m_path, "contact.fann"))
			return false;
		m_nnRace = std::shared_ptr<FunctionApproximator>(new FannFA());
		if(!m_nnRace->loadNN(m_path, "race.fann"))
			return false;
		m_nnCrashed = std::shared_ptr<FunctionApproximator>(new FannFA());
		if(!m_nnCrashed->loadNN(m_path, "crashed.fann"))
			return false;

		return true;
	}
	else
	{
		return false;
	}
}

void FlexAgent::load()
{
	fs::path cfg = m_path;
	cfg /= "agent.xml";

    std::ifstream ifs(cfg.string().c_str());
    if(ifs.good())
	{
		boost::archive::xml_iarchive ia(ifs);
		ia >>  boost::serialization::make_nvp("FlexAgent", *this);
	}
}

void FlexAgent::save()
{
	fs::create_directories(m_path);

	m_nnContact->saveNN(m_path, "contact.fann");
	m_nnRace->saveNN(m_path, "race.fann");
	m_nnCrashed->saveNN(m_path, "crashed.fann");

	fs::path cfg = m_path;
	cfg /= "agent.xml";

    std::ofstream ofs(cfg.string().c_str());
    assert(ofs.good());
    boost::archive::xml_oarchive oa(ofs);
	oa <<  boost::serialization::make_nvp("FlexAgent", *this);
}

void FlexAgent::startGame(bgvariation bgv)
{
	BgAgent::startGame(bgv);
	m_eligibilityTraces.clear();
	m_step = 0;
}

void FlexAgent::endGame()
{
	BgAgent::endGame();

	if(m_learnMode)
	{
		//Anneal parameters
		m_alpha *= m_alphaAnnealFactor;
	}
}

void FlexAgent::prepareStep0(const bgmove& pm)
{
	if(m_bgv == VARIATION_STANDARD && pm.pc == CLASS_CONTACT)
	{
		BgBoard initBoard;
		initBoard.InitBoard(m_bgv);
		m_prevEntry.pc = BgEval::Instance()->ClassifyPosition(&initBoard, m_bgv);
		m_prevEntry.auch = initBoard.PositionKey();
	}
	else
	{
		m_prevEntry.pc = pm.pc;
		m_prevEntry.auch = pm.auch;
	}
}

//Q update rule
BgReward FlexAgent::calcDeltaReward(const bgmove& pm, const BgReward& reward)
{
	//prev reward
	BgReward prevQValue;
	BgBoard prevBoard = BgBoard::PositionFromKey(m_prevEntry.auch);
	evaluatePosition(&prevBoard, m_prevEntry.pc, prevQValue);

	//Predicted greedy reward
	BgReward predictedGreedyReward = pm.arEvalMove;

	BgReward deltaReward = (reward + predictedGreedyReward * m_gamma - prevQValue);
	return deltaReward;
}

void FlexAgent::doMove(const bgmove& pm)
{
	BgAgent::doMove(pm);
	if(!m_learnMode)
		return;

	if(m_step == 0)
		prepareStep0(pm);

	BgReward reward(0.0f), deltaReward(0.0f);
	if(pm.pc == CLASS_OVER)
	{
		reward = pm.arEvalMove;
		deltaReward = calcDeltaReward(pm, reward);
	}
		
	ETraceEntry entry;
	entry.auch = pm.auch;
	entry.pc = CLASS_CONTACT;//pm.pc;

	BgBoard board = BgBoard::PositionFromKey(pm.auch);
	entry.input.resize(m_representation->getContactInputs());
	m_representation->calculateContactInputs(&board, &entry.input[0]);
	/*
	switch(pm.pc)
	{
	case CLASS_CONTACT:
		entry.input.resize(m_representation->getContactInputs());
		m_representation->calculateContactInputs(&board, &entry.input[0]);
		break;
	case CLASS_RACE:
		entry.input.resize(m_representation->getRaceInputs());
		m_representation->calculateRaceInputs(&board, &entry.input[0]);
		break;
	case CLASS_CRASHED:
		entry.input.resize(m_representation->getCrashedInputs());
		m_representation->calculateCrashedInputs(&board, &entry.input[0]);
		break;
	default:
	}
	*/
	m_eligibilityTraces.push_front(entry);
	
	if(pm.pc == CLASS_OVER)
	{
		m_step = 0;
		deltaReward[OUTPUT_EQUITY] = 0;
		deltaReward = deltaReward * m_alpha;
		UpdateETrace(deltaReward);
	}

	m_step++;
	m_prevEntry = entry;
}

void FlexAgent::UpdateETrace(BgReward deltaReward)
{
	//Update Q values and eligibility traces
	std::list<ETraceEntry>::iterator i;
	std::list<ETraceEntry>::iterator toBeErased = m_eligibilityTraces.end();

	float eTrace = 1.0f;
	for(i = m_eligibilityTraces.begin(); i != m_eligibilityTraces.end(); i++)
	{
		const std::vector<float> &input = i->input;
		
		FunctionApproximator *Q = getQ(i->pc);
		if(Q)
			Q->AddToReward(input, deltaReward * eTrace);

		eTrace *= (m_gamma * m_lambda);

		if(eTrace < 0.00000001f)
		{
			toBeErased = i;
			break;
		}
	}

	m_eligibilityTraces.erase(toBeErased, m_eligibilityTraces.end());
}

FunctionApproximator *FlexAgent::getQ(positionclass pc) const
{
	switch(pc)
	{
	case CLASS_CONTACT:
		return m_nnContact.get();
		break;
	//case CLASS_RACE:
	//	return m_nnRace.get();
	//	break;
	//case CLASS_CRASHED:
	//	return m_nnCrashed.get();
	//	break;
	default:
		throw std::exception("Unknown position class");
		return NULL;
	}
}

void FlexAgent::evalRace(const BgBoard *board, BgReward& reward)
{
	std::vector<float> arInput(m_representation->getRaceInputs(), 0);
	m_representation->calculateRaceInputs( board, &arInput[0] );
	reward = m_nnRace->GetReward(arInput);
  
	if(m_supportsSanityCheck && !isLearnMode())
	{
		/* anBoard[1] is on roll */
		/* total men for side not on roll */
		int totMen0 = 0;
    
		/* total men for side on roll */
		int totMen1 = 0;

		/* a set flag for every possible outcome */
		int any = 0, i;
    
		for(i = 23; i >= 0; --i) 
		{
			totMen0 += board->anBoard[0][i];
			totMen1 += board->anBoard[1][i];
		}

		if( totMen1 == 15 )
			any |= OG_POSSIBLE;
    
		if( totMen0 == 15 )
			any |= G_POSSIBLE;

		if( any ) 
		{
			if( any & OG_POSSIBLE ) 
			{
				for(i = 23; i >= 18; --i) 
				{
					if( board->anBoard[1][i] > 0 ) 
					{
						break;
					}
				}

				if( i >= 18 )
					any |= OBG_POSSIBLE;
			}

			if( any & G_POSSIBLE ) 
			{
				for(i = 23; i >= 18; --i) 
				{
					if( board->anBoard[0][i] > 0 ) 
						break;
				}

				if( i >= 18 ) 
					any |= BG_POSSIBLE;
			}
		}
    
		if( any & (BG_POSSIBLE | OBG_POSSIBLE) ) 
		{
			/* side that can have the backgammon */
			int side = (any & BG_POSSIBLE) ? 1 : 0;
			float pr = BgEval::Instance()->raceBGprob(board, side, m_bgv);

			if( pr > 0.0 ) 
			{
				if( side == 1 ) 
				{
					reward[OUTPUT_WINBACKGAMMON] = pr;

					if( reward[OUTPUT_WINGAMMON] < reward[OUTPUT_WINBACKGAMMON] ) 
						reward[OUTPUT_WINGAMMON] = reward[OUTPUT_WINBACKGAMMON];
				} 
				else 
				{
					reward[OUTPUT_LOSEBACKGAMMON] = pr;

					if(reward[OUTPUT_LOSEGAMMON] < reward[OUTPUT_LOSEBACKGAMMON])
						reward[OUTPUT_LOSEGAMMON] = reward[OUTPUT_LOSEBACKGAMMON];
				}
			} 
			else 
			{
				if( side == 1 )
					reward[OUTPUT_WINBACKGAMMON] = 0.0;
				else
					reward[OUTPUT_LOSEBACKGAMMON] = 0.0;
			}
		}  
  
		// sanity check will take care of rest
	}
}

void FlexAgent::evalCrashed(const BgBoard *board, BgReward& reward)
{
	std::vector<float> arInput(m_representation->getCrashedInputs(), 0);
	m_representation->calculateCrashedInputs( board, &arInput[0] );
	reward = m_nnCrashed->GetReward(arInput);
}

void FlexAgent::evalContact(const BgBoard *board, BgReward& reward)
{
	std::vector<float> arInput(m_representation->getContactInputs(), 0);
	m_representation->calculateContactInputs( board, &arInput[0] );
	reward = m_nnContact->GetReward(arInput);
}

void FlexAgent::evaluatePosition(const BgBoard *board, positionclass& pc, BgReward& reward)
{
	if(isLearnMode() && pc != CLASS_OVER && m_step > 1000)
	{
		m_heuristic->evaluatePosition(board, pc, reward);
		return;
	}

	switch(pc)
	{
	case CLASS_OVER:
		evalOver(board, reward);
		break;
		/*
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
		*/
	default:
		//throw std::exception("Unknown class. How did we get here?");
		evalContact(board, reward);
		pc = CLASS_CONTACT;
	}
}
