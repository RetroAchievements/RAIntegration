#ifndef RA_LEADERBOARD_H
#define RA_LEADERBOARD_H
#pragma once

#include "RA_MemValue.h"

class RA_Leaderboard
{
public:
    RA_Leaderboard(const ra::LeaderboardID nLBID);
    virtual ~RA_Leaderboard() noexcept = default;

    void ParseFromString(const char* sBuffer, MemValue::Format format);

    void Test();
    virtual void Reset();

    unsigned int GetCurrentValue() const { return m_value.GetValue(); } // Gets the final value for submission
    unsigned int GetCurrentValueProgress() const;	                    // Gets the value to display while the leaderboard is active

    ra::LeaderboardID ID() const { return m_nID; }

    const std::string& Title() const { return m_sTitle; }
    void SetTitle(const std::string& sValue) { m_sTitle = sValue; }

    const std::string& Description() const { return m_sDescription; }
    void SetDescription(const std::string& sValue) { m_sDescription = sValue; }

    std::string FormatScore(unsigned int nValue) const
    {
        return MemValue::FormatValue(nValue, m_nFormat);
    }

    struct Entry
    {
        unsigned int m_nRank       = 0U;
        std::string m_sUsername;
        int m_nScore               = 0;
        std::time_t m_TimeAchieved = 0LL;
    };

    void SubmitRankInfo(unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved);
    void ClearRankInfo() { m_RankInfo.clear(); }
    const Entry& GetRankInfo(unsigned int nAt) const { return m_RankInfo.at(nAt); }
    size_t GetRankInfoCount() const { return m_RankInfo.size(); }
    void SortRankInfo();

protected:
    virtual void Start();
    virtual void Cancel();
    virtual void Submit(unsigned int nScore);

private:
    const ra::LeaderboardID m_nID;          //  DB ID for this LB
    ConditionSet            m_startCond;    //  Start monitoring if this is true
    ConditionSet            m_cancelCond;   //  Cancel monitoring if this is true
    ConditionSet            m_submitCond;   //  Submit new score if this is true

    bool                    m_bStarted;     //  False = check start condition. True = check cancel or submit conditions.
    bool                    m_bSubmitted;   //  True if already submitted.

    MemValue                m_value;        //  A collection of memory addresses and values to produce one value.
    MemValue                m_progress;     //  A collection of memory addresses, used to show progress towards completion.
    MemValue::Format        m_nFormat;      //  A format to output. Typically "%d" for score or "%02d:%02d.%02d" for time

    std::string             m_sTitle;       //  The title of the leaderboard
    std::string             m_sDescription; //  

    std::vector<Entry>      m_RankInfo;     //  Recent users ranks
};

/// <summary>
///   Determines whether two <see cref="RA_Leaderboard::Entry" /> are the same.
/// </summary>
/// <param name="a">The current <see cref="RA_Leaderboard::Entry" />.</param>
/// <param name="b">
///   The <see cref="RA_Leaderboard::Entry" /> to test against.
/// </param>
/// <returns><c>true</c> if the are the same, otherwise <c>false</c>.</returns>
/// <remarks>
///   This comparison should almost always return <c>false</c> and should be
///   considered failure if any two <see cref="RA_Leaderboard::Entry" /> have the
///   same ID.
/// </remarks>
_Success_(return != true) _NODISCARD _CONSTANT_FN operator==(
    _In_ const RA_Leaderboard::Entry& a,
    _In_ const RA_Leaderboard::Entry& b) noexcept
{
    assert(a.m_nRank != b.m_nRank);
    return (a.m_nRank == b.m_nRank);
}

/// <summary>
///   Determines whether the ID of <paramref name="a" /> is less than the ID of
///   <paramref name="b" />.
/// </summary>
/// <param name="a">The current <see cref="RA_Leaderboard::Entry" />.</param>
/// <param name="b">
///   The current <see cref="RA_Leaderboard::Entry" /> to test against.
/// </param>
/// <returns>
///   <c>true</c> if the ID of <paramref name="a" /> is less than the ID of
///   <paramref name="b" />., otherwise <c>false</c>.
/// </returns>
/// <remarks>This function's main purpose is to be used for sorting.</remarks>
_Success_(return) _NODISCARD _CONSTANT_FN operator<(
    _In_ const RA_Leaderboard::Entry& a,
    _In_ const RA_Leaderboard::Entry& b) noexcept
{
    return (a.m_nRank < b.m_nRank);
}

#endif // !RA_LEADERBOARD_H
