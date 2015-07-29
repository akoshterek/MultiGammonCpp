#include "BgMatch.h"

matchstate::matchstate()
{
	anDice[0] = anDice[1] = 0;
	fTurn = 0;
	fResigned = fResignationDeclined = fDoubled = false;
	cGames = fMove = 0;
	fCubeOwner = -1;
	fCrawford = fPostCrawford = false;
	nMatchTo = 0;
	anScore[0] = anScore[1] = 0;
	nCube = 1;
	cBeavers = 0;
	bgv = VARIATION_STANDARD;
	fCubeUse = fJacoby = false;
	gs = GAME_NONE;
}

void matchstate::GetMatchStateCubeInfo(cubeinfo* pci) const
{
	SetCubeInfo( pci, nCube, fCubeOwner, fMove,
		nMatchTo, anScore, fCrawford,
		fJacoby, 0, bgv );
}

int matchstate::SetCubeInfoMoney( cubeinfo *pci, const int nCube, const int fCubeOwner,
                  const int fMove, const int fJacoby, const int fBeavers,
                  const bgvariation bgv ) const
{
	/* FIXME also illegal if nCube is not a power of 2 */
    if( nCube < 1 || fCubeOwner < -1 || fCubeOwner > 1 || fMove < 0 || fMove > 1 ) 
    {
		memset(pci, 0, sizeof(cubeinfo));
		return -1;
    }

    pci->nCube = nCube;
    pci->fCubeOwner = fCubeOwner;
    pci->fMove = fMove;
    pci->fJacoby = fJacoby;
    pci->fBeavers = fBeavers;
    pci->nMatchTo = pci->anScore[ 0 ] = pci->anScore[ 1 ] = pci->fCrawford = 0;
    pci->bgv = bgv;
    
    pci->arGammonPrice[ 0 ] = pci->arGammonPrice[ 1 ] =
	pci->arGammonPrice[ 2 ] = pci->arGammonPrice[ 3 ] =
	    ( fJacoby && fCubeOwner == -1 ) ? 0.0f : 1.0f;

    return 0;
}

int matchstate::SetCubeInfo ( cubeinfo *pci, const int nCube, const int fCubeOwner, 
              const int fMove, const int nMatchTo, const int anScore[ 2 ], 
              const int fCrawford, const int fJacoby, const int fBeavers, 
              const bgvariation bgv ) const
{
	if(nMatchTo)
	{
		//return  SetCubeInfoMatch( pci, nCube, fCubeOwner, fMove,
		//		nMatchTo, anScore, fCrawford, bgv );
		//no matches
		assert(0);
		return -1;
	}
	else
		return SetCubeInfoMoney( pci, nCube, fCubeOwner, fMove, fJacoby, fBeavers, bgv );
}

