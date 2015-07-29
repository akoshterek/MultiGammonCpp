#if !defined __RAW_REPRESENTATION_H
#define __RAW_REPRESENTATION_H

#pragma once
#include "Inputrepresentation.h"

enum BoardEncoding {encSutton, encTes89, encTes92, encGnu};

class RawRepresentation : public InputRepresentation
{
public:
	RawRepresentation(BoardEncoding encoding) 
		: InputRepresentation(200, 200, 200), m_encoding(encoding) {}
	virtual ~RawRepresentation(void) {}

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
	void calculateHalfBoard(const char *halfBoard, float *halfInputs) const;
	BoardEncoding m_encoding;

	void setPointSutton(const char men, float *inputs) const;
	void setPointTes89(const char men, float *inputs) const;
	void setPointTes92(const char men, float *inputs) const;
	void setPointGnu(const char men, float *inputs) const;
};

#endif
