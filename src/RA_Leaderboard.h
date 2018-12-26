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
    virtual void Reset() noexcept;

    unsigned int GetCurrentValue() const noexcept { return m_nCurrentValue; }

    ra::LeaderboardID ID() const noexcept { return m_nID; }

    const std::string& Title() const noexcept { return m_sTitle; }
    void SetTitle(const std::string& sValue) { m_sTitle = sValue; }

    const std::string& Description() const noexcept { return m_sDescription; }
    void SetDescription(const std::string& sValue) { m_sDescription = sValue; }

    std::string FormatScore(unsigned int nValue) const;

    struct Entry
    {
        Entry() noexcept = default;
        explicit Entry(unsigned int nRank, const std::string& sUsername, int nScore, std::time_t nAchieved) :
            m_nRank{nRank},
            m_sUsername{sUsername},
            m_nScore{nScore},
            m_TimeAchieved{nAchieved}
        {}
        ~Entry() noexcept = default;
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
        Entry(Entry&&) noexcept = default;
        Entry& operator=(Entry&&) noexcept = default;

        unsigned int m_nRank{};
        std::string m_sUsername;
        int m_nScore{};
        std::time_t m_TimeAchieved{};
    };

    void SubmitRankInfo(unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved);
    bool EntryExists(unsigned int nRank);
    void ClearRankInfo() noexcept { m_RankInfo.clear(); }
    const Entry& GetRankInfo(unsigned int nAt) const { return m_RankInfo.at(nAt); }
    size_t GetRankInfoCount() const noexcept { return m_RankInfo.size(); }
    void SortRankInfo();

protected:
    virtual void Start();
    virtual void Cancel();
    virtual void Submit(unsigned int nScore);

private:
    const ra::LeaderboardID m_nID = ra::LeaderboardID(); //  DB ID for this LB

    void* m_pLeaderboard = nullptr;                                   //  rc_lboard_t
    std::shared_ptr<std::vector<unsigned char>> m_pLeaderboardBuffer; //  buffer for rc_lboard_t
    unsigned int m_nCurrentValue = 0U;

    int m_nFormat = 0; // A format to output. Typically "%d" for score or "%02d:%02d.%02d" for time

    std::string m_sTitle;       //  The title of the leaderboard
    std::string m_sDescription; //  A brief description of the leaderboard

    std::vector<Entry> m_RankInfo; //  Recent users ranks
};

// Comparison
_NODISCARD _CONSTANT_FN operator==(const RA_Leaderboard::Entry& a, const RA_Leaderboard::Entry& b) noexcept
{
    return (a.m_nRank == b.m_nRank);
}

// Sorting
_NODISCARD _CONSTANT_FN operator<(const RA_Leaderboard::Entry& a, const RA_Leaderboard::Entry& b) noexcept
{
    return (a.m_nRank < b.m_nRank);
}

// binary search
_NODISCARD _CONSTANT_FN operator<(unsigned int key, const RA_Leaderboard::Entry& value) noexcept
{
    return key < value.m_nRank;
}

_NODISCARD _CONSTANT_FN operator<(const RA_Leaderboard::Entry& value, unsigned int key) noexcept
{
    return value.m_nRank < key;
}

#endif // !RA_LEADERBOARD_H
