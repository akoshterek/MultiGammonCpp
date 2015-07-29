#if !defined __BG_GNUBG_H
#define __BG_GNUBG_H

#pragma once
#include "BgAgent.h"
#include "gnunn/neuralnet.h"

class GnubgAgent : public BgAgent
{
public:
	GnubgAgent(fs::path path);
	virtual ~GnubgAgent(void);

	virtual void evalRace(const BgBoard *board, BgReward& reward);
	virtual void evalCrashed(const BgBoard *board, BgReward& reward);
	virtual void evalContact(const BgBoard *board, BgReward& reward);

private:
	neuralnet nnContact, nnRace, nnCrashed;
	//neuralnet nnpContact, nnpRace, nnpCrashed;
	int anEscapes[ 0x1000 ];
	int anEscapes1[ 0x1000 ];

	void load(fs::path binPath, fs::path txtPath);
	int binary_weights_failed(const char * filename, FILE * weights);
	int weights_failed(const char * filename, FILE * weights);
	void PrintError(const char* str);

	void CalculateRaceInputs(const BgBoard *anBoard, float inputs[]) const;
	void CalculateCrashedInputs(const BgBoard *anBoard, float inputs[]) const;
	void CalculateContactInputs(const BgBoard *anBoard, float arInput[]) const;
	void CalculateHalfInputs( const char anBoard[ 25 ], const char anBoardOpp[ 25 ], float afInput[] ) const;
	void baseInputs(const BgBoard *anBoard, float arInput[]) const;
	void menOffAll(const char* anBoard, float* afInput) const;
	void menOffNonCrashed(const char* anBoard, float* afInput) const;

	void ComputeTable0( void );
	int Escapes( const char anBoard[ 25 ], int n ) const;
	void ComputeTable1( void );
	int Escapes1( const char anBoard[ 25 ], int n ) const;
	void ComputeTable( void );
};

#endif
