#include <memory.h>
#include <assert.h>
#include <algorithm>

#include "BgBoard.h"
#include "PositionId.h"
#include "BgReward.h"

BgBoard::BgBoard(void)
{
	m_fClockwise = false;
	clearBoard();
}

void BgBoard::clearBoard()
{
	memset(anBoard, 0, sizeof(anBoard));
}

BgBoard::BgBoard(const BgBoard& src)
{
	m_fClockwise = src.m_fClockwise;
	memcpy(anBoard, src.anBoard, sizeof(anBoard));
}

void BgBoard::InitBoard(const bgvariation bgv)
{
	unsigned int i, j;

	for( i = 0; i < 25; i++ )
		anBoard[ 0 ][ i ] = anBoard[ 1 ][ i ] = 0;

	switch( bgv ) 
	{
	case VARIATION_STANDARD:
	case VARIATION_NACKGAMMON:
		anBoard[ 0 ][ 5 ] = anBoard[ 1 ][ 5 ] =
		anBoard[ 0 ][ 12 ] = anBoard[ 1 ][ 12 ] = 
			( bgv == VARIATION_NACKGAMMON ) ? 4 : 5;
		anBoard[ 0 ][ 7 ] = anBoard[ 1 ][ 7 ] = 3;
		anBoard[ 0 ][ 23 ] = anBoard[ 1 ][ 23 ] = 2;

		if( bgv == VARIATION_NACKGAMMON )
			anBoard[ 0 ][ 22 ] = anBoard[ 1 ][ 22 ] = 2;
		break;

	case VARIATION_HYPERGAMMON_1:
	case VARIATION_HYPERGAMMON_2:
	case VARIATION_HYPERGAMMON_3:
		for ( i = 0; i < 2; ++i )
			for ( j = 0; j < (unsigned int)( bgv - VARIATION_HYPERGAMMON_1 + 1 ); ++j )
				anBoard[ i ][ 23 - j ] = 1;
		break;
    
	default:
		assert ( false );
		break;
	}
}

// See https://savannah.gnu.org/bugs/index.php?28421 regarding the code below
// Portable code 
AuchKey BgBoard::PositionKey() const
{
	unsigned int i, iBit = 0;
	AuchKey auchKey;
	memset((void *)&auchKey[0], 0, auchKey.size() * sizeof(AuchKey::value_type));

	for(i = 0; i < 2; ++i) 
	{
		const char* b = anBoard[i];
		for(const char* j = b; j < b + 25; ++j)
		{
			const unsigned int nc = *j;

			if( nc ) 
			{
				addBits(auchKey, iBit, nc);
				iBit += nc + 1;
			} 
			else 
			{
				++iBit;
			}
		}
	}

	return auchKey;
}

BgBoard BgBoard::PositionFromKey(const AuchKey auch)
{
	int i = 0, j = 0;
	BgBoard newBoard;

	for(size_t a = 0; a < auch.size(); a++) 
	{
		unsigned char cur = auch[a];
    
		for(int k = 0; k < 8; ++k)
		{
			if( (cur & 0x1) )
			{
				if (i >= 2 || j >= 25)
				{	// Error, so return - will probably show error message
					return newBoard;
				}
				++newBoard.anBoard[i][j];
			}
			else
			{
				if( ++j == 25 )
				{
					++i;
					j = 0;
				}
			}
			cur >>= 1;
		}
	}

	return newBoard;
}

bool BgBoard::PositionFromID(BgBoard& anBoard, const char* pchEnc)
{
	AuchKey auchKey;
	unsigned char ach[ PositionId::L_POSITIONID +1 ], *pch = ach, *puch = &auchKey[0];
	int i;

	memset ( ach, 0, PositionId::L_POSITIONID +1 );

	for( i = 0; i < PositionId::L_POSITIONID && pchEnc[ i ]; i++ )
		pch[ i ] = PositionId::Base64( (unsigned char)pchEnc[ i ] );

	for( i = 0; i < 3; i++ ) 
	{
		*puch++ = (unsigned char)( pch[ 0 ] << 2 ) | ( pch[ 1 ] >> 4 );
		*puch++ = (unsigned char)( pch[ 1 ] << 4 ) | ( pch[ 2 ] >> 2 );
		*puch++ = (unsigned char)( pch[ 2 ] << 6 ) | pch[ 3 ];

		pch += 4;
	}

	*puch = (unsigned char)( pch[ 0 ] << 2 ) | ( pch[ 1 ] >> 4 );
	anBoard = PositionFromKey( auchKey );
	return anBoard.CheckPosition();
}

bool BgBoard::CheckPosition() const
{
    unsigned int ac[ 2 ], i;

    /* Check for a player with over 15 chequers */
    for( i = ac[ 0 ] = ac[ 1 ] = 0; i < 25; i++ )
        if( ( ac[ 0 ] += anBoard[ 0 ][ i ] ) > 15 ||
            ( ac[ 1 ] += anBoard[ 1 ][ i ] ) > 15 ) {
            errno = EINVAL;
            return false;
        }

    /* Check for both players having chequers on the same point */
    for( i = 0; i < 24; i++ )
        if( anBoard[ 0 ][ i ] && anBoard[ 1 ][ 23 - i ] ) {
            errno = EINVAL;
            return false;
        }

    /* Check for both players on the bar against closed boards */
    for( i = 0; i < 6; i++ )
        if( anBoard[ 0 ][ i ] < 2 || anBoard[ 1 ][ i ] < 2 )
            return true;

    if( !anBoard[ 0 ][ 24 ] || !anBoard[ 1 ][ 24 ] )
        return true;
    
    errno = EINVAL;
    return false;
}

inline void BgBoard::addBits(AuchKey& auchKey, unsigned int bitPos, unsigned int nBits)
{
	unsigned int k = bitPos >> 3;
	unsigned int r = (bitPos & 0x7);
	unsigned int b = (((unsigned int)0x1 << nBits) - 1) << r;

	auchKey[k] |= (unsigned char)b;

	if( k < 8 )
	{
		auchKey[k+1] |= (unsigned char)(b >> 8);
		auchKey[k+2] |= (unsigned char)(b >> 16);
	} 
	else 
	if( k == 8 ) 
	{
		auchKey[k+1] |= (unsigned char)(b >> 8);
	}
}


bool BgBoard::LegalMove(int iSrc, int nPips ) const
{
    int i, nBack = 0, iDest = iSrc - nPips;
	
	if (iDest >= 0)
	{ 
		// Here we can do the Chris rule check
		return ( anBoard[ OPPONENT ][ 23 - iDest ] < 2 );
    }
    
	// otherwise, attempting to bear off 
    for( i = 1; i < 25; i++ )
		if( anBoard[ SELF ][ i ] > 0 )
		    nBack = i;

    return ( nBack <= 5 && ( iSrc == nBack || iDest == -1 ) );
}

int BgBoard::ApplySubMove(const int iSrc, const int nRoll, const bool fCheckLegal) 
{
    int iDest = iSrc - nRoll;

    if( fCheckLegal && ( nRoll < 1 || nRoll > 6 ) ) 
	{
		// Invalid dice roll
		errno = EINVAL;
		return -1;
    }
    
    if( iSrc < 0 || iSrc > 24 || iDest > 24 || anBoard[ 1 ][ iSrc ] < 1 ) 
	{
		// Invalid point number, or source point is empty
		errno = EINVAL;
		return -1;
    }
    
    anBoard[ SELF ][ iSrc ]--;

    if( iDest < 0 )
		return 0;
    
    if( anBoard[ 0 ][ 23 - iDest ] ) 
	{
		if( anBoard[ 0 ][ 23 - iDest ] > 1 ) 
		{
			// Trying to move to a point already made by the opponent 
			errno = EINVAL;
			return -1;
		}

		//blot hit
		anBoard[ SELF ][ iDest ] = 1;
		anBoard[ OPPONENT ][ 23 - iDest ] = 0;
		//send to bar
		anBoard[ OPPONENT ][ BAR ]++;
    } 
	else
		anBoard[ SELF ][ iDest ]++;
	
    return 0;
}

int BgBoard::ApplyMove(const ChequerMove& anMove, const bool fCheckLegal) 
{
    for(int i = 0; i < 8 && anMove[ i ] >= 0; i += 2 )
		if( ApplySubMove(anMove[ i ], anMove[ i ] - anMove[ i + 1 ], fCheckLegal ) )
			return -1;
    
    return 0;
}

void BgBoard::SaveMoves(movelist *pml, unsigned int cMoves, unsigned int cPip, const ChequerMove& anMoves, bool fPartial) 
{
    unsigned int i, j;
    bgmove *pm;

	if( fPartial ) 
	{
		//Save all moves, even incomplete ones *
		if( cMoves > pml->cMaxMoves )
			pml->cMaxMoves = cMoves;
	
		if( cPip > pml->cMaxPips )
			pml->cMaxPips = cPip;
    } 
	else 
	{
		//Save only legal moves: if the current move moves plays less
	    //chequers or pips than those already found, it is illegal; if
	    //it plays more, the old moves are illegal. 
		if( cMoves < pml->cMaxMoves || cPip < pml->cMaxPips )
			return;

		if( cMoves > pml->cMaxMoves || cPip > pml->cMaxPips )
			pml->cMoves = 0;
	
		pml->cMaxMoves = cMoves;
		pml->cMaxPips = cPip;
    }
    
    pm = &pml->amMoves[pml->cMoves];
    
    AuchKey auch = PositionKey();
    
    for( i = 0; i < pml->cMoves; i++ )
	{
		if( PositionId::EqualKeys( &auch[0], &pml->amMoves[ i ].auch[0] ) )
		{
			if( cMoves > pml->amMoves[ i ].cMoves ||
				cPip > pml->amMoves[ i ].cPips )
			{
				for( j = 0; j < cMoves * 2; j++ )
					pml->amMoves[ i ].anMove[ j ] = anMoves[ j ] > -1 ? anMoves[ j ] : -1;
			
				if( cMoves < 4 )
					pml->amMoves[ i ].anMove[ cMoves * 2 ] = -1;

				pml->amMoves[ i ].cMoves = cMoves;
				pml->amMoves[ i ].cPips = cPip;
			}
		    
			return;
		}
	}
    
    for( i = 0; i < cMoves * 2; i++ )
		pm->anMove[ i ] = anMoves[ i ] > -1 ? anMoves[ i ] : -1;
    
    if( cMoves < 4 )
		pm->anMove[ cMoves * 2 ] = -1;
    
	pm->auch = auch;

    pm->cMoves = cMoves;
    pm->cPips = cPip;
	pm->backChequer = BackChequer();
    //pm->cmark = CMARK_NONE;

	pm->arEvalMove.reset();
    pml->cMoves++;
    assert( pml->cMoves < movelist::MAX_INCOMPLETE_MOVES );
}

int BgBoard::BackChequer() const
{
	int back = -1;
	for (int b = 24; b > -1; b--) 
	{
		if (anBoard[SELF][b] > 0) 
		{
			back = b;
			break;
		}
	}

	return back;
}

bool BgBoard::GenerateMovesSub( movelist *pml, int anRoll[], int nMoveDepth,
			     int iPip, int cPip, ChequerMove& anMoves, bool fPartial ) const
{
    bool fUsed = false;

    if( nMoveDepth > 3 || !anRoll[ nMoveDepth ] )
		return true;

    if( anBoard[ SELF ][ BAR ] ) // on bar
	{ 
		if( anBoard[ OPPONENT ][ anRoll[ nMoveDepth ] - 1 ] >= 2 )
			return true;

		anMoves[ nMoveDepth * 2 ] = 24;
		anMoves[ nMoveDepth * 2 + 1 ] = 24 - anRoll[ nMoveDepth ];

	    BgBoard anBoardNew(*this);
		anBoardNew.ApplySubMove(24, anRoll[ nMoveDepth ], true);
	
		if(anBoardNew.GenerateMovesSub( pml, anRoll, nMoveDepth + 1, 23, cPip +
			      anRoll[ nMoveDepth ], anMoves, fPartial ) )
		{
			anBoardNew.SaveMoves( pml, nMoveDepth + 1, cPip + anRoll[ nMoveDepth ],
				   anMoves, fPartial );
		}

		return fPartial;
    } 
	else 
	{
		for(int i = iPip; i >= 0; i-- )
		{
			if( anBoard[ SELF ][ i ] && LegalMove(i, anRoll[ nMoveDepth ] ) ) 
			{
				anMoves[ nMoveDepth * 2 ] = i;
				anMoves[ nMoveDepth * 2 + 1 ] = i -	anRoll[ nMoveDepth ];

			    BgBoard anBoardNew(*this);
				anBoardNew.ApplySubMove(i, anRoll[ nMoveDepth ], true);
		
				if( anBoardNew.GenerateMovesSub( pml, anRoll, nMoveDepth + 1,
					   anRoll[ 0 ] == anRoll[ 1 ] ? i : 23,
					   cPip + anRoll[ nMoveDepth ],  anMoves, fPartial ) )
				{
					anBoardNew.SaveMoves( pml, nMoveDepth + 1, cPip +
					   anRoll[ nMoveDepth ], anMoves, fPartial );
				}
		
				fUsed = true;
			}
		}
    }

    return !fUsed || fPartial;
}

int BgBoard::GenerateMoves( movelist *pml, bgmove* amMoves, int n0, int n1, bool fPartial ) const
{
	int anRoll[ 4 ];
	ChequerMove anMoves;
	//static move amMoves[MAX_NUMTHREADS][ MAX_INCOMPLETE_MOVES ];

    anRoll[ 0 ] = n0;
    anRoll[ 1 ] = n1;
    anRoll[ 2 ] = anRoll[ 3 ] = ( ( n0 == n1 ) ? n0 : 0 );

    pml->cMoves = pml->cMaxMoves = pml->cMaxPips = pml->iMoveBest = 0;
	pml->amMoves = amMoves;
    GenerateMovesSub( pml, anRoll, 0, 23, 0, anMoves, fPartial );

    if( anRoll[ 0 ] != anRoll[ 1 ] ) 
	{
		std::swap( anRoll[0], anRoll[1] );
		GenerateMovesSub( pml, anRoll, 0, 23, 0, anMoves, fPartial );
    }

	return pml->cMoves;
}

void BgBoard::SwapSides() 
{
    for(int i = 0; i < 25; i++ ) 
	{
		int n = anBoard[ OPPONENT ][ i ];
		anBoard[ OPPONENT ][ i ] = anBoard[ SELF ][ i ];
		anBoard[ SELF ][ i ] = n;
    }
}

void BgBoard::PipCount(unsigned int anPips[ 2 ] ) const
{
    anPips[ OPPONENT ] = 0;
    anPips[ SELF ] = 0;
    
    for(int i = 0; i < 25; i++ ) 
	{
		anPips[ OPPONENT ] += anBoard[ OPPONENT ][ i ] * ( i + 1 );
		anPips[ SELF ] += anBoard[ SELF ][ i ] * ( i + 1 );
    }
}

void BgBoard::ChequersCount(unsigned int anChequers[ 2 ]) const
{
    anChequers[ OPPONENT ] = 0;
    anChequers[ SELF ] = 0;

    for(int i = 0; i < 25; i++ ) 
	{
		anChequers[ OPPONENT ] += anBoard[ OPPONENT ][ i ];
		anChequers[ SELF ] += anBoard[ SELF ][ i ];
    }
}

AuchKey BgBoard::MoveKey (const ChequerMove& anMove) const
{
	BgBoard anBoardMove(*this);
	anBoardMove.ApplyMove(anMove, false);
	AuchKey auch = anBoardMove.PositionKey();

	return auch;
}

unsigned int BgBoard::locateMove (const ChequerMove& anMove, const movelist *pml ) const
{
	AuchKey key = MoveKey (anMove);

	for ( unsigned int i = 0; i < pml->cMoves; ++i ) 
	{
		AuchKey auch = MoveKey ( pml->amMoves[ i ].anMove);
		if ( PositionId::EqualKeys (&auch[0], &key[0]) )
			return i;
	}

	return 0;
}

int BgBoard::GameStatus(const bgvariation bgv ) const
{
	BgReward ar;
    
	if( BgEval::Instance()->ClassifyPosition( this, bgv ) != CLASS_OVER )
		return 0;

	BgEval::Instance()->EvalOver( this, ar, bgv );

	if( ar[ OUTPUT_WINBACKGAMMON ] || ar[ OUTPUT_LOSEBACKGAMMON ] )
		return 3;

	if( ar[ OUTPUT_WINGAMMON ] || ar[ OUTPUT_LOSEGAMMON ] )
		return 2;

	return 1;
}

char *BgBoard::PositionID() const
{
    AuchKey auch = PositionKey();
    return PositionIDFromKey(auch);
}

char *BgBoard::PositionIDFromKey(const AuchKey auchKey) const
{
    const unsigned char *puch = &auchKey[0];
    static char szID[ PositionId::L_POSITIONID + 1 ];
    char *pch = szID;
    static char aszBase64[ 65 ] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i;
    
    for( i = 0; i < 3; i++ ) 
	{
        *pch++ = aszBase64[ puch[ 0 ] >> 2 ];
        *pch++ = aszBase64[ ( ( puch[ 0 ] & 0x03 ) << 4 ) |
                          ( puch[ 1 ] >> 4 ) ];
        *pch++ = aszBase64[ ( ( puch[ 1 ] & 0x0F ) << 2 ) |
                          ( puch[ 2 ] >> 6 ) ];
        *pch++ = aszBase64[ puch[ 2 ] & 0x3F ];

        puch += 3;
    }

    *pch++ = aszBase64[ *puch >> 2 ];
    *pch++ = aszBase64[ ( *puch & 0x03 ) << 4 ];
    *pch = 0;
    return szID;
}

char *BgBoard::DrawBoard( char *pch, int fRoll, char *asz[7], char *szMatchID, int nChequers ) const
{
    if(!m_fClockwise) 
        return DrawBoardStd( pch, fRoll, asz, szMatchID, nChequers );
	else
		return DrawBoardCls( pch, fRoll, asz, szMatchID, nChequers );   
}

/*
 *  GNU Backgammon  Position ID: 0123456789ABCD 
 *                  Match ID   : 0123456789ABCD
 *  +13-14-15-16-17-18------19-20-21-22-23-24-+     O: gnubg (Cube: 2)
 *  |                  |   | O  O  O  O     O | OO  0 points
 *  |                  |   | O     O          | OO  Cube offered at 2
 *  |                  |   |       O          | O
 *  |                  |   |                  | O
 *  |                  |   |                  | O   
 * v|                  |BAR|                  |     Cube: 1 (7 point match)
 *  |                  |   |                  | X    
 *  |                  |   |                  | X
 *  |                  |   |                  | X
 *  |                  |   |       X  X  X  X | X   Rolled 11
 *  |                  |   |    X  X  X  X  X | XX  0 points
 *  +12-11-10--9--8--7-------6--5--4--3--2--1-+     X: Gary (Cube: 2)
 *
 */
char *BgBoard::DrawBoardStd( char *sz, int fRoll, char *asz[7], char *szMatchID, int nChequers ) const
{
    char *pch = sz, *pchIn;
    int x, y;
	int cOffO = nChequers, cOffX = nChequers;
	BgBoard an(*this);
    static char achX[ 17 ] = "     X6789ABCDEF",  
				achO[ 17 ] = "     O6789ABCDEF";

    for( x = 0; x < 25; x++ ) 
	{
        cOffO -= anBoard[ 0 ][ x ];
        cOffX -= anBoard[ 1 ][ x ];
    }
    
	pch += sprintf_s(pch, 2048, " %-15s %s: ", "MultiGammon", "Position ID");

    if( fRoll )
        strcpy( pch, PositionID() );
    else 
	{
		an.SwapSides();
        strcpy( pch, an.PositionID() );
    }
    
    pch += 14;
    *pch++ = '\n';

    /* match id */

    if ( szMatchID && *szMatchID ) 
      pch += sprintf_s(pch, 80, "                 %s   : %s\n", ("Match ID"), szMatchID);
            
    strcpy_s( pch, 80, fRoll ? " +13-14-15-16-17-18------19-20-21-22-23-24-+     " :
            " +12-11-10--9--8--7-------6--5--4--3--2--1-+     " );
    pch += 49;

    if( asz[ 0 ] )
        for( pchIn = asz[ 0 ]; *pchIn; pchIn++ )
            *pch++ = *pchIn;

    *pch++ = '\n';
    
    for( y = 0; y < 4; y++ ) 
	{
        *pch++ = ' ';
        *pch++ = '|';

        for( x = 12; x < 18; x++ ) 
		{
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[ 0 ][ 24 ] > y ? 'O' : ' ';
        *pch++ = ' ';
        *pch++ = '|';
        
        for( ; x < 24; x++ ) 
		{
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';

        for( x = 0; x < 3; x++ )
            *pch++ = ( cOffO > 5 * x + y ) ? 'O' : ' ';

        *pch++ = ' ';
        
        if( y < 2 && asz[ y + 1 ] )
            for( pchIn = asz[ y + 1 ]; *pchIn; pchIn++ )
                *pch++ = *pchIn;
        *pch++ = '\n';
    }

    *pch++ = ' ';
    *pch++ = '|';

    for( x = 12; x < 18; x++ ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achO[ anBoard[ 0 ][ 24 ] ];
    *pch++ = ' ';
    *pch++ = '|';
        
    for( ; x < 24; x++ ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';
        
    for( x = 0; x < 3; x++ )
        *pch++ = ( cOffO > 5 * x + 4 ) ? 'O' : ' ';

    *pch++ = '\n';
    
    *pch++ = fRoll ? 'v' : '^';
    strcpy( pch, "|                  |BAR|                  |     " );
    pch = strchr ( pch, 0 );
    
    if( asz[ 3 ] )
        for( pchIn = asz[ 3 ]; *pchIn; pchIn++ )
            *pch++ = *pchIn;

    *pch++ = '\n';

    *pch++ = ' ';
    *pch++ = '|';

    for( x = 11; x > 5; x-- ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achX[ anBoard[ 1 ][ 24 ] ];
    *pch++ = ' ';
    *pch++ = '|';
        
    for( ; x < UINT_MAX; x-- ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';
        
    for( x = 0; x < 3; x++ )
        *pch++ = ( cOffX > 5 * x + 4 ) ? 'X' : ' ';

    *pch++ = '\n';
    
    for( y = 3; y < UINT_MAX; y-- ) 
	{
        *pch++ = ' ';
        *pch++ = '|';

        for( x = 11; x > 5; x-- ) 
		{
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ 24 ] > y ? 'X' : ' ';
        *pch++ = ' ';
        *pch++ = '|';
        
        for( ; x < UINT_MAX; x-- ) 
		{
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }
        
        *pch++ = '|';
        *pch++ = ' ';

        for( x = 0; x < 3; x++ )
            *pch++ = ( cOffX > 5 * x + y ) ? 'X' : ' ';

        *pch++ = ' ';
        
        if( y < 2 && asz[ 5 - y ] )
            for( pchIn = asz[ 5 - y ]; *pchIn; pchIn++ )
                *pch++ = *pchIn;
        
        *pch++ = '\n';
    }

    strcpy( pch, fRoll ? " +12-11-10--9--8--7-------6--5--4--3--2--1-+     " :
            " +13-14-15-16-17-18------19-20-21-22-23-24-+     " );
    pch += 49;

    if( asz[ 6 ] )
        for( pchIn = asz[ 6 ]; *pchIn; pchIn++ )
            *pch++ = *pchIn;
    
    *pch++ = '\n';
    *pch = 0;

    return sz;
}


/*
 *     GNU Backgammon  Position ID: 0123456789ABCD
 *                     Match ID   : 0123456789ABCD
 *     +24-23-22-21-20-19------18-17-16-15-14-13-+  O: gnubg (Cube: 2)     
 *  OO | O     O  O  O  O |   |                  |  0 points               
 *  OO |          O     O |   |                  |  Cube offered at 2      
 *   O |          O       |   |                  |                         
 *   O |                  |   |                  |                         
 *   O |                  |   |                  |                         
 *     |                  |BAR|                  |v Cube: 1 (7 point match)
 *   X |                  |   |                  |                         
 *   X |                  |   |                  |                         
 *   X |                  |   |                  |                         
 *   X | X  X  X  X       |   |                  |  Rolled 11              
 *  XX | X  X  X  X  X    |   |                  |  0 points               
 *     +-1--2--3--4--5--6-------7--8--9-10-11-12-+  X: Gary (Cube: 2)      
 *
 */
char *BgBoard::DrawBoardCls( char *sz, int fRoll, char *asz[7], char *szMatchID, int nChequers ) const
{
    char *pch = sz, *pchIn;
    int x, y, cOffO = nChequers, cOffX = nChequers;
	BgBoard an(*this);
    static char achX[ 17 ] = "     X6789ABCDEF",
				achO[ 17 ] = "     O6789ABCDEF";

    for( x = 0; x < 25; x++ ) 
	{
        cOffO -= anBoard[ 0 ][ x ];
        cOffX -= anBoard[ 1 ][ x ];
    }
    
	pch += sprintf_s(pch, 2048, "%18s  %s: ", ("MultiGammon"), ("Position ID"));

    if( fRoll )
        strcpy_s( pch, 2048, PositionID() );
    else 
	{
		an.SwapSides();
        strcpy_s( pch, 2048, an.PositionID() );
    }
    
    pch += 14;
    *pch++ = '\n';
            
    /* match id */

    if ( szMatchID && *szMatchID )
		pch += sprintf(pch, "                    %s   : %s\n", ("Match ID"), szMatchID);
            
    strcpy_s( pch, 2048, fRoll ? "    +24-23-22-21-20-19------18-17-16-15-14-13-+  " :
            "    +-1--2--3--4--5--6-------7--8--9-10-11-12-+  " );
    pch += 49;

    if( asz[ 0 ] )
        for( pchIn = asz[ 0 ]; *pchIn; pchIn++ )
            *pch++ = *pchIn;

    *pch++ = '\n';

    for( y = 0; y < 4; y++ ) 
	{
        for( x = 2; x < UINT_MAX; x-- )
            *pch++ = ( cOffO > 5 * x + y ) ? 'O' : ' ';

        *pch++ = ' ';
        *pch++ = '|';

        for( x = 23 ; x > 17; x-- ) 
		{
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[ 0 ][ 24 ] > y ? 'O' : ' ';
        *pch++ = ' ';
        *pch++ = '|';
        
        for( ; x > 11; x-- ) 
		{
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
		*pch++ = ' ';
        if( y < 2 && asz[ y + 1 ] )
            for( pchIn = asz[ y + 1 ]; *pchIn; pchIn++ )
                *pch++ = *pchIn;

        *pch++ = '\n';
    }

    for( x = 2; x < UINT_MAX; x-- )
        *pch++ = ( cOffO > 5 * x + 4 ) ? 'O' : ' ';

    *pch++ = ' ';
    *pch++ = '|';

    for( x = 23; x > 17; x-- ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achO[ anBoard[ 0 ][ 24 ] ];
    *pch++ = ' ';
    *pch++ = '|';
        
    for( ; x > 11; x-- ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';
    *pch++ = ' ';

    *pch++ = '\n';
    
    strcpy( pch, "    |                  |BAR|                  |" );
    pch = strchr ( pch, 0 );
    *pch++ = fRoll ? 'v' : '^';
    *pch++ = ' ';   
    
    if( asz[ 3 ] )
        for( pchIn = asz[ 3 ]; *pchIn; pchIn++ )
            *pch++ = *pchIn;

    *pch++ = '\n';

    for( x = 2; x < UINT_MAX; x-- )
        *pch++ = ( cOffX > 5 * x + 4 ) ? 'X' : ' ';

    *pch++ = ' ';
    *pch++ = '|';

    for( x = 0; x < 6; x++ ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achX[ anBoard[ 1 ][ 24 ] ];
    *pch++ = ' ';
    *pch++ = '|';
        
    for( ; x < 12; x++ ) 
	{
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ x ] ? achX[ anBoard[ 1 ][ x ] ] :
                achO[ anBoard[ 0 ][ 23 - x ] ];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';
    *pch++ = ' ';

    *pch++ = '\n';
    
    for( y = 3; y < UINT_MAX; y-- ) 
	{
        for( x = 2; x < UINT_MAX; x-- )
            *pch++ = ( cOffX > 5 * x + y ) ? 'X' : ' ';

        *pch++ = ' ';
        *pch++ = '|';

        for( x = 0; x < 6; x++ ) {
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[ 1 ][ 24 ] > y ? 'X' : ' ';
        *pch++ = ' ';
        *pch++ = '|';
        
        for( ; x < 12; x++ ) {
            *pch++ = ' ';
            *pch++ = anBoard[ 1 ][ x ] > y ? 'X' :
                anBoard[ 0 ][ 23 - x ] > y ? 'O' : ' ';
            *pch++ = ' ';
        }
        
        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = ' ';       
        if( y < 2 && asz[ 5 - y ] )
            for( pchIn = asz[ 5 - y ]; *pchIn; pchIn++ )
                *pch++ = *pchIn;
        
        *pch++ = '\n';
    }

    strcpy( pch, fRoll ? "    +-1--2--3--4--5--6-------7--8--9-10-11-12-+   " :
            "    +24-23-22-21-20-19------18-17-16-15-14-13-+  " );
    pch += 49;

    if( asz[ 6 ] )
        for( pchIn = asz[ 6 ]; *pchIn; pchIn++ )
            *pch++ = *pchIn;
    
    *pch++ = '\n';
    *pch = 0;

    return sz;
}

char *BgBoard::FIBSBoardShort( char *pch, int nChequers ) const
{
    char *sz = pch;
    int anOff[ 2 ];
    
    /* Opponent on bar */
    sprintf_s( sz, 5, "%d:", -(int)anBoard[ 0 ][ 24 ] );

    /* Board */
    for(int i = 0; i < 24; i++ )
	{
		int point = (int)anBoard[ 0 ][ 23 - i ];
		sprintf_s( strchr( sz, 0 ), 6, "%d:", (point > 0) ?  -point : (int)anBoard[ 1 ][ i ] );
	}

    /* Player on bar */
    sprintf_s( strchr( sz, 0 ), 6, "%d:", anBoard[ 1 ][ 24 ] );

    anOff[ 0 ] = anOff[ 1 ] = nChequers ? nChequers : 15;
    for(int i = 0; i < 25; i++ ) 
	{
		anOff[ 0 ] -= anBoard[ 0 ][ i ];
		anOff[ 1 ] -= anBoard[ 1 ][ i ];
    }

    sprintf_s( strchr( sz, 0 ), 6, "%d:%d", anOff[ 1 ], -anOff[ 0 ]);
    return pch;
}


int BgBoard::CompareMoves( const void *p0, const void *p1 )
{
    int n0 = *( (int *) p0 ), n1 = *( (int *) p1 );

    if( n0 != n1 )
        return n1 - n0;
    else
        return *( (int *) p1 + 1 ) - *( (int *) p0 + 1 );
}

char *BgBoard::FormatMove(char *sz, const ChequerMove& anMove) const
{
    char *pch = sz;
    int aanMove[ 4 ][ 4 ], *pnSource[ 4 ], *pnDest[ 4 ], i, j;
    int fl = 0;
    int anCount[4], nMoves, nDuplicate, k;
  
    /* Re-order moves into 2-dimensional array. */
    for( i = 0; i < 4 && anMove[ i << 1 ] >= 0; i++ ) 
	{
        aanMove[ i ][ 0 ] = anMove[ i << 1 ] + 1;
        aanMove[ i ][ 1 ] = anMove[ ( i << 1 ) | 1 ] + 1;
        pnSource[ i ] = aanMove[ i ];
        pnDest[ i ] = aanMove[ i ] + 1;
    }

    while( i < 4 ) 
	{
        aanMove[ i ][ 0 ] = aanMove[ i ][ 1 ] = -1;
        pnSource[ i++ ] = NULL;
    }
    
    /* Order the moves in decreasing order of source point. */
    qsort( aanMove, 4, 4 * sizeof( int ), CompareMoves );

    /* Combine moves of a single chequer. */
    for( i = 0; i < 4; i++ )
	{
        for( j = i; j < 4; j++ )
		{
            if( pnSource[ i ] && pnSource[ j ] &&
                *pnDest[ i ] == *pnSource[ j ] ) 
			{
                if( anBoard[ 0 ][ 24 - *pnDest[ i ] ] )
                    /* Hitting blot; record intermediate point. */
                    *++pnDest[ i ] = *pnDest[ j ];
                else
                    /* Non-hit; elide intermediate point. */
                    *pnDest[ i ] = *pnDest[ j ];

                pnSource[ j ] = NULL;           
            }
		}
	}

    /* Compact array. */
    i = 0;

    for( j = 0; j < 4; j++ )
	{
        if( pnSource[ j ] )
		{
            if( j > i ) 
			{
                pnSource[ i ] = pnSource[ j ];
                pnDest[ i ] = pnDest[ j ];
            }

	    i++;
        }
	}

    while( i < 4 )
        pnSource[ i++ ] = NULL;

    for ( i = 0; i < 4; i++)
        anCount[i] = pnSource[i] ? 1 : 0;

    for ( i = 0; i < 3; i++) 
	{
        if (pnSource[i]) 
		{
            nMoves = int(pnDest[i] - pnSource[i]);
            for (j = i + 1; j < 4; j++) 
			{
                if (pnSource[j]) 
				{
                    nDuplicate = 1;
		    
                    if (pnDest[j] - pnSource[j] != nMoves)
                        nDuplicate = 0;
                    else
					{
                        for (k = 0; k <= nMoves && nDuplicate; k++)
						{
							if (pnSource[i][k] != pnSource[j][k])
								nDuplicate = 0;
						}
					}
                    if (nDuplicate) {
                        anCount[i]++;
                        pnSource[j] = NULL;
                    }
                }
            }
        }
    }

    /* Compact array. */
    i = 0;

    for( j = 0; j < 4; j++ )
        if( pnSource[ j ] ) {
            if( j > i ) {
                pnSource[ i ] = pnSource[ j ];
                pnDest[ i ] = pnDest[ j ];
		anCount[ i ] = anCount[ j ];
            }

	    i++;
        }

    if( i < 4 )
        pnSource[ i ] = NULL;

    for( i = 0; i < 4 && pnSource[ i ]; i++ ) {
        if( i )
            *pch++ = ' ';
        
        pch = FormatPoint( pch, *pnSource[ i ] );

        for( j = 1; pnSource[ i ] + j < pnDest[ i ]; j++ ) {
            *pch = '/';
            pch = FormatPoint( pch + 1, pnSource[ i ][ j ] );
            *pch++ = '*';
            fl |= 1 << pnSource[ i ][ j ];
        }

        *pch = '/';
        pch = FormatPoint( pch + 1, *pnDest[ i ] );
        
        if( *pnDest[ i ] && anBoard[ 0 ][ 24 - *pnDest[ i ] ] &&
            !( fl & ( 1 << *pnDest[ i ] ) ) ) {
            *pch++ = '*';
            fl |= 1 << *pnDest[ i ];
        }
	
        if (anCount[i] > 1) {
            *pch++ = '(';
            *pch++ = '0' + anCount[i];
            *pch++ = ')';
        }
    }

    *pch = 0;
    
    return sz;
}

char *BgBoard::FormatPoint( char *pch, int n ) 
{
    assert( n >= 0 );
    
    /*don't translate 'off' and 'bar' as these may be used in UserCommand at a later
     * point */
    if( !n ) 
	{
        strcpy_s( pch, 4, ("off") );
        return pch + strlen(("off") );
    } 
	else 
	if( n == 25 ) 
	{
        strcpy_s( pch, 4, ("bar") );
        return pch + strlen(("bar") );
    } 
	else 
	if( n > 9 )
        *pch++ = n / 10 + '0';

    *pch++ = ( n % 10 ) + '0';

    return pch;
}

char *BgBoard::FormatPointPlain( char *pch, int n )
{
    assert( n >= 0 );
    
    if( n > 9 )
        *pch++ = n / 10 + '0';

    *pch++ = ( n % 10 ) + '0';

    return pch;
}

char *BgBoard::FormatMovePlain(char *sz, const ChequerMove& anMove) const
{
    char *pch = sz;
    int i, j;
    
    for( i = 0; i < 8 && anMove[ i ] >= 0; i += 2 ) 
	{
        pch = FormatPointPlain( pch, anMove[ i ] + 1 );
        *pch++ = '/';
        pch = FormatPointPlain( pch, anMove[ i + 1 ] + 1 );

        if( anBoard && anMove[ i + 1 ] >= 0 &&
            anBoard[ 0 ][ 23 - anMove[ i + 1 ] ] ) 
		{
            for( j = 1; ; j += 2 )
			{
                if( j > i ) 
				{
                    *pch++ = '*';
                    break;
                } 
				else 
				if( anMove[ i + 1 ] == anMove[ j ] )
                    break;
			}
        }
        
        if( i < 6 )
            *pch++ = ' '; 
    }

    *pch = 0;

    return sz;
}
