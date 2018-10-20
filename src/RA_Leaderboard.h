#ifndef RA_LEADERBOARD_H
#define RA_LEADERBOARD_H
#pragma once

#include "RA_MemValue.h"

class RA_Leaderboard
{
public:
    explicit RA_Leaderboard(_In_ const ra::LeaderboardID nLBID) noexcept;
    virtual ~RA_Leaderboard() noexcept = default;
    RA_Leaderboard(const RA_Leaderboard&) noexcept = delete;
    RA_Leaderboard& operator=(const RA_Leaderboard&) noexcept = delete;
    RA_Leaderboard(RA_Leaderboard&&) noexcept = default;
    RA_Leaderboard& operator=(RA_Leaderboard&&) noexcept = default;

public:
    void ParseFromString(const char* sBuffer, MemValue::Format format);

    void Test();
    virtual void Reset();

    unsigned int GetCurrentValue() const { return m_value.GetValue(); } // Gets the final value for submission
    unsigned int GetCurrentValueProgress() const;                       // Gets the value to display while the leaderboard is active

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
        unsigned int m_nRank;
        std::string m_sUsername;
        int m_nScore;
        time_t m_TimeAchieved;
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
    /// <summary>DB ID for this LB.</summary>
    ra::LeaderboardID m_nID;

    /// <summary>Start monitoring if this is true.</summary>
    ConditionSet m_startCond;

    /// <summary>Cancel monitoring if this is true.</summary>
    ConditionSet m_cancelCond;

    /// <summary>Submit new score if this is true.</summary>
    ConditionSet m_submitCond;

    /// <summary>
    ///   <para><c>false</c> = check start condition.</para>
    ///   <para><c>true</c> = check cancel or submit conditions.</para>
    /// </summary>
    bool m_bStarted{ false };

    /// <summary><c>true</c> if already submitted.</summary>
    bool m_bSubmitted{ false };

    /// <summary>A collection of memory addresses and values to produce one value.</summary>
    MemValue m_value;

    /// <summary>
    ///   A collection of memory addresses, used to show progress towards completion.
    /// </summary>
    MemValue m_progress;

    /// <summary>
    ///   A format to output. Typically '<c>%d</c>' for score or '<c>%02d:%02d.%02d</c>' for time.
    /// </summary>
    MemValue::Format m_nFormat{ MemValue::Format::Value };

    /// <summary>The title of the leaderboard.</summary>
    std::string m_sTitle;

    /// <summary>A description of the leaderboard.</summary>
    std::string m_sDescription;

    /// <summary>Recent users ranks.</summary>
    std::vector<Entry> m_RankInfo;
};

#endif // !RA_LEADERBOARD_H
