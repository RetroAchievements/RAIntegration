#ifndef RA_LEADERBOARD_H
#define RA_LEADERBOARD_H
#pragma once

#include "RA_MemValue.h"

class RA_Leaderboard
{
public:
    RA_Leaderboard(_In_ const ra::LeaderboardID nLBID) noexcept;
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
        std::size_t m_nRank{};
        const char* m_sUsername{};
        int         m_nScore{};
        std::time_t m_TimeAchieved{};
    };

    void SubmitRankInfo(_In_                       unsigned int      nRank,
                        _In_z_ _In_range_(1, 2048) const char* const sUsername,
                        _In_                       int               nScore,
                        _In_                       std::time_t       nAchieved) noexcept;
    void ClearRankInfo() { m_RankInfo.clear(); }
    const Entry& GetRankInfo(unsigned int nAt) const { return m_RankInfo.at(nAt); }
    size_t GetRankInfoCount() const { return m_RankInfo.size(); }
    void SortRankInfo();

protected:
    virtual void Start();
    virtual void Cancel();
    virtual void Submit(unsigned int nScore);

private:
    const ra::LeaderboardID m_nID{};      // DB ID for this LB
    ConditionSet			m_startCond;  // Start monitoring if this is true
    ConditionSet			m_cancelCond; // Cancel monitoring if this is true
    ConditionSet			m_submitCond; // Submit new score if this is true

    bool					m_bStarted{};   // False = check start condition. True = check cancel or submit conditions.
    bool                    m_bSubmitted{}; // True if already submitted.

    MemValue				m_value;     // A collection of memory addresses and values to produce one value.
    MemValue				m_progress;  // A collection of memory addresses, used to show progress towards completion.
    MemValue::Format        m_nFormat{ MemValue::Format::Value }; // A format to output. Typically "%d" for score or "%02d:%02d.%02d" for time

    std::string				m_sTitle;       // The title of the leaderboard
    std::string				m_sDescription;	//	

    std::vector<Entry>	    m_RankInfo; // Recent users ranks
};

_Success_(return) _NODISCARD _CONSTANT_FN
operator==(_In_ const RA_Leaderboard::Entry& a, _In_ const RA_Leaderboard::Entry& b) noexcept
{
    return(a.m_nRank == b.m_nRank);
}

_Success_(return) _NODISCARD _CONSTANT_FN
operator<(_In_ const RA_Leaderboard::Entry& a, _In_ const RA_Leaderboard::Entry& b) noexcept
{
    return(a.m_nRank < b.m_nRank);
}

#endif // !RA_LEADERBOARD_H
