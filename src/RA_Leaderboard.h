#ifndef RA_LEADERBOARD_H
#define RA_LEADERBOARD_H
#pragma once

#include "ra_fwd.h"

class RA_Leaderboard
{
public:
    explicit RA_Leaderboard(_In_ const ra::LeaderboardID nLBID) noexcept;
    virtual ~RA_Leaderboard() noexcept = default;
    RA_Leaderboard(const RA_Leaderboard&) noexcept = delete;
    RA_Leaderboard& operator=(const RA_Leaderboard&) noexcept = delete;
    RA_Leaderboard(RA_Leaderboard&&) noexcept = default;
    RA_Leaderboard& operator=(RA_Leaderboard&&) noexcept = default;

    void ParseFromString(const char* sBuffer, const char* sFormat);

    void Test();
    virtual void Reset();

    unsigned int GetCurrentValue() const { return m_nCurrentValue; }

    ra::LeaderboardID ID() const { return m_nID; }

    const std::string& Title() const { return m_sTitle; }
    void SetTitle(const std::string& sValue) { m_sTitle = sValue; }

    const std::string& Description() const { return m_sDescription; }
    void SetDescription(const std::string& sValue) { m_sDescription = sValue; }

    std::string FormatScore(unsigned int nValue) const;

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
    const ra::LeaderboardID          m_nID;                 //  DB ID for this LB

    void*                            m_pLeaderboard;        //  rc_lboard_t
    std::shared_ptr<unsigned char[]> m_pLeaderboardBuffer;  //  buffer for rc_lboard_t
    unsigned int                     m_nCurrentValue = 0;

    int                              m_nFormat = 0;         // A format to output. Typically "%d" for score or "%02d:%02d.%02d" for time

    std::string                      m_sTitle;              //  The title of the leaderboard
    std::string                      m_sDescription;        //  A brief description of the leaderboard

    std::vector<Entry>               m_RankInfo;            //  Recent users ranks
};

#endif // !RA_LEADERBOARD_H
