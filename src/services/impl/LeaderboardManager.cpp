#include "LeaderboardManager.hh"

#include "RA_MemManager.h"
#include "RA_PopupWindows.h"
#include "RA_md5factory.h"
#include "RA_httpthread.h"

#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace impl {

LeaderboardManager::LeaderboardManager(const ra::services::IConfiguration& pConfiguration) noexcept
    : m_pConfiguration(pConfiguration)
{
}

RA_Leaderboard* LeaderboardManager::FindLB(ra::LeaderboardID nID)
{
    std::vector<RA_Leaderboard>::iterator iter = m_Leaderboards.begin();
    while (iter != m_Leaderboards.end())
    {
        if ((*iter).ID() == nID)
            return &(*iter);

        iter++;
    }

    return nullptr;
}

void LeaderboardManager::ActivateLeaderboard(const RA_Leaderboard& lb) const
{
    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
    {
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup("Challenge Available: " + lb.Title(),
            lb.Description(),
            PopupLeaderboardInfo));
    }

    g_PopupWindows.LeaderboardPopups().Activate(lb.ID());
}

void LeaderboardManager::DeactivateLeaderboard(const RA_Leaderboard& lb) const
{
    g_PopupWindows.LeaderboardPopups().Deactivate(lb.ID());

    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
    {
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup("Leaderboard attempt canceled!",
            lb.Title(),
            PopupLeaderboardCancel));
    }
}

void LeaderboardManager::SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const
{
    g_PopupWindows.LeaderboardPopups().Deactivate(lb.ID());

    if (g_bRAMTamperedWith)
    {
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup("Not posting to leaderboard: memory tamper detected!",
            "Reset game to re-enable posting.",
            PopupInfo));
    }
    else if (!m_pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup("Leaderboard submission post canceled.",
            "Enable Hardcore Mode to enable posting.",
            PopupInfo));
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

    const auto nLBID{ static_cast<ra::LeaderboardID>(LBData["LeaderboardID"].GetUint()) };
    const auto nGameID{ LBData["GameID"].GetUint() };
    const auto bLowerIsBetter{ LBData["LowerIsBetter"].GetUint() == 1U };

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
        const std::string& sUser{ NextEntry["User"].GetString() };
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
#pragma region TBD
    //char sTestData[ 4096 ];
    //sprintf_s( sTestData, 4096, "Leaderboard for %s (%s)\n\n", pLB->Title().c_str(), pLB->Description().c_str() );
    //for( size_t i = 0; i < pLB->GetRankInfoCount(); ++i )
    //{
    // const LB_Entry& NextScore = pLB->GetRankInfo( i );

    // std::string sScoreFormatted = pLB->FormatScore( NextScore.m_nScore );

    // char bufferMessage[ 512 ];
    // sprintf_s( bufferMessage, 512, "%02d: %s - %s\n", NextScore.m_nRank, NextScore.m_sUsername, sScoreFormatted.c_str() );
    // strcat_s( sTestData, 4096, bufferMessage );
    //}  
#pragma endregion
    g_PopupWindows.LeaderboardPopups().ShowScoreboard(pLB->ID());
}

void LeaderboardManager::AddLeaderboard(RA_Leaderboard&& lb)
{
    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards)) // If not, simply ignore them.
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
}

} // namespace impl
} // namespace services
} // namespace ra
