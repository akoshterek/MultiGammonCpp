#include <boost/algorithm/string.hpp>

#include "BgAgentFactory.h"
#include "RandomAgent.h"
#include "HeuristicAgent.h"
#include "PubevalAgent.h"
#include "GnubgAgent.h"
#include "FlexAgent.h"
#include "PubevalRepresentation.h"
#include "RawRepresentation.h"

BgAgent * BgAgentFactory::createAgent(std::string fullName)
{
	BgEval *eval = BgEval::Instance();
	std::string fullNameLower = fullName;
	std::transform(fullNameLower.begin(), fullNameLower.end(), fullNameLower.begin(), ::tolower);
	std::vector<std::string> tokens;
	boost::algorithm::split(tokens, fullNameLower, boost::algorithm::is_any_of("-"),  boost::algorithm::token_compress_on);
	
	BgAgent *agent = NULL;
	if(fullNameLower == "random")
	{
		agent = new RandomAgent(eval->getBasePath());
	}
	else
	if(fullNameLower == "heuristic")
	{
		agent = new HeuristicAgent(eval->getBasePath());
	}
	else
	if(fullNameLower == "pubeval")
	{
		agent = new PubevalAgent(eval->getBasePath());
	}
	else
	if(fullNameLower == "gnubg")
	{
		agent = new GnubgAgent(eval->getBasePath());
	}
	else
	if(fullNameLower == "pubevalex")
	{
		FlexAgent *a = new FlexAgent(eval->getBasePath(), std::shared_ptr<InputRepresentation>(new PubevalRepresentation()), fullName);
		if(!a->loadNN(atFann))
		{
			a->createContactFA(123, 0, 5, atFann);
			a->createCrashedFA(123, 0, 5, atFann);
			a->createRaceFA(123, 0, 5, atFann);
		}
		a->setLambda(0.7f);
		a->setSanityCheck(false);
		a->load();
		agent = a;
	}
	else
	if(tokens.size() == 2 && tokens[0] == "raw")
	{
		BoardEncoding boardEnc = encSutton;
		if(tokens[1] == "tesauro89")
			boardEnc = encTes89;
		else
		if(tokens[1] == "tesauro92")
			boardEnc = encTes92;
		else
		if(tokens[1] == "sutton")
			boardEnc = encSutton;
		else
		if(tokens[1] == "gnu")
			boardEnc = encGnu;
		else
			throw std::exception("Unknown board encoding");
		
		FlexAgent *a = new FlexAgent(eval->getBasePath(), std::shared_ptr<InputRepresentation>(new RawRepresentation(boardEnc)), fullName);
		if(!a->loadNN(atFann))
		{
			a->createContactFA(199, 39, 5, atFann);
			a->createCrashedFA(199, 39, 5, atFann);
			a->createRaceFA   (199, 39, 5, atFann);
		}
		a->setLambda(0.7f);
		a->setSanityCheck(false);
		a->load();
		agent = a;
	}
	
	return agent;
}
