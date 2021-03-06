/*
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
 * neuralnet.h
 *
 * by Gary Wong, 1998
 * $Id: neuralnet.h,v 1.24 2011/02/08 22:41:38 plm Exp $
 */

#ifndef _NEURALNET_H_
#define _NEURALNET_H_
#pragma once

#include <stdio.h>

struct neuralnet 
{
	unsigned int cInput;
	unsigned int cHidden;
	unsigned int cOutput;
	unsigned int fDirect;
	int nTrained;
	float rBetaHidden;
	float rBetaOutput;
	float *arHiddenWeight;
	float *arOutputWeight;
	float *arHiddenThreshold;
	float *arOutputThreshold;
} ;

enum NNEvalType
{
	NNEVAL_NONE,
	NNEVAL_SAVE,
	NNEVAL_FROMBASE
};

enum NNStateType
{
	NNSTATE_NONE = -1,
	NNSTATE_INCREMENTAL,
	NNSTATE_DONE
};

struct NNState 
{
	NNStateType state;
	float *savedBase;
	float *savedIBase;
};

extern int NeuralNetCreate(neuralnet *pnn, unsigned int cInput, unsigned int cHidden, unsigned int cOutput, float rBetaHidden, float rBetaOutput);
extern void NeuralNetDestroy(neuralnet *pnn);
extern int NeuralNetEvaluate(const neuralnet *pnn, float arInput[], float arOutput[], NNState *pnState);
extern int NeuralNetEvaluateSSE(const neuralnet *pnn, float arInput[], float arOutput[], NNState *pnState);
extern int NeuralNetResize(neuralnet *pnn, unsigned int cInput, unsigned int cHidden, unsigned int cOutput);
extern int NeuralNetLoad(neuralnet *pnn, FILE *pf);
extern int NeuralNetLoadBinary(neuralnet *pnn, FILE *pf);
extern int NeuralNetSaveBinary(const neuralnet *pnn, FILE *pf);
extern int SSE_Supported(void);

#endif
