#if !defined __INPUT_REPRESENTATION_H
#define __INPUT_REPRESENTATION_H

#pragma once
#include "BgBoard.h"

class InputRepresentation
{
public:
	InputRepresentation(int raceInputs, int crashedInputs, int contactInputs)
		: m_raceInputs(raceInputs), m_crashedInputs(crashedInputs), m_contactInputs(contactInputs)
	{}

	int getRaceInputs() const {return m_raceInputs;}
	int getCrashedInputs() const {return m_crashedInputs;}
	int getContactInputs() const {return m_contactInputs;}

	virtual void calculateRaceInputs(const BgBoard *anBoard, float inputs[]) const = 0;
	virtual void calculateCrashedInputs(const BgBoard *anBoard, float inputs[]) const = 0;
	virtual void calculateContactInputs(const BgBoard *anBoard, float arInput[]) const = 0;

private:
	int m_raceInputs, m_crashedInputs, m_contactInputs;
};

#endif
