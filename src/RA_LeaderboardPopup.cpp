#include "RA_LeaderboardPopup.h"

#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"

#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"
#include "services\ServiceLocator.hh"

//	No emulator-specific code here please!

namespace {

const float SCOREBOARD_APPEAR_AT = 0.8f;
const float SCOREBOARD_FADEOUT_AT = 6.2f;
const float SCOREBOARD_FINISH_AT = 7.0f;

const char* FONT_TO_USE = "Tahoma";

const COLORREF g_ColBG = RGB(32, 32, 32);

const int FONT_SIZE_TITLE = 28;
const int FONT_SIZE_SUBTITLE = 22;
const int FONT_SIZE_TEXT = 16;

}


LeaderboardPopup::LeaderboardPopup()
{
    Reset();
}

void LeaderboardPopup::ShowScoreboard(ra::LeaderboardID nID)
{
    m_vScoreboardQueue.push(nID);

    if (m_fScoreboardShowTimer == SCOREBOARD_FINISH_AT)
    {
        m_fScoreboardShowTimer = 0.0f;
        m_nState = PopupState::ShowingScoreboard;
    }
}

void LeaderboardPopup::Reset()
{
    m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
    while (!m_vScoreboardQueue.empty())
        m_vScoreboardQueue.pop();

    m_vActiveLBIDs.clear();
    m_nState = PopupState::ShowingProgress;
}

void LeaderboardPopup::Update(_UNUSED ControllerInput, float fDelta, _UNUSED BOOL, BOOL bPaused)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards))	//	If not, simply ignore them.
        return;

    if (bPaused)
        fDelta = 0.0f;

    if (m_fScoreboardShowTimer >= SCOREBOARD_FINISH_AT)
    {
        //	No longer showing the scoreboard
        if (!m_vScoreboardQueue.empty())
        {
            m_vScoreboardQueue.pop();

            if (!m_vScoreboardQueue.empty())
            {
                //	Still not empty: restart timer
                m_fScoreboardShowTimer = 0.0f;	//	Show next scoreboard
            }
            else
            {
                m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
                m_nState = PopupState::ShowingProgress;
            }
        }
        else
        {
            m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
            m_nState = PopupState::ShowingProgress;
        }
    }
    else
    {
        m_fScoreboardShowTimer += fDelta;
    }
}

_Use_decl_annotations_
BOOL LeaderboardPopup::Activate(ra::LeaderboardID nLBID)
{
    std::vector<unsigned int>::iterator iter = m_vActiveLBIDs.begin();
    while (iter != m_vActiveLBIDs.end())
    {
        if ((*iter) == nLBID)
            return FALSE;
        iter++;
    }

    m_vActiveLBIDs.push_back(nLBID);
    return TRUE;
}

_Use_decl_annotations_
BOOL LeaderboardPopup::Deactivate(ra::LeaderboardID nLBID)
{
    std::vector<unsigned int>::iterator iter = m_vActiveLBIDs.begin();
    while (iter != m_vActiveLBIDs.end())
    {
        if ((*iter) == nLBID)
        {
            m_vActiveLBIDs.erase(iter);
            return TRUE;
        }
        iter++;
    }

    RA_LOG("Could not deactivate leaderboard %u", nLBID);

    return FALSE;
}

float LeaderboardPopup::GetOffsetPct() const
{
    float fVal = 0.0f;

    if (m_fScoreboardShowTimer < SCOREBOARD_APPEAR_AT)
    {
        //	Fading in.
        float fDelta = (SCOREBOARD_APPEAR_AT - m_fScoreboardShowTimer);

        fDelta *= fDelta;	//	Quadratic

        fVal = fDelta;
    }
    else if (m_fScoreboardShowTimer < (SCOREBOARD_FADEOUT_AT))
    {
        //	Faded in - held
        fVal = 0.0f;
    }
    else if (m_fScoreboardShowTimer < (SCOREBOARD_FINISH_AT))
    {
        //	Fading out
        float fDelta = (SCOREBOARD_FADEOUT_AT - m_fScoreboardShowTimer);

        fDelta *= fDelta;	//	Quadratic

        fVal = (fDelta);
    }
    else
    {
        //	Finished!
        fVal = 1.0f;
    }

    return fVal;
}

_Use_decl_annotations_
void LeaderboardPopup::Render(HDC hDC, const RECT& rcDest)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards))	//	If not, simply ignore them.
        return;

    SetBkColor(hDC, COL_TEXT_HIGHLIGHT);
    SetTextColor(hDC, COL_POPUP);

    HFONT hFontTitle = CreateFont(FONT_SIZE_TITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
        DEFAULT_PITCH, NativeStr(FONT_TO_USE).c_str());

    HFONT hFontDesc = CreateFont(FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
        DEFAULT_PITCH, NativeStr(FONT_TO_USE).c_str());

    HFONT hFontText = CreateFont(FONT_SIZE_TEXT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
        DEFAULT_PITCH, NativeStr(FONT_TO_USE).c_str());


    const int nWidth = rcDest.right - rcDest.left;
    const int nHeight = rcDest.bottom - rcDest.top;

    //float fOffscreenAmount = ( GetOffsetPct() * ( POPUP_DIST_FROM_PCT * (float)nWidth ) );
    const float fOffscreenAmount = (GetOffsetPct() * 600);
    //float fFadeOffs = (POPUP_DIST_TO_PCT * (float)nWidth ) + fOffscreenAmount;
    const float fFadeOffs = (nWidth - 300) + fOffscreenAmount;

    const int nScoreboardX = (int)fFadeOffs;
    const int nScoreboardY = (int)nHeight - 200;

    const int nRightLim = (int)((nWidth - 8) + fOffscreenAmount);

    //if( GetMessageType() == 1 )
    //{
    //	DrawImage( hDC, GetImage(), nTitleX, nTitleY, 64, 64 );

    //	nTitleX += 64+4+2;	//	Negate the 2 from earlier!
    //	nDescX += 64+4;
    //}


    HGDIOBJ hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));

    HBRUSH hBrushBG = CreateSolidBrush(g_ColBG);

    RECT rcScoreboardFrame;
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards))
    {
        SetRect(&rcScoreboardFrame, nScoreboardX, nScoreboardY, nRightLim, nHeight - 8);
        InflateRect(&rcScoreboardFrame, 2, 2);
    }
    else
        SetRect(&rcScoreboardFrame, 0, 0, 0, 0);
    FillRect(hDC, &rcScoreboardFrame, hBrushBG);

    HGDIOBJ hOld = SelectObject(hDC, hFontDesc);

    switch (m_nState)
    {
        case PopupState::ShowingProgress:
        {
            if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters))
                break;

            auto& pLeaderboardManager = ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>();
            int nProgressYOffs = 0;
            std::vector<unsigned int>::const_iterator iter = m_vActiveLBIDs.begin();
            while (iter != m_vActiveLBIDs.end())
            {
                const RA_Leaderboard* pLB = pLeaderboardManager.FindLB(*iter);
                if (pLB != nullptr)
                {
                    //	Show current progress:
                    std::string sScoreSoFar = std::string(" ") + pLB->FormatScore(static_cast<int>(pLB->GetCurrentValue())) + std::string(" ");

                    SIZE szProgress;
                    GetTextExtentPoint32(hDC, NativeStr(sScoreSoFar).c_str(), sScoreSoFar.length(), &szProgress);

                    HGDIOBJ hBkup = SelectObject(hDC, hPen);

                    MoveToEx(hDC, nWidth - 8, nHeight - 8 - szProgress.cy + nProgressYOffs, nullptr);
                    LineTo(hDC, nWidth - 8, nHeight - 8 + nProgressYOffs);							//	down
                    LineTo(hDC, nWidth - 8 - szProgress.cx, nHeight - 8 + nProgressYOffs);			//	left

                    RECT rcProgress;
                    SetRect(&rcProgress, 0, 0, nWidth - 8, nHeight - 8 + nProgressYOffs);
                    DrawText(hDC, NativeStr(sScoreSoFar).c_str(), sScoreSoFar.length(), &rcProgress, DT_BOTTOM | DT_RIGHT | DT_SINGLELINE);

                    SelectObject(hDC, hBkup);
                    nProgressYOffs -= 26;
                }

                iter++;
            }
        }
        break;

        case PopupState::ShowingScoreboard:
        {
            if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards))
                break;

            auto& pLeaderboardManager = ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>();
            const RA_Leaderboard* pLB = pLeaderboardManager.FindLB(m_vScoreboardQueue.front());
            if (pLB != nullptr)
            {
                char buffer[1024];
                sprintf_s(buffer, 1024, " Results: %s ", pLB->Title().c_str());
                RECT rcTitle = { nScoreboardX + 2, nScoreboardY + 2, nRightLim - 2, nHeight - 8 };
                DrawText(hDC, NativeStr(buffer).c_str(), strlen(buffer), &rcTitle, DT_TOP | DT_LEFT | DT_SINGLELINE);

                //	Show scoreboard
                RECT rcScoreboard = { nScoreboardX + 2, nScoreboardY + 32, nRightLim - 2, nHeight - 16 };
                for (size_t i = 0; i < pLB->GetRankInfoCount(); ++i)
                {
                    const RA_Leaderboard::Entry& lbInfo = pLB->GetRankInfo(i);

                    if (lbInfo.m_sUsername.compare(RAUsers::LocalUser().Username()) == 0)
                    {
                        SetBkMode(hDC, OPAQUE);
                        SetTextColor(hDC, COL_POPUP);
                    }
                    else
                    {
                        SetBkMode(hDC, TRANSPARENT);
                        SetTextColor(hDC, COL_TEXT_HIGHLIGHT);
                    }

                    
                    {
                        const auto str{
                            ra::StringPrintf(_T(" %u %s "), lbInfo.m_nRank, lbInfo.m_sUsername.c_str())
                        };

                        DrawText(hDC, str.c_str(), ra::narrow_cast<int>(str.length()),
                                    &rcScoreboard, DT_TOP | DT_LEFT | DT_SINGLELINE);
                    }
                    std::string sScore(" " + pLB->FormatScore(lbInfo.m_nScore) + " ");
                    DrawText(hDC, NativeStr(sScore).c_str(), sScore.length(), &rcScoreboard, DT_TOP | DT_RIGHT | DT_SINGLELINE);

                    rcScoreboard.top += 24;

                    //	If we're about to draw the local, outranked player, offset a little more
                    if (i == 5)
                        rcScoreboard.top += 4;
                }
            }

            //	Restore
            //SetBkMode( hDC, nOldBkMode );
        }
        break;

        default:
            break;
    }

    //	Restore old obj
    SelectObject(hDC, hOld);

    DeleteObject(hBrushBG);
    DeleteObject(hPen);
    DeleteObject(hFontTitle);
    DeleteObject(hFontDesc);
    DeleteObject(hFontText);
}
