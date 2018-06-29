#ifndef RA_SERVICES_ILEADERBOARD_MANAGER_H
#define RA_SERVICES_ILEADERBOARD_MANAGER_H
#pragma once

#include "RA_Leaderboard.h"

namespace ra {
namespace services {

class ILeaderboardManager
{
public:
    virtual void Test() = 0;
    virtual void Reset() = 0;

    virtual void ActivateLeaderboard(const RA_Leaderboard& lb) const = 0;
    virtual void DeactivateLeaderboard(const RA_Leaderboard& lb) const = 0;
    virtual void SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const = 0;

    virtual void AddLeaderboard(const RA_Leaderboard& lb) = 0;
    virtual size_t Count() const = 0;
    virtual const RA_Leaderboard& GetLB(size_t index) const = 0;
    virtual RA_Leaderboard* FindLB(LeaderboardID nID) = 0;
    const RA_Leaderboard* FindLB(LeaderboardID nID) const { return const_cast<ILeaderboardManager*>(this)->FindLB(nID); }
    virtual void Clear() = 0;
};

} // namespace services
} // namespace ra

#endif !RA_SERVICES_ILEADERBOARD_MANAGER_H