#ifndef RA_SERVICES_LEADERBOARD_MANAGER_H
#define RA_SERVICES_LEADERBOARD_MANAGER_H
#pragma once

#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"

#include <rapidjson\include\rapidjson\document.h>
#include <vector>

namespace ra {
namespace services {
namespace impl {

class LeaderboardManager : public ILeaderboardManager
{
public:
    LeaderboardManager();
    LeaderboardManager(const ra::services::IConfiguration& pConfiguration);

    static void OnSubmitEntry(const rapidjson::Document& doc);

public:
    void Test() override;
    void Reset() override;

    void ActivateLeaderboard(const RA_Leaderboard& lb) const override;
    void DeactivateLeaderboard(const RA_Leaderboard& lb) const override;
    void SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const override;

    void AddLeaderboard(const RA_Leaderboard& lb) override;
    size_t Count() const override { return m_Leaderboards.size(); }
    const RA_Leaderboard& GetLB(size_t iter) const override { return m_Leaderboards[iter]; }
    RA_Leaderboard* FindLB(ra::LeaderboardID nID) override;
    void Clear() override { m_Leaderboards.clear(); }

private:
    std::vector<RA_Leaderboard> m_Leaderboards;

    const ra::services::IConfiguration& m_pConfiguration;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif !RA_SERVICES_LEADERBOARD_MANAGER_H