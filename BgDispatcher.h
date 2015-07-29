#if !defined __BG_DISPATCHER_H
#define __BG_DISPATCHER_H

#pragma once
#include "BgEval.h"
#include "Agent/BgAgent.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

class BgDispatcher
{
public:
	BgDispatcher(void);
	bool init(int argc, char **argv);
	void run();

private:
	int m_argc;
	char **m_argv;

	po::options_description m_desc;
	po::variables_map m_vm;
	void runAgentIteration(const char *agentName, const char *benchAgentName);
	void runIteration(BgAgent *agent1, BgAgent *benchAgent, BgAgent *agent2,
		int trainGames, int benchmarkGames, int benchmarkPeriod);

	static void banner();
	static void showTextArray(char *textArr[]);
};

#endif
