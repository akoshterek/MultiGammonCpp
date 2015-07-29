#if !defined __BGAGENTFACTORY_H
#define __BGAGENTFACTORY_H
#pragma once

#include "BgAgent.h"

class BgAgentFactory
{
public:
	static BgAgent *createAgent(std::string fullName);
};

#endif
