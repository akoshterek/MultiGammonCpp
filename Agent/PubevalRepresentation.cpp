#include "PubevalRepresentation.h"

void PubevalRepresentation::calculateContactInputs(const BgBoard *anBoard, float arInput[]) const
{
	char pos[28];
	preparePos(anBoard, pos);

	/* sets input vector x[] given board position pos[] */
	int jm1, n;
	/* initialize */
	for(int j = 0; j < 122; ++j) 
		arInput[j] = 0;

	/* first encode board locations 24-1 */
	for(int j=1; j <= 24; ++j) 
	{
	    jm1 = j - 1;
	    n = pos[25-j];
	    if(n != 0) 
		{
			if(n==-1) arInput[5*jm1+0] = 1.0f;
			if(n==1)  arInput[5*jm1+1] = 1.0f;
			if(n>=2)  arInput[5*jm1+2] = 1.0f;
			if(n==3)  arInput[5*jm1+3] = 1.0f;
			if(n>=4)  arInput[5*jm1+4] = (float)(n-3)/2.0f;
	    }
	}
	/* encode opponent barmen */
	arInput[120] = -(float)(pos[0])/2.0f;
	/* encode computer's menoff */
	arInput[121] = (float)(pos[26])/15.0f;

	//padding
	arInput[122] = arInput[123] = 0;
}

void PubevalRepresentation::preparePos(const BgBoard *board, char pos[28]) const
{
	unsigned int men[2];
	board->ChequersCount(men);

	for(int i = 0; i < 24; i++)
	{
		pos[i+1] = board->anBoard[BgBoard::SELF][i];
		if(board->anBoard[BgBoard::OPPONENT][23 - i])
			pos[i+1] = -board->anBoard[BgBoard::OPPONENT][23 - i];
	}

	pos[25] = board->anBoard[BgBoard::SELF][BgBoard::BAR];
	pos[0] = -board->anBoard[BgBoard::OPPONENT][BgBoard::BAR];
	pos[26] = 15 - men[BgBoard::SELF];
	pos[27] = -char(15 - men[BgBoard::OPPONENT]);
}
