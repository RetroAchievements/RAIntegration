#pragma once

#include "RA_MemValue.h"

#include <vector>

struct LB_Entry
{
    unsigned int m_nRank;
    std::string	 m_sUsername;
    int m_nScore;
    time_t m_TimeAchieved;
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
    RA_Leaderboard(const LeaderboardID nLBID);
    ~RA_Leaderboard();

public:
    void LoadFromJSON(const Value& element);
    void ParseLine(char* sBuffer);
    void ParseLBData(const char* sBuffer);

    void Test();
    double GetCurrentValue();
    double GetCurrentValueProgress() const;	//	Attempt to get a 'progress' alternative
    void Clear();

    //	Helper: tbd; refactor *into* RA_Formattable
    static std::string RA_Leaderboard::FormatScore(FormatType nType, int nScoreIn);
    std::string FormatScore(int nScoreIn) const;
    void Reset();

    LeaderboardID ID() const { return m_nID; }
    const std::string& Title() const { return m_sTitle; }
    const std::string& Description() const { return m_sDescription; }

    void SubmitRankInfo(unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved);
    void ClearRankInfo() { m_RankInfo.clear(); }
    const LB_Entry& GetRankInfo(unsigned int nAt) const { return m_RankInfo.at(nAt); }
    size_t GetRankInfoCount() const { return m_RankInfo.size(); }
    void SortRankInfo();

    FormatType GetFormatType() const { return m_format; }

private:
    const LeaderboardID		m_nID;			//	DB ID for this LB
    ConditionSet			m_startCond;	//	Start monitoring if this is true
    ConditionSet			m_cancelCond;	//	Cancel monitoring if this is true
    ConditionSet			m_submitCond;	//	Submit new score if this is true

    std::vector<MemValueSet::OperationType> m_sOperations;

    bool					m_bStarted;		//	False = check start condition. True = check cancel or submit conditions.
    bool                    m_bSubmitted;   //  True if already submitted.

    MemValueSet				m_value;		//	A collection of memory addresses and values to produce one value.
    MemValueSet				m_progress;		//	A collection of memory addresses, used to show progress towards completion.
    FormatType				m_format;		//	A format to output. Typically "%d" for score or "%02d:%02d.%02d" for time

    std::string				m_sTitle;		//	
    std::string				m_sDescription;	//	

    std::vector<LB_Entry>	m_RankInfo;		//	Recent users ranks
};
