#include "RA_Leaderboard.h"

#include "RA_LeaderboardManager.h"
#include "RA_MemManager.h"
#include "RA_PopupWindows.h"
#include "RA_md5factory.h"
#include "RA_httpthread.h"

#include <time.h>


namespace {
const char* FormatTypeToString[] =
{
    "TIME",			//	TimeFrames
    "TIMESECS",		//	TimeSecs
    "MILLISECS",	//	TimeMillisecs
    "SCORE",		//	Score	/POINTS
    "VALUE",		//	Value
    "OTHER",		//	Other
};
static_assert(SIZEOF_ARRAY(FormatTypeToString) == RA_Leaderboard::Format__MAX, "These must match!");
}

RA_Leaderboard::RA_Leaderboard(const LeaderboardID nLeaderboardID) :
    m_nID(nLeaderboardID),
    m_bStarted(false),
    m_bSubmitted(false),
    m_format(Format_Value)
{
}

RA_Leaderboard::~RA_Leaderboard()
{
}

//{"ID":"3","Mem":"STA:0xfe10=h0001_0xhf601=h0c_d0xhf601!=h0c_0xhfffb=0::CAN:0xhfe13<d0xhfe13::SUB:0xf7cc!=0_d0xf7cc=0::VAL:0xhfe24*1_0xhfe25*60_0xhfe22*3600","Format":"TIME","Title":"Green Hill Act 2","Description":"Complete this act in the fastest time!"},

void RA_Leaderboard::LoadFromJSON(const Value& element)
{
    const LeaderboardID nID = element["ID"].GetUint();
    ASSERT(nID == m_nID);	//	Must match!

    const std::string sMem = element["Mem"].GetString();
    char buffer[4096];
    strcpy_s(buffer, 4096, sMem.c_str());
    ParseLBData(buffer);

    m_sTitle = element["Title"].GetString();
    m_sDescription = element["Description"].GetString();

    const std::string sFmt = element["Format"].GetString();
    for (size_t i = 0; i < Format__MAX; ++i)
    {
        if (sFmt.compare(FormatTypeToString[i]) == 0)
        {
            m_format = (FormatType)i;
            break;
        }
    }
}

void RA_Leaderboard::ParseLBData(const char* pChar)
{
    while (((*pChar) != '\n') && ((*pChar) != '\0'))
    {
        if ((pChar[0] == ':') && (pChar[1] == ':'))	//	New Phrase (double colon)
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
            //	Parse Value condition
            //	Value is a collection of memory addresses and modifiers.
            do
            {
                while ((*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' || (*pChar) == '$')
                    pChar++; // Skip any chars up til this point :S

                MemValue newMemVal;
                pChar = newMemVal.ParseFromString(pChar);
                m_value.AddNewValue(newMemVal);

                switch (*pChar)
                {
                    case ('$'): m_sOperations.push_back(MemValueSet::Operation_Maximum); break;
                    case ('_'): m_sOperations.push_back(MemValueSet::Operation_Addition); break;
                }
            } while (*pChar == '_' || (*pChar) == '$');
        }
        else if (std::string("PRO:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            //	Progress: normally same as value:
            //	Progress is a collection of memory addresses and modifiers.
            do
            {
                while ((*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|')
                    pChar++; // Skip any chars up til this point :S

                MemValue newMemVal;
                pChar = newMemVal.ParseFromString(pChar);
                m_progress.AddNewValue(newMemVal);
            } while (*pChar == '_');
        }
        else if (std::string("FOR:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;

            //	Format
            if (strncmp(pChar, "TIMESECS", sizeof("TIMESECS") - 1) == 0)
            {
                m_format = Format_TimeSecs;
                pChar += sizeof("TIMESECS") - 1;
            }
            else if (strncmp(pChar, "TIME", sizeof("TIME") - 1) == 0)
            {
                m_format = Format_TimeFrames;
                pChar += sizeof("TIME") - 1;
            }
            else if (strncmp(pChar, "MILLISECS", sizeof("MILLISECS") - 1) == 0)
            {
                m_format = Format_TimeMillisecs;
                pChar += sizeof("MILLISECS") - 1;
            }
            else if (strncmp(pChar, "POINTS", sizeof("POINTS") - 1) == 0)
            {
                m_format = Format_Score;
                pChar += sizeof("POINTS") - 1;
            }
            else if (strncmp(pChar, "SCORE", sizeof("SCORE") - 1) == 0)	//dupe
            {
                m_format = Format_Score;
                pChar += sizeof("SCORE") - 1;
            }
            else if (strncmp(pChar, "VALUE", sizeof("VALUE") - 1) == 0)
            {
                m_format = Format_Value;
                pChar += sizeof("VALUE") - 1;
            }
            else
            {
                m_format = Format_Other;
                while (isalnum(*pChar))
                    pChar++;
            }
        }
        else if (std::string("TTL:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            //	Title:
            char* pDest = &m_sTitle[0];
            while (!(pChar[0] == ':' && pChar[1] == ':'))
            {
                *pDest = *pChar;	//	char copy
                pDest++;
                pChar++;
            }
            *pDest = '\0';
        }
        else if (std::string("DES:").compare(0, 4, pChar, 0, 4) == 0)
        {
            pChar += 4;
            //	Description:
            char* pDest = &m_sDescription[0];
            while ((pChar[0] != '\n') && (pChar[0] != '\0'))
            {
                *pDest = *pChar;	//	char copy
                pDest++;
                pChar++;
            }
            *pDest = '\0';
        }
        else
        {
            //	badly formatted... cannot progress!
            ASSERT(!"Badly formatted: this leaderboard makes no sense!");
            break;
        }
    }
}

void RA_Leaderboard::ParseLine(char* sBuffer)
{
    char* pChar = &sBuffer[0];
    pChar++;											//	Skip over 'L' character
    ASSERT(m_nID == strtol(pChar, &pChar, 10));		//	Skip over Leaderboard ID
    ParseLBData(pChar);
}

double RA_Leaderboard::GetCurrentValue()
{
    return m_value.GetValue();
}

double RA_Leaderboard::GetCurrentValueProgress() const
{
    if (m_progress.NumValues() > 0)
        return m_progress.GetValue();
    else
        return m_value.GetOperationsValue(m_sOperations);
}

void RA_Leaderboard::Clear()
{
    m_value.Clear();
    m_progress.Clear();
    //	Clear all locally stored lb info too tbd
}

void RA_Leaderboard::Reset()
{
    m_bStarted = false;

    m_startCond.Reset();
    m_cancelCond.Reset();
    m_submitCond.Reset();
}

void RA_Leaderboard::Test()
{
    bool bUnused;

    //	Ensure these are always tested once every frame, to ensure delta
    //	 variables work properly :)
    BOOL bStartOK = m_startCond.Test(bUnused, bUnused);
    BOOL bCancelOK = m_cancelCond.Test(bUnused, bUnused);
    BOOL bSubmitOK = m_submitCond.Test(bUnused, bUnused);

    if (m_bSubmitted)
    {
        // if we've already submitted or canceled the leaderboard, don't reactivate it until it becomes unactive.
        if (!bStartOK)
            m_bSubmitted = false;
    }
    else if (!m_bStarted)
    {
        if (bStartOK)
        {
            m_bStarted = true;

            if (g_bLBDisplayNotification)
            {
                g_PopupWindows.AchievementPopups().AddMessage(
                    MessagePopup("Challenge Available: " + m_sTitle,
                        m_sDescription,
                        PopupLeaderboardInfo,
                        nullptr));
            }

            g_PopupWindows.LeaderboardPopups().Activate(m_nID);
        }
    }
    else
    {
        if (bCancelOK)
        {
            m_bStarted = false;
            g_PopupWindows.LeaderboardPopups().Deactivate(m_nID);

            if (g_bLBDisplayNotification)
            {
                g_PopupWindows.AchievementPopups().AddMessage(
                    MessagePopup("Leaderboard attempt cancelled!",
                        m_sTitle,
                        PopupLeaderboardCancel,
                        nullptr));
            }

            // prevent multiple cancel notifications
            m_bSubmitted = true;
        }
        else if (bSubmitOK)
        {
            m_bStarted = false;
            g_PopupWindows.LeaderboardPopups().Deactivate(m_nID);

            if (g_bRAMTamperedWith)
            {
                g_PopupWindows.AchievementPopups().AddMessage(
                    MessagePopup("Not posting to leaderboard: memory tamper detected!",
                        "Reset game to reenable posting.",
                        PopupInfo));
            }
            else if (g_bHardcoreModeActive)
            {
                //	TBD: move to keys!
                char sValidationSig[50];
                sprintf_s(sValidationSig, 50, "%d%s%d", m_nID, RAUsers::LocalUser().Username().c_str(), m_nID);
                std::string sValidationMD5 = RAGenerateMD5(sValidationSig);

                PostArgs args;
                args['u'] = RAUsers::LocalUser().Username();
                args['t'] = RAUsers::LocalUser().Token();
                args['i'] = std::to_string(m_nID);
                args['v'] = sValidationMD5;
                args['s'] = std::to_string(static_cast<int>(m_value.GetOperationsValue(m_sOperations)));	//	Concern about rounding?

                RAWeb::CreateThreadedHTTPRequest(RequestSubmitLeaderboardEntry, args);
            }
            else
            {
                g_PopupWindows.AchievementPopups().AddMessage(
                    MessagePopup("Leaderboard submission post cancelled.",
                        "Enable Hardcore Mode to enable posting.",
                        PopupInfo));
            }

            // prevent multiple submissions/notifications
            m_bSubmitted = true;
        }
    }
}

void RA_Leaderboard::SubmitRankInfo(unsigned int nRank, const std::string& sUsername, int nScore, time_t nAchieved)
{
    LB_Entry newEntry;
    newEntry.m_nRank = nRank;
    newEntry.m_sUsername = sUsername;
    newEntry.m_nScore = nScore;
    newEntry.m_TimeAchieved = nAchieved;

    std::vector<LB_Entry>::iterator iter = m_RankInfo.begin();
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
        for (size_t j = i; j < m_RankInfo.size(); ++j)
        {
            if (i == j)
                continue;

            if (m_RankInfo.at(i).m_nRank > m_RankInfo.at(j).m_nRank)
            {
                LB_Entry temp = m_RankInfo[i];
                m_RankInfo[i] = m_RankInfo[j];
                m_RankInfo[j] = temp;
            }
        }
    }
}

//	static
std::string RA_Leaderboard::FormatScore(FormatType nType, int nScoreIn)
{
    //	NB. This is accessed by RA_Formattable... ought to be refactored really
    char buffer[256];
    switch (nType)
    {
        case Format_TimeFrames:
        {
            int nMins = nScoreIn / 3600;
            int nSecs = (nScoreIn % 3600) / 60;
            int nMilli = static_cast<int>(((nScoreIn % 3600) % 60) * (100.0 / 60.0));	//	Convert from frames to 'millisecs'
            sprintf_s(buffer, 256, "%02d:%02d.%02d", nMins, nSecs, nMilli);
        }
        break;

        case Format_TimeSecs:
        {
            int nMins = nScoreIn / 60;
            int nSecs = nScoreIn % 60;
            sprintf_s(buffer, 256, "%02d:%02d", nMins, nSecs);
        }
        break;

        case Format_TimeMillisecs:
        {
            int nMins = nScoreIn / 6000;
            int nSecs = (nScoreIn % 6000) / 100;
            int nMilli = static_cast<int>(nScoreIn % 100);
            sprintf_s(buffer, 256, "%02d:%02d.%02d", nMins, nSecs, nMilli);
        }
        break;

        case Format_Score:
            sprintf_s(buffer, 256, "%06d Points", nScoreIn);
            break;

        case Format_Value:
            sprintf_s(buffer, 256, "%01d", nScoreIn);
            break;

        default:
            sprintf_s(buffer, 256, "%06d", nScoreIn);
            break;
    }
    return buffer;
}

std::string RA_Leaderboard::FormatScore(int nScoreIn) const
{
    return FormatScore(m_format, nScoreIn);
}
