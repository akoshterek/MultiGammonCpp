#include "RawRepresentation.h"

void RawRepresentation::calculateContactInputs(const BgBoard *anBoard, float arInput[]) const
{
	calculateHalfBoard(anBoard->anBoard[0], arInput);
	calculateHalfBoard(anBoard->anBoard[1], arInput + 100);
	
	//padding
	//arInput[200] = arInput[201] = arInput[202] = arInput[203] = 0;
}

void RawRepresentation::calculateHalfBoard(const char *halfBoard, float *halfInputs) const
{
	switch(m_encoding)
	{
	case encSutton:
		for(int i = 0; i < 24; i++)
			setPointSutton(halfBoard[i], halfInputs + 4*i);
		break;
	case encTes89:
		for(int i = 0; i < 24; i++)
			setPointTes89(halfBoard[i], halfInputs + 4*i);
		break;
	case encTes92:
		for(int i = 0; i < 24; i++)
			setPointTes92(halfBoard[i], halfInputs + 4*i);
		break;
	case encGnu:
		for(int i = 0; i < 24; i++)
			setPointGnu(halfBoard[i], halfInputs + 4*i);
		break;
	default:
		throw std::exception("Unknown board encoding");
	}

	halfInputs[96 + 0] = halfBoard[24] * 0.5f;
	int home = BgBoard::TOTAL_MEN;
	for(int i = 0; i < 25; i++)
		home -= halfBoard[i];
	halfInputs[96 + 1] = home / 15.0f;
}

void RawRepresentation::setPointSutton(const char men, float *inputs) const
{
    inputs[0] = men >= 1 ? 1.0f : 0.0f;
    inputs[1] = men >= 2 ? 1.0f : 0.0f;
    inputs[2] = men >= 3 ? 1.0f : 0.0f;
    inputs[3] = men >= 4 ? (men - 3) / 2.0f : 0.0f;
}

void RawRepresentation::setPointTes89(const char men, float *inputs) const
{
    inputs[0] = men == 1 ? 1.0f : 0.0f;
    inputs[1] = men == 2 ? 1.0f : 0.0f;
    inputs[2] = men == 3 ? 1.0f : 0.0f;
    inputs[3] = men >= 4 ? (men - 3) / 2.0f : 0.0f;
}

void RawRepresentation::setPointTes92(const char men, float *inputs) const
{
    inputs[0] = men == 1 ? 1.0f : 0.0f;
    inputs[1] = men >= 2 ? 1.0f : 0.0f;
    inputs[2] = men == 3 ? 1.0f : 0.0f;
    inputs[3] = men >= 4 ? (men - 3) / 2.0f : 0.0f;
}

void RawRepresentation::setPointGnu(const char men, float *inputs) const
{
    inputs[0] = men == 1 ? 1.0f : 0.0f;
    inputs[1] = men == 2 ? 1.0f : 0.0f;
    inputs[2] = men >= 3 ? 1.0f : 0.0f;
    inputs[3] = men >= 4 ? (men - 3) / 2.0f : 0.0f;
}
