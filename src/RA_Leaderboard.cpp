#include "RA_Leaderboard.h"

#include "services\ILeaderboardManager.hh"
#include "services\ServiceLocator.hh"

#ifndef PCH_H
#include <ctime>  
#endif /* !PCH_H */

_Use_decl_annotations_
RA_Leaderboard::RA_Leaderboard(const ra::LeaderboardID nLeaderboardID) noexcept :
    m_nID{ nLeaderboardID }
{
}

//{"ID":"3","Mem":"STA:0xfe10=h0001_0xhf601=h0c_d0xhf601!=h0c_0xhfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 2","Description":"Complete this act in the fastest time!"},

void RA_Leaderboard::ParseFromString(const char* pChar, MemValue::Format nFormat)
{
    m_nFormat = nFormat;

    while (*pChar != '\n' && *pChar != '\0')
    {
        if (pChar[0] == ':' && pChar[1] == ':')	//	New Phrase (double colon)
            pChar += 2;

        if (std::string("STA:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            m_startCond.ParseFromString(pChar);
        }
        else if (std::string("CAN:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            m_cancelCond.ParseFromString(pChar);
        }
        else if (std::string("SUB:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            m_submitCond.ParseFromString(pChar);
        }
        else if (std::string("VAL:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            pChar = m_value.ParseFromString(pChar);
        }
        else if (std::string("PRO:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            pChar = m_progress.ParseFromString(pChar);
        }
        else
        {
            //	badly formatted... cannot progress!
            m_startCond.Clear();
            ASSERT(!"Badly formatted: this leaderboard makes no sense!");
            break;
        }
    }
}

unsigned int RA_Leaderboard::GetCurrentValueProgress() const
{
    if (!m_progress.IsEmpty())
        return m_progress.GetValue();
    else
        return m_value.GetValue();
}

void RA_Leaderboard::Reset()
{
    m_bStarted = false;
    m_bSubmitted = false;

    m_startCond.Reset();
    m_cancelCond.Reset();
    m_submitCond.Reset();
}

void RA_Leaderboard::Test()
{
    bool bUnused;

    //	ASSERT: these are always tested once every frame, to ensure delta variables work properly
    bool bStartOK = m_startCond.Test(bUnused, bUnused);
    bool bCancelOK = m_cancelCond.Test(bUnused, bUnused);
    bool bSubmitOK = m_submitCond.Test(bUnused, bUnused);

    if (m_bSubmitted)
    {
        // if we've already submitted or canceled the leaderboard, don't reactivate it until it becomes inactive.
        if (!bStartOK)
            m_bSubmitted = false;
    }
    else if (!m_bStarted)
    {
        // leaderboard is not active, if the start condition is true, activate it
        if (bStartOK && !bCancelOK)
        {
            if (bSubmitOK)
            {
                // start and submit both true in the same frame, just submit without announcing the leaderboard is available
                Submit(GetCurrentValue());

                // prevent multiple submissions/notifications
                m_bSubmitted = true;
            }
            else if (m_startCond.GroupCount() > 0)
            {
                m_bStarted = true;
                Start();
            }
        }
    }
    else
    {
        // leaderboard is active
        if (bCancelOK)
        {
            // cancel condition is true, deactivate the leaderboard
            m_bStarted = false;
            Cancel();

            // prevent multiple cancel notifications
            m_bSubmitted = true;
        }
        else if (bSubmitOK)
        {
            // submit condition is true, submit the current value
            m_bStarted = false;
            Submit(GetCurrentValue());

            // prevent multiple submissions/notifications
            m_bSubmitted = true;
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

_Use_decl_annotations_
void RA_Leaderboard::SubmitRankInfo(unsigned int nRank,
                                    const char* const sUsername,
                                    int nScore,
                                    std::time_t nAchieved) noexcept
{
    Entry entry{ nRank, sUsername, nScore, nAchieved };
    if (std::binary_search(m_RankInfo.begin(), m_RankInfo.end(), entry))
        return;

    //	If not found, add new entry.
    m_RankInfo.emplace_back(std::move_if_noexcept(entry));
}

void RA_Leaderboard::SortRankInfo()
{
    std::sort(m_RankInfo.begin(), m_RankInfo.end());
}

