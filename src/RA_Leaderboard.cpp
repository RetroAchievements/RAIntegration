#include "RA_Leaderboard.h"

#include "RA_Defs.h"

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"

#include <ctime>

RA_Leaderboard::~RA_Leaderboard() noexcept
{
    if (m_bActive)
    {
        if (ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            pRuntime.DeactivateLeaderboard(ID());
        }
    }
}

void RA_Leaderboard::ParseFromString(const char* sBuffer, const char* sFormat)
{
    // call with nullptr to determine space required
    const int nSize = rc_lboard_size(sBuffer);
    if (nSize < 0)
    {
        // parse error occurred
        RA_LOG("rc_parse_lboard returned %d", nSize);
        m_pLeaderboard = nullptr;

        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
            ra::StringPrintf(L"Unable to activate leaderboard: %s", Title()),
            ra::StringPrintf(L"Parse error %d", nSize));
    }
    else
    {
        // allocate space and parse again
        m_pLeaderboardBuffer = std::make_shared<std::vector<unsigned char>>(nSize);
        m_pLeaderboard = rc_parse_lboard(m_pLeaderboardBuffer->data(), sBuffer, nullptr, 0);

        m_nFormat = rc_parse_format(sFormat);
    }
}

void RA_Leaderboard::SetActive(bool bActive) noexcept
{
    if (m_bActive != bActive)
    {
        m_bActive = bActive;

        if (m_pLeaderboard && ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            if (!m_bActive)
            {
                pRuntime.DeactivateLeaderboard(ID());

                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().RemoveScoreTracker(ID());
            }
            else
            {
                auto pLeaderboard = static_cast<rc_lboard_t*>(m_pLeaderboard);
                rc_reset_lboard(pLeaderboard);
                pLeaderboard->submitted = 1;

                pRuntime.ActivateLeaderboard(ID(), pLeaderboard);
            }
        }
    }
}

std::string RA_Leaderboard::FormatScore(unsigned int nValue) const
{
    char buffer[32];
    rc_format_value(buffer, sizeof(buffer), nValue, m_nFormat);
    return std::string(buffer);
}

void RA_Leaderboard::SubmitRankInfo(unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved)
{
    if (EntryExists(nRank))
        return;

    // If not found, add new entry.
    m_RankInfo.emplace_back(nRank, sUsername, nScore, nAchieved);
}

bool RA_Leaderboard::EntryExists(unsigned int nRank)
{
    SortRankInfo();
    return (std::binary_search(m_RankInfo.begin(), m_RankInfo.end(), nRank));
}

void RA_Leaderboard::SortRankInfo() { std::sort(m_RankInfo.begin(), m_RankInfo.end()); }
