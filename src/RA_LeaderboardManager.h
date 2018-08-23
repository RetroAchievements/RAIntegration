#ifndef RA_LEADERBOARD_MANAGER_H
#define RA_LEADERBOARD_MANAGER_H
#pragma once

#include "RA_Leaderboard.h"

#if RA_EXPORTS
#define RAPIDJSON_NOMEMBERITERATORCLASS
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#endif // RA_EXPORTS

#include <vector>

class RA_LeaderboardManager
{
public:
#if RA_EXPORTS
    static void OnSubmitEntry(const rapidjson::Document& doc);
#endif // RA_EXPORTS


public:
    void Reset();

    void AddLeaderboard(const RA_Leaderboard& lb);
    void Test();
    void Clear() { m_Leaderboards.clear(); }
    size_t Count() const { return m_Leaderboards.size(); }
    inline RA_Leaderboard& GetLB(size_t iter) { return m_Leaderboards[iter]; }

    RA_Leaderboard* FindLB(ra::LeaderboardID nID);

    void ActivateLeaderboard(const RA_Leaderboard& lb) const;
    void DeactivateLeaderboard(const RA_Leaderboard& lb) const;
    void SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const;

private:
    std::vector<RA_Leaderboard> m_Leaderboards;
};

extern RA_LeaderboardManager g_LeaderboardManager;

#endif !RA_LEADERBOARD_MANAGER_H
