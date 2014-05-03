#pragma once
#include <vector>
#include "RA_Condition.h"


class MemValue
{
public:
	MemValue()	{ m_nAddress = 0; m_fModifier = 1.0f; m_nVarSize = CMP_SZ_8BIT; m_bBCDParse = false; m_bParseVal = false; }
	MemValue( unsigned int nAddr, double fMod, CompVariableSize nSize, bool bBCDParse, bool bParseVal )
		: m_nAddress(nAddr), m_fModifier(fMod), m_nVarSize(nSize), m_bBCDParse(bBCDParse), m_bParseVal(bParseVal) {}
	
public:
	char* ParseFromString( char* pBuffer );		//	Parse string into values, returns end of string
	double GetValue() const;					//	Get the value in-memory with modifiers

public:
	unsigned int		m_nAddress;					//	Raw address of an 8-bit, or value.
	CompVariableSize	m_nVarSize;				
	double				m_fModifier;				//	* 60 etc
	bool				m_bBCDParse;				//	Parse as a binary coded decimal.
	bool				m_bParseVal;				//	Parse as a value
};


//	Encapsulates a vector of MemValues
class ValueSet
{
public:
	void ParseMemString( char* sBuffer );
	double GetValue() const;
	void AddNewValue( MemValue nVal );
	void Clear();

	size_t NumValues() const	{ return m_Values.size(); }

protected:
	std::vector<MemValue> m_Values;
};

struct LB_Entry
{
	unsigned int m_nRank;
	char		 m_sUsername[50];
	unsigned int m_nScore;
	time_t		 m_TimeAchieved;
};

class RA_Leaderboard
{
public:
	enum FormatType
	{
		Format_TimeFrames,		//	Value is a number describing the amount of frames.
		Format_TimeSecs,		//	Value is a number in amount of seconds.
		Format_TimeMillisecs,	//	Value is the number of milliseconds. * 100 to get seconds.
		Format_Score,			//	Value is a nondescript 'score' Points.
		Format_Value,			//	Generic value - %01d
		Format_Other,			//	Point-based value (i.e. 1942 ships killed). %06d

		Format__MAX
	};

public:
	RA_Leaderboard();
	~RA_Leaderboard();

public:
	void ParseLine( char* sBuffer );
	void Test();
	double GetCurrentValue();
	double GetCurrentValueProgress() const;	//	Attempt to get a 'progress' alternative
	void Clear();

	static void FormatScore( FormatType nType, unsigned int nScoreIn, char* pBuffer, unsigned int nLen );
	void Reset();

	unsigned int ID() const									{ return m_nID; }
	const char* Title() const								{ return m_sTitle; }
	const char* Description() const							{ return m_sDescription; }

	void SubmitRankInfo( unsigned int nRank, char* sUsername, unsigned int nScore, time_t nAchieved );
	void ClearRankInfo()									{ m_RankInfo.clear(); }
	const LB_Entry& GetRankInfo( unsigned int nAt ) const	{ return m_RankInfo.at( nAt ); }
	size_t GetRankInfoCount() const							{ return m_RankInfo.size(); }
	void SortRankInfo();

	FormatType GetFormatType() const						{ return m_format; }

private:
	ConditionSet	m_startCond;	//	Start monitoring if this is true
	ConditionSet	m_cancelCond;	//	Cancel monitoring if this is true
	ConditionSet	m_submitCond;	//	Submit new score if this is true

	bool			m_bStarted;		//	False = check start condition. True = check cancel or submit conditions.

	ValueSet		m_value;		//	A collection of memory addresses and values to produce one value.
	ValueSet		m_progress;		//	A collection of memory addresses, used to show progress towards completion.
	FormatType		m_format;		//	A format to output. Typically "%d" for score or "%02d:%02d.%02d" for time
	unsigned int	m_nID;			//	DB ID for this LB

	char			m_sTitle[80];			//	
	char			m_sDescription[300];	//	

	std::vector<LB_Entry> m_RankInfo;
};


class RA_LeaderboardManager
{
public:
	static void s_OnSubmitEntry( void* pDataSent );

public:
	void AddLeaderboard( const RA_Leaderboard& lb );
	void Test();
	void Clear()					{ m_Leaderboards.clear(); }
	size_t Count() const			{ return m_Leaderboards.size(); }

	void Reset();

	inline RA_Leaderboard& GetLB( unsigned int nOffs )	{ return m_Leaderboards[nOffs]; }

	RA_Leaderboard* FindLB( unsigned int nID );

public:
	std::vector<RA_Leaderboard> m_Leaderboards;
};

extern RA_LeaderboardManager g_LeaderboardManager;