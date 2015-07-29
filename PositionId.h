#if !defined __POSITIONID_H
#define __POSITIONID_H
#pragma once

#include <memory.h>

class BgBoard;
class PositionId
{
public:
	static const int L_POSITIONID = 14;

	static bool EqualKeys(const unsigned char k1[10], const unsigned char k2[10])
	{
		return !memcmp(k1, k2, sizeof(unsigned char) * 10);
		//return k1[0]==k2[0] && k1[1]==k2[1] && k1[2]==k2[2] 
		//	   && k1[3]==k2[3] && k1[4]==k2[4] && k1[5]==k2[5] 
		//	   && k1[6]==k2[6] && k1[7]==k2[7] && k1[8]==k2[8] 
		//	   && k1[9]==k2[9];
	}

	static unsigned short PositionIndex(unsigned int g, const unsigned int anBoard[6]);
	static unsigned int Combination ( const unsigned int n, const unsigned int r );

	static unsigned int PositionBearoff( const char anBoard[],
								  unsigned int nPoints,
								  unsigned int nChequers );

	static void PositionFromBearoff(char anBoard[], unsigned int usID,
		    unsigned int nPoints, unsigned int nChequers );

	static unsigned char Base64( const unsigned char ch );

private:
	static unsigned int PositionF( unsigned int fBits, unsigned int n, unsigned int r );
	static unsigned int PositionInv( unsigned int nID, unsigned int n, unsigned int r );
};



#endif
