#ifndef __BGGAME_H
#define __BGGAME_H

#pragma once

#include "BgBoard.h"

enum gamestate { GAME_NONE, GAME_PLAYING, GAME_OVER, GAME_RESIGNED, GAME_DROP };

class matchstate 
{
public:
	BgBoard anBoard;
	unsigned int anDice[2];	/* (0,0) for unrolled dice */
	int fTurn;		/* who makes the next decision */
	int fResigned;
	int fResignationDeclined;
	int fDoubled;
	int cGames;
	int fMove;		/* player on roll */
	int fCubeOwner;
	int fCrawford;
	int fPostCrawford;
	int nMatchTo;
	int anScore[2];
	int nCube;
	unsigned int cBeavers;
	bgvariation bgv;
	int fCubeUse;
	int fJacoby;
	gamestate gs;

	matchstate();
	
	const BgBoard& getBoard() const {return anBoard;}
	void rollDice()
	{
		anDice[0] = BgEval::Instance()->nextDice();
		anDice[1] = BgEval::Instance()->nextDice();
	}

	void GetMatchStateCubeInfo(cubeinfo* pci) const;
	int SetCubeInfo ( cubeinfo *pci, const int nCube, const int fCubeOwner, 
		const int fMove, const int nMatchTo, const int anScore[ 2 ], 
        const int fCrawford, const int fJacoby, const int fBeavers, 
        const bgvariation bgv ) const;

private:
	int SetCubeInfoMoney( cubeinfo *pci, const int nCube, const int fCubeOwner,
		const int fMove, const int fJacoby, const int fBeavers,
        const bgvariation bgv ) const;
	//int SetCubeInfoMatch( cubeinfo *pci, const int nCube, const int fCubeOwner,
    //    const int fMove, const int nMatchTo, const int anScore[ 2 ],
    //    const int fCrawford, const bgvariation bgv ) const

};

#endif
