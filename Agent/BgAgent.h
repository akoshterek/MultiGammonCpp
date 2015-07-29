#if !defined __BG_AGENT_H
#define __BG_AGENT_H
#pragma once

#include <string>
#include <exception>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "BgReward.h"
#include "BgEval.h"
#include "BgMove.h"

class BgAgent
{
public:
	BgAgent(fs::path path);
	virtual ~BgAgent(void) {}

	std::string getFullName() const {return m_fullName;}
	fs::path getPath() const {return m_path;}
	int getPlayedGames() const {return m_playedGames;}

	virtual void startGame(bgvariation bgv) {m_bgv = bgv;}
	virtual void endGame() { if(m_learnMode) m_playedGames++;}
	virtual void doMove(const bgmove& pm) {}
	void setCurrentBoard(const BgBoard *board) {m_curBoard = board;}
	
	virtual void evaluatePosition(const BgBoard *board, positionclass& pc, BgReward& reward);
	virtual void evalOver(const BgBoard *board, BgReward& reward);
	virtual void evalHypergammon1(const BgBoard *board, BgReward& reward);
	virtual void evalHypergammon2(const BgBoard *board, BgReward& reward);
	virtual void evalHypergammon3(const BgBoard *board, BgReward& reward);
	virtual void evalBearoff2(const BgBoard *board, BgReward& reward);
	virtual void evalBearoffTS(const BgBoard *board, BgReward& reward);
	virtual void evalBearoff1(const BgBoard *board, BgReward& reward);
	virtual void evalBearoffOS(const BgBoard *board, BgReward& reward);
	virtual void evalRace(const BgBoard *board, BgReward& reward) {throw std::exception("not implemented");}
	virtual void evalCrashed(const BgBoard *board, BgReward& reward) {throw std::exception("not implemented");}
	virtual void evalContact(const BgBoard *board, BgReward& reward) {throw std::exception("not implemented");}
	
	bool isLearnMode() const {return m_learnMode;}
	void setLearnMode(bool learn) {m_learnMode = learn;}
	//fixed means it's unable to learn
	bool isFixed() const {return m_isFixed;}
	bool needsInvertedEval() const {return m_needsInvertedEval;}
	bool supportsSanityCheck() const {return m_supportsSanityCheck;}
	void setSanityCheck(bool sc) {m_supportsSanityCheck = sc;}

	virtual bool isCloneable() const {return false;}
	virtual BgAgent *clone() {return NULL;}
	virtual void load() {}
	virtual void save() {}

protected:
	bool m_learnMode, m_supportsSanityCheck, m_isFixed, m_needsInvertedEval;
	std::string m_fullName;
	int m_playedGames;
	fs::path m_path;
	bgvariation m_bgv;

	const BgBoard *m_curBoard;
};

#endif
