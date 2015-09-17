#include <iostream>
#include "BgDispatcher.h"
#include "Agent/BgAgentFactory.h"
#include "BgGameDispatcher.h"

extern char *aszCopying[];
extern char *aszWarranty[];

BgDispatcher::BgDispatcher(void)
	: m_desc("Allowed options")
{
	m_argc = 0;
	m_argv = NULL;

	m_desc.add_options()
    ("help,h", "produce help message")
    ("copying,c", "show license")
    ("warranty,w", "show warranty")
	("agent,A", po::value< std::vector<std::string> >(),   "agent(s) to train")
	("bench-agent,B", po::value< std::string >()->default_value("Pubeval"),   "benchmark agent")
	("train-games,T", po::value<int>()->default_value(10000),   "number of games for training")
	("bench-games,G", po::value<int>()->default_value(1000),   "number of games for benchmark")
	("bench-period,P", po::value<int>()->default_value(10000),   "benchmark every n games")
	;
}

void BgDispatcher::banner()
{
	printf("Multigammon - the backgammon agents evaluation and training tool.\n");
	printf("(c) 2011 Alex Koshterek.\n");
	printf("The most code is licensed with GNU GPLv3 (many things were taken from GnuBg).\n");
	printf("Some parts were covered by zlib and LGPL but GPL eats them (and I hate this).\n\n");

    printf("This program comes with ABSOLUTELY NO WARRANTY; for details use `--warranty'.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions; use `--copying' switch for details.\n\n");
}

void BgDispatcher::showTextArray(char *textArr[]) 
{
	for(int line = 0; textArr[line] != NULL; line++)
	{
		printf("%s\n", textArr[line]);
	}
}

bool BgDispatcher::init(int argc, char **argv)
{
	BgBoard b;
	b.InitBoard(VARIATION_STANDARD);
	b.PositionID();


	m_argc = argc;
	m_argv = argv;
	srand((unsigned)time(NULL));
	std::ios_base::sync_with_stdio();

	banner();
	po::store(po::parse_command_line(argc, argv, m_desc), m_vm);
	po::notify(m_vm);  

	if (m_vm.count("help")) 
	{
		std::cout << m_desc << "\n";
		return false;
	}
	else
	if(m_vm.count("copying"))
	{
		showTextArray(aszCopying);
		return false;
	}
	else
	if(m_vm.count("warranty"))
	{
		showTextArray(aszWarranty);
		return false;
	}

	BgEval::Instance()->load(m_argv[0]);
	BgEval::Instance()->getRng().seed((unsigned __int32)16000000);
	return true;
}

void BgDispatcher::run()
{
	std::vector<std::string> agentsList;
	try
	{
		agentsList = m_vm["agent"].as< std::vector<std::string> >();
	}
	catch(...)
	{
		fprintf(stderr, "No agents to train\n");
		return;
	}
	std::string benchAgenName = m_vm["bench-agent"].as< std::string >();
	time_t start = time(NULL);

	int numAgents = (int)agentsList.size();
#pragma omp parallel for
	for(int i = 0; i < numAgents; i++)
	{
		runAgentIteration(agentsList[i].c_str(), benchAgenName.c_str());
	}

	time_t end = time(NULL);
	time_t total = end - start;

	printf("Elapsed time %02ld:%02ld:%02ld\n", total/3600, (total/60) % 60, total % 60);
}

void BgDispatcher::runAgentIteration(const char *agentName, const char *benchAgentName)
{
	std::auto_ptr<BgAgent> agent1(BgAgentFactory::createAgent(agentName));
	std::auto_ptr<BgAgent> benchAgent(BgAgentFactory::createAgent(benchAgentName));
	assert(agent1.get() && benchAgent.get());
	std::auto_ptr<BgAgent> agent2;
	if(agent1->isCloneable())
		agent2.reset(agent1->clone());
	else
		agent2.reset(BgAgentFactory::createAgent(agentName));

	int trainGames = m_vm["train-games"].as<int>();
	int benchmarkGames = m_vm["bench-games"].as<int>();
	int benchmarkPeriod = m_vm["bench-period"].as<int>();

	runIteration(agent1.get(), benchAgent.get(), agent2.get(), trainGames, benchmarkGames, benchmarkPeriod);
}

void BgDispatcher::runIteration(BgAgent *agent1, BgAgent *benchAgent, BgAgent *agent2,	
		int trainGames, int benchmarkGames, int benchmarkPeriod)
{
	BgGameDispatcher gameDispatcher(agent1, agent2);
	gameDispatcher.setShowLog(false);
	
	if(trainGames > 0)
	{
		for(int game = 0; game < trainGames; game += benchmarkPeriod)
		{
			//training
			gameDispatcher.playGames(benchmarkPeriod, true);
			agent1->save();

			//benchmark
			BgGameDispatcher *benchDispatcher = new BgGameDispatcher(agent1, benchAgent);
			benchDispatcher->setShowLog(false);
			int half = benchmarkGames / 2;
			benchDispatcher->playGames(half, false);
			benchDispatcher->swapAgents();
			half = benchmarkGames - half;
			benchDispatcher->playGames(half, false);
			benchDispatcher->swapAgents();
			benchDispatcher->printStatistics();
			delete benchDispatcher;
		}
	}
	else
	{
		//benchmark
		BgGameDispatcher *benchDispatcher = new BgGameDispatcher(agent1, benchAgent);
		benchDispatcher->setShowLog(false);
		int half = benchmarkGames / 2;
		benchDispatcher->playGames(half, false);
		benchDispatcher->swapAgents();
		half = benchmarkGames - half;
		benchDispatcher->playGames(half, false);
		benchDispatcher->swapAgents();
		benchDispatcher->printStatistics();
		delete benchDispatcher;
	}
}
