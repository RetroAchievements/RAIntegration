#ifndef RA_SERVICES_LEADERBOARD_MANAGER_H
#define RA_SERVICES_LEADERBOARD_MANAGER_H
#pragma once

#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"

#include "RA_Json.h" // rapidjson::Document

namespace ra {
namespace services {
namespace impl {

class LeaderboardManager : public ILeaderboardManager
{
public:
    explicit LeaderboardManager(const ra::services::IConfiguration& pConfiguration) noexcept;

    static void OnSubmitEntry(const rapidjson::Document& doc);

public:
    void Test() override;
    void Reset() override;

    void ActivateLeaderboard(const RA_Leaderboard& lb) const override;
    void DeactivateLeaderboard(const RA_Leaderboard& lb) const override;
    void SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const override;

    void AddLeaderboard(RA_Leaderboard&& lb) override;
    size_t Count() const noexcept override { return m_Leaderboards.size(); }
    const RA_Leaderboard& GetLB(size_t iter) const override { return m_Leaderboards[iter]; }
    RA_Leaderboard* FindLB(ra::LeaderboardID nID) override;
    void Clear() noexcept override;

private:
    std::vector<RA_Leaderboard> m_Leaderboards;

    const ra::services::IConfiguration& m_pConfiguration;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif !RA_SERVICES_LEADERBOARD_MANAGER_H
