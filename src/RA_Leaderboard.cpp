#include "RA_Leaderboard.h"

#include "RA_MemManager.h"

#ifndef RA_UTEST
#include "RA_LeaderboardManager.h"
#endif

#include "rcheevos/include/rcheevos.h"

#include <ctime>

RA_Leaderboard::RA_Leaderboard(const ra::LeaderboardID nLeaderboardID) :
    m_nID(nLeaderboardID)
{
}

RA_Leaderboard::~RA_Leaderboard()
{
}

//{"ID":"3","Mem":"STA:0xfe10=h0001_0xhf601=h0c_d0xhf601!=h0c_0xhfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 2","Description":"Complete this act in the fastest time!"},

void RA_Leaderboard::ParseFromString(const char* pChar, MemValue::Format nFormat)
{
    // call with nullptr to determine space required
    int nResult;
    rc_parse_lboard(&nResult, nullptr, pChar, nullptr, 0);

    if (nResult < 0)
    {
        // parse error occurred
        RA_LOG("rc_parse_lboard returned %d", nResult);
        m_pLeaderboard = nullptr;
    }
    else
    {
        // allocate space and parse again
        m_pLeaderboardBuffer.resize(nResult);
        m_pLeaderboard = rc_parse_lboard(&nResult, static_cast<void*>(m_pLeaderboardBuffer.data()), pChar, nullptr, 0);
    }
}

void RA_Leaderboard::Reset()
{
    if (m_pLeaderboard != nullptr)
    {
        rc_lboard_t* pLboard = static_cast<rc_lboard_t*>(m_pLeaderboard);
        rc_reset_lboard(pLboard);
    }
}

static unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, void* pData)
{
    switch (nBytes)
    {
        case 1:
            return g_MemManager.ActiveBankRAMRead(nAddress, EightBit);
        case 2:
            return g_MemManager.ActiveBankRAMRead(nAddress, SixteenBit);
        case 4:
            return g_MemManager.ActiveBankRAMRead(nAddress, ThirtyTwoBit);
        default:
            return 0;
    }
}

void RA_Leaderboard::Test()
{
    if (m_pLeaderboard != nullptr)
    {
        rc_lboard_t* pLboard = static_cast<rc_lboard_t*>(m_pLeaderboard);
        int nResult = rc_evaluate_lboard(pLboard, &m_nCurrentValue, rc_peek_callback, nullptr, nullptr);
        switch (nResult)
        {
            default:
            case RC_LBOARD_INACTIVE:
            case RC_LBOARD_ACTIVE:
                // no change, do nothing
                break;

            case RC_LBOARD_STARTED:
                Start();
                break;

            case RC_LBOARD_CANCELED:
                Cancel();
                break;

            case RC_LBOARD_TRIGGERED:
                Submit(m_nCurrentValue);
                break;
        }
    }
}

void RA_Leaderboard::Start()
{
#ifndef RA_UTEST
    g_LeaderboardManager.ActivateLeaderboard(*this);
#endif
}

void RA_Leaderboard::Cancel()
{
#ifndef RA_UTEST
    g_LeaderboardManager.DeactivateLeaderboard(*this);
#endif
}

void RA_Leaderboard::Submit(unsigned int nScore)
{
#ifndef RA_UTEST
    g_LeaderboardManager.SubmitLeaderboardEntry(*this, nScore);
#endif
}

void RA_Leaderboard::SubmitRankInfo(unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved)
{
    Entry newEntry;
    newEntry.m_nRank = nRank;
    newEntry.m_sUsername = sUsername;
    newEntry.m_nScore = nScore;
    newEntry.m_TimeAchieved = nAchieved;

    std::vector<Entry>::iterator iter = m_RankInfo.begin();
    while (iter != m_RankInfo.end())
    {
        if ((*iter).m_nRank == nRank)
        {
            (*iter) = newEntry;
            return;
        }
        iter++;
    }

    //	If not found, add new entry.
    m_RankInfo.push_back(newEntry);
}

void RA_Leaderboard::SortRankInfo()
{
    for (size_t i = 0; i < m_RankInfo.size(); ++i)
    {
        for (size_t j = i + 1; j < m_RankInfo.size(); ++j)
        {
            if (m_RankInfo.at(i).m_nRank > m_RankInfo.at(j).m_nRank)
            {
                Entry temp = m_RankInfo[i];
                m_RankInfo[i] = m_RankInfo[j];
                m_RankInfo[j] = temp;
            }
        }
    }
}

