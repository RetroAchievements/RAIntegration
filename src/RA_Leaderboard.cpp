#include "RA_Leaderboard.h"

#include "RA_Defs.h"
#include "RA_MemManager.h"

#include "services\ILeaderboardManager.hh"
#include "services\ServiceLocator.hh"

#include <ctime>

/* clang-format off */
_Use_decl_annotations_
RA_Leaderboard::RA_Leaderboard(const ra::LeaderboardID nLeaderboardID) noexcept : m_nID(nLeaderboardID) {}
/* clang-format on */


//{"ID":"3","Mem":"STA:0xfe10=h0001_0xhf601=h0c_d0xhf601!=h0c_0xhfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green
//Hill Act 2","Description":"Complete this act in the fastest time!"},

void RA_Leaderboard::ParseFromString(const char* sBuffer, const char* sFormat)
{
    // call with nullptr to determine space required
    const int nSize = rc_lboard_size(sBuffer);
    if (nSize < 0)
    {
        // parse error occurred
        RA_LOG("rc_parse_lboard returned %d", nSize);
        m_pLeaderboard = nullptr;
    }
    else
    {
        // allocate space and parse again
        m_pLeaderboardBuffer = std::make_shared<std::vector<unsigned char>>(nSize);
        m_pLeaderboard = rc_parse_lboard(m_pLeaderboardBuffer->data(), sBuffer, nullptr, 0);

        m_nFormat = rc_parse_format(sFormat);
    }
}

std::string RA_Leaderboard::FormatScore(unsigned int nValue) const
{
    char buffer[32];
    rc_format_value(buffer, sizeof(buffer), nValue, m_nFormat);
    return std::string(buffer);
}

void RA_Leaderboard::Reset() noexcept
{
    if (m_pLeaderboard != nullptr)
    {
        rc_lboard_t* pLboard = static_cast<rc_lboard_t*>(m_pLeaderboard);
        rc_reset_lboard(pLboard);
    }
}

void RA_Leaderboard::Test()
{
    if (m_pLeaderboard != nullptr)
    {
        rc_lboard_t* pLboard = static_cast<rc_lboard_t*>(m_pLeaderboard);
        const int nResult    = rc_evaluate_lboard(pLboard, &m_nCurrentValue, rc_peek_callback, nullptr, nullptr);
        switch (nResult)
        {
            default:
            case RC_LBOARD_INACTIVE:
            case RC_LBOARD_ACTIVE:
                // no change, do nothing
                break;

            case RC_LBOARD_STARTED: Start(); break;

            case RC_LBOARD_CANCELED: Cancel(); break;

            case RC_LBOARD_TRIGGERED: Submit(m_nCurrentValue); break;
        }
    }
}

void RA_Leaderboard::Start()
{
    ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>().ActivateLeaderboard(*this);
}

void RA_Leaderboard::Cancel()
{
    ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>().DeactivateLeaderboard(*this);
}

void RA_Leaderboard::Submit(unsigned int nScore)
{
    ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>().SubmitLeaderboardEntry(*this, nScore);
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
