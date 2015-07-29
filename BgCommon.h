#if !defined __BG_COMMON_H
#define __BG_COMMON_H
#pragma once

#define NUM_OUTPUTS 5
#define NUM_ROLLOUT_OUTPUTS 6

#define OUTPUT_WIN 0
#define OUTPUT_WINGAMMON 1
#define OUTPUT_WINBACKGAMMON 2
#define OUTPUT_LOSEGAMMON 3
#define OUTPUT_LOSEBACKGAMMON 4

//NB: neural nets do not output equity, only rollouts do
#define OUTPUT_EQUITY 5  

enum bgvariation 
{
	VARIATION_STANDARD,	/* standard backgammon */
	VARIATION_NACKGAMMON,	/* standard backgammon with nackgammon starting position */
	VARIATION_HYPERGAMMON_1,	/* 1-chequer hypergammon */
	VARIATION_HYPERGAMMON_2,	/* 2-chequer hypergammon */
	VARIATION_HYPERGAMMON_3,	/* 3-chequer hypergammon */
	NUM_VARIATIONS
};

#if defined(HUGE_VAL)
#define ERR_VAL (-HUGE_VAL)
#else
#define ERR_VAL (-FLT_MAX)
#endif

#ifndef SGN
#define SGN(x) ((x) > 0 ? 1 : -1)
#endif


#endif
