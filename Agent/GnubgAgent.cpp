#include "GnubgAgent.h"
#include "BgBoard.h"

#include "gnunn\sse.h"

#define WEIGHTS_VERSION "0.15"
#define WEIGHTS_VERSION_BINARY 1.0f
#define WEIGHTS_MAGIC_BINARY 472.3782f

/* Race inputs */
enum {
  /* In a race position, bar and the 24 points are always empty, so only */
  /* 23*4 (92) are needed */

  /* (0 <= k < 14), RI_OFF + k = */
  /*                       1 if exactly k+1 checkers are off, 0 otherwise */

  RI_OFF = 92,

  /* Number of cross-overs by outside checkers */
  
  RI_NCROSS = 92 + 14,
  
  HALF_RACE_INPUTS
};


/* Contact inputs -- see Berliner for most of these */
enum {
  /* n - number of checkers off
     
     off1 -  1         n >= 5
             n/5       otherwise
	     
     off2 -  1         n >= 10
             (n-5)/5   n < 5 < 10
	     0         otherwise
	     
     off3 -  (n-10)/5  n > 10
             0         otherwise
  */
     
  I_OFF1, I_OFF2, I_OFF3,
  
  /* Minimum number of pips required to break contact.

     For each checker x, N(x) is checker location,
     C(x) is max({forall o : N(x) - N(o)}, 0)

     Break Contact : (sum over x of C(x)) / 152

     152 is dgree of contact of start position.
  */
  I_BREAK_CONTACT,

  /* Location of back checker (Normalized to [01])
   */
  I_BACK_CHEQUER,

  /* Location of most backward anchor.  (Normalized to [01])
   */
  I_BACK_ANCHOR,

  /* Forward anchor in opponents home.

     Normalized in the following way:  If there is an anchor in opponents
     home at point k (1 <= k <= 6), value is k/6. Otherwise, if there is an
     anchor in points (7 <= k <= 12), take k/6 as well. Otherwise set to 2.
     
     This is an attempt for some continuity, since a 0 would be the "same" as
     a forward anchor at the bar.
   */
  I_FORWARD_ANCHOR,

  /* Average number of pips opponent loses from hits.
     
     Some heuristics are required to estimate it, since we have no idea what
     the best move actually is.

     1. If board is weak (less than 3 anchors), don't consider hitting on
        points 22 and 23.
     2. Dont break anchors inside home to hit.
   */
  I_PIPLOSS,

  /* Number of rolls that hit at least one checker.
   */
  I_P1,

  /* Number of rolls that hit at least two checkers.
   */
  I_P2,

  /* How many rolls permit the back checker to escape (Normalized to [01])
   */
  I_BACKESCAPES,

  /* Maximum containment of opponent checkers, from our points 9 to op back 
     checker.
     
     Value is (1 - n/36), where n is number of rolls to escape.
   */
  I_ACONTAIN,
  
  /* Above squared */
  I_ACONTAIN2,

  /* Maximum containment, from our point 9 to home.
     Value is (1 - n/36), where n is number of rolls to escape.
   */
  I_CONTAIN,

  /* Above squared */
  I_CONTAIN2,

  /* For all checkers out of home, 
     sum (Number of rolls that let x escape * distance from home)

     Normalized by dividing by 3600.
  */
  I_MOBILITY,

  /* One sided moment.
     Let A be the point of weighted average: 
     A = sum of N(x) for all x) / nCheckers.
     
     Then for all x : A < N(x), M = (average (N(X) - A)^2)

     Diveded by 400 to normalize. 
   */
  I_MOMENT2,

  /* Average number of pips lost when on the bar.
     Normalized to [01]
  */
  I_ENTER,

  /* Probablity of one checker not entering from bar.
     1 - (1 - n/6)^2, where n is number of closed points in op home.
   */
  I_ENTER2,

  I_TIMING,
  
  I_BACKBONE,

  I_BACKG, 
  
  I_BACKG1,
  
  I_FREEPIP,
  
  I_BACKRESCAPES,
  
  MORE_INPUTS 
};

#define MINPPERPOINT 4

#define NUM_INPUTS ((25 * MINPPERPOINT + MORE_INPUTS) * 2)
#define NUM_RACE_INPUTS ( HALF_RACE_INPUTS * 2 )

 
GnubgAgent::GnubgAgent(fs::path path)
	: BgAgent(path)
{
	m_fullName = "Gnubg";
	m_path /= "gnubg";

	m_supportsSanityCheck = true;
	m_needsInvertedEval = true;

	fs::path binPath(m_path);
	binPath /= "gnubg.wd";

	fs::path txtPath(m_path);
	txtPath /= "gnubg.weights";

	load(binPath, txtPath);
	ComputeTable();
}

GnubgAgent::~GnubgAgent(void)
{
	NeuralNetDestroy( &nnContact );
	NeuralNetDestroy( &nnCrashed );
	NeuralNetDestroy( &nnRace );

//	NeuralNetDestroy( &nnpContact );
//	NeuralNetDestroy( &nnpCrashed );
//	NeuralNetDestroy( &nnpRace );
}

int GnubgAgent::binary_weights_failed(const char * filename, FILE * weights)
{
	float r;

	if (!weights)
	{
		printf("%s", filename);
		return -1;
	}
	if (fread(&r, sizeof r, 1, weights) < 1) {
		printf("%s", filename);
		return -2;
	}
	if (r != WEIGHTS_MAGIC_BINARY) {
		printf("%s is not a weights file", filename);
		printf("\n");
		return -3;
	}
	if (fread(&r, sizeof r, 1, weights) < 1) {
		printf("%s", filename);
		return -4;
	}
	if (r != WEIGHTS_VERSION_BINARY)
	{
		char buf[20];
		sprintf(buf, "%.2f", r);
		printf(("weights file %s, has incorrect version (%s), expected (%s)"),
			     filename, buf, WEIGHTS_VERSION);
		printf("\n");
		return -5;
	}

	return 0;
}

int GnubgAgent::weights_failed(const char * filename, FILE * weights)
{
	char file_version[16];
	if (!weights)
	{
		printf("%s", filename);
		return -1;
	}

	if( fscanf( weights, "GNU Backgammon %15s\n",
				file_version ) != 1 )
	{
		printf(("%s is not a weights file"), filename);
		printf("\n");
		return -2;
	}
	if( strcmp( file_version, WEIGHTS_VERSION ) )
	{
		printf(("weights file %s, has incorrect version (%s), expected (%s)"),
			     filename, file_version, WEIGHTS_VERSION);
		printf("\n");
		return -3;
	}
	return 0;
}

void GnubgAgent::load(fs::path binPath, fs::path txtPath)
{
	bool fReadWeights = false;
    if(binPath.string().length())
    { 
		FILE *pfWeights = fopen(binPath.string().c_str(), "rb");
	    if (!binary_weights_failed(binPath.string().c_str(), pfWeights))
	    {
		    if( !fReadWeights && !( fReadWeights =
					    !NeuralNetLoadBinary(&nnContact, pfWeights ) &&
					    !NeuralNetLoadBinary(&nnRace, pfWeights ) &&
					    !NeuralNetLoadBinary(&nnCrashed, pfWeights ) //&&

					    //!NeuralNetLoadBinary(&nnpContact, pfWeights ) &&
					    //!NeuralNetLoadBinary(&nnpCrashed, pfWeights ) &&
					    //!NeuralNetLoadBinary(&nnpRace, pfWeights ) 
						) ) 
			{ 
			    perror( binPath.string().c_str() );
		    }
	    }
	    if (pfWeights)
		    fclose( pfWeights );
	    pfWeights = NULL;
    }

    if( !fReadWeights && txtPath.string().length() )
	{
		FILE *pfWeights = fopen(txtPath.string().c_str(), "r");
	    if (!weights_failed(txtPath.string().c_str(), pfWeights))
	    {
		    if( !( fReadWeights =
					    !NeuralNetLoad( &nnContact, pfWeights ) &&
					    !NeuralNetLoad( &nnRace, pfWeights ) &&
					    !NeuralNetLoad( &nnCrashed, pfWeights )// &&

					    //!NeuralNetLoad( &nnpContact, pfWeights ) &&
					    //!NeuralNetLoad( &nnpCrashed, pfWeights ) &&
					    //!NeuralNetLoad( &nnpRace, pfWeights ) 
			 ) )
			    perror( txtPath.string().c_str() );

	    }
	    if (pfWeights)
		    fclose( pfWeights );
	    pfWeights = NULL;
    }

	assert(fReadWeights);

	if( nnContact.cInput != NUM_INPUTS || nnContact.cOutput != NUM_OUTPUTS )
	{
		if (NeuralNetResize( &nnContact, NUM_INPUTS, nnContact.cHidden, NUM_OUTPUTS ) == -1)
		{
			PrintError( "NeuralNetResize" );
			return;
		}
	}
	if( nnCrashed.cInput != NUM_INPUTS || nnCrashed.cOutput != NUM_OUTPUTS )
	{
		if (NeuralNetResize( &nnCrashed, NUM_INPUTS, nnCrashed.cHidden, NUM_OUTPUTS ) == -1)
		{
			PrintError( "NeuralNetResize" );
			return;
		}
	}		
	if( nnRace.cInput != NUM_RACE_INPUTS || nnRace.cOutput != NUM_OUTPUTS )
	{
		if (NeuralNetResize( &nnRace, NUM_RACE_INPUTS, nnRace.cHidden, NUM_OUTPUTS ) == -1)
		{
			PrintError( "NeuralNetResize" );
			return;
		}
	}
}

void GnubgAgent::PrintError(const char* str)
{
	fprintf(stderr, "%s: %s", str, strerror(errno));
}

void GnubgAgent::evalRace(const BgBoard *board, BgReward& reward)
{
	float SSE_ALIGN( arInput[ NUM_INPUTS ]);
	CalculateRaceInputs( board, arInput );

	NeuralNetEvaluateSSE( &nnRace, arInput, &reward[0], NULL);
  
	/* anBoard[1] is on roll */
    /* total men for side not on roll */
    int totMen0 = 0;
    
    /* total men for side on roll */
    int totMen1 = 0;

    /* a set flag for every possible outcome */
    int any = 0, i;
    
    for(i = 23; i >= 0; --i) 
	{
		totMen0 += board->anBoard[0][i];
		totMen1 += board->anBoard[1][i];
    }

    if( totMen1 == 15 )
		any |= OG_POSSIBLE;
    
    if( totMen0 == 15 )
		any |= G_POSSIBLE;

    if( any ) 
	{
		if( any & OG_POSSIBLE ) 
		{
			for(i = 23; i >= 18; --i) 
			{
				if( board->anBoard[1][i] > 0 ) 
				{
					break;
				}
			}

			if( i >= 18 )
				any |= OBG_POSSIBLE;
		}

		if( any & G_POSSIBLE ) 
		{
			for(i = 23; i >= 18; --i) 
			{
				if( board->anBoard[0][i] > 0 ) 
					break;
			}

			if( i >= 18 ) 
				any |= BG_POSSIBLE;
		}
    }
    
    if( any & (BG_POSSIBLE | OBG_POSSIBLE) ) 
	{
		/* side that can have the backgammon */
		int side = (any & BG_POSSIBLE) ? 1 : 0;
		float pr = BgEval::Instance()->raceBGprob(board, side, m_bgv);

		if( pr > 0.0 ) 
		{
			if( side == 1 ) 
			{
				reward[OUTPUT_WINBACKGAMMON] = pr;

				if( reward[OUTPUT_WINGAMMON] < reward[OUTPUT_WINBACKGAMMON] ) 
					reward[OUTPUT_WINGAMMON] = reward[OUTPUT_WINBACKGAMMON];
			} 
			else 
			{
				reward[OUTPUT_LOSEBACKGAMMON] = pr;

				if(reward[OUTPUT_LOSEGAMMON] < reward[OUTPUT_LOSEBACKGAMMON])
					reward[OUTPUT_LOSEGAMMON] = reward[OUTPUT_LOSEBACKGAMMON];
			}
		} 
		else 
		{
			if( side == 1 )
				reward[OUTPUT_WINBACKGAMMON] = 0.0;
			else
				reward[OUTPUT_LOSEBACKGAMMON] = 0.0;
		}
    }  
  
	/* sanity check will take care of rest */
}

void GnubgAgent::CalculateRaceInputs(const BgBoard *anBoard, float inputs[]) const
{
	for(int side = 0; side < 2; ++side) 
	{
		unsigned int i, k;

		const char* board = anBoard->anBoard[side];
		float* const afInput = inputs + side * HALF_RACE_INPUTS;

		unsigned int menOff = 15;
    	assert( board[23] == 0 && board[24] == 0 );
    
		/* Points */
		for(i = 0; i < 23; ++i) 
		{
			unsigned int const nc = board[i];
			k = i * 4;

			menOff -= nc;
      
			afInput[ k++ ] = (nc == 1) ? 1.0f : 0.0f;
			afInput[ k++ ] = (nc == 2) ? 1.0f : 0.0f;
			afInput[ k++ ] = (nc >= 3) ? 1.0f : 0.0f;
			afInput[ k ] = nc > 3 ? ( nc - 3 ) / 2.0f : 0.0f;
		}

		/* Men off */
		for(k = 0; k < 14; ++k)
			afInput[ RI_OFF + k ] = (menOff == (k+1)) ? 1.0f : 0.0f;
    
		unsigned int nCross = 0;
      
		for(k = 1; k < 4; ++k) 
		{
			for(i = 6*k; i < 6*k + 6; ++i) 
			{
				unsigned int const nc = board[i];
				if( nc )
					nCross += nc * k;
			}
		}
      
		afInput[RI_NCROSS] = nCross / 10.0f;
	}
}

void GnubgAgent::evalCrashed(const BgBoard *board, BgReward& reward)
{
	float SSE_ALIGN(arInput[ NUM_INPUTS ]);

	CalculateCrashedInputs( board, arInput );
    
#if FANN_USE_SSE
	NeuralNetEvaluateSSE( &nnCrashed, arInput, &reward[0], NULL);
#else
	NeuralNetEvaluate( &nnCrashed, arInput, &reward[0], NULL);
#endif

}

void GnubgAgent::CalculateCrashedInputs(const BgBoard *anBoard, float arInput[]) const
{
	baseInputs(anBoard, arInput);

	{
		float* b = arInput + 4 * 25 * 2;
		menOffAll(anBoard->anBoard[1], b + I_OFF1);
		CalculateHalfInputs(anBoard->anBoard[1], anBoard->anBoard[0], b);
	}

	{
		float* b = arInput + (4 * 25 * 2 + MORE_INPUTS);
		menOffAll(anBoard->anBoard[0], b + I_OFF1);
		CalculateHalfInputs( anBoard->anBoard[0], anBoard->anBoard[1], b);
	}
}

void GnubgAgent::baseInputs(const BgBoard *anBoard, float arInput[]) const
{
	for(int j = 0; j < 2; ++j) 
	{
		float* afInput = arInput + j * 25*4;
		const char* board = anBoard->anBoard[j];
    
		/* Points */
		for(int i = 0; i < 24; i++) 
		{
			int nc = board[ i ];
      
			afInput[ i * 4 + 0 ] = (nc == 1) ? 1.0f : 0.0f;
			afInput[ i * 4 + 1 ] = (nc == 2) ? 1.0f : 0.0f;
			afInput[ i * 4 + 2 ] = (nc >= 3) ? 1.0f : 0.0f;
			afInput[ i * 4 + 3 ] = nc > 3 ? ( nc - 3 ) / 2.0f : 0.0f;
		}

		/* Bar */
		int nc = board[ 24 ];
		afInput[ 24 * 4 + 0 ] = (nc >= 1) ? 1.0f : 0.0f;
		afInput[ 24 * 4 + 1 ] = (nc >= 2) ? 1.0f : 0.0f;
		afInput[ 24 * 4 + 2 ] = (nc >= 3) ? 1.0f : 0.0f;
		afInput[ 24 * 4 + 3 ] = nc > 3 ? ( nc - 3 ) / 2.0f : 0.0f;
	}
}

void GnubgAgent::menOffAll(const char* anBoard, float* afInput) const
{
	/* Men off */
	int menOff = 15;
  
	for(int i = 0; i < 25; i++ ) 
		menOff -= anBoard[i];

	if( menOff > 10 ) 
	{
		afInput[ 0 ] = 1.0;
		afInput[ 1 ] = 1.0;
		afInput[ 2 ] = ( menOff - 10 ) / 5.0f;
	} 
	else 
	if( menOff > 5 ) 
	{
		afInput[ 0 ] = 1.0f;
		afInput[ 1 ] = ( menOff - 5 ) / 5.0f;
		afInput[ 2 ] = 0.0f;
	} 
	else 
	{
		afInput[ 0 ] = menOff ? menOff / 5.0f : 0.0f;
		afInput[ 1 ] = 0.0f;
		afInput[ 2 ] = 0.0f;
	}
}

void GnubgAgent::CalculateHalfInputs( const char anBoard[ 25 ], const char anBoardOpp[ 25 ], float afInput[] ) const
{
	int i, j, k, l, nOppBack, n, aHit[ 39 ], nBoard;
    
	/* aanCombination[n] -
     How many ways to hit from a distance of n pips.
     Each number is an index into aIntermediate below. 
	*/
	static const int aanCombination[ 24 ][ 5 ] = 
	{
		{  0, -1, -1, -1, -1 }, /*  1 */
		{  1,  2, -1, -1, -1 }, /*  2 */
		{  3,  4,  5, -1, -1 }, /*  3 */
		{  6,  7,  8,  9, -1 }, /*  4 */
		{ 10, 11, 12, -1, -1 }, /*  5 */
		{ 13, 14, 15, 16, 17 }, /*  6 */
		{ 18, 19, 20, -1, -1 }, /*  7 */
		{ 21, 22, 23, 24, -1 }, /*  8 */
		{ 25, 26, 27, -1, -1 }, /*  9 */
		{ 28, 29, -1, -1, -1 }, /* 10 */
		{ 30, -1, -1, -1, -1 }, /* 11 */
		{ 31, 32, 33, -1, -1 }, /* 12 */
		{ -1, -1, -1, -1, -1 }, /* 13 */
		{ -1, -1, -1, -1, -1 }, /* 14 */
		{ 34, -1, -1, -1, -1 }, /* 15 */
		{ 35, -1, -1, -1, -1 }, /* 16 */
		{ -1, -1, -1, -1, -1 }, /* 17 */
		{ 36, -1, -1, -1, -1 }, /* 18 */
		{ -1, -1, -1, -1, -1 }, /* 19 */
		{ 37, -1, -1, -1, -1 }, /* 20 */
		{ -1, -1, -1, -1, -1 }, /* 21 */
		{ -1, -1, -1, -1, -1 }, /* 22 */
		{ -1, -1, -1, -1, -1 }, /* 23 */
		{ 38, -1, -1, -1, -1 }  /* 24 */
	};
    
	/* One way to hit */ 
	struct Inter 
	{
		/* if true, all intermediate points (if any) are required;
		   if false, one of two intermediate points are required.
		   Set to true for a direct hit, but that can be checked with
		   nFaces == 1,
		*/
		int fAll;

		/* Intermediate points required */
		int anIntermediate[ 3 ];

		/* Number of faces used in hit (1 to 4) */
		int nFaces;

		/* Number of pips used to hit */
		int nPips;
	};
  
	const Inter *pi;
      /* All ways to hit */
	static const Inter aIntermediate[ 39 ] = 
	{
		{ 1, { 0, 0, 0 }, 1, 1 }, /*  0: 1x hits 1 */
		{ 1, { 0, 0, 0 }, 1, 2 }, /*  1: 2x hits 2 */
		{ 1, { 1, 0, 0 }, 2, 2 }, /*  2: 11 hits 2 */
		{ 1, { 0, 0, 0 }, 1, 3 }, /*  3: 3x hits 3 */
		{ 0, { 1, 2, 0 }, 2, 3 }, /*  4: 21 hits 3 */
		{ 1, { 1, 2, 0 }, 3, 3 }, /*  5: 11 hits 3 */
		{ 1, { 0, 0, 0 }, 1, 4 }, /*  6: 4x hits 4 */
		{ 0, { 1, 3, 0 }, 2, 4 }, /*  7: 31 hits 4 */
		{ 1, { 2, 0, 0 }, 2, 4 }, /*  8: 22 hits 4 */
		{ 1, { 1, 2, 3 }, 4, 4 }, /*  9: 11 hits 4 */
		{ 1, { 0, 0, 0 }, 1, 5 }, /* 10: 5x hits 5 */
		{ 0, { 1, 4, 0 }, 2, 5 }, /* 11: 41 hits 5 */
		{ 0, { 2, 3, 0 }, 2, 5 }, /* 12: 32 hits 5 */
		{ 1, { 0, 0, 0 }, 1, 6 }, /* 13: 6x hits 6 */
		{ 0, { 1, 5, 0 }, 2, 6 }, /* 14: 51 hits 6 */
		{ 0, { 2, 4, 0 }, 2, 6 }, /* 15: 42 hits 6 */
		{ 1, { 3, 0, 0 }, 2, 6 }, /* 16: 33 hits 6 */
		{ 1, { 2, 4, 0 }, 3, 6 }, /* 17: 22 hits 6 */
		{ 0, { 1, 6, 0 }, 2, 7 }, /* 18: 61 hits 7 */
		{ 0, { 2, 5, 0 }, 2, 7 }, /* 19: 52 hits 7 */
		{ 0, { 3, 4, 0 }, 2, 7 }, /* 20: 43 hits 7 */
		{ 0, { 2, 6, 0 }, 2, 8 }, /* 21: 62 hits 8 */
		{ 0, { 3, 5, 0 }, 2, 8 }, /* 22: 53 hits 8 */
		{ 1, { 4, 0, 0 }, 2, 8 }, /* 23: 44 hits 8 */
		{ 1, { 2, 4, 6 }, 4, 8 }, /* 24: 22 hits 8 */
		{ 0, { 3, 6, 0 }, 2, 9 }, /* 25: 63 hits 9 */
		{ 0, { 4, 5, 0 }, 2, 9 }, /* 26: 54 hits 9 */
		{ 1, { 3, 6, 0 }, 3, 9 }, /* 27: 33 hits 9 */
		{ 0, { 4, 6, 0 }, 2, 10 }, /* 28: 64 hits 10 */
		{ 1, { 5, 0, 0 }, 2, 10 }, /* 29: 55 hits 10 */
		{ 0, { 5, 6, 0 }, 2, 11 }, /* 30: 65 hits 11 */
		{ 1, { 6, 0, 0 }, 2, 12 }, /* 31: 66 hits 12 */
		{ 1, { 4, 8, 0 }, 3, 12 }, /* 32: 44 hits 12 */
		{ 1, { 3, 6, 9 }, 4, 12 }, /* 33: 33 hits 12 */
		{ 1, { 5, 10, 0 }, 3, 15 }, /* 34: 55 hits 15 */
		{ 1, { 4, 8, 12 }, 4, 16 }, /* 35: 44 hits 16 */
		{ 1, { 6, 12, 0 }, 3, 18 }, /* 36: 66 hits 18 */
		{ 1, { 5, 10, 15 }, 4, 20 }, /* 37: 55 hits 20 */
		{ 1, { 6, 12, 18 }, 4, 24 }  /* 38: 66 hits 24 */
	};

	/* aaRoll[n] - All ways to hit with the n'th roll
     Each entry is an index into aIntermediate above.
	*/
    
	static const int aaRoll[ 21 ][ 4 ] = 
	{
		{  0,  2,  5,  9 }, /* 11 */
		{  0,  1,  4, -1 }, /* 21 */
		{  1,  8, 17, 24 }, /* 22 */
		{  0,  3,  7, -1 }, /* 31 */
		{  1,  3, 12, -1 }, /* 32 */
		{  3, 16, 27, 33 }, /* 33 */
		{  0,  6, 11, -1 }, /* 41 */
		{  1,  6, 15, -1 }, /* 42 */
		{  3,  6, 20, -1 }, /* 43 */
		{  6, 23, 32, 35 }, /* 44 */
		{  0, 10, 14, -1 }, /* 51 */
		{  1, 10, 19, -1 }, /* 52 */
		{  3, 10, 22, -1 }, /* 53 */
		{  6, 10, 26, -1 }, /* 54 */
		{ 10, 29, 34, 37 }, /* 55 */
		{  0, 13, 18, -1 }, /* 61 */
		{  1, 13, 21, -1 }, /* 62 */
		{  3, 13, 25, -1 }, /* 63 */
		{  6, 13, 28, -1 }, /* 64 */
		{ 10, 13, 30, -1 }, /* 65 */
		{ 13, 31, 36, 38 }  /* 66 */
	};

	/* One roll stat */
	struct 
	{
		/* count of pips this roll hits */
		int nPips;
      
		/* number of chequers this roll hits */
		int nChequers;
	} aRoll[ 21 ];

	for(nOppBack = 24; nOppBack >= 0; --nOppBack) 
	{
		if( anBoardOpp[nOppBack] ) 
			break;
	}
    
	nOppBack = 23 - nOppBack;

	n = 0;
	for( i = nOppBack + 1; i < 25; i++ )
		if( anBoard[ i ] )
			n += ( i + 1 - nOppBack ) * anBoard[ i ];

	assert( n );
    
	afInput[ I_BREAK_CONTACT ] = n / (15 + 152.0f);

	{
		unsigned int p  = 0;
    
		for( i = 0; i < nOppBack; i++ ) 
		{
			if( anBoard[i] )
				p += (i+1) * anBoard[i];
		}
    
		afInput[I_FREEPIP] = p / 100.0f;
	}

	{
		int t = 0;
		int no = 0;
      
		t += 24 * anBoard[24];
		no += anBoard[24];
      
		for( i = 23;  i >= 12 && i > nOppBack; --i ) 
		{
			if( anBoard[i] && anBoard[i] != 2 ) 
			{
				int n = ((anBoard[i] > 2) ? (anBoard[i] - 2) : 1);
				no += n;
				t += i * n;
			}
		}

		for( ; i >= 6; --i ) 
		{
			if( anBoard[i] ) 
			{
				int n = anBoard[i];
				no += n;
				t += i * n;
			}
		}
    
		for( i = 5;  i >= 0; --i ) 
		{
			if( anBoard[i] > 2 ) 
			{
				t += i * (anBoard[i] - 2);
				no += (anBoard[i] - 2);
			} 
			else 
			if( anBoard[i] < 2 ) 
			{
				int n = (2 - anBoard[i]);

				if( no >= n ) 
				{
					t -= i * n;
					no -= n;
				}
			}
		}

		if( t < 0 ) 
			t = 0;

		afInput[ I_TIMING ] = t / 100.0f;
	}

	/* Back chequer */

	{
		int nBack;
    
		for( nBack = 24; nBack >= 0; --nBack ) 
		{
			if( anBoard[nBack] ) 
				break;
		}
    
		afInput[ I_BACK_CHEQUER ] = nBack / 24.0f;

		/* Back anchor */
		for( i = nBack == 24 ? 23 : nBack; i >= 0; --i ) 
		{
			if( anBoard[i] >= 2 ) 
				break;
		}
    
		afInput[ I_BACK_ANCHOR ] = i / 24.0f;
    
		/* Forward anchor */
		n = 0;
		for( j = 18; j <= i; ++j ) 
		{
			if( anBoard[j] >= 2 ) 
			{
				n = 24 - j;
				break;
			}
		}

		if( n == 0 ) 
		{
			for( j = 17; j >= 12 ; --j ) 
			{
				if( anBoard[j] >= 2 ) 
				{
					n = 24 - j;
					break;
				}
			}
		}
	
		afInput[ I_FORWARD_ANCHOR ] = n == 0 ? 2.0f : n / 6.0f;
	}
    
	/* Piploss */
	nBoard = 0;
	for( i = 0; i < 6; i++ )
		if( anBoard[ i ] )
			nBoard++;

    memset(aHit, 0, sizeof(aHit));
    
    /* for every point we'd consider hitting a blot on, */
	for( i = ( nBoard > 2 ) ? 23 : 21; i >= 0; i-- )
		/* if there's a blot there, then */
      
		if( anBoardOpp[ i ] == 1 )
			/* for every point beyond */
	
			for( j = 24 - i; j < 25; j++ )
				/* if we have a hitter and are willing to hit */
	  
				if( anBoard[ j ] && !( j < 6 && anBoard[ j ] == 2 ) )
					/* for every roll that can hit from that point */
	    
					for( n = 0; n < 5; n++ ) 
					{
						if( aanCombination[ j - 24 + i ][ n ] == -1 )
							break;

						/* find the intermediate points required to play */
	      
						pi = aIntermediate + aanCombination[ j - 24 + i ][ n ];

						if( pi->fAll ) 
						{
							/* if nFaces is 1, there are no intermediate points */
		
							if( pi->nFaces > 1 ) 
							{
								/* all the intermediate points are required */
		  
								for( k = 0; k < 3 && pi->anIntermediate[k] > 0; k++ )
									if( anBoardOpp[ i - pi->anIntermediate[ k ] ] > 1 )
										/* point is blocked; look for other hits */
										goto cannot_hit;
							}
						} 
						else 
						{
							/* either of two points are required */
		
							if( anBoardOpp[ i - pi->anIntermediate[ 0 ] ] > 1
								&& anBoardOpp[ i - pi->anIntermediate[ 1 ] ] > 1 ) 
							{
								/* both are blocked; look for other hits */
								goto cannot_hit;
							}
						}
	      
						/* enter this shot as available */
	      
						aHit[ aanCombination[ j - 24 + i ][ n ] ] |= 1 << j;
						cannot_hit: ;
					}

	memset(aRoll, 0, sizeof(aRoll));
    
	if( !anBoard[ 24 ] ) 
	{
		/* we're not on the bar; for each roll, */
      
		for( i = 0; i < 21; i++ ) 
		{
			n = -1; /* (hitter used) */
	
			/* for each way that roll hits, */
			for( j = 0; j < 4; j++ ) 
			{
				int r = aaRoll[ i ][ j ];
				if( r < 0 )
					break;

				if( !aHit[ r ] )
					continue;

				pi = aIntermediate + r;
		
				if( pi->nFaces == 1 ) 
				{
					/* direct shot */
					for( k = 23; k > 0; k-- ) 
					{
						if( aHit[ r ] & ( 1 << k ) ) 
						{
							/* select the most advanced blot; if we still have
							a chequer that can hit there */
		      
							if( n != k || anBoard[ k ] > 1 )
								aRoll[ i ].nChequers++;

							n = k;

							if( k - pi->nPips + 1 > aRoll[ i ].nPips )
								aRoll[ i ].nPips = k - pi->nPips + 1;
		
							/* if rolling doubles, check for multiple
							direct shots */
		      
							if( aaRoll[ i ][ 3 ] >= 0 && aHit[ r ] & ~( 1 << k ) )
								aRoll[ i ].nChequers++;
			    
							break;
						}
					}
				} 
				else 
				{
					/* indirect shot */
					if( !aRoll[ i ].nChequers )
						aRoll[ i ].nChequers = 1;

					/* find the most advanced hitter */
					for( k = 23; k >= 0; k-- )
						if( aHit[ r ] & ( 1 << k ) )
							break;

					if( k - pi->nPips + 1 > aRoll[ i ].nPips )
						aRoll[ i ].nPips = k - pi->nPips + 1;

					/* check for blots hit on intermediate points */
		    
					for( l = 0; l < 3 && pi->anIntermediate[ l ] > 0; l++ )
					{
						if( anBoardOpp[ 23 - k + pi->anIntermediate[ l ] ] == 1 ) 
						{
							aRoll[ i ].nChequers++;
							break;
						}
					}
				}
			}
		}
	} 
	else 
	if( anBoard[ 24 ] == 1 ) 
	{
		/* we have one on the bar; for each roll, */
      
		for( i = 0; i < 21; i++ ) 
		{
			n = 0; /* (free to use either die to enter) */
	
			for( j = 0; j < 4; j++ ) 
			{
				int r = aaRoll[ i ][ j ];
				if( r < 0 )
					break;
		
				if( !aHit[ r ] )
					continue;

				pi = aIntermediate + r;
		
				if( pi->nFaces == 1 ) 
				{
					/* direct shot */
	  
					for( k = 24; k > 0; k-- ) 
					{
						if( aHit[ r ] & ( 1 << k ) ) 
						{
							/* if we need this die to enter, we can't hit elsewhere */
	      
							if( n && k != 24 )
								break;
			    
							/* if this isn't a shot from the bar, the
							other die must be used to enter */
	      
							if( k != 24 ) 
							{
								int npip = aIntermediate[aaRoll[ i ][ 1 - j ] ].nPips;
		
								if( anBoardOpp[npip - 1] > 1 )
									break;
				
								n = 1;
							}

							aRoll[ i ].nChequers++;

							if( k - pi->nPips + 1 > aRoll[ i ].nPips )
								aRoll[ i ].nPips = k - pi->nPips + 1;
						}
					}
				} 
				else 
				{
					/* indirect shot -- consider from the bar only */
					if( !( aHit[ r ] & ( 1 << 24 ) ) )
						continue;
		    
					if( !aRoll[ i ].nChequers )
						aRoll[ i ].nChequers = 1;
		    
					 if( 25 - pi->nPips > aRoll[ i ].nPips )
						aRoll[ i ].nPips = 25 - pi->nPips;
		    
					/* check for blots hit on intermediate points */
					for( k = 0; k < 3 && pi->anIntermediate[ k ] > 0; k++ )
					{
						if( anBoardOpp[ pi->anIntermediate[ k ] + 1 ] == 1 ) 
						{
							aRoll[ i ].nChequers++;
							break;
						}
					}
				}
			}
		}
	} 
	else 
	{
		/* we have more than one on the bar --
		   count only direct shots from point 24 */
      
		for( i = 0; i < 21; i++ ) 
		{
		  /* for the first two ways that hit from the bar */
	
			for( j = 0; j < 2; j++ ) 
			{
				int r = aaRoll[ i ][ j ];
				if( !( aHit[r] & ( 1 << 24 ) ) )
					continue;

				pi = aIntermediate + r;

				/* only consider direct shots */
				if( pi->nFaces != 1 )
					continue;

				aRoll[ i ].nChequers++;

				if( 25 - pi->nPips > aRoll[ i ].nPips )
					aRoll[ i ].nPips = 25 - pi->nPips;
			}
		}
	}

	{
		int np = 0;
		int n1 = 0;
		int n2 = 0;
      
		for(i = 0; i < 21; i++) 
		{
			int w = aaRoll[i][3] > 0 ? 1 : 2;
			int nc = aRoll[i].nChequers;
			np += aRoll[i].nPips * w;
	
			if( nc > 0 ) 
			{
				n1 += w;
				if( nc > 1 ) 
					n2 += w;
			}
		}

		afInput[ I_PIPLOSS ] = np / ( 12.0f * 36.0f );
		afInput[ I_P1 ] = n1 / 36.0f;
		afInput[ I_P2 ] = n2 / 36.0f;
	}

	afInput[ I_BACKESCAPES ]  = Escapes( anBoard, 23 - nOppBack ) / 36.0f;
	afInput[ I_BACKRESCAPES ] = Escapes1( anBoard, 23 - nOppBack ) / 36.0f;
  
	for( n = 36, i = 15; i < 24 - nOppBack; i++ )
		if( ( j = Escapes( anBoard, i ) ) < n )
			n = j;

	afInput[ I_ACONTAIN ] = ( 36 - n ) / 36.0f;
	afInput[ I_ACONTAIN2 ] = afInput[ I_ACONTAIN ] * afInput[ I_ACONTAIN ];

	if( nOppBack < 0 ) 
	{
		/* restart loop, point 24 should not be included */
		i = 15;
		n = 36;
	}
    
	for( ; i < 24; i++ )
		if( ( j = Escapes( anBoard, i ) ) < n )
			n = j;

    
	afInput[ I_CONTAIN ] = ( 36 - n ) / 36.0f;
	afInput[ I_CONTAIN2 ] = afInput[ I_CONTAIN ] * afInput[ I_CONTAIN ];
    
	for( n = 0, i = 6; i < 25; i++ )
		if( anBoard[ i ] )
			n += ( i - 5 ) * anBoard[ i ] * Escapes( anBoardOpp, i );

	afInput[ I_MOBILITY ] = n / 3600.0f;

	j = 0;
	n = 0; 
	for(i = 0; i < 25; i++ ) 
	{
		int ni = anBoard[ i ];
      
		if( ni ) 
		{
			j += ni;
			n += i * ni;
		}
	}

	if( j ) 
		n = (n + j - 1) / j;

	j = 0;
	for(k = 0, i = n + 1; i < 25; i++ ) 
	{
		int ni = anBoard[ i ];

	    if( ni ) 
		{
			j += ni;
			k += ni * ( i - n ) * ( i - n );
		}
	}

	if( j ) 
		k = (k + j - 1) / j;

	afInput[ I_MOMENT2 ] = k / 400.0f;

	if( anBoard[ 24 ] > 0 ) 
	{
		int loss = 0;
		int two = anBoard[ 24 ] > 1;
      
		for(i = 0; i < 6; ++i) 
		{
			if( anBoardOpp[ i ] > 1 ) 
			{
				/* any double loses */
				loss += 4*(i+1);

				for(j = i+1; j < 6; ++j) 
				{
					if( anBoardOpp[ j ] > 1 ) 
					{
						loss += 2*(i+j+2);
					} 
					else 
					{
						if( two ) 
						{
							loss += 2*(i+1);
						}
					}
				}
			} 
			else 
			{
				if( two ) 
				{
					for(j = i+1; j < 6; ++j) 
					{
						if( anBoardOpp[ j ] > 1 ) 
							loss += 2*(j+1);
					}
				}
			}
		}
      
		afInput[ I_ENTER ] = loss / (36.0f * (49.0f/6.0f));
	} 
	else 
	{
		afInput[ I_ENTER ] = 0.0f;
	}

	n = 0;
	for(i = 0; i < 6; i++ ) 
		n += anBoardOpp[ i ] > 1;
    
	afInput[ I_ENTER2 ] = ( 36 - ( n - 6 ) * ( n - 6 ) ) / 36.0f; 

	{
		int pa = -1;
		int w = 0;
		int tot = 0;
		int np;
    
		for(np = 23; np > 0; --np) 
		{
			if( anBoard[np] >= 2 ) 
			{
				if( pa == -1 ) 
				{
					pa = np;
					continue;
				}

				{
					int d = pa - np;
					int c = 0;
	
					if( d <= 6 ) 
						c = 11;
					else 
					if( d <= 11 ) 
						c = 13 - d;

					w += c * anBoard[pa];
					tot += anBoard[pa];
				}
			}
		}

		if( tot ) 
			afInput[I_BACKBONE] = 1 - (w / (tot * 11.0f));
		else 
			afInput[I_BACKBONE] = 0;
	}

	{
		unsigned int nAc = 0;
    
		for( i = 18; i < 24; ++i ) 
		{
			if( anBoard[i] > 1 ) 
				++nAc;
		}
    
		afInput[I_BACKG] = 0.0;
		afInput[I_BACKG1] = 0.0;

		if( nAc >= 1 ) 
		{
			unsigned int tot = 0;
			for( i = 18; i < 25; ++i ) 
			{
				tot += anBoard[i];
			}

			if( nAc > 1 ) 
			{
				/* g_assert( tot >= 4 ); */
      
				afInput[I_BACKG] = (tot - 3) / 4.0f;
			} 
			else 
			if( nAc == 1 ) 
			{
				afInput[I_BACKG1] = tot / 8.0f;
			}
		}
	}
}

void GnubgAgent::ComputeTable0( void )
{
	int i, c, n0, n1;

	for( i = 0; i < 0x1000; i++ ) 
	{
		c = 0;
	
		for( n0 = 0; n0 <= 5; n0++ )
			for( n1 = 0; n1 <= n0; n1++ )
				if( !( i & ( 1 << ( n0 + n1 + 1 ) ) ) &&
					!( ( i & ( 1 << n0 ) ) && ( i & ( 1 << n1 ) ) ) )
					c += ( n0 == n1 ) ? 1 : 2;
	
		anEscapes[ i ] = c;
	}
}

int GnubgAgent::Escapes( const char anBoard[ 25 ], int n ) const
{
    int i, af = 0, m;
    m = (n < 12) ? n : 12;

    for( i = 0; i < m; i++ )
		if( anBoard[ 24 + i - n ] > 1 )
		    af |= ( 1 << i );
    
    return anEscapes[ af ];
}

void GnubgAgent::ComputeTable1( void )
{
	int i, c, n0, n1, low;
	anEscapes1[ 0 ] = 0;
  
	for( i = 1; i < 0x1000; i++ ) 
	{
		c = 0;
		low = 0;
		while( ! (i & (1 << low)) ) 
			++low;
    
		for( n0 = 0; n0 <= 5; n0++ )
		{
			for( n1 = 0; n1 <= n0; n1++ ) 
			{
				if( (n0 + n1 + 1 > low) &&
					!( i & ( 1 << ( n0 + n1 + 1 ) ) ) &&
					!( ( i & ( 1 << n0 ) ) && ( i & ( 1 << n1 ) ) ) ) 
				{
					c += ( n0 == n1 ) ? 1 : 2;
				}
			}
		}
	
		anEscapes1[ i ] = c;
	}
}

int GnubgAgent::Escapes1( const char anBoard[ 25 ], int n ) const
{
    int i, af = 0, m;
    m = (n < 12) ? n : 12;

    for( i = 0; i < m; i++ )
		if( anBoard[ 24 + i - n ] > 1 )
			af |= ( 1 << i );
    
    return anEscapes1[ af ];
}

void GnubgAgent::ComputeTable( void )
{
	ComputeTable0();
	ComputeTable1();
}

void GnubgAgent::evalContact(const BgBoard *board, BgReward& reward)
{
	float SSE_ALIGN(arInput[ NUM_INPUTS ]);
	CalculateContactInputs( board, arInput );
    
#if defined FANN_USE_SSE
	NeuralNetEvaluateSSE( &nnContact, arInput, &reward[0], NULL);
#else
	NeuralNetEvaluate( &nnContact, arInput, &reward[0], NULL);
#endif
}

void GnubgAgent::CalculateContactInputs(const BgBoard *anBoard, float arInput[]) const
{
	baseInputs(anBoard, arInput);

	{
		float* b = arInput + 4 * 25 * 2;
		/* I accidentally switched sides (0 and 1) when I trained the net */
		menOffNonCrashed(anBoard->anBoard[0], b + I_OFF1);
  		CalculateHalfInputs(anBoard->anBoard[1], anBoard->anBoard[0], b);
	}

	{
		float* b = arInput + (4 * 25 * 2 + MORE_INPUTS);
		menOffNonCrashed(anBoard->anBoard[1], b + I_OFF1);
		CalculateHalfInputs( anBoard->anBoard[0], anBoard->anBoard[1], b);
	}
}

void GnubgAgent::menOffNonCrashed(const char* anBoard, float* afInput) const
{
	int menOff = 15;
	int i;
  
	for(i = 0; i < 25; ++i)
		menOff -= anBoard[i];
  
	assert( menOff <= 8 );
    
	if( menOff > 5 ) 
	{
		afInput[ 0 ] = 1.0f;
		afInput[ 1 ] = 1.0f;
		afInput[ 2 ] = ( menOff - 6 ) / 3.0f;
	} 
	else 
	if( menOff > 2 ) 
	{
		afInput[ 0 ] = 1.0f;
		afInput[ 1 ] = ( menOff - 3 ) / 3.0f;
		afInput[ 2 ] = 0.0f;
	} 
	else 
	{
		afInput[ 0 ] = menOff ? menOff / 3.0f : 0.0f;
		afInput[ 1 ] = 0.0f;
		afInput[ 2 ] = 0.0f;
	}
}
