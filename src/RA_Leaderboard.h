#ifndef RA_LEADERBOARD_H
#define RA_LEADERBOARD_H
#pragma once

#include "RA_Condition.h"


#pragma pack(push, 1)
#pragma pack(push, 4)
#pragma pack(push, 1)
class MemValue
{
public:
    
    // Wanna just make this a struct with helper functions/class?
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions, compiler bug maybe?
    inline constexpr MemValue() noexcept = default;
    inline constexpr MemValue(unsigned int nAddr, double fMod, ComparisonVariableSize nSize, bool bBCDParse,
        bool bParseVal) noexcept :
        m_nAddress{ nAddr },
        m_nVarSize{ nSize },
        m_fModifier{ fMod },
        m_bBCDParse{ bBCDParse },
        m_bParseVal{ bParseVal }
    {
    }
#pragma warning(pop)

public:
    const char* ParseFromString(const char* pBuffer);		//	Parse string into values, returns end of string
    double GetValue() const;					//	Get the value in-memory with modifiers

public:
    unsigned int			m_nAddress ={};					//	Raw address of an 8-bit, or value.
    ComparisonVariableSize	m_nVarSize{ ComparisonVariableSize::EightBit };
    double					m_fModifier{ 1.0F };				//	* 60 etc
    bool					m_bBCDParse ={};				//	Parse as a binary coded decimal.
    bool					m_bParseVal ={};				//	Parse as a value
    bool m_bInvertBit ={};
    unsigned int m_nSecondAddress ={};
    ComparisonVariableSize m_nSecondVarSize ={};
};
#pragma pack(pop)
#pragma pack(pop)
#pragma pack(pop)

//	Encapsulates a vector of MemValues
class ValueSet
{
public:
    enum OperationType
    {
        Operation_None,
        Operation_Addition,
        Operation_Maximum
    };

public:
    void ParseMemString(const char* sBuffer);
    double GetValue() const;
    double GetOperationsValue(std::vector<OperationType>) const;
    void AddNewValue(MemValue nVal);
    void Clear();

#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    size_t NumValues() const { return m_Values.size(); }
#pragma warning(pop)

    

protected:
    std::vector<MemValue> m_Values;
};
#pragma pack(push, 4)
struct LB_Entry
{
    unsigned int m_nRank ={};
    std::string	 m_sUsername;
    int m_nScore ={};
    time_t m_TimeAchieved ={};
};
#pragma pack(pop)

#pragma pack(push, 1)
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
    RA_Leaderboard() noexcept = default;
    ~RA_Leaderboard() noexcept = default;
    explicit RA_Leaderboard(const LeaderboardID nID) noexcept;
    RA_Leaderboard(const RA_Leaderboard&) = delete;
    RA_Leaderboard& operator=(const RA_Leaderboard&) = delete;
    RA_Leaderboard(RA_Leaderboard&&) noexcept = default;
    RA_Leaderboard& operator=(RA_Leaderboard&&) noexcept = default;

public:
    void LoadFromJSON(const Value& element);
    void ParseLine(char* sBuffer);
    void ParseLBData(const char* sBuffer);

    void Test();
    double GetCurrentValue();
    double GetCurrentValueProgress() const;	//	Attempt to get a 'progress' alternative
    void Clear();

    //	Helper: tbd; refactor *into* RA_Formattable
    // Error C4596
    static std::string FormatScore(FormatType nType, int nScoreIn);
    std::string FormatScore(int nScoreIn) const;
    void Reset();

#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    LeaderboardID ID() const { return m_nID; }
    const std::string& Title() const { return m_sTitle; }
    const std::string& Description() const { return m_sDescription; }

    void ClearRankInfo() { m_RankInfo.clear(); }
    const LB_Entry& GetRankInfo(unsigned int nAt) const { return m_RankInfo.at(nAt); }
    size_t GetRankInfoCount() const { return m_RankInfo.size(); }

    FormatType GetFormatType() const { return m_format; }
#pragma warning(pop)

    void SubmitRankInfo(unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved);
    void SortRankInfo();
    std::vector< ValueSet::OperationType > m_sOperations;

private:
    // Just delete copying, making it const complicates things, the compiler deleted it anyway

    /// <summary>DB ID for this LB</summary>
    LeaderboardID m_nID ={};

    /// <summary>Start monitoring if this is <c>true</c></summary>
    ConditionSet m_startCond;

    /// <summary>Cancel monitoring if this is <c>true</c></summary>
    ConditionSet m_cancelCond;

    /// <summary>Submit new score if this is <c>true</c></summary>
    ConditionSet m_submitCond;

    /// <summary>
    ///   <para><c>false</c> = check <see cref="m_startCond" />.</para>
    ///   <para>
    ///     <c>true</c> = check <see cref="m_cancelCond" /> or
    ///     <see cref="m_submitCond" />.
    ///   </para>
    /// </summary>
    bool m_bStarted{ false };

    /// <summary><c>true</c> if already submitted.</summary>
    bool m_bSubmitted{ false };

    /// <summary>
    ///   A collection of memory addresses and values to produce one value.
    /// </summary>
    ValueSet m_value;

    /// <summary>
    ///   A collection of memory addresses, used to show progress towards
    ///   completion.
    /// </summary>
    ValueSet m_progress;	

    /// <summary>
    ///   A format to output. Typically <c>'%d'</c> for score or
    ///   <c>'%02d:%02d.%02d'</c> for time.
    /// </summary>
    FormatType m_format{ FormatType::Format_Value };

    std::string m_sTitle;
    std::string	m_sDescription;

    /// <summary>Recent users ranks.</summary>
    std::vector<LB_Entry> m_RankInfo;
};
#pragma pack(pop)

class RA_LeaderboardManager
{
public:
    static void OnSubmitEntry(const Document& doc);

public:
    void Reset();

    void AddLeaderboard(RA_Leaderboard&& lb);
    void Test();

    // we could abstract a lot of this stuff to get rid of these warnings
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    void Clear() { m_Leaderboards.clear(); }
    size_t Count() const { return m_Leaderboards.size(); }
    inline RA_Leaderboard& GetLB(size_t iter) { return m_Leaderboards[iter]; }
#pragma warning(pop)

    RA_Leaderboard* FindLB(LeaderboardID nID);

public:
    std::vector<RA_Leaderboard> m_Leaderboards;
};

extern RA_LeaderboardManager g_LeaderboardManager;


#endif // !RA_LEADERBOARD_H
