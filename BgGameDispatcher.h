#if !defined __BGGAMEDISPATCHER_H
#define __BGGAMEDISPATCHER_H
#pragma once

#include <list>
#include "Agent/BgAgent.h"
#include "BgMatch.h"

class BgGameDispatcher
{
public:
	BgGameDispatcher(BgAgent *agent1, BgAgent *agent2);
	~BgGameDispatcher(void);

	void playGames(int numGames, bool learn = true);
	void printStatistics();

	bool isShowLog() const {return m_isShowLog;}
	void setShowLog(bool show) {m_isShowLog = show;}

	void swapAgents();

	//export
	void ExportMatchMat( char *sz, bool fSst ) const;

private:
	static const char *aszGameResult[];

	BgAgent *m_agents[2];
	int m_wonGames[2];
	int m_wonPoints[2];
	bool m_learnMode;
	bool m_isShowLog;
	int m_numGames;
	matchstate m_currentMatch;
	std::list<std::list<moverecord> > m_lMatch;
	moverecord *pmr_hint;
	bgmove m_amMoves[movelist::MAX_INCOMPLETE_MOVES];

	bool m_fAutoCrawford;

	void playGame();
	void startGame(bgvariation bgv);
	void nextTurn();
	void computerTurn();
	void showScore() const;

	void FixMatchState(const moverecord *pmr);
	moverecord *get_current_moverecord(int *pfHistory);
	void add_moverecord_sanity_check(const moverecord *pmr) const;
	void copy_from_pmr_cur(moverecord *pmr, bool get_move, bool get_cube);
	void add_moverecord_get_cur(moverecord *pmr);
	void DiceRolled() const;
	void ShowBoard() const;

	void AddMoveRecord(  moverecord *pmr );
	void ApplyMoveRecord(const moverecord* pmr);
	void ApplyGameOver();
	void PlayMove(const ChequerMove& anMove, int const fPlayer);
	void RefreshMoveList (movelist& pml, int *ai) const;
	void ShowAutoMove(const ChequerMove& anMove) const;
	int FindMove(findData *pfd);
	int FindnSaveBestMoves( movelist *pml, int nDice0, int nDice1,
		const BgBoard& anBoard, unsigned char *auchMove, const
		float rThr, const cubeinfo* pci);
	int ScoreMoves( movelist *pml) const;
	int ScoreMove(bgmove& pm) const;
	int GeneralEvaluationEPlied (BgReward& arOutput, const BgBoard *anBoard, positionclass& pc ) const;
	int EvaluatePositionFull(const BgBoard *anBoard, BgReward& arOutput, positionclass& pc) const;

	//export-import
	void ExportGameJF( FILE *pf, const std::list<moverecord>& plGame, int iGame, bool withScore, bool fSst ) const;
	void ExportSnowieTxt (char *sz, const matchstate *pms) const;
};

#endif
