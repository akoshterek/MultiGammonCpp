#if !defined __BG_EVAL_H
#define __BG_EVAL_H
#pragma once

#include "BgCommon.h"
#include "bearoff.h"
#include "BgReward.h"

struct movefilter 
{
  int   Accept;    /* always allow this many moves. 0 means don't use this */
		   /* level, since at least 1 is needed when used. */
  int   Extra;     /* and add up to this many more... */
  float Threshold; /* ...if they are within this equity difference */
};

enum evaltype { EVAL_NONE, EVAL_EVAL, EVAL_ROLLOUT } ;

/* we'll have filters for 1..4 ply evaluation */
#define MAX_FILTER_PLIES	4
//extern movefilter defaultFilters[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

struct evalcontext
{
    /* FIXME expand this... e.g. different settings for different position
       classes */
    unsigned int fCubeful : 1; /* cubeful evaluation */
    unsigned int nPlies   : 3;
    unsigned int fUsePrune : 1;
    unsigned int fDeterministic : 1;
    float        rNoise;       /* standard deviation */

	evalcontext()
	{
		fCubeful = 0;
		nPlies = 0;
		fUsePrune = 0;
		rNoise = 0;
	}
};

/* identifies the format of evaluation info in .sgf files
   early (pre extending rollouts) had no version numbers
   extendable rollouts have a version number of 1
   with pruning nets, we have two possibilities - reduction could still
   be enabled (version = 2) or, given the speedup and performance 
   improvements, I assume we will drop reduction entirely. (ver = 3 or more)
   When presented with an .sgf file, gnubg will attempt to work out what
   data is present in the file based on the version number
*/

#define SGF_FORMAT_VER 3

struct evalsetup
{
  evaltype et;
  evalcontext ec;
  //rolloutcontext rc;
};


/* position classes */
enum positionclass 
{
    CLASS_OVER = 0,     /* Game already finished */
    CLASS_HYPERGAMMON1, /* hypergammon with 1 chequers */
    CLASS_HYPERGAMMON2, /* hypergammon with 2 chequers */
    CLASS_HYPERGAMMON3, /* hypergammon with 3 chequers */
    CLASS_BEAROFF2,     /* Two-sided bearoff database (in memory) */
    CLASS_BEAROFF_TS,   /* Two-sided bearoff database (on disk) */
    CLASS_BEAROFF1,     /* One-sided bearoff database (in memory) */
    CLASS_BEAROFF_OS,   /* One-sided bearoff database (on disk) */
    CLASS_RACE,         /* Race neural network */
    CLASS_CRASHED,      /* Contact, one side has less than 7 active checkers */
    CLASS_CONTACT       /* Contact neural network */
} ;

#define CLASS_PERFECT CLASS_BEAROFF_TS
#define N_CLASSES (CLASS_CONTACT + 1)

/*
 * Cubeinfo contains the information necesary for evaluation
 * of a position.
 * These structs are placed here so that the move struct can be defined
 */

struct cubeinfo
{
    /*
     * nCube: the current value of the cube,
     * fCubeOwner: the owner of the cube,
     * fMove: the player for which we are
     *        calculating equity for,
     * fCrawford, fJacoby, fBeavers: optional rules in effect,
     * arGammonPrice: the gammon prices;
     *   [ 0 ] = gammon price for player 0,
     *   [ 1 ] = gammon price for player 1,
     *   [ 2 ] = backgammon price for player 0,
     *   [ 3 ] = backgammon price for player 1.
     *
     */
    int nCube, fCubeOwner, fMove, nMatchTo, anScore[ 2 ], fCrawford, fJacoby,
	fBeavers;
    float arGammonPrice[ 4 ];
    bgvariation bgv;
};


enum 
{
  /* gammon possible by side on roll */
  G_POSSIBLE = 0x1,
  /* backgammon possible by side on roll */
  BG_POSSIBLE = 0x2,
  
  /* gammon possible by side not on roll */
  OG_POSSIBLE = 0x4,
  
  /* backgammon possible by side not on roll */
  OBG_POSSIBLE = 0x8
};

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <random>
using namespace std;

class BgBoard;
class BgEval
{
public:
	static int anChequers[ NUM_VARIATIONS ];

	static BgEval *Instance();
	static void Destroy();

	bearoffcontext *pbcOS;
	bearoffcontext *pbcTS;
	bearoffcontext *pbc1;
	bearoffcontext *pbc2;
	bearoffcontext *apbcHyper[ 3 ];
	
	//initialization
	bool load(const char *path);

	//validation
	void SanityCheck(const BgBoard *board, BgReward& reward) const;

	//classification
	positionclass ClassifyPosition(const BgBoard *board, const bgvariation bgv) const;

	//Bearoff evaluation
	int EvalOver( const BgBoard *anBoard, BgReward& arOutput, const bgvariation bgv) const;
	int EvalHypergammon1(const BgBoard *anBoard, BgReward& arOutput) const;
	int EvalHypergammon2(const BgBoard *anBoard, BgReward& arOutput) const;
	int EvalHypergammon3(const BgBoard *anBoard, BgReward& arOutput) const;
	int EvalBearoff2(const BgBoard *anBoard, BgReward& arOutput) const;
	int EvalBearoffTS(const BgBoard *anBoard, BgReward& arOutput) const;
	int EvalBearoff1(const BgBoard *anBoard, BgReward& arOutput) const;
	int EvalBearoffOS(const BgBoard *anBoard, BgReward& arOutput) const;

	//side - side that potentially can win a backgammon
	//Return - Probablity that side will win a backgammon
	float raceBGprob(const BgBoard *anBoard, const int side, const bgvariation bgv) const;

	mt19937& getRng() { return m_rng; }
	const fs::path getBasePath() const {return m_basePath;}
	const int nextDice() { return m_distribution(m_rng); }

private:
	BgEval();
	~BgEval();

	fs::path m_basePath;
	mt19937 m_rng;
	uniform_int_distribution<int> m_distribution;

	// An upper bound on the number of turns it can take to complete a bearoff
    //from bearoff position ID i. 
	int MaxTurns( int id ) const;

	static BgEval *m_instance;
	static void BearoffProgress( unsigned int i );
};

#endif
