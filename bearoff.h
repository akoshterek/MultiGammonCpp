/*
 * bearoff.h
 *
 * by Joern Thyssen <jthyssen@dk.ibm.com>, 2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: bearoff.h,v 1.27 2008/12/05 09:32:16 c_anthon Exp $
 */

#ifndef _BEAROFF_H_
#define _BEAROFF_H_

#include <stdio.h>
#include "BgReward.h"

#include <boost/iostreams/device/mapped_file.hpp>
namespace io = boost::iostreams;

enum bearofftype 
{
  BEAROFF_INVALID,
  BEAROFF_ONESIDED,
  BEAROFF_TWOSIDED,
  BEAROFF_HYPERGAMMON
};

struct bearoffcontext
{
	bearoffcontext()
	{
		pf = NULL;
		szFilename = NULL;
		p = NULL;
	}
	~bearoffcontext()
	{
		if(map.is_open())
			map.close();
	}

	FILE *pf;          /* file pointer */
	bearofftype bt; /* type of bearoff database */
	unsigned int nPoints;    /* number of points covered by database */
	unsigned int nChequers;  /* number of chequers for one-sided database */
	char *szFilename; /* filename */
	/* one sided dbs */
	int fCompressed; /* is database compressed? */
	int fGammon;     /* gammon probs included */
	int fND;         /* normal distibution instead of exact dist? */
	int fHeuristic;  /* heuristic database? */
	/* two sided dbs */
	int fCubeful;    /* cubeful equities included */
	io::mapped_file_source map;
	unsigned char *p;        /* pointer to data in memory */
	unsigned long int nReads; /* number of reads */
};

enum _bearoffoptions 
{
  BO_NONE              = 0,
  BO_IN_MEMORY         = 1,
  BO_MUST_BE_ONE_SIDED = 2,
  BO_MUST_BE_TWO_SIDED = 4,
  BO_HEURISTIC         = 8
};


class BgBoard;
bearoffcontext *BearoffInit ( const char *szFilename, const int bo, void (*p)(unsigned int) );

//bearoffcontext *BearoffInitBuiltin ( void );

int BearoffEval ( const bearoffcontext *pbc, const BgBoard * anBoard, BgReward& arOutput );

extern void
BearoffStatus ( const bearoffcontext *pbc, char *sz );

extern int
BearoffDump ( const bearoffcontext *pbc, const BgBoard * anBoard, char *sz );

extern int
BearoffDist ( const bearoffcontext *pbc, const unsigned int nPosID,
              float arProb[ 32 ], float arGammonProb[ 32 ],
              float ar[ 4 ],
              unsigned short int ausProb[ 32 ], 
              unsigned short int ausGammonProb[ 32 ] );

extern int
BearoffCubeful ( const bearoffcontext *pbc,
                 const unsigned int iPos,
                 float ar[ 4 ], unsigned short int aus[ 4 ] );

extern void BearoffClose ( bearoffcontext *ppbc );

bool isBearoff ( const bearoffcontext *pbc, const BgBoard * anBoard );

extern float
fnd ( const float x, const float mu, const float sigma  );

extern int
BearoffHyper( const bearoffcontext *pbc,
              const unsigned int iPos,
              BgReward& arOutput, float arEquity[] );

#endif /* _BEAROFF_H_ */
