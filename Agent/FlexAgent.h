#if !defined __BG_FLEX_H
#define __BG_FLEX_H

#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/version.hpp>

#include "BgAgent.h"
#include "FannFA.h"
#include "InputRepresentation.h"
#include "HeuristicAgent.h"

struct ETraceEntry
{
	ETraceEntry() : pc(CLASS_CONTACT) {}
	
	std::vector<float> input;
	positionclass pc;
	AuchKey auch;
};

class FlexAgent : public BgAgent
{
public:
	FlexAgent(fs::path path, std::shared_ptr<InputRepresentation> representation, std::string name);
	virtual ~FlexAgent(void);

	virtual void evaluatePosition(const BgBoard *board, positionclass& pc, BgReward& reward);
	virtual void evalRace(const BgBoard *board, BgReward& reward);
	virtual void evalCrashed(const BgBoard *board, BgReward& reward);
	virtual void evalContact(const BgBoard *board, BgReward& reward);

	virtual void startGame(bgvariation bgv);
	virtual void endGame();
	virtual void doMove(const bgmove& pm);

	virtual bool isCloneable() const {return true;}
	virtual BgAgent *clone();
	bool loadNN(ApproxType annType);
	virtual void load();
	virtual void save();

	void createContactFA(int input, int hidden, int output, ApproxType annType);
	void createRaceFA(int input, int hidden, int output, ApproxType annType);
	void createCrashedFA(int input, int hidden, int output, ApproxType annType);

    //friend class boost::serialization::access;
    //friend std::ostream & operator<<(std::ostream &os, const FlexAgent &fa);
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */)
	{
        ar  & boost::serialization::make_nvp("SupportsSanityCheck", m_supportsSanityCheck);
		ar	& boost::serialization::make_nvp("Alpha", m_alpha);
		ar	& boost::serialization::make_nvp("AlphaAnnealFactor", m_alphaAnnealFactor);
		ar	& boost::serialization::make_nvp("Gamma", m_gamma);
		ar	& boost::serialization::make_nvp("Lambda", m_lambda);
		ar	& boost::serialization::make_nvp("LearnMode", m_learnMode);
		ar	& boost::serialization::make_nvp("PlayedGames", m_playedGames);
    }


	float getAlpha() const {return m_alpha;}
	void setAlpha(float v) {m_alpha = filterVal(v);}
	float getAlphaAnnealFactor() const {return m_alphaAnnealFactor;}
	void setAlphAnnealFactora(float v) {m_alphaAnnealFactor = filterVal(v);}
	float getGamma() const {return m_gamma;}
	void setGamma(float v) {m_gamma = filterVal(v);}
	float getLambda() const {return m_lambda;}
	void setLambda(float v) {m_lambda = filterVal(v);}

private:
	FlexAgent(fs::path path) : BgAgent(path) {}

    float m_alpha; //Learning rate
    float m_alphaAnnealFactor;
    float m_gamma; //eligibility trace discounting
    float m_lambda; //discounting
	float filterVal(float val)
	{
		assert(val >= 0 && val <= 1);
		return val;
	}

	std::shared_ptr<InputRepresentation> m_representation;
	std::shared_ptr<FunctionApproximator> m_nnContact, m_nnRace, m_nnCrashed;

	std::list<ETraceEntry> m_eligibilityTraces;
	ETraceEntry m_prevEntry;
	size_t m_step;

	FunctionApproximator *getQ(positionclass pc) const;
	void UpdateETrace(BgReward deltaReward);
	void prepareStep0(const bgmove& pm);
	//Q update rule
	BgReward calcDeltaReward(const bgmove& pm, const BgReward& reward);

	std::auto_ptr<HeuristicAgent> m_heuristic;
};

#endif
