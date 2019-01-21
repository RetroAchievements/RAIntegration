#include "LeaderboardManager.hh"

#include "RA_MemManager.h"
#include "RA_md5factory.h"
#include "RA_httpthread.h"
#include "RA_LeaderboardPopup.h"

#include "services\IAudioSystem.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\OverlayManager.hh"

#ifndef RA_UTEST
extern LeaderboardPopup g_LeaderboardPopups;
#endif

namespace ra {
namespace services {
namespace impl {

LeaderboardManager::LeaderboardManager(const ra::services::IConfiguration& pConfiguration) noexcept
    : m_pConfiguration(pConfiguration)
{
}

void LeaderboardManager::ActivateLeaderboard(const RA_Leaderboard& lb) const
{
    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lb.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
            ra::StringPrintf(L"Challenge Available: %s", lb.Title()),
            ra::Widen(lb.Description()));
    }

#ifndef RA_UTEST
    g_LeaderboardPopups.Activate(lb.ID());
#endif
}

void LeaderboardManager::DeactivateLeaderboard(const RA_Leaderboard& lb) const
{
#ifndef RA_UTEST
    g_LeaderboardPopups.Deactivate(lb.ID());
#endif

    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\lbcancel.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
            L"Leaderboard attempt canceled!", ra::Widen(lb.Title()));
    }
}

void LeaderboardManager::SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const
{
#ifndef RA_UTEST
    g_LeaderboardPopups.Deactivate(lb.ID());
#endif

    if (g_bRAMTamperedWith)
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
            L"Not posting to leaderboard: memory tamper detected!", L"Reset game to re-enable posting.");
    }
    else if (!m_pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().PlayAudioFile(L"Overlay\\info.wav");
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().QueueMessage(
            L"Leaderboard submission post canceled.", L"Enable Hardcore Mode to enable posting.");
    }
    else
    {
        char sValidationSig[50];
        sprintf_s(sValidationSig, 50, "%u%s%u", lb.ID(), RAUsers::LocalUser().Username().c_str(), lb.ID());
        std::string sValidationMD5 = RAGenerateMD5(sValidationSig);

        PostArgs args;
        args['u'] = RAUsers::LocalUser().Username();
        args['t'] = RAUsers::LocalUser().Token();
        args['i'] = std::to_string(lb.ID());
        args['v'] = sValidationMD5;
        args['s'] = std::to_string(nValue);

        RAWeb::CreateThreadedHTTPRequest(RequestSubmitLeaderboardEntry, args);
    }
}

void LeaderboardManager::OnSubmitEntry(const rapidjson::Document& doc)
{
    if (!doc.HasMember("Response"))
    {
        ASSERT(!"Cannot process this LB Response!");
        return;
    }

    const auto& Response{ doc["Response"] };
    const auto& LBData{ Response["LBData"] };

    const ra::LeaderboardID nLBID{LBData["LeaderboardID"].GetUint()};
    const auto nGameID{LBData["GameID"].GetUint()};
    const auto bLowerIsBetter{LBData["LowerIsBetter"].GetUint() == 1U};

    auto& pLeaderboardManager = ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>();
    RA_Leaderboard* pLB = pLeaderboardManager.FindLB(nLBID);

    const auto nSubmittedScore{ Response["Score"].GetInt() };
    const auto nBestScore{ Response["BestScore"].GetInt() };
    pLB->ClearRankInfo();

    RA_LOG("LB Data, Top Entries:\n");
    const auto& TopEntries{ Response["TopEntries"] };
    for (auto& NextEntry : TopEntries.GetArray())
    {
        const auto nRank{ NextEntry["Rank"].GetUint() };
        const std::string& sUser{ NextEntry["User"].IsString() ? NextEntry["User"].GetString() : "?????" };
        const auto nUserScore{ NextEntry["Score"].GetInt() };
        const auto nSubmitted{ static_cast<std::time_t>(ra::to_signed(NextEntry["DateSubmitted"].GetUint())) };

        {
            std::ostringstream oss;
            oss << "(" << nRank << ") " << sUser << ": " << pLB->FormatScore(nUserScore);
            auto str{ oss.str() };
            RA_LOG("%s", str.c_str());
        }

        pLB->SubmitRankInfo(nRank, sUser, nUserScore, nSubmitted);
    }

    pLB->SortRankInfo();

#ifndef RA_UTEST
    g_LeaderboardPopups.ShowScoreboard(pLB->ID());
#endif
}

void LeaderboardManager::AddLeaderboard(RA_Leaderboard&& lb)
{
    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards))	//	If not, simply ignore them.
        m_Leaderboards.emplace_back(std::move(lb));
}

void LeaderboardManager::Test()
{
    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards))
    {
        std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
        while (iter != m_Leaderboards.end())
        {
            (*iter).Test();
            iter++;
        }
    }
}

void LeaderboardManager::Reset()
{
    std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
    while (iter != m_Leaderboards.end())
    {
        (*iter).Reset();
        iter++;
    }

#ifndef RA_UTEST
    g_LeaderboardPopups.Reset();
#endif
}

void LeaderboardManager::Clear()
{
    m_Leaderboards.clear();

#ifndef RA_UTEST
    g_LeaderboardPopups.Reset();
#endif
}

} // namespace impl
} // namespace services
} // namespace ra
