#include "LeaderboardManager.hh"

#include "RA_MemManager.h"
#include "RA_PopupWindows.h"
#include "RA_md5factory.h"
#include "RA_httpthread.h"

#include "services\ServiceLocator.hh"

#include <ctime>

namespace ra {
namespace services {
namespace impl {

LeaderboardManager::LeaderboardManager()
    : LeaderboardManager(ra::services::ServiceLocator::Get<ra::services::IConfiguration>())
{
}

LeaderboardManager::LeaderboardManager(const ra::services::IConfiguration& pConfiguration)
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
                PopupLeaderboardInfo,
                nullptr));
    }

    g_PopupWindows.LeaderboardPopups().Activate(lb.ID());
}

void LeaderboardManager::DeactivateLeaderboard(const RA_Leaderboard& lb) const
{
    g_PopupWindows.LeaderboardPopups().Deactivate(lb.ID());

    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications))
    {
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup("Leaderboard attempt cancelled!",
                lb.Title(),
                PopupLeaderboardCancel,
                nullptr));
    }
}

void LeaderboardManager::SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const
{
    g_PopupWindows.LeaderboardPopups().Deactivate(lb.ID());

    if (g_bRAMTamperedWith)
    {
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup("Not posting to leaderboard: memory tamper detected!",
                "Reset game to reenable posting.",
                PopupInfo));
    }
    else if (!m_pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        g_PopupWindows.AchievementPopups().AddMessage(
            MessagePopup("Leaderboard submission post cancelled.",
                "Enable Hardcore Mode to enable posting.",
                PopupInfo));
    }
    else
    {
        char sValidationSig[50];
        sprintf_s(sValidationSig, 50, "%d%s%d", lb.ID(), RAUsers::LocalUser().Username().c_str(), lb.ID());
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

void LeaderboardManager::OnSubmitEntry(const Document& doc)
{
    if (!doc.HasMember("Response"))
    {
        ASSERT(!"Cannot process this LB Response!");
        return;
    }

    const Value& Response = doc["Response"];

    const Value& LBData = Response["LBData"];

    const std::string& sFormat = LBData["Format"].GetString();
    const ra::LeaderboardID nLBID = static_cast<ra::LeaderboardID>(LBData["LeaderboardID"].GetUint());
    const ra::GameID nGameID = static_cast<ra::GameID>(LBData["GameID"].GetUint());
    const std::string& sLBTitle = LBData["Title"].GetString();
    const bool bLowerIsBetter = (LBData["LowerIsBetter"].GetUint() == 1);

    auto& pLeaderboardManager = ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>();
    RA_Leaderboard* pLB = pLeaderboardManager.FindLB(nLBID);

    const int nSubmittedScore = Response["Score"].GetInt();
    const int nBestScore = Response["BestScore"].GetInt();
    const std::string& sScoreFormatted = Response["ScoreFormatted"].GetString();

    pLB->ClearRankInfo();

    RA_LOG("LB Data, Top Entries:\n");
    const Value& TopEntries = Response["TopEntries"];
    for (SizeType i = 0; i < TopEntries.Size(); ++i)
    {
        const Value& NextEntry = TopEntries[i];

        const unsigned int nRank = NextEntry["Rank"].GetUint();
        const std::string& sUser = NextEntry["User"].GetString();
        const int nUserScore = NextEntry["Score"].GetInt();
        time_t nSubmitted = NextEntry["DateSubmitted"].GetUint();

        RA_LOG(std::string("(" + std::to_string(nRank) + ") " + sUser + ": " + pLB->FormatScore(nUserScore)).c_str());

        pLB->SubmitRankInfo(nRank, sUser, nUserScore, nSubmitted);
    }

    pLB->SortRankInfo();

    const Value& TopEntriesFriends = Response["TopEntriesFriends"];
    const Value& RankData = Response["RankInfo"];

    //	TBD!
    //char sTestData[ 4096 ];
    //sprintf_s( sTestData, 4096, "Leaderboard for %s (%s)\n\n", pLB->Title().c_str(), pLB->Description().c_str() );
    //for( size_t i = 0; i < pLB->GetRankInfoCount(); ++i )
    //{
    //	const LB_Entry& NextScore = pLB->GetRankInfo( i );

    //	std::string sScoreFormatted = pLB->FormatScore( NextScore.m_nScore );

    //	char bufferMessage[ 512 ];
    //	sprintf_s( bufferMessage, 512, "%02d: %s - %s\n", NextScore.m_nRank, NextScore.m_sUsername, sScoreFormatted.c_str() );
    //	strcat_s( sTestData, 4096, bufferMessage );
    //}

    g_PopupWindows.LeaderboardPopups().ShowScoreboard(pLB->ID());
}

void LeaderboardManager::AddLeaderboard(const RA_Leaderboard& lb)
{
    if (m_pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards))	//	If not, simply ignore them.
        m_Leaderboards.push_back(lb);
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
