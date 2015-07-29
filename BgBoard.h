#ifndef __BGBOARD_H
#define __BGBOARD_H

#pragma once

#include <errno.h>
#include "BgCommon.h"
#include "BgMove.h"
#include "BgReward.h"

class BgBoard
{
public:
	static const int OPPONENT = 0;
	static const int SELF = 1;

	static const int BAR = 24;
	static const int TOTAL_MEN = 15;

	BgBoard(void);
	BgBoard(const BgBoard& src);
	
	//utility
	void InitBoard(const bgvariation bgv);
	void SwapSides();
	int GameStatus(const bgvariation bgv ) const;
	void PipCount(unsigned int anPips[ 2 ] ) const;
	void ChequersCount(unsigned int anChequers[ 2 ]) const;
	int  BackChequer() const;

	//position key stuff
	AuchKey PositionKey() const;
	char *PositionID() const;
	char *PositionIDFromKey(const AuchKey auchKey) const;
	static BgBoard PositionFromKey(const AuchKey auchKey);
	static bool PositionFromID(BgBoard& anBoard, const char* pchEnc);
	bool CheckPosition() const;
	char *DrawBoard( char *pch, int fRoll, char *asz[7], char *szMatchID, int nChequers ) const;
	char *FIBSBoardShort( char *pch, int nChequers ) const;

	//moves
	int GenerateMoves(movelist *pml, bgmove* amMoves, int n0, int n1, bool fPartial) const;
	unsigned int locateMove (const ChequerMove& anMove, const movelist *pml) const;
	char *FormatMove( char *sz, const ChequerMove& anMove) const;
	char *FormatMovePlain( char *sz, const ChequerMove& anMove) const;
	int ApplyMove(const ChequerMove& anMove, const bool fCheckLegal);

	char anBoard[2][25];
private:
	bool m_fClockwise;

	void clearBoard();
	static inline void addBits(AuchKey& auchKey, unsigned int bitPos, unsigned int nBits);

	//Moves
	bool LegalMove(int iSrc, int nPips) const;
	int ApplySubMove(const int iSrc, const int nRoll, const bool fCheckLegal);
	AuchKey MoveKey (const ChequerMove& anMove) const;
	void SaveMoves(movelist *pml, unsigned int cMoves, unsigned int cPip, const ChequerMove& anMoves, bool fPartial);
	bool GenerateMovesSub( movelist *pml, int anRoll[], int nMoveDepth,
		int iPip, int cPip, ChequerMove& anMoves, bool fPartial ) const;

	//draw board
	char *DrawBoardStd( char *sz, int fRoll, char *asz[7], char *szMatchID, int nChequers ) const;
	char *DrawBoardCls( char *sz, int fRoll, char *asz[7], char *szMatchID, int nChequers ) const;
	static int CompareMoves( const void *p0, const void *p1 );
	static char *FormatPoint( char *pch, int n );
	static char *FormatPointPlain( char *pch, int n );
};

#endif
