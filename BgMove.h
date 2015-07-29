#ifndef __BGMOVE_H
#define __BGMOVE_H

#pragma once

#include <vector>
#include "BgCommon.h"
#include "BgEval.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

class AuchKey : public std::vector<unsigned char>
{
public:
	AuchKey() 
	{
		resize(10, 0);
	}
};

enum movetype {
	MOVE_INVALID = -1,
	MOVE_GAMEINFO = 0,
	MOVE_NORMAL,
	MOVE_DOUBLE,
	MOVE_TAKE,
	MOVE_DROP,
	MOVE_RESIGN,
	MOVE_SETBOARD,
	MOVE_SETDICE,
	MOVE_SETCUBEVAL,
	MOVE_SETCUBEPOS
};

struct xmoveresign 
{
	int nResigned;
	//evalsetup esResign;
	//float arResign[NUM_ROLLOUT_OUTPUTS];
	//skilltype stResign;
	//skilltype stAccept;
};

struct xmovegameinfo 
{
	/* ordinal number of the game within a match */
	int i;
	/* match length */
	int nMatch;
	/* match score BEFORE the game */
	int anScore[2];
	/* the Crawford rule applies during this match */
	int fCrawford;
	/* this is the Crawford game */
	int fCrawfordGame;
	int fJacoby;
	/* who won (-1 = unfinished) */
	int fWinner;
	/* how many points were scored by the winner */
	int nPoints;
	/* the game was ended by resignation */
	int fResigned;
	/* how many automatic doubles were rolled */
	int nAutoDoubles;
	/* Type of game */
	bgvariation bgv;
	/* Cube used in game */
	int fCubeUse;
};

class ChequerMove : public std::vector<int>
{
public:
	ChequerMove() 
	{
		resize(8, 0);
	}
};

struct xmovenormal 
{
	/* Move made. */
	ChequerMove anMove;
	/* index into the movelist of the move that was made */
	unsigned int iMove;
};

struct xmovesetboard 
{
	AuchKey auchKey;	/* always stored as if player 0 was on roll */
};

struct xmovesetcubeval 
{
	int nCube;
};

struct xmovesetcubepos 
{
	int fCubeOwner;
};

class bgmove
{
public:
	ChequerMove anMove;
	AuchKey auch;
	unsigned int cMoves, cPips;
	/* scores for this move */
	float rScore; 
	/* evaluation for this move */
	BgReward arEvalMove;
	positionclass pc;
	int backChequer;

	bool operator < (const bgmove& m) const
	{
		if(&m == this) return false;

		if (rScore != m.rScore)
			return (bgmove::CompareMoves(this, &m) < 0);
	
		return (backChequer < m.backChequer);
	}
	bool operator >(const bgmove& m) const
	{
		if(&m == this) return false;

		if (rScore != m.rScore)
			return (bgmove::CompareMoves(this, &m) > 0);
	
		return (backChequer > m.backChequer);
	}

	static int CompareMoves(const bgmove *pm0, const bgmove *pm1)
	{
		/*high score first */
		return (pm1->rScore > pm0->rScore) ? 1 : -1;
	}

	static int CompareMovesGeneral(const bgmove *pm0, const bgmove *pm1);
};


struct movelist
{
	/* A trivial upper bound on the number of (complete or incomplete)
	 * legal moves of a single roll: if all 15 chequers are spread out,
	 * then there are 18 C 4 + 17 C 3 + 16 C 2 + 15 C 1 = 3875
	 * combinations in which a roll of 11 could be played (up to 4 choices from
	 * 15 chequers, and a chequer may be chosen more than once).  The true
	 * bound will be lower than this (because there are only 26 points,
	 * some plays of 15 chequers must "overlap" and map to the same
	 * resulting position), but that would be more difficult to
	 * compute. */
	static const int MAX_INCOMPLETE_MOVES = 3875;
	static const int MAX_MOVES = 3060;

	movelist()
	{
		amMoves = NULL;
		cMoves = 0;
	}

	void CopyMoveList ( const movelist *pmlSrc );
	void deleteMoves()
	{
		if(amMoves)
		{
			delete [] amMoves;
			amMoves = NULL;
		}
		cMoves = 0;
	}

    unsigned int cMoves; /* and current move when building list */
    unsigned int cMaxMoves, cMaxPips;
    int iMoveBest;
    float rBestScore;
	bgmove *amMoves;
};

struct moverecord 
{
	/* 
	 * Common variables
	 */
	/* type of the move */
	movetype mt;
	/* annotation */
	std::string sz;
	/* move record is for player */
	int fPlayer;
	/* luck analysis (shared between MOVE_SETDICE and MOVE_NORMAL) */
	/* dice rolled */
	unsigned int anDice[2];
	/* move analysis (shared between MOVE_SETDICE and MOVE_NORMAL) */
	/* evaluation setup for move analysis */
	evalsetup esChequer;
	/* evaluation of the moves */
	movelist ml;
	/* cube analysis (shared between MOVE_NORMAL and MOVE_DOUBLE) */
	/* 0 in match play, even numbers are doubles, raccoons 
	   odd numbers are beavers, aardvarken, etc. */
	int nAnimals;
	/* "private" data */
	xmovegameinfo g;	/* game information */
	xmovenormal n;		/* chequerplay move */
	xmoveresign r;		/* resignation */
	xmovesetboard sb;	/* setting up board */
	xmovesetcubeval scv;	/* setting cube */
	xmovesetcubepos scp;	/* setting cube owner */

	moverecord();

	static void destroy(moverecord *& pmr)
	{
		if (!pmr)
			return;
		delete pmr;
		pmr = NULL;
	}
};

struct findData
{
	movelist *pml;
	const BgBoard *pboard;
	unsigned char *auchMove;
	float rThr;
	const cubeinfo* pci;
};

#endif
