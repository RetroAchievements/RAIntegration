#pragma once

#include "RA_Leaderboard.h"

#include <vector>

class RA_LeaderboardManager
{
public:
    static void OnSubmitEntry(const Document& doc);

public:
    void Reset();

    void AddLeaderboard(const RA_Leaderboard& lb);
    void Test();
    void Clear() { m_Leaderboards.clear(); }
    size_t Count() const { return m_Leaderboards.size(); }
    inline RA_Leaderboard& GetLB(size_t iter) { return m_Leaderboards[iter]; }

    RA_Leaderboard* FindLB(unsigned int nID);

public:
    std::vector<RA_Leaderboard> m_Leaderboards;
};

extern RA_LeaderboardManager g_LeaderboardManager;
