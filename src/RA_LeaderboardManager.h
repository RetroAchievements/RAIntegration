#ifndef RA_LEADERBOARD_MANAGER_H
#define RA_LEADERBOARD_MANAGER_H
#pragma once

#include "RA_Leaderboard.h"

// These should be included already, but just in-case. They look faded to me -SBS
#ifndef RAPIDJSON_DOCUMENT_H_
#include <rapidjson/document.h>
#endif // !RAPIDJSON_DOCUMENT_H_

#ifndef _VECTOR_
#include <vector>
#endif // !_VECTOR_

class RA_LeaderboardManager
{
public:
    static void OnSubmitEntry(const rapidjson::Document& doc);

public:
    void Reset();

    void AddLeaderboard(RA_Leaderboard&& lb) noexcept;
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

#endif /* !RA_LEADERBOARD_MANAGER_H */
