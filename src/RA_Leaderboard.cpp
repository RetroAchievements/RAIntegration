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
    m_sDefinition = sBuffer;
    m_nFormat = rc_parse_format(sFormat);
}

void RA_Leaderboard::SetActive(bool bActive) noexcept
{
    if (m_bActive != bActive)
    {
        m_bActive = bActive;

        if (!m_sDefinition.empty() && ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            if (!m_bActive)
            {
                pRuntime.DeactivateLeaderboard(ID());

                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().RemoveScoreTracker(ID());
            }
            else
            {
                int nResult = pRuntime.ActivateLeaderboard(ID(), m_sDefinition.c_str());
                if (nResult < 0)
                {
                    // parse error occurred
                    RA_LOG_WARN("rc_parse_trigger returned %d for achievement %u", nResult, ID());

                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                        ra::StringPrintf(L"Unable to activate leaderboard: %s", Title()),
                        ra::StringPrintf(L"Parse error %d\n%s", nResult, rc_error_str(nResult)));
                }
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
