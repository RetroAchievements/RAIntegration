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
    const RA_Leaderboard& GetLB(size_t iter) const override
    {
        Ensures((iter >= 0) && (iter < Count()));
        return m_Leaderboards.at(iter);
    }
    RA_Leaderboard* FindLB(ra::LeaderboardID nID) noexcept override { return find_lb_impl(*this, nID); }
    const RA_Leaderboard* FindLB(LeaderboardID nID) const noexcept override { return find_lb_impl(*this, nID); } 
    void Clear() override;

    inline auto begin() noexcept { return m_Leaderboards.begin(); }
    inline auto begin() const noexcept { return m_Leaderboards.begin(); }
    inline auto end() noexcept { return m_Leaderboards.end(); }
    inline auto end() const noexcept { return m_Leaderboards.end(); }

private:
    template<typename T>
    GSL_SUPPRESS_F6 static auto find_lb_impl(T& lbm, ra::LeaderboardID nID) noexcept -> decltype(lbm.FindLB(nID))
    {
        for (auto& lb : lbm)
        {
            if (lb.ID() == nID)
                return &lb;
        }

        return nullptr;
    }

    std::vector<RA_Leaderboard> m_Leaderboards;

    const ra::services::IConfiguration& m_pConfiguration;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif !RA_SERVICES_LEADERBOARD_MANAGER_H
