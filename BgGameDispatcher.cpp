#include "BgGameDispatcher.h"
#include "matchid.h"
#include "PositionId.h"

const char *BgGameDispatcher::aszGameResult[] = { "single game", "gammon", "backgammon" };
typedef int ( *cfunc )( const void *, const void *);

BgGameDispatcher::BgGameDispatcher(BgAgent *agent1, BgAgent *agent2)
{
	assert(agent1);
	assert(agent2);

	m_agents[0] = agent1;
	m_agents[1] = agent2;

	m_wonGames[0] = m_wonGames[1] = 0;
	m_wonPoints[0] = m_wonPoints[1] = 0;
	m_numGames = 0;
	pmr_hint = NULL;

	m_learnMode = false;
	m_isShowLog = false;
	m_fAutoCrawford = true;

	//m_amMoves.resize(movelist::MAX_INCOMPLETE_MOVES);
}

BgGameDispatcher::~BgGameDispatcher(void)
{
}

void BgGameDispatcher::swapAgents()
{
	std::swap(m_agents[0], m_agents[1]);
	std::swap(m_wonGames[0], m_wonGames[1]);
	std::swap(m_wonPoints[0], m_wonPoints[1]);
}

void BgGameDispatcher::playGames(int numGames, bool learn)
{
	m_learnMode = learn;

	m_agents[0]->setLearnMode(learn);
	m_agents[1]->setLearnMode(learn);

	for(int i = m_numGames+1; i < m_numGames + numGames + 1; i++)
	{
		playGame();	
		if(i % 100 == 0)
			printf("%d ", i);
		//if(i % 20 == 0) printf("\n");

		//ExportMatchMat("-", true);
	}

	m_numGames += numGames;
}

void BgGameDispatcher::playGame()
{
	for(int i = 0; i < 2; i++)
		m_agents[i]->startGame(VARIATION_STANDARD);

	startGame(VARIATION_STANDARD);
	//main game loop
	do
	{
		nextTurn();
	} 
	while (m_currentMatch.gs == GAME_PLAYING);

	for(int i = 0; i < 2; i++)
		m_agents[i]->endGame();
	
	for(int i = 0; i < 2; i++)
	{
		if(m_currentMatch.anScore[i])
			m_wonGames[i]++;
		m_wonPoints[i] += m_currentMatch.anScore[i];
	}
}

void BgGameDispatcher::printStatistics()
{
	printf("\n\tStatistics after %d game(s)\n", m_numGames);
	char signs[2] = {'O', 'X'};
	for(int i = 0; i < 2; i++)
	{
		printf("%c:%s: games %d/%d = %5.2f%%, points %d = %5.2f%%\n", 
			signs[i], m_agents[i]->getFullName().c_str(), 
			m_wonGames[i], m_numGames, 
			float(m_wonGames[i]) / m_numGames * 100, m_wonPoints[i],
			float(m_wonPoints[i]) / (m_wonPoints[0] + m_wonPoints[1]) * 100);
	}

	printf("%c:%s: won %+5.3f ppg\n", signs[0], m_agents[0]->getFullName().c_str(), 
		float(m_wonPoints[0] - m_wonPoints[1]) / m_numGames);

	fs::path logPath =  m_agents[0]->getPath();
	logPath /= m_agents[0]->getFullName() + " vs " + m_agents[1]->getFullName() + ".csv";
	FILE *f = fopen(logPath.string().c_str(), "at");
	if(f)
	{
		fprintf(f, "%d;%f\n",  m_agents[0]->getPlayedGames(), float(m_wonPoints[0] - m_wonPoints[1]) / m_numGames);
		fclose(f);
	}
}

void BgGameDispatcher::startGame(bgvariation bgv)
{
	m_currentMatch = matchstate();
	matchstate& ms = m_currentMatch;
	ms.bgv = bgv;

	ms.anBoard.InitBoard(ms.bgv);

    m_lMatch.clear();
	m_lMatch.push_back(std::list<moverecord>());

	{
		moverecord pmr;
		pmr.mt = MOVE_GAMEINFO;
		//pmr.sz = NULL;

		pmr.g.i = ms.cGames;
		pmr.g.nMatch = ms.nMatchTo;
		pmr.g.anScore[ 0 ] = ms.anScore[ 0 ];
		pmr.g.anScore[ 1 ] = ms.anScore[ 1 ];
		pmr.g.fCrawford = m_fAutoCrawford && ms.nMatchTo > 1;
		pmr.g.fCrawfordGame = ms.fCrawford;
		pmr.g.fJacoby = ms.fJacoby && !ms.nMatchTo;
		pmr.g.fWinner = -1;
		pmr.g.nPoints = 0;
		pmr.g.fResigned = false;
		pmr.g.nAutoDoubles = 0;
		pmr.g.bgv = ms.bgv;
		pmr.g.fCubeUse = ms.fCubeUse;
		//pmr.g.sc = statcontext();
		AddMoveRecord( &pmr );
	}
    
    do
	{
		ms.rollDice();
		if( m_isShowLog ) 
		{
			printf("%s rolls %d, %s rolls %d.\n", 
				m_agents[0]->getFullName().c_str(), ms.anDice[ 0 ],
				m_agents[1]->getFullName().c_str(), ms.anDice[ 1 ] );
		}
    } while(ms.anDice[ 1 ] == ms.anDice[ 0 ]);

	{
		moverecord pmr;
		pmr.mt = MOVE_SETDICE;

		pmr.anDice[ 0 ] = ms.anDice[ 0 ];
		pmr.anDice[ 1 ] = ms.anDice[ 1 ];
		pmr.fPlayer = ms.anDice[ 1 ] > ms.anDice[ 0 ];

		AddMoveRecord( &pmr );
	}

	DiceRolled();
}

void BgGameDispatcher::DiceRolled() const
{
	if(m_isShowLog)
		ShowBoard();
}

void BgGameDispatcher::ShowBoard() const
{
    char szBoard[ 2048 ];
    char sz[ 50 ], szCube[ 50 ], szScore0[ 50 ], szScore1[ 50 ], szMatch[ 50 ], szPlayer0[ 32 + 3 ], szPlayer1[ 32 + 3 ];
    char *apch[ 7 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    //moverecord *pmr;

	BgBoard an(m_currentMatch.anBoard);
	if( !m_currentMatch.fMove )
		an.SwapSides();

    sprintf_s( szPlayer0, 35, "O: %s", m_agents[0]->getFullName().c_str());
    sprintf_s( szPlayer1, 35, "X: %s", m_agents[1]->getFullName().c_str());
	apch[ 0 ] = szPlayer0;
	apch[ 6 ] = szPlayer1;

	sprintf_s(apch[ 1 ] = szScore0, 50, "%d point(s)", m_currentMatch.anScore[ 0 ] );
	sprintf_s(apch[ 5 ] = szScore1, 50, "%d point(s)", m_currentMatch.anScore[ 1 ] );

	if( m_currentMatch.fDoubled ) 
	{
	    apch[ m_currentMatch.fTurn ? 4 : 2 ] = szCube;
	    sprintf( szCube, "Cube offered at %d", m_currentMatch.nCube << 1 );
	} 
	else 
	{
	    apch[ m_currentMatch.fMove ? 4 : 2 ] = sz;
	
	    if( m_currentMatch.anDice[ 0 ] )
			sprintf_s( sz, 50, "%s %d%d", "Rolled", m_currentMatch.anDice[ 0 ], m_currentMatch.anDice[ 1 ] );
	    else 
		if( !m_currentMatch.anBoard.GameStatus( m_currentMatch.bgv ) )
			strcpy_s( sz, 50, "On roll" );
	    else
			sz[ 0 ] = 0;
	    
	    if( m_currentMatch.fCubeOwner < 0 ) 
		{
			apch[ 3 ] = szCube;

			if( m_currentMatch.nMatchTo )
				sprintf_s( szCube, 50,
					"%d point match (Cube: %d)", m_currentMatch.nMatchTo,
					m_currentMatch.nCube );
			else
				sprintf (szCube, "(%s: %d)", "Cube", m_currentMatch.nCube);
	    } 
		else 
		{
			int cch = std::min(20, int(m_agents[m_currentMatch.fCubeOwner]->getFullName().size()));
					sprintf_s( szCube, 50, "%c: %*s (%s: %d)", m_currentMatch.fCubeOwner ? 'X' : 'O',
				cch, m_agents[m_currentMatch.fCubeOwner]->getFullName().c_str(), "Cube", m_currentMatch.nCube );

			apch[ m_currentMatch.fCubeOwner ? 6 : 0 ] = szCube;

			if( m_currentMatch.nMatchTo )
				sprintf_s( apch[ 3 ] = szMatch, 50, "%d point match",  m_currentMatch.nMatchTo );
	    }
	}
	if( m_currentMatch.fResigned > 0 )
	{
	    /* FIXME it's not necessarily the player on roll that resigned */
	    sprintf_s( strchr( sz, 0 ), 50, ", %s %s", "resigns", aszGameResult[ m_currentMatch.fResigned - 1 ] );
	}
	
	printf("%s\n", an.DrawBoard( szBoard, m_currentMatch.fMove, apch,
                            MatchIDFromMatchState ( &m_currentMatch ), 
                            BgEval::anChequers[ m_currentMatch.bgv ] ) );

	if (m_lMatch.size() && m_lMatch.back().size())
	{
		const moverecord *pmr = &m_lMatch.back().back();
		/* FIXME word wrap */
	    if( pmr->sz.size() )
			printf("%s\n", pmr->sz.c_str() ); 
	    //if( pmr->sz )
		//	printf("%s\n", pmr->sz );
	}
}

void BgGameDispatcher::AddMoveRecord(  moverecord *pmr )
{
    moverecord *pmrOld;

    add_moverecord_get_cur(pmr);
    add_moverecord_sanity_check(pmr);


    if(  m_lMatch.size() && m_lMatch.back().size() && pmr->mt == MOVE_NORMAL && 
		( pmrOld = &m_lMatch.back().back() )->mt == MOVE_SETDICE &&	pmrOld->fPlayer == pmr->fPlayer )
	{
	    m_lMatch.back().pop_back();
	}

    /* FIXME perform other elision (e.g. consecutive "set" records) */
    FixMatchState ( pmr );
	m_lMatch.back().push_back(*pmr);
    ApplyMoveRecord( pmr );
}

void BgGameDispatcher::ApplyMoveRecord(const moverecord* pmr)
{
	std::list<moverecord>& lGame = m_lMatch.back();

    int n;
    moverecord *pmrx = &(*lGame.begin());
    xmovegameinfo* pmgi;
    /* FIXME this is wrong -- plGame is not necessarily the right game */

    assert( pmr->mt == MOVE_GAMEINFO || pmrx->mt == MOVE_GAMEINFO );
    pmgi = &pmrx->g;
    
    m_currentMatch.gs = GAME_PLAYING;
       m_currentMatch.fResigned = m_currentMatch.fResignationDeclined = 0;

    switch( pmr->mt ) 
	{
    case MOVE_GAMEINFO:
		m_currentMatch.anBoard.InitBoard( pmr->g.bgv );

		m_currentMatch.nMatchTo = pmr->g.nMatch;
		m_currentMatch.anScore[ 0 ] = pmr->g.anScore[ 0 ];
		m_currentMatch.anScore[ 1 ] = pmr->g.anScore[ 1 ];
		m_currentMatch.cGames = pmr->g.i;
	
		m_currentMatch.gs = GAME_NONE;
		m_currentMatch.fMove = m_currentMatch.fTurn = m_currentMatch.fCubeOwner = -1;
		m_currentMatch.anDice[ 0 ] = m_currentMatch.anDice[ 1 ] = m_currentMatch.cBeavers = 0;
		m_currentMatch.fDoubled = FALSE;
		m_currentMatch.nCube = 1 << pmr->g.nAutoDoubles;
		m_currentMatch.fCrawford = pmr->g.fCrawfordGame;
		m_currentMatch.fPostCrawford = !m_currentMatch.fCrawford &&
			( m_currentMatch.anScore[ 0 ] == m_currentMatch.nMatchTo - 1 ||
			  m_currentMatch.anScore[ 1 ] == m_currentMatch.nMatchTo - 1 );
		m_currentMatch.bgv = pmr->g.bgv;
		m_currentMatch.fCubeUse = pmr->g.fCubeUse;
		m_currentMatch.fJacoby = pmr->g.fJacoby;
	break;
	
    case MOVE_DOUBLE:
		if( m_currentMatch.fMove < 0 )
			m_currentMatch.fMove = pmr->fPlayer;
	
		if( m_currentMatch.fDoubled ) 
		{
			m_currentMatch.cBeavers++;
			m_currentMatch.nCube <<= 1;
			m_currentMatch.fCubeOwner = !m_currentMatch.fMove;
		} else
			m_currentMatch.fDoubled = TRUE;
	
		m_currentMatch.fTurn = !pmr->fPlayer;
		break;

    case MOVE_TAKE:
		if( !m_currentMatch.fDoubled )
			break;
	
		m_currentMatch.nCube <<= 1;
		m_currentMatch.cBeavers = 0;
		m_currentMatch.fDoubled = FALSE;
		m_currentMatch.fCubeOwner = !m_currentMatch.fMove;
		m_currentMatch.fTurn = m_currentMatch.fMove;
		break;

    case MOVE_DROP:
		if( !m_currentMatch.fDoubled )
			break;
	
		m_currentMatch.fDoubled = FALSE;
		m_currentMatch.cBeavers = 0;
		m_currentMatch.gs = GAME_DROP;
		pmgi->nPoints = m_currentMatch.nCube;
		pmgi->fWinner = !pmr->fPlayer;
		pmgi->fResigned = FALSE;
	
		ApplyGameOver();
		break;

    case MOVE_NORMAL:
		m_currentMatch.fDoubled = FALSE;

		PlayMove( pmr->n.anMove, pmr->fPlayer );
		m_currentMatch.anDice[ 0 ] = m_currentMatch.anDice[ 1 ] = 0;

		if( ( n = m_currentMatch.anBoard.GameStatus( m_currentMatch.bgv ) ) ) 
		{
			if( m_currentMatch.fJacoby && m_currentMatch.fCubeOwner == -1 && ! m_currentMatch.nMatchTo )
				  /* gammons do not count on a centred cube during money
					 sessions under the Jacoby rule */
					n = 1;

			m_currentMatch.gs = GAME_OVER;
			pmgi->nPoints = m_currentMatch.nCube * n;
			pmgi->fWinner = pmr->fPlayer;
			pmgi->fResigned = FALSE;
			ApplyGameOver();
		}
		break;

    case MOVE_RESIGN:
		m_currentMatch.gs = GAME_RESIGNED;
		pmgi->nPoints = m_currentMatch.nCube * ( m_currentMatch.fResigned = pmr->r.nResigned );
		pmgi->fWinner = !pmr->fPlayer;
		pmgi->fResigned = TRUE;
	
		ApplyGameOver();
		break;

    case MOVE_SETBOARD:
		m_currentMatch.anBoard = BgBoard::PositionFromKey( pmr->sb.auchKey );

		if( m_currentMatch.fMove < 0 )
			m_currentMatch.fTurn = m_currentMatch.fMove = 0;
	
		if( m_currentMatch.fMove )
			 m_currentMatch.anBoard.SwapSides();
		break;
	
    case MOVE_SETDICE:
		m_currentMatch.anDice[ 0 ] = pmr->anDice[ 0 ];
		m_currentMatch.anDice[ 1 ] = pmr->anDice[ 1 ];
		if( m_currentMatch.fMove != pmr->fPlayer )
			m_currentMatch.anBoard.SwapSides();
		m_currentMatch.fTurn = m_currentMatch.fMove = pmr->fPlayer;
		m_currentMatch.fDoubled = FALSE;
		break;
	
    case MOVE_SETCUBEVAL:
		if( m_currentMatch.fMove < 0 )
			m_currentMatch.fMove = 0;
	
		m_currentMatch.nCube = pmr->scv.nCube;
		m_currentMatch.fDoubled = FALSE;
		m_currentMatch.fTurn = m_currentMatch.fMove;
		break;
	
    case MOVE_SETCUBEPOS:
		if( m_currentMatch.fMove < 0 )
			m_currentMatch.fMove = 0;
	
		m_currentMatch.fCubeOwner = pmr->scp.fCubeOwner;
		m_currentMatch.fDoubled = FALSE;
		m_currentMatch.fTurn = m_currentMatch.fMove;
		break;
    }
}

void BgGameDispatcher::ApplyGameOver()
{
	moverecord& pmr = m_lMatch.back().front();
	xmovegameinfo& pmgi = pmr.g;

	assert( pmr.mt == MOVE_GAMEINFO );

	if( pmgi.fWinner < 0 )
		return;
    
	int n = m_currentMatch.anBoard.GameStatus(m_currentMatch.bgv);
	m_currentMatch.anScore[ pmgi.fWinner ] += pmgi.nPoints;
	m_currentMatch.cGames++;
	if (m_isShowLog)
	{
		printf("\nEnd Game done.\n%c:%s wins a %s and %d point(s)\n", 
			pmgi.fWinner ? 'X' : 'O',
			m_agents[pmgi.fWinner]->getFullName().c_str(), 
			aszGameResult[ n - 1 ], pmgi.nPoints);
	}
}

void BgGameDispatcher::PlayMove(const ChequerMove& anMove, int const fPlayer)
{
	if( m_currentMatch.fMove != -1 && fPlayer != m_currentMatch.fMove )
		 m_currentMatch.anBoard.SwapSides();
    
	for(int i = 0; i < 8; i += 2 ) 
	{
		int nSrc = anMove[ i ];
		int nDest = anMove[ i + 1 ];

		if( nSrc < 0 )
			/* move is finished */
			break;
	
		if( !m_currentMatch.anBoard.anBoard[ 1 ][ nSrc ] )
			/* source point is empty; ignore */
			continue;

		m_currentMatch.anBoard.anBoard[ 1 ][ nSrc ]--;
		if( nDest >= 0 )
			m_currentMatch.anBoard.anBoard[ 1 ][ nDest ]++;

		if( nDest >= 0 && nDest <= 23 ) 
		{
			m_currentMatch.anBoard.anBoard[ 0 ][ 24 ] += m_currentMatch.anBoard.anBoard[ 0 ][ 23 - nDest ];
			m_currentMatch.anBoard.anBoard[ 0 ][ 23 - nDest ] = 0;
		}
	}

	m_currentMatch.fMove = m_currentMatch.fTurn = !fPlayer;
	m_currentMatch.anBoard.SwapSides();
}

void BgGameDispatcher::nextTurn()
{
	matchstate& ms = m_currentMatch;
	
    if ( ms.anBoard.GameStatus( m_currentMatch.bgv ) || ms.gs == GAME_DROP || ms.gs == GAME_RESIGNED)
	{
		moverecord *pmr = &m_lMatch.back().back();
		xmovegameinfo *pmgi = &pmr->g;

	    int n;
		if( ms.fJacoby && ms.fCubeOwner == -1 && !ms.nMatchTo )
			/* gammons do not count on a centred cube during money
			   sessions under the Jacoby rule */
			n = 1;
		else if (ms.gs == GAME_DROP)
			n = 1;
		else if (ms.gs == GAME_RESIGNED)
			n = ms.fResigned;
		else
			n = ms.anBoard.GameStatus(ms.bgv);

		printf( "%s wins a %s and %d point(s).\n",
			m_agents[ pmgi->fWinner ]->getFullName().c_str(),
			aszGameResult[ n - 1 ], pmgi->nPoints);

		if( ms.nMatchTo && m_fAutoCrawford ) 
		{
			ms.fPostCrawford |= ms.fCrawford &&
			ms.anScore[ pmgi->fWinner ] < ms.nMatchTo;
			ms.fCrawford = !ms.fPostCrawford && !ms.fCrawford &&
			ms.anScore[ pmgi->fWinner ] == ms.nMatchTo - 1 &&
			ms.anScore[ !pmgi->fWinner ] != ms.nMatchTo - 1;
		}

	    if(m_isShowLog)
			showScore();

		if( ms.nMatchTo && ms.anScore[ pmgi->fWinner ] >= ms.nMatchTo )
		{
		    if(m_isShowLog)
			{
				printf( "%s has won the match.\n", m_agents[ pmgi->fWinner ]->getFullName().c_str() );
				ShowBoard();
			}
			return;
		}
	}

    assert( m_currentMatch.gs == GAME_PLAYING );
    
    //if(m_isShowLog)
	//	ShowBoard();
   
	computerTurn();
}

void BgGameDispatcher::computerTurn()
{
	matchstate& ms = m_currentMatch;
	cubeinfo ci;

	if( ms.gs != GAME_PLAYING )
		return;

	ms.GetMatchStateCubeInfo( &ci );

	if( ms.fResigned ) 
	{
		//no resignation support
		assert(0);
		return;
	}
	else 
	if( ms.fDoubled )
	{
		//no doubling support
		assert(0);
		return;
	}
	else 
	{
		findData fd;
		static char achResign[ 3 ] = { 'n', 'g', 'b' };
	 
		/* Don't use the global board for this call, to avoid
		race conditions with updating the board and aborting the
		move with an interrupt. */
		BgBoard anBoardMove(ms.anBoard);

		/* Roll dice and move */
		if ( !ms.anDice[ 0 ] ) 
		{
			ms.rollDice();
			/* write line to status bar if using GTK */
			DiceRolled();      
		}

		moverecord pmr;
		pmr.mt = MOVE_NORMAL;
		pmr.anDice[ 0 ] = ms.anDice[ 0 ];
		pmr.anDice[ 1 ] = ms.anDice[ 1 ];
		pmr.fPlayer = ms.fTurn;

		fd.pml = &pmr.ml;
		fd.pboard = &anBoardMove;
		fd.auchMove = NULL;
		fd.rThr = 0.0f;
		fd.pci = &ci;
		if (FindMove(&fd) != 0)
		{
			return;
		}
		/* resorts the moves according to cubeful (if applicable),
		* cubeless and chequer on highest point to avoid some silly
		* looking moves */
		//RefreshMoveList(pmr.ml, NULL);

		/* make the move found above */
		if ( pmr.ml.cMoves ) 
		{
			pmr.n.anMove = pmr.ml.amMoves[0].anMove;
			pmr.n.iMove = 0;
			m_agents[m_currentMatch.fMove]->doMove(pmr.ml.amMoves[0]);
			if(pmr.ml.amMoves[0].pc == CLASS_OVER)
			{
				bgmove endMove = pmr.ml.amMoves[0];
				BgBoard board = BgBoard::PositionFromKey(endMove.auch);
				board.SwapSides();
				endMove.auch = board.PositionKey();
				endMove.arEvalMove.invert();
				m_agents[!m_currentMatch.fMove]->doMove(endMove);
			}
		}
		pmr.ml.deleteMoves();
     
		/* write move to status bar or stdout */
		ShowAutoMove( pmr.n.anMove );
		AddMoveRecord( &pmr );      
		return;
	}
  
	assert(false);
}

void BgGameDispatcher::ShowAutoMove(const ChequerMove& anMove) const
{
    char sz[ 40 ];

	if(!m_isShowLog)
		return;

	char symbol = m_currentMatch.fTurn ? 'X' : 'O';
    if( anMove[ 0 ] == -1 )
		printf( "%c:%s cannot move.\n", symbol, m_agents[ m_currentMatch.fTurn ]->getFullName().c_str() );
    else
		printf( "%c:%s moves %s.\n", symbol, m_agents[ m_currentMatch.fTurn ]->getFullName().c_str(),
			m_currentMatch.anBoard.FormatMove( sz, anMove ) );
}

void BgGameDispatcher::RefreshMoveList (movelist& pml, int *ai) const
{
	unsigned int i, j;
	movelist ml;

	if ( !pml.cMoves )
		return;

	if ( ai )
	{
		ml.CopyMoveList ( &pml );
	}

	//std::sort(pml.amMoves, pml.amMoves + pml.cMoves, std::less<move>());
	qsort( pml.amMoves, pml.cMoves, sizeof( bgmove ), (cfunc) bgmove::CompareMovesGeneral );

	pml.rBestScore = pml.amMoves[ 0 ].rScore;

	if ( ai == NULL) 
		return;
	
	for ( i = 0; i < pml.cMoves; i++ ) 
	{
		for ( j = 0; j < pml.cMoves; j++ ) 
		{
			if ( ! memcmp ( &ml.amMoves[j].anMove[0], &pml.amMoves[i].anMove[0], 8 * sizeof ( int ) ) )
			{
				ai[ j ] = i;
			}
		}
	}

	//delete [] ml.amMoves;
	ml.deleteMoves();
}

void BgGameDispatcher::showScore() const
{
    printf((m_currentMatch.cGames == 1
	     ? "The score (after %d game) is: %s %d, %s %d"
	     : "The score (after %d games) is: %s %d, %s %d"),
	    m_currentMatch.cGames,
	    m_agents[ 0 ]->getFullName().c_str(), m_currentMatch.anScore[ 0 ],
	    m_agents[ 1 ]->getFullName().c_str(), m_currentMatch.anScore[ 1 ] );

    if ( m_currentMatch.nMatchTo > 0 ) 
	{
        printf ( m_currentMatch.nMatchTo == 1 ? 
	         " (match to %d point%s).\n" :
	         " (match to %d points%s).\n",
                 m_currentMatch.nMatchTo,
		 m_currentMatch.fCrawford ? 
		 ", Crawford game" : ( m_currentMatch.fPostCrawford ?
					 ", post-Crawford play" : ""));
    } 
    else 
	{
        if ( m_currentMatch.fJacoby )
			printf ( " (money session,\nwith Jacoby rule)." );
        else
			printf ( " (money session,\nwithout Jacoby rule)." );
    }
}

int BgGameDispatcher::FindMove(findData *pfd)
{
	return FindnSaveBestMoves( pfd->pml, m_currentMatch.anDice[0], m_currentMatch.anDice[1], 
		*pfd->pboard, pfd->auchMove, pfd->rThr, pfd->pci);
}

int BgGameDispatcher::FindnSaveBestMoves( movelist *pml, int nDice0, int nDice1,
		const BgBoard& anBoard, unsigned char *auchMove, const
		float rThr, const cubeinfo* pci)
{
	// Find best moves. 
	unsigned int nMoves;
	//std::vector<move> pm;
	int cOldMoves;
    
	/* Find all moves -- note that pml contains internal pointers to static
		data, so we can't call GenerateMoves again (or anything that calls
		it, such as ScoreMoves at more than 0 plies) until we have saved
		the moves we want to keep in amCandidates. */
	//it doesn't now
	anBoard.GenerateMoves( pml, m_amMoves, nDice0, nDice1, false );
	m_agents[m_currentMatch.fMove]->setCurrentBoard(&m_currentMatch.anBoard);
	if ( pml->cMoves == 0 ) 
	{
		/* no legal moves */
		pml->amMoves = NULL;
		return 0;
	}
 
	/* Save moves */
	bgmove *pm = new bgmove [ pml->cMoves ];
	for(size_t i = 0; i < pml->cMoves; i++)
		pm[i] = pml->amMoves[i];
	pml->amMoves = pm;
	nMoves = pml->cMoves;

	/* evaluate moves on top ply */
	if( ScoreMoves( pml ) < 0 ) 
	{
		delete [] pm;
		pml->deleteMoves();
		return -1;
	}

	/* Resort the moves, in case the new evaluation reordered them. */
	//std::vector<move>::iterator it(pml->amMoves); 
	std::sort(pml->amMoves, pml->amMoves + pml->cMoves, std::less<bgmove>() );
	//qsort( pml->amMoves, pml->cMoves, sizeof( move ), (cfunc) move::CompareMovesGeneral );
	pml->iMoveBest = 0;
	/* set the proper size of the movelist */
  	cOldMoves = pml->cMoves;
	pml->cMoves = nMoves;

	return 0;
}

int BgGameDispatcher::ScoreMoves( movelist *pml ) const
{
	unsigned int i;
	int r = 0;	/* return value */
	pml->rBestScore = -99999.9f;

	for( i = 0; i < pml->cMoves; i++ ) 
	{
		if( ScoreMove(pml->amMoves[i]) < 0 ) 
		{
			r = -1;
			break;
		}

		if( pml->amMoves[ i ].rScore > pml->rBestScore )
		{
			pml->iMoveBest = i;
			pml->rBestScore = pml->amMoves[ i ].rScore;
		}
	}

 	return r;
}

int BgGameDispatcher::ScoreMove(bgmove& pm) const
{
    BgBoard anBoardTemp = BgBoard::PositionFromKey( pm.auch );
    anBoardTemp.SwapSides();

    BgReward arEval;
    if ( GeneralEvaluationEPlied (arEval, &anBoardTemp, pm.pc ) )
		return -1;

	BgAgent *agent = m_agents[m_currentMatch.fMove];

	if(agent->needsInvertedEval())
		arEval.invert();
	else
	if(pm.pc != CLASS_CONTACT && pm.pc != CLASS_CRASHED && pm.pc != CLASS_RACE)
		arEval.invert();

    /* Save evaluations */  
    pm.arEvalMove = arEval;
    pm.rScore = arEval[ OUTPUT_EQUITY ];

    return 0;
}

int BgGameDispatcher::GeneralEvaluationEPlied (BgReward& arOutput, const BgBoard *anBoard, positionclass& pc ) const
{
	pc = BgEval::Instance()->ClassifyPosition ( anBoard, VARIATION_STANDARD );
	if ( EvaluatePositionFull ( anBoard, arOutput, pc ) )
		return -1;

	arOutput[ OUTPUT_EQUITY ] = arOutput.utility();
	return 0;
}

int BgGameDispatcher::EvaluatePositionFull(const BgBoard *anBoard, BgReward& arOutput, positionclass& pc ) const
{
	/* at leaf node; use static evaluation */
	BgAgent *agent = m_agents[m_currentMatch.fMove];
	agent->evaluatePosition(anBoard, pc, arOutput);

	if (pc > CLASS_PERFECT && agent->supportsSanityCheck() && !agent->isLearnMode())
	{
		/* no sanity check needed for exact evaluations */
		BgEval::Instance()->SanityCheck( anBoard, arOutput );
	}

	return 0;
}

void BgGameDispatcher::FixMatchState(const moverecord *pmr)
{
	switch ( pmr->mt ) 
	{
	case MOVE_NORMAL:
		if ( m_currentMatch.fTurn != pmr->fPlayer ) 
		{
			// previous moverecord is missing
			m_currentMatch.anBoard.SwapSides();
			m_currentMatch.fMove = m_currentMatch.fTurn = pmr->fPlayer;
		}
		break;
	case MOVE_DOUBLE:
		if ( m_currentMatch.fTurn != pmr->fPlayer ) 
		{
			// previous record is missing: this must be an normal double
			m_currentMatch.anBoard.SwapSides();
		    m_currentMatch.fMove = m_currentMatch.fTurn = pmr->fPlayer;
		}
		break;
	default:
		// no-op
		break;
	}
}

void BgGameDispatcher::add_moverecord_sanity_check(const moverecord *pmr) const
{
	assert(pmr->fPlayer >= 0 && pmr->fPlayer <= 1);
	assert(pmr->ml.cMoves < movelist::MAX_MOVES);
	switch (pmr->mt) {
	case MOVE_GAMEINFO:
		assert(pmr->g.nMatch >= 0);
		assert(pmr->g.i >= 0);
		if (pmr->g.nMatch) {
			assert(pmr->g.i <= pmr->g.nMatch * 2 + 1);
			assert(pmr->g.anScore[0] < pmr->g.nMatch);
			assert(pmr->g.anScore[1] < pmr->g.nMatch);
		}
		if (!pmr->g.fCrawford)
			assert(!pmr->g.fCrawfordGame);

		break;

	case MOVE_NORMAL:
		if (pmr->ml.cMoves)
			assert(pmr->n.iMove <= pmr->ml.cMoves);
		break;

		break;

	case MOVE_RESIGN:
		//assert(pmr->r.nResigned >= 1 && pmr->r.nResigned <= 3);
		break;

	case MOVE_DOUBLE:
	case MOVE_TAKE:
	case MOVE_DROP:
	case MOVE_SETDICE:
	case MOVE_SETBOARD:
	case MOVE_SETCUBEVAL:
	case MOVE_SETCUBEPOS:
		break;

	default:
		assert(FALSE);
	}
}

moverecord * BgGameDispatcher::get_current_moverecord(int *pfHistory)
{
	if(m_lMatch.size() && m_lMatch.back().size())
	{
		if (pfHistory)
			*pfHistory = TRUE;
		return &m_lMatch.back().back();
	}

	if (pfHistory)
		*pfHistory = FALSE;

	if (m_currentMatch.gs != GAME_PLAYING) 
	{
		moverecord::destroy(pmr_hint);
		return NULL;
	}

	/* invalidate on changed dice */
	if (m_currentMatch.anDice[0] > 0 && pmr_hint && pmr_hint->anDice[0] > 0 
	    && (pmr_hint->anDice[0] != m_currentMatch.anDice[0]
		|| pmr_hint->anDice[1] != m_currentMatch.anDice[1]))
	{
		moverecord::destroy(pmr_hint);
	}

	if (!pmr_hint) 
	{
		pmr_hint = new moverecord();
		pmr_hint->fPlayer = m_currentMatch.fTurn;
	}

	if (m_currentMatch.anDice[0] > 0) 
	{
		pmr_hint->mt = MOVE_NORMAL;
		pmr_hint->anDice[0] = m_currentMatch.anDice[0];
		pmr_hint->anDice[1] = m_currentMatch.anDice[1];
	} 
	else 
	if (m_currentMatch.fDoubled) 
	{
		pmr_hint->mt = MOVE_TAKE;
	} 
	else 
	{
		pmr_hint->mt = MOVE_DOUBLE;
		pmr_hint->fPlayer = m_currentMatch.fTurn;
	}
	return pmr_hint;
}

void BgGameDispatcher::copy_from_pmr_cur(moverecord *pmr, bool get_move, bool get_cube)
{
	moverecord *pmr_cur;
	pmr_cur = get_current_moverecord(NULL);
	if (!pmr_cur)
		return;
	
	if (get_move && pmr_cur->ml.cMoves > 0) 
	{
		//if (pmr->ml.cMoves > 0)
		//	delete [] pmr->ml.amMoves;
		pmr->ml.CopyMoveList(&pmr_cur->ml);
		pmr->n.iMove = m_currentMatch.anBoard.locateMove(pmr->n.anMove, &pmr->ml);
	}

	/*
	if (get_cube && pmr_cur->CubeDecPtr->esDouble.et != EVAL_NONE) {
		memcpy(pmr->CubeDecPtr->aarOutput,
		       pmr_cur->CubeDecPtr->aarOutput,
		       sizeof pmr->CubeDecPtr->aarOutput);
		memcpy(pmr->CubeDecPtr->aarStdDev,
		       pmr_cur->CubeDecPtr->aarStdDev,
		       sizeof pmr->CubeDecPtr->aarStdDev);
		memcpy(&pmr->CubeDecPtr->esDouble,
		       &pmr_cur->CubeDecPtr->esDouble,
		       sizeof pmr->CubeDecPtr->esDouble);
		pmr->CubeDecPtr->cmark = pmr_cur->CubeDecPtr->cmark;
	}
	*/
}

void BgGameDispatcher::add_moverecord_get_cur(moverecord *pmr)
{
	switch (pmr->mt) 
	{
	case MOVE_NORMAL:
	case MOVE_RESIGN:
		copy_from_pmr_cur(pmr, TRUE, TRUE);
		moverecord::destroy(pmr_hint);
		//find_skills(pmr, &ms, FALSE, -1);
		break;
	case MOVE_DOUBLE:
		copy_from_pmr_cur(pmr, FALSE, TRUE);
		//find_skills(pmr, &ms, TRUE, -1);
		break;
	case MOVE_TAKE:
		copy_from_pmr_cur(pmr, FALSE, TRUE);
		moverecord::destroy(pmr_hint);
		//find_skills(pmr, &ms, -1, TRUE);
		break;
	case MOVE_DROP:
		copy_from_pmr_cur(pmr, FALSE, TRUE);
		moverecord::destroy(pmr_hint);
		//find_skills(pmr, &ms, -1, FALSE);
		break;
	case MOVE_SETDICE:
		copy_from_pmr_cur(pmr, FALSE, TRUE);
		//find_skills(pmr, &ms, FALSE, -1);
		break;
	default:
		moverecord::destroy(pmr_hint);
		break;
	}
}

void BgGameDispatcher::ExportMatchMat( char *sz, bool fSst ) const
{
    FILE *pf;

    if( !strcmp( sz, "-" ) )
	pf = stdout;
    else if( !( pf = fopen( sz, "w" ) ) ) 
	{
		perror( sz );
		return;
    }

    fprintf( pf, " %d point match\n\n", m_currentMatch.nMatchTo );

	int i = 0;
	std::list<std::list<moverecord> >::const_iterator it;
	for(it = m_lMatch.begin(); it != m_lMatch.end(); i++, it++)
      ExportGameJF( pf, *it, i, TRUE, fSst );
    
    if( pf != stdout )
	fclose( pf );
}

void BgGameDispatcher::ExportGameJF( FILE *pf, const std::list<moverecord>& plGame, int iGame, bool withScore, bool fSst ) const
{
    matchstate msExport;
    char sz[ 128 ], buffer[ 256 ];
    int i = 0, n, nFileCube = 1, fWarned = FALSE, diceRolled = 0;

    /* FIXME It would be nice if this function was updated to use the
       new matchstate struct and ApplyMoveRecord()... but otherwise
       it's not broken, so I won't fix it. */

    if (fSst && iGame == -1) /* Snowie export of single game */
      fprintf( pf, " 0 point match\n\n Game 1\n" );
    
    if( iGame >= 0 )
	fprintf( pf, " Game %d\n", iGame + 1 );

	BgBoard anBoard;
    anBoard.InitBoard( m_currentMatch.bgv );
    
	std::list<moverecord>::const_iterator it;
    //for( pl = plGame->plNext; pl != plGame; pl = pl->plNext ) {
	for(it = plGame.begin(); it != plGame.end(); it++)
	{
		std::list<moverecord>::const_iterator it_next = it;
		it_next++;

		const moverecord *pmr = &(*it);
		switch( pmr->mt ) 
		{
		case MOVE_GAMEINFO:
			if( withScore ) 
			{
				sprintf( sz, "%s : %d", m_agents[0]->getFullName().c_str(), pmr->g.anScore[ 0 ] );
				fprintf( pf, " %-31s%s : %d\n", sz, m_agents[1]->getFullName().c_str(), pmr->g.anScore[ 1 ] );
				msExport.anScore[0] = pmr->g.anScore[0];
				msExport.anScore[1] = pmr->g.anScore[1];
				msExport.fCrawford = pmr->g.fCrawford;
				msExport.fPostCrawford = !pmr->g.fCrawfordGame;
			} 
			else 
			if (fSst && iGame == -1) 
			{ 
				/* Snowie export of single game */
				sprintf( sz, "%s : 0", m_agents[0]->getFullName().c_str() );
				fprintf( pf, " %-31s%s : 0\n", sz, m_agents[1]->getFullName().c_str() );
			} 
			else
				fprintf( pf, " %-31s%s\n", m_agents[0]->getFullName().c_str(), m_agents[1]->getFullName().c_str() );
			
			msExport.fCubeOwner = -1;
			/* FIXME what about automatic doubles? */
			continue;
			break;
		
		case MOVE_NORMAL:
            diceRolled = 0;
			sprintf( sz, "%d%d: ", pmr->anDice[ 0 ], pmr->anDice[ 1 ] );
			if (fSst) 
			{ 
				/* Snowie standard text */
				const moverecord *pnextmr = NULL;
				if (it_next != plGame.end()) 
				{
					pnextmr = &(*it_next);
					if (pnextmr->mt == MOVE_SETBOARD)
					{
						/* Illegal move entered in gnubg as bogus move followed by
						 editing position : don't export the move, only the dice */
						diceRolled = 1;
					}
					else 
					{
						anBoard.FormatMovePlain( sz + 4, pmr->n.anMove );
					}
				} 
				else
				{
					anBoard.FormatMovePlain( sz + 4, pmr->n.anMove );
				}
			}
			else 
			{
				anBoard.FormatMovePlain( sz + 4, pmr->n.anMove );
			}
			anBoard.ApplyMove( pmr->n.anMove, false );
			anBoard.SwapSides( );
			 /*   if (( sz[ strlen(sz)-1 ] == ' ') && (strlen(sz) > 5 ))
				  sz[ strlen(sz) - 1 ] = 0;  Don't need this..  */
			break;
		case MOVE_DOUBLE:
			sprintf( sz, " Doubles => %d", nFileCube <<= 1 );
            msExport.fCubeOwner = !(i & 1);
			break;
		case MOVE_TAKE:
			strcpy( sz, " Takes" ); /* FIXME beavers? */
			break;
		case MOVE_DROP:
            sprintf( sz, " Drops%sWins %d point%s",
                   (i & 1) ? "\n      " : "                       ",
                   nFileCube / 2, (nFileCube == 2) ? "" :"s" );
			break;
        case MOVE_RESIGN:
            if (pmr->fPlayer)
              sprintf( sz, "%s      Wins %d point%s\n", ((i & 1) ^ diceRolled) ? "\n" : "",
                       pmr->r.nResigned * nFileCube,
                       ((pmr->r.nResigned * nFileCube ) > 1) ? "s" : "");
            else
			if (i & 1) 
			{
				sprintf( sz, "%sWins %d point%s\n", ((i & 1) ^ diceRolled) ? " " : "                        ",
					pmr->r.nResigned * nFileCube,
					((pmr->r.nResigned * nFileCube ) > 1) ? "s" : "");
			} 
			else 
			{
				sprintf( sz, "%sWins %d point%s\n", ((i & 1) ^ diceRolled) ? " " : "                                  ",
					pmr->r.nResigned * nFileCube,
					((pmr->r.nResigned * nFileCube ) > 1) ? "s" : "");
			}
            break;
		case MOVE_SETDICE:
			/* Could be rolled dice just before resign or an illegal move */
			sprintf( sz, "%d%d: ", pmr->anDice[ 0 ], pmr->anDice[ 1 ] );
            diceRolled = 1;
			break;
		case MOVE_SETBOARD:
		case MOVE_SETCUBEVAL:
		case MOVE_SETCUBEPOS:
			if( !fWarned && !fSst) 
			{
				fWarned = TRUE;
				fprintf(stderr, "Warning: this game was edited during play and "
					"cannot be exported correctly in this format.\n"
					"Use Snowie text format instead.");
			}
			break;
		}

		if ( pmr->mt == MOVE_SETBOARD
			 || pmr->mt == MOVE_SETCUBEVAL
			 || pmr->mt == MOVE_SETCUBEPOS) 
		{
			anBoard = BgBoard::PositionFromKey(pmr->sb.auchKey);
			if (i & 1)
				anBoard.SwapSides();

			if (fSst) 
			{ 
				/* Snowie standard text */
				const moverecord *pnextmr = &(*it_next);
				char *ct;

				msExport.nMatchTo = m_currentMatch.nMatchTo;
				msExport.nCube = nFileCube;
				msExport.fMove = (i & 1);
				msExport.fTurn = (i & 1);
				msExport.anBoard = BgBoard::PositionFromKey(pmr->sb.auchKey);
				msExport.anDice[0] = pnextmr->anDice[0];
				msExport.anDice[1] = pnextmr->anDice[1];
				if (i & 1)
					msExport.anBoard.SwapSides();
				ExportSnowieTxt (buffer, &msExport);
	    
				// I don't understand why we need to swap this field!
				ct = strstr(buffer, ";");
				ct[7] = '0' + ('1' - ct[7]);
			}
			if ( !(i & 1) )
			{
				if (fSst)
					fprintf( pf, "Illegal play (%s)\n", buffer);
				else
					fprintf( pf, "\n");
			}
			else
			{
				if (fSst)
					fprintf( pf, "Illegal play (%-22s) ", buffer);
				else
					fprintf( pf, "%-23s ", "");
			}
			continue;
		}

		if( !i && pmr->mt == MOVE_NORMAL && pmr->fPlayer ) 
		{
			fputs( "  1)                             ", pf );
			i++;
		}

		if(( i & 1 ) || (pmr->mt == MOVE_RESIGN)) 
		{
			fputs( sz, pf );
			if (pmr->mt != MOVE_SETDICE && !(pmr->mt == MOVE_NORMAL && diceRolled == 1))
				fputc( '\n', pf );
		} 
		else 
		if (pmr->mt != MOVE_SETDICE)
			fprintf( pf, "%3d) %-27s ", ( i >> 1 ) + 1, sz );
		else
			fprintf( pf, "%3d) %-3s", ( i >> 1 ) + 1, sz );

        if ( pmr->mt == MOVE_DROP ) 
		{
			fputc( '\n', pf );
			if ( ! ( i & 1 ) )
				fputc( '\n', pf );
        }

		if( ( n = anBoard.GameStatus( m_currentMatch.bgv ) ) ) 
		{
			fprintf( pf, "%sWins %d point%s%s\n\n",
				i & 1 ? "                                  " : "\n      ",
				n * nFileCube, n * nFileCube > 1 ? "s" : "", "" /* FIXME " and the match" if appropriate */ );
		}
		i++;
    }
}

/*
 * Snowie .txt files
 *
 * The files are a single line with fields separated by
 * semicolons. Fields are numeric, except for player names.
 *
 * Field no    meaning
 * 0           length of match (0 in money games)
 * 1           1 if Jacoby enabled, 0 in match play or disabled
 * 2           Don't know, all samples had 0. Maybe it's Nack gammon or
 *            some other variant?
 * 3           Don't know. It was one in all money game samples, 0 in
 *            match samples
 * 4           Player on roll 0 = 1st player
 * 5,6         Player names
 * 7           1 = Crawford game
 * 8,9         Scores for player 0, 1
 * 10          Cube value
 * 11          Cube owner 1 = player on roll, 0 = centred, -1 opponent
 * 12          Chequers on bar for player on roll
 * 13..36      Points 1..24 from player on roll's point of view
 *            0 = empty, positive nos. for player on roll, negative
 *            for opponent
 * 37          Chequers on bar for opponent
 * 38.39       Current roll (0,0 if not rolled)
 *
 */

void BgGameDispatcher::ExportSnowieTxt (char *sz, const matchstate *pms) const
{
	int i;

	/* length of match */
	sz += sprintf (sz, "%d;", pms->nMatchTo);

	/* Jacoby rule */
	sz += sprintf (sz, "%d;", (!pms->nMatchTo && pms->fJacoby) ? 1 : 0);

	/* unknown field (perhaps variant) */
	sz += sprintf (sz, "%d;", 0);

	/* unknown field */
	sz += sprintf (sz, "%d;", pms->nMatchTo ? 0 : 1);

	/* player on roll (0 = 1st player) */
	sz += sprintf (sz, "%d;", pms->fMove);

	/* player names */
	sz += sprintf (sz, "%s;%s;", m_agents[pms->fMove]->getFullName().c_str(), 
		 m_agents[!pms->fMove]->getFullName().c_str());

	/* crawford game */
	i = pms->nMatchTo &&
		((pms->anScore[0] == (pms->nMatchTo - 1)) ||
		(pms->anScore[1] == (pms->nMatchTo - 1))) &&
		pms->fCrawford && !pms->fPostCrawford;

	sz += sprintf (sz, "%d;", i);

	/* scores */
	sz += sprintf (sz, "%d;%d;", pms->anScore[pms->fMove], pms->anScore[!pms->fMove]);

	/* cube value */
	sz += sprintf (sz, "%d;", pms->nCube);

	/* cube owner */
	if (pms->fCubeOwner == -1)
		i = 0;
	else 
	if (pms->fCubeOwner == pms->fMove)
		i = 1;
	else
		i = -1;

	sz += sprintf (sz, "%d;", i);

	/* chequers on the bar for player on roll */
	sz += sprintf (sz, "%d;", -(int)pms->anBoard.anBoard[BgBoard::OPPONENT][24]);

	/* chequers on the board */
	for (i = 0; i < 24; ++i)
    {
		int j;
		if (pms->anBoard.anBoard[BgBoard::SELF][i])
			j = pms->anBoard.anBoard[BgBoard::SELF][i];
		else
			j = -(int)pms->anBoard.anBoard[BgBoard::OPPONENT][23 - i];

		sz += sprintf (sz, "%d;", j);
    }

	/* chequers on the bar for opponent */
	sz += sprintf (sz, "%d;", pms->anBoard.anBoard[BgBoard::SELF][24]);

	/* dice */
	sz += sprintf (sz, "%d;%d;",
	   (pms->anDice[0] > 0) ? pms->anDice[0] : 0,
	   (pms->anDice[1] > 0) ? pms->anDice[1] : 0);
}
