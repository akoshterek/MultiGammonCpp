#include <assert.h>
#include "BgMove.h"
#include "BgBoard.h"
#include "PositionId.h"

int bgmove::CompareMovesGeneral(const bgmove *pm0, const bgmove *pm1)
{
	if(pm0 == pm1) return 0;
	//unsigned int back[2] = { -1, -1 };

	if (pm0->rScore != pm1->rScore)
		return (bgmove::CompareMoves(pm0, pm1) > 0);
	
	/*
	// find the "back" chequer 
 	board[0] = BgBoard::PositionFromKey(pm0->auch);
	board[1] = BgBoard::PositionFromKey(pm1->auch);
	for (int a = 0; a < 2; a++) 
	{
		for (int b = 24; b > -1; b--) 
		{
			if (board[a].anBoard[1][b] > 0) 
			{
				back[a] = b;
				break;
			}
		}
	}
	// "back" chequer at high point bad 
	return (back[0] < back[1] ? 1 : -1);
	*/
	return (pm0->backChequer < pm1->backChequer ? 1 : -1);
}

void movelist::CopyMoveList ( const movelist *pmlSrc )
{
	if ( this == pmlSrc )
		return;

	if ( pmlSrc->cMoves ) 
	{
		if(pmlSrc->cMoves > cMoves)
		{
			deleteMoves();
			amMoves = new bgmove[pmlSrc->cMoves];
		}
		for(size_t i = 0; i < pmlSrc->cMoves; i++)
			amMoves[i] = pmlSrc->amMoves[i];
	}
	else
		deleteMoves();

	cMoves = pmlSrc->cMoves;
	cMaxMoves = pmlSrc->cMaxMoves;
	cMaxPips = pmlSrc->cMaxPips;
	iMoveBest = pmlSrc->iMoveBest;
	rBestScore = pmlSrc->rBestScore;
}

moverecord::moverecord()
{
	//memset( this, 0, sizeof(*this));

	mt = MOVE_INVALID;
	//sz = NULL;
	fPlayer = 0;
	anDice[ 0 ] = anDice[ 1 ] = 0;
	//lt = LUCK_NONE;
	//rLuck = (float)ERR_VAL;
	esChequer.et = EVAL_NONE;
	nAnimals = 0;
	//CubeDecPtr = &CubeDec;
	//CubeDecPtr->esDouble.et = EVAL_NONE;
	//CubeDecPtr->cmark = CMARK_NONE;
	//stCube = SKILL_NONE;
	//ml.cMoves = 0;
	//ml.amMoves = NULL;

	/* movenormal */
	//n.stMove = SKILL_NONE;
	n.anMove[ 0 ] = n.anMove[ 1 ] = -1;
	n.iMove = UINT_MAX;

	/* moveresign */
	//r.esResign.et = EVAL_NONE;
}
