#if !defined __PUEVAL_REPRESENTATION_H
#define __PUEVAL_REPRESENTATION_H

#pragma once
#include "Inputrepresentation.h"

class PubevalRepresentation : public InputRepresentation
{
public:
	PubevalRepresentation(void) : InputRepresentation(124, 124, 124) {}
	virtual ~PubevalRepresentation(void) {}

	virtual void init() {}

	virtual void calculateRaceInputs(const BgBoard *anBoard, float inputs[]) const
	{
		calculateContactInputs(anBoard, inputs);
	};
	virtual void calculateCrashedInputs(const BgBoard *anBoard, float inputs[]) const 
	{
		calculateContactInputs(anBoard, inputs);
	};
	virtual void calculateContactInputs(const BgBoard *anBoard, float arInput[]) const;

private:
	void preparePos(const BgBoard *board, char pos[28]) const;
};

#endif
