#include <memory.h>
#include <assert.h>

#include "PositionId.h"
#include "BgBoard.h"

#define MAX_N 40
#define MAX_R 25

static unsigned int anCombination[ MAX_N ][ MAX_R ], fCalculated = 0;

static void InitCombination( void )
{
    unsigned int i, j;

    for( i = 0; i < MAX_N; i++ )
        anCombination[ i ][ 0 ] = i + 1;
    
    for( j = 1; j < MAX_R; j++ )
        anCombination[ 0 ][ j ] = 0;

    for( i = 1; i < MAX_N; i++ )
        for( j = 1; j < MAX_R; j++ )
            anCombination[ i ][ j ] = anCombination[ i - 1 ][ j - 1 ] +
                anCombination[ i - 1 ][ j ];

    fCalculated = 1;
}

unsigned int PositionId::Combination( const unsigned int n, const unsigned int r )
{
    assert( n <= MAX_N && r <= MAX_R );

    if( !fCalculated )
        InitCombination();

    return anCombination[ n - 1 ][ r - 1 ];
}

unsigned int PositionId::PositionF( unsigned int fBits, unsigned int n, unsigned int r )
{
    if( n == r )
        return 0;

    return ( fBits & ( 1u << ( n - 1 ) ) ) ? Combination( n - 1, r ) +
        PositionF( fBits, n - 1, r - 1 ) : PositionF( fBits, n - 1, r );
}

unsigned short PositionId::PositionIndex(unsigned int g, const unsigned int anBoard[6])
{
  unsigned int i, fBits;
  unsigned int j = g - 1;

  for(i = 0; i < g; i++ )
    j += anBoard[ i ];

  fBits = 1u << j;
    
  for(i = 0; i < g; i++)
  {
    j -= anBoard[ i ] + 1;
    fBits |= ( 1u << j );
  }

  /* FIXME: 15 should be replaced by nChequers, but the function is
     only called from bearoffgammon, so this should be fine. */
  return (unsigned short)PositionF( fBits, 15, g );
}

unsigned int PositionId::PositionBearoff(const char anBoard[], unsigned int nPoints, unsigned int nChequers)
{
    unsigned int i, fBits, j;

    for( j = nPoints - 1, i = 0; i < nPoints; i++ )
        j += anBoard[ i ];

    fBits = 1u << j;
    
    for( i = 0; i < nPoints; i++ ) {
        j -= anBoard[ i ] + 1;
        fBits |= ( 1u << j );

    }

    return PositionF( fBits, nChequers + nPoints, nPoints );
}

unsigned char PositionId::Base64( const unsigned char ch )
{
    if( ch >= 'A' && ch <= 'Z' )
        return ch - 'A';

    if( ch >= 'a' && ch <= 'z' )
        return (ch - 'a') + 26;

    if( ch >= '0' && ch <= '9' )
        return (ch - '0') + 52;

    if( ch == '+' )
        return 62;

    if( ch == '/' )
        return 63;

    return 255;
}

unsigned int PositionId::PositionInv( unsigned int nID, unsigned int n, unsigned int r )
{
    unsigned int nC;

    if( !r )
        return 0;
    else if( n == r )
        return ( 1u << n ) - 1;

    nC = Combination( n - 1, r );

    return ( nID >= nC ) ? ( 1u << ( n - 1 ) ) |
        PositionInv( nID - nC, n - 1, r - 1 ) : PositionInv( nID, n - 1, r );
}

void PositionId::PositionFromBearoff(char anBoard[], unsigned int usID,
		    unsigned int nPoints, unsigned int nChequers )
{
    unsigned int fBits = PositionInv( usID, nChequers + nPoints, nPoints );
    unsigned int i, j;

    for( i = 0; i < nPoints; i++ )
        anBoard[ i ] = 0;

    j = nPoints - 1;
    for( i = 0; i < ( nChequers + nPoints ); i++ )
	{
        if( fBits & ( 1u << i ) )
		{
			if (j == 0)
				break;
            j--;
		}
        else
            anBoard[ j ]++;
    }
}
