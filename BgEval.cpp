#include <assert.h>

#include "BgEval.h"
#include "PositionId.h"
#include "BgBoard.h"
#include "bearoffgammon.h"

int BgEval::anChequers[ NUM_VARIATIONS ] = { 15, 15, 1, 2, 3 };


//classevalfunc acef[ N_CLASSES ] = {
    //EvalOver, 
    //EvalHypergammon1,
    //EvalHypergammon2,
    //EvalHypergammon3,
    //EvalBearoff2, EvalBearoffTS,
    //EvalBearoff1, EvalBearoffOS, 
    // **EvalRace, EvalCrashed, EvalContact
//};
//*/

BgEval *BgEval::m_instance = NULL;
BgEval::BgEval()
: m_rng(random_device()()), m_distribution(1, 6)
{
	pbcOS = NULL;
	pbcTS = NULL;
	pbc1 = NULL;
	pbc2 = NULL;
	for(int i = 0; i < 3; i++)
		apbcHyper[ i ] = NULL;
}

BgEval::~BgEval()
{
	BearoffClose( pbc1 );
	BearoffClose( pbc2 );
	BearoffClose( pbcOS );
	BearoffClose( pbcTS );

	for (int i = 0; i < 3; ++i )
		BearoffClose( apbcHyper[ i ] );
}

BgEval *BgEval::Instance()
{
	if(!m_instance)
		m_instance = new BgEval();

	return m_instance;
}

void BgEval::Destroy()
{
	if(m_instance)
	{
		delete m_instance;
		m_instance = NULL;
	}
}

void BgEval::SanityCheck(const BgBoard *board, BgReward& reward) const
{
	int i, j, nciq, ac[ 2 ], anBack[ 2 ], anCross[ 2 ], anGammonCross[ 2 ],
		anBackgammonCross[ 2 ], anMaxTurns[ 2 ], fContact;

	if( reward[ OUTPUT_WIN ] < 0.0f )
		reward[ OUTPUT_WIN ] = 0.0f;
	else if( reward[ OUTPUT_WIN ] > 1.0f )
		reward[ OUTPUT_WIN ] = 1.0f;
    
	ac[ 0 ] = ac[ 1 ] = anBack[ 0 ] = anBack[ 1 ] = anCross[ 0 ] =
		anCross[ 1 ] = anBackgammonCross[ 0 ] = anBackgammonCross[ 1 ] = 0;
	anGammonCross[ 0 ] = anGammonCross[ 1 ] = 1;
	
	for( j = 0; j < 2; j++ ) 
	{
		for( i = 0, nciq = 0; i < 6; i++ )
			if( board->anBoard[ j ][ i ] ) 
			{
				anBack[ j ] = i;
				nciq += board->anBoard[ j ][ i ];
			}
		ac[ j ] = anCross[ j ] = nciq;

		for( i = 6, nciq = 0; i < 12; i++ )
			if( board->anBoard[ j ][ i ] ) 
			{
				anBack[ j ] = i;
				nciq += board->anBoard[ j ][ i ];
			}
		ac[ j ] += nciq;
		anCross[ j ] += 2*nciq;
		anGammonCross[ j ] += nciq;

		for( i = 12, nciq = 0; i < 18; i++ )
			if( board->anBoard[ j ][ i ] ) 
			{
				anBack[ j ] = i;
				nciq += board->anBoard[ j ][ i ];
			}
		ac[ j ] += nciq;
		anCross[ j ] += 3*nciq;
		anGammonCross[ j ] += 2*nciq;

		for( i = 18, nciq = 0; i < 24; i++ )
			if( board->anBoard[ j ][ i ] ) 
			{
				anBack[ j ] = i;
				nciq += board->anBoard[ j ][ i ];
			}
		ac[ j ] += nciq;
		anCross[ j ] += 4*nciq;
		anGammonCross[ j ] += 3*nciq;
		anBackgammonCross[ j ] = nciq;

		if( board->anBoard[ j ][ 24 ] ) 
		{
			anBack[ j ] = 24;
			ac[ j ] += board->anBoard[ j ][ 24 ];
			anCross[ j ] += 5 * board->anBoard[ j ][ 24 ];
			anGammonCross[ j ] += 4 * board->anBoard[ j ][ 24 ];
			anBackgammonCross[ j ] += 2 * board->anBoard[ j ][ 24 ];
		}
	}

	fContact = anBack[ 0 ] + anBack[ 1 ] >= 24;

	if( !fContact ) 
	{
		for( i = 0; i < 2; i++ )
		{
			if( anBack[ i ] < 6 && pbc1 )
			{
				anMaxTurns[ i ] = 
					MaxTurns( PositionId::PositionBearoff( board->anBoard[ i ], pbc1->nPoints, pbc1->nChequers ) );
			}
			else
			{
				anMaxTurns[ i ] = anCross[ i ] * 2;
			}
		}
      
		if ( ! anMaxTurns[ 1 ] ) anMaxTurns[ 1 ] = 1;
	}
    
    if( !fContact && anCross[ 0 ] > 4 * ( anMaxTurns[ 1 ] - 1 ) )
	{
		// Certain win
		reward[ OUTPUT_WIN ] = 1.0f;
	}
    
    if( ac[ 0 ] < 15 )
	{
		// Opponent has borne off; no gammons or backgammons possible 
		reward[ OUTPUT_WINGAMMON ] = reward[ OUTPUT_WINBACKGAMMON ] = 0.0f;
	}
    else 
	if( !fContact ) 
	{
		if( anCross[ 1 ] > 8 * anGammonCross[ 0 ] )
		{
			// Gammon impossible
			reward[ OUTPUT_WINGAMMON ] = 0.0f;
		}
		else 
		if( anGammonCross[ 0 ] > 4 * ( anMaxTurns[ 1 ] - 1 ) )
		{
			// Certain gammon
			reward[ OUTPUT_WINGAMMON ] = 1.0f;
		}
	
		if( anCross[ 1 ] > 8 * anBackgammonCross[ 0 ] )
		{
			// Backgammon impossible
			reward[ OUTPUT_WINBACKGAMMON ] = 0.0f;
		}
		else 
		if( anBackgammonCross[ 0 ] > 4 * ( anMaxTurns[ 1 ] - 1 ) )
		{
			// Certain backgammon
			reward[ OUTPUT_WINGAMMON ] = reward[ OUTPUT_WINBACKGAMMON ] = 1.0f;
		}
    }

    if( !fContact && anCross[ 1 ] > 4 * anMaxTurns[ 0 ] )
	{
		// Certain loss
		reward[ OUTPUT_WIN ] = 0.0f;
	}
    
    if( ac[ 1 ] < 15 )
	{
		// Player has borne off; no gammon or backgammon losses possible
		reward[ OUTPUT_LOSEGAMMON ] = reward[ OUTPUT_LOSEBACKGAMMON ] = 0.0f;
	}
    else if( !fContact ) 
	{
		if( anCross[ 0 ] > 8 * anGammonCross[ 1 ] - 4 )
		{
			// Gammon loss impossible
			reward[ OUTPUT_LOSEGAMMON ] = 0.0f;
		}
		else 
		if( anGammonCross[ 1 ] > 4 * anMaxTurns[ 0 ] )
		{
			// Certain gammon loss
			reward[ OUTPUT_LOSEGAMMON ] = 1.0f;
		}
	
		if( anCross[ 0 ] > 8 * anBackgammonCross[ 1 ] - 4 )
		{
			// Backgammon loss impossible
			reward[ OUTPUT_LOSEBACKGAMMON ] = 0.0f;
		}
		else if( anBackgammonCross[ 1 ] > 4 * anMaxTurns[ 0 ] )
		{
			// Certain backgammon loss 
			reward[ OUTPUT_LOSEGAMMON ] = reward[ OUTPUT_LOSEBACKGAMMON ] = 1.0f;
		}
    }

    // gammons must be less than wins
    if( reward[ OUTPUT_WINGAMMON ] > reward[ OUTPUT_WIN ] )
		reward[ OUTPUT_WINGAMMON ] = reward[ OUTPUT_WIN ];

    float lose = 1.0f - reward[ OUTPUT_WIN ];
    if( reward[ OUTPUT_LOSEGAMMON ] > lose )
		reward[ OUTPUT_LOSEGAMMON ] = lose;

    // Backgammons cannot exceed gammons
    if( reward[ OUTPUT_WINBACKGAMMON ] > reward[ OUTPUT_WINGAMMON ] )
		reward[ OUTPUT_WINBACKGAMMON ] = reward[ OUTPUT_WINGAMMON ];
    
    if( reward[ OUTPUT_LOSEBACKGAMMON ] > reward[ OUTPUT_LOSEGAMMON ] )
		reward[ OUTPUT_LOSEBACKGAMMON ] = reward[ OUTPUT_LOSEGAMMON ];

	float noise = 1/10000.0f;
	for(int i = OUTPUT_WINGAMMON; i < 5; ++i) 
	{
		if( reward[i] < noise ) 
			reward[i] = 0;
    }
}

int BgEval::MaxTurns( int id ) const
{
	unsigned short int aus[ 32 ];
    
	BearoffDist ( pbc1, id, NULL, NULL, NULL, aus, NULL );
	for(int i = 31; i >= 0; i-- )
	{
		if( aus[i] )
			return i;
	}

	return -1;
}

positionclass BgEval::ClassifyPosition(const BgBoard *anBoard, const bgvariation bgv ) const
{
	int nOppBack = -1, nBack = -1;

	for(nOppBack = 24; nOppBack >= 0; --nOppBack) 
		if( anBoard->anBoard[0][nOppBack] ) 
			break;

	for(nBack = 24; nBack >= 0; --nBack) 
		if( anBoard->anBoard[1][nBack] ) 
			break;

	if( nBack < 0 || nOppBack < 0 )
		return CLASS_OVER;

	// special classes for hypergammon variants 

	switch ( bgv ) 
	{
	case VARIATION_HYPERGAMMON_1:
		return CLASS_HYPERGAMMON1;
		break;

	case VARIATION_HYPERGAMMON_2:
		return CLASS_HYPERGAMMON2;
		break;

	case VARIATION_HYPERGAMMON_3:
		return CLASS_HYPERGAMMON3;
		break;

	case VARIATION_STANDARD:
	case VARIATION_NACKGAMMON:
		// normal backgammon 
	    if( nBack + nOppBack > 22 ) 
		{
			// contact position
			unsigned int const N = 6;
			unsigned int i;
			unsigned int side;
    
			for(side = 0; side < 2; ++side) 
			{
				unsigned int tot = 0;
				const char* board = anBoard->anBoard[side];
      
		        for(i = 0;  i < 25; ++i) 
					tot += board[i];

				if( tot <= N ) 
					return CLASS_CRASHED;
				else 
				{
					if( board[0] > 1 ) 
					{
						if( tot <= (N + board[0]) ) 
							return CLASS_CRASHED;
						else 
						if( board[1] > 1 && (1 + tot - (board[0] + board[1])) <= N ) 
							return CLASS_CRASHED;
					} 
					else 
					if( tot <= (N + (board[1] - 1))) 
						return CLASS_CRASHED;
				}
			}

			return CLASS_CONTACT;
		}
		else 
		{
    		if ( isBearoff ( pbc2, anBoard ) )
				return CLASS_BEAROFF2;

			if ( isBearoff ( pbcTS, anBoard ) )
				return CLASS_BEAROFF_TS;

			if ( isBearoff ( pbc1, anBoard ) )
				return CLASS_BEAROFF1;

			if ( isBearoff ( pbcOS, anBoard ) )
				return CLASS_BEAROFF_OS;

			return CLASS_RACE;
		}
		break;

	default:
		assert ( false );
		break;
	}

	return CLASS_OVER;   // for fussy compilers
}

int BgEval::EvalBearoff2(const BgBoard *anBoard, BgReward& arOutput) const
{
	assert ( pbc2 );
	return BearoffEval ( pbc2, anBoard, arOutput );
}

int BgEval::EvalBearoffOS(const BgBoard *anBoard, BgReward& arOutput) const 
{
	assert ( pbcOS );
	return BearoffEval ( pbcOS, anBoard, arOutput );
}

int BgEval::EvalBearoffTS(const BgBoard *anBoard, BgReward& arOutput) const 
{
	assert ( pbcTS );
	return BearoffEval ( pbcTS, anBoard, arOutput );
}

int BgEval::EvalHypergammon1(const BgBoard *anBoard, BgReward& arOutput) const 
{
	assert ( apbcHyper[ 0 ] );
	return BearoffEval ( apbcHyper[ 0 ], anBoard, arOutput );
}

int BgEval::EvalHypergammon2(const BgBoard *anBoard, BgReward& arOutput) const 
{
	assert ( apbcHyper[ 1 ] );
	return BearoffEval ( apbcHyper[ 1 ], anBoard, arOutput );
}

int BgEval::EvalHypergammon3(const BgBoard *anBoard, BgReward& arOutput) const 
{
	assert ( apbcHyper[ 2 ] );
	return BearoffEval ( apbcHyper[ 2 ], anBoard, arOutput );
}

int BgEval::EvalBearoff1(const BgBoard *anBoard, BgReward& arOutput) const
{
	assert ( pbc1 );
	return BearoffEval( pbc1, anBoard, arOutput );
}

float BgEval::raceBGprob(const BgBoard *anBoard, const int side, const bgvariation bgv) const
{
	int totMenHome = 0;
	int totPipsOp = 0;
	BgBoard dummy;
  
	for(int i = 0; i < 6; ++i)
		totMenHome += anBoard->anBoard[side][i];
	
	for(int i = 22; i >= 18; --i) 
		totPipsOp += anBoard->anBoard[1-side][i] * (i-17);
	

	if(! ((totMenHome + 3) / 4 - (side == 1 ? 1 : 0) <= (totPipsOp + 2) / 3) )
		return 0.0f;

	for(int i = 0; i < 25; ++i) 
		dummy.anBoard[side][i] = anBoard->anBoard[side][i];

	for(int i = 0; i < 6; ++i)
		dummy.anBoard[1-side][i] = anBoard->anBoard[1-side][18+i];

	for(int i = 6; i < 25; ++i)
		dummy.anBoard[1-side][i] = 0;

	{
		const long* bgp = getRaceBGprobs(dummy.anBoard[1-side]);
		if( bgp ) 
		{
			int k = PositionId::PositionBearoff(anBoard->anBoard[side], pbc1->nPoints, pbc1->nChequers);
			unsigned short int aProb[32];
			float p = 0.0f;
			unsigned long scale = (side == 0) ? 36 : 1;
			BearoffDist ( pbc1, k, NULL, NULL, NULL, aProb, NULL );

			for(int j = 1-side; j < RBG_NPROBS; j++) 
			{
				unsigned long sum = 0;
				scale *= 36;
				for(int i = 1; i <= j + side; ++i)
					sum += aProb[i];
				p += ((float)bgp[j])/scale * sum;
			}

			p /= 65535.0;
			return p;
		} 
		else 
		{
			BgReward p;
      		if( PositionId::PositionBearoff( dummy.anBoard[0], 6, 15 ) > 923 ||
				PositionId::PositionBearoff( dummy.anBoard[1], 6, 15 ) > 923 ) 
			{
				EvalBearoff1(&dummy, p);
			} 
			else 
			{
				EvalBearoff2(&dummy, p);
			}

			return side == 1 ? p[0] : 1 - p[0];
		}
	}
}

bool BgEval::load(const char *path)
{
	m_basePath = fs::path(path).parent_path();

	// read one-sided db from gnubg.bd
	fs::path gnubg_bearoff_os = m_basePath;
	gnubg_bearoff_os /= "gnubg_os0.bd";

	if(pbc1) delete pbc1;
	pbc1 = BearoffInit( gnubg_bearoff_os.string().c_str(), (int)BO_IN_MEMORY, NULL );

	if( !pbc1 )
	{
		printf("Create bearoff db: ");
		pbc1 = BearoffInit ( NULL, BO_HEURISTIC, BearoffProgress );
		printf(" done!\n");
	}

	// read two-sided db from gnubg.bd 
	fs::path gnubg_bearoff = m_basePath;
	gnubg_bearoff /= "gnubg_ts0.bd";
	pbc2 = BearoffInit ( gnubg_bearoff.string().c_str(), BO_IN_MEMORY | BO_MUST_BE_TWO_SIDED, NULL );

	if ( !pbc2 )
	{
		fprintf ( stderr, 
			"\n***WARNING***\n\n" 
			"Note that Multigammon does not use the gnubg.bd file.\n"
			"You should obtain the file gnubg_ts0.bd or generate\n"
			"it yourself using the program 'makebearoff'.\n"
			"You can generate the file with the command:\n"
			"makebearoff -t 6x6 -f gnubg_ts0.bd\n"
			"You can also generate other bearoff databases; see\n"
			"GNU Backgammon README for more details\n\n" );
	}

	/*
	gnubg_bearoff_os = m_basePath;
	gnubg_bearoff_os /= "gnubg_os.bd";
	// init one-sided db
	pbcOS = BearoffInit ( gnubg_bearoff_os.string().c_str(), BO_IN_MEMORY, NULL );

	gnubg_bearoff = m_basePath;
	gnubg_bearoff /= "gnubg_ts.bd";
	// init two-sided db 
	pbcTS = BearoffInit ( gnubg_bearoff.string().c_str(), BO_IN_MEMORY, NULL );

	// hyper-gammon databases 
	for (int i = 0; i < 3; ++i) 
	{
		gnubg_bearoff = m_basePath;
		char sz[10];
		sprintf(sz, "hyper%1d.bd", i + 1);
		gnubg_bearoff /= sz;
		apbcHyper[i] = BearoffInit(gnubg_bearoff.string().c_str(), BO_NONE, NULL);
	}
	*/

	return pbc1 && pbc2;
}

void BgEval::BearoffProgress( unsigned int i )
{
	printf(".");
    //putchar( "\\|/-"[ ( i / 1000 ) % 4 ] );
    //putchar( '\r' );
    //fflush( stdout );
}

int BgEval::EvalOver( const BgBoard *anBoard, BgReward& arOutput, const bgvariation bgv) const
{
	int i, c;
	int n = anChequers[ bgv ];

	for( i = 0; i < 25; i++ )
		if( anBoard->anBoard[ 0 ][ i ] )
		break;

	if( i == 25 ) 
	{
		/* opponent has no pieces on board; player has lost */
		arOutput[ OUTPUT_WIN ] = arOutput[ OUTPUT_WINGAMMON ] = arOutput[ OUTPUT_WINBACKGAMMON ] = 0.0;

		for( i = 0, c = 0; i < 25; i++ )
			c += anBoard->anBoard[ 1 ][ i ];

		if( c == n ) 
		{
			/* player still has all pieces on board; loses gammon */
			arOutput[ OUTPUT_LOSEGAMMON ] = 1.0;

			for( i = 18; i < 25; i++ )
			{
				if( anBoard->anBoard[ 1 ][ i ] ) 
				{
					/* player still has pieces in opponent's home board;
					loses backgammon */
					arOutput[ OUTPUT_LOSEBACKGAMMON ] = 1.0;
					return 0;
				}
			}
	    
			arOutput[ OUTPUT_LOSEBACKGAMMON ] = 0.0;
			return 0;
		}

		arOutput[ OUTPUT_LOSEGAMMON ] = arOutput[ OUTPUT_LOSEBACKGAMMON ] = 0.0;
		return 0;
	}
    
	for( i = 0; i < 25; i++ )
		if( anBoard->anBoard[ 1 ][ i ] )
			break;

	if( i == 25 ) 
	{
		/* player has no pieces on board; wins */
		arOutput[ OUTPUT_WIN ] = 1.0;
		arOutput[ OUTPUT_LOSEGAMMON ] = arOutput[ OUTPUT_LOSEBACKGAMMON ] = 0.0;

		for( i = 0, c = 0; i < 25; i++ )
			c += anBoard->anBoard[ 0 ][ i ];

		if( c == n ) 
		{
			/* opponent still has all pieces on board; win gammon */
			arOutput[ OUTPUT_WINGAMMON ] = 1.0;

			for( i = 18; i < 25; i++ )
			{
				if( anBoard->anBoard[ 0 ][ i ] ) 
				{
					/* opponent still has pieces in player's home board;
					win backgammon */
					arOutput[ OUTPUT_WINBACKGAMMON ] = 1.0;
					return 0;
				}
			}
	    
			arOutput[ OUTPUT_WINBACKGAMMON ] = 0.0;
			return 0;
		}

		arOutput[ OUTPUT_WINGAMMON ] =  arOutput[ OUTPUT_WINBACKGAMMON ] = 0.0;
	}
  
	return 0;
}
