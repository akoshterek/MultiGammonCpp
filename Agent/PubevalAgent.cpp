#include "PubevalAgent.h"
#include "BgBoard.h"

	/* Backgammon move-selection evaluation function
	   for benchmark comparisons.  Computes a linear
	   evaluation function:  Score = W * X, where X is
	   an input vector encoding the board state (using
	   a raw encoding of the number of men at each location),
	   and W is a weight vector.  Separate weight vectors
	   are used for racing positions and contact positions.
	   Makes lots of obvious mistakes, but provides a
	   decent level of play for benchmarking purposes. */

	/* Provided as a public service to the backgammon
	   programming community by Gerry Tesauro, IBM Research.
	   (e-mail: tesauro@watson.ibm.com)			*/

	/* The following inputs are needed for this routine:

	   race   is an integer variable which should be set
	   based on the INITIAL position BEFORE the move.
	   Set race=1 if the position is a race (i.e. no contact)
	   and 0 if the position is a contact position.

	   pos[]  is an integer array of dimension 28 which
	   should represent a legal final board state after
	   the move. Elements 1-24 correspond to board locations
	   1-24 from computer's point of view, i.e. computer's
	   men move in the negative direction from 24 to 1, and
	   opponent's men move in the positive direction from
	   1 to 24. Computer's men are represented by positive
	   integers, and opponent's men are represented by negative
	   integers. Element 25 represents computer's men on the
	   bar (positive integer), and element 0 represents opponent's
	   men on the bar (negative integer). Element 26 represents
	   computer's men off the board (positive integer), and
	   element 27 represents opponent's men off the board
	   (negative integer).					*/

	/* Also, be sure to call rdwts() at the start of your
	   program to read in the weight values. Happy hacking] */
 
PubevalAgent::PubevalAgent(fs::path path)
	: BgAgent(path)
{
	m_fullName = "Pubeval";
	m_path /= "pubeval";

	fs::path contactPath = m_path;
	contactPath /= "WT.cntc";
	fs::path racePath = m_path;
	racePath /= "WT.race";
	loadWeights(contactPath, racePath);
}

PubevalAgent::~PubevalAgent(void)
{
}

void PubevalAgent::loadWeights(fs::path contact, fs::path race)
{
	/* read weight files into arrays wr[], wc[] */
	FILE *fp = fopen(race.string().c_str(), "r");
	if(fp == NULL)
		throw std::exception("Can not open race network weights");
	FILE *fq = fopen(contact.string().c_str(), "r");
	if(fq == NULL)
		throw std::exception("Can not open contact network weights");
	
	for(int i=0; i < 122; ++i) 
	{
		fscanf(fp, "%f", m_wr+i);
		fscanf(fq, "%f", m_wc+i);
	}
	
	//padding
	m_wr[122] = m_wc[122] = 0;
	m_wr[123] = m_wc[123] = 0;
	
	fclose(fp);
	fclose(fq); 
}

float PubevalAgent::pubeval(bool race, char pos[28])
{
	float score;

	if(pos[26]==15) return(99999999.);
	/* all men off, best possible move */

	setX(pos); /* sets input array x[] */
	score = 0.0f;
	
	//race or contacts weights
	const float *weights = race ? m_wr : m_wc;

    for(int i = 0; i < 122; ++i) 
		score += weights[i]*m_x[i];

	return(score); 
}

void PubevalAgent::preparePos(const BgBoard *board, char pos[28]) const
{
	unsigned int men[2];
	board->ChequersCount(men);

	for(int i = 0; i < 24; i++)
	{
		pos[i+1] = board->anBoard[BgBoard::SELF][i];
		if(board->anBoard[BgBoard::OPPONENT][23 - i])
			pos[i+1] = -board->anBoard[BgBoard::OPPONENT][23 - i];
	}

	pos[25] = board->anBoard[BgBoard::SELF][BgBoard::BAR];
	pos[0] = -board->anBoard[BgBoard::OPPONENT][BgBoard::BAR];
	pos[26] = 15 - men[BgBoard::SELF];
	pos[27] = -char(15 - men[BgBoard::OPPONENT]);
}

void PubevalAgent::setX(const char pos[28])
{
	/* sets input vector x[] given board position pos[] */
	int jm1, n;
	/* initialize */
	for(int j = 0; j < 122; ++j) 
		m_x[j] = 0;

	/* first encode board locations 24-1 */
	for(int j=1; j <= 24; ++j) 
	{
	    jm1 = j - 1;
	    n = pos[25-j];
	    if(n != 0) 
		{
			if(n==-1) m_x[5*jm1+0] = 1.0f;
			if(n==1)  m_x[5*jm1+1] = 1.0f;
			if(n>=2)  m_x[5*jm1+2] = 1.0f;
			if(n==3)  m_x[5*jm1+3] = 1.0f;
			if(n>=4)  m_x[5*jm1+4] = (float)(n-3)/2.0f;
	    }
	}
	/* encode opponent barmen */
	m_x[120] = -(float)(pos[0])/2.0f;
	/* encode computer's menoff */
	m_x[121] = (float)(pos[26])/15.0f;

	//padding
	m_x[122] = m_x[123] = 0;
} 

void PubevalAgent::evaluatePosition(const BgBoard *board, positionclass& pc, 
		const bgvariation bgv, BgReward& reward)
{
	if(pc == CLASS_OVER)
	{
		evalOver(board, reward);
	}
	else
	{
		reward.reset();
		char pos[28];
		preparePos(board, pos);

		positionclass pcPresent = BgEval::Instance()->ClassifyPosition(m_curBoard, VARIATION_STANDARD);
		reward[OUTPUT_WIN] = pubeval(pcPresent == CLASS_RACE, pos); 
		pc = (pcPresent == CLASS_RACE) ? CLASS_RACE : CLASS_CONTACT;
	}
}
