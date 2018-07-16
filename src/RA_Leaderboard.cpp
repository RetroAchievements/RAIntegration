#include "RA_Leaderboard.h"

#include "services\ILeaderboardManager.hh"
#include "services\ServiceLocator.hh"

#include <ctime>

RA_Leaderboard::RA_Leaderboard(const ra::LeaderboardID nLeaderboardID) :
    m_nID(nLeaderboardID),
    m_bStarted(false),
    m_bSubmitted(false),
    m_format(MemValue::Format::Value)
{
}

RA_Leaderboard::~RA_Leaderboard()
{
}

//{"ID":"3","Mem":"STA:0xfe10=h0001_0xhf601=h0c_d0xhf601!=h0c_0xhfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 2","Description":"Complete this act in the fastest time!"},

void RA_Leaderboard::ParseFromString(const char* pChar, MemValue::Format nFormat)
{
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

            // temporary backwards compatibility support: all conditions in CANCEL should be OR'd:
            if (m_cancelCond.GroupCount() == 1 && m_cancelCond.GetGroup(0).Count() > 1)
            {
                for (size_t i = 0; i < m_cancelCond.GetGroup(0).Count(); ++i)
                {
                    m_cancelCond.AddGroup();
                    m_cancelCond.GetGroup(i + 1).Add(m_cancelCond.GetGroup(0).GetAt(i));
                }

                m_cancelCond.GetGroup(0).Clear();
            }
            // end backwards compatibility conversion
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

