#include "RA_AchievementOverlay.h"

#include "RA_Interface.h"
#include "RA_AchievementSet.h"
#include "RA_httpthread.h"
#include "RA_Resource.h"
#include "RA_ImageFactory.h"
#include "RA_PopupWindows.h"

#include "data\GameContext.hh"

#include "services\ILeaderboardManager.hh"
#include "services\ServiceLocator.hh"

#include "ra_math.h"

namespace ra {

inline constexpr std::array<LPCTSTR, 11> PAGE_TITLES
{
    _T(" Achievements "),
    _T(" Friends "),
    _T(" Messages "),
    _T(" News "),
    _T(" Leaderboards "),
    _T(" Achievement Info "),
    _T(" Achievement Compare "),
    _T(" Friend Info "),
    _T(" Friend Add "),
    _T(" Leaderboard Examine "),
    _T(" Message Viewer ")
};

} // namespace ra

HFONT g_hFontTitle;
HFONT g_hFontDesc;
HFONT g_hFontDesc2;
HFONT g_hFontTiny;
HBRUSH g_hBrushBG;
HBRUSH g_hBrushSelectedBG;

AchievementOverlay g_AchievementOverlay;
AchievementExamine g_AchExamine;
LeaderboardExamine g_LBExamine;

const COLORREF COL_TEXT = RGB(17, 102, 221);
const COLORREF COL_TEXT_HIGHLIGHT = RGB(251, 102, 0);
const COLORREF COL_SELECTED = RGB(255, 255, 255);
const COLORREF COL_TEXT_LOCKED = RGB(137, 137, 137);
const COLORREF COL_SELECTED_LOCKED = RGB(202, 202, 202);
const COLORREF COL_BLACK = RGB(0, 0, 0);
const COLORREF COL_WHITE = RGB(255, 255, 255);
const COLORREF COL_BAR = RGB(0, 40, 0);
const COLORREF COL_BAR_BG = RGB(0, 212, 0);
const COLORREF COL_POPUP = RGB(0, 0, 40);
const COLORREF COL_POPUP_BG = RGB(212, 212, 212);
const COLORREF COL_POPUP_SHADOW = RGB(0, 0, 0);
const COLORREF COL_USER_FRAME_BG = RGB(32, 32, 32);
const COLORREF COL_SELECTED_BOX_BG = RGB(22, 22, 60);
const COLORREF COL_WARNING = RGB(255, 0, 0);
const COLORREF COL_WARNING_BG = RGB(80, 0, 0);

const unsigned int OVERLAY_WIDTH = 1024;
const unsigned int OVERLAY_HEIGHT = 1024;


void AchievementOverlay::SelectNextTopLevelPage(BOOL bPressedRight)
{
    switch (m_Pages.at(m_nPageStackPointer))
    {
        case Page::Achievements:
            m_Pages.at(m_nPageStackPointer) = (bPressedRight ? Page::Friends : Page::Leaderboards);
            break;
        case Page::Friends:
            m_Pages.at(m_nPageStackPointer) = (bPressedRight ? Page::Messages : Page::Achievements);
            break;
        case Page::Messages:
            m_Pages.at(m_nPageStackPointer) = (bPressedRight ? Page::News : Page::Friends);
            break;
        case Page::News:
            m_Pages.at(m_nPageStackPointer) = (bPressedRight ? Page::Leaderboards : Page::Messages);
            break;
        case Page::Leaderboards:
            m_Pages.at(m_nPageStackPointer) = (bPressedRight ? Page::Achievements : Page::News);
            break;
        default:
            //	Not on a toplevel page: cannot do anything!
            //assert(0);
            break;
    }
}

void AchievementOverlay::Activate()
{
    if (m_nTransitionState != TransitionState::Hold)
    {
        m_nTransitionState = TransitionState::In;
        m_fTransitionTimer = PAGE_TRANSITION_IN;
    }
}

void AchievementOverlay::Deactivate()
{
    if (m_nTransitionState != TransitionState::Off && m_nTransitionState != TransitionState::Out)
    {
        m_nTransitionState = TransitionState::Out;
        m_fTransitionTimer = 0.0F;

        RA_CauseUnpause();
    }
}

//	Returns TRUE if we are ready to exit the overlay.
BOOL AchievementOverlay::GoBack()
{
    if (m_nPageStackPointer == 0)
    {
        Deactivate();
        return TRUE;
    }
    else
    {
        m_nPageStackPointer--;
        return FALSE;
    }
}

_Use_decl_annotations_
BOOL AchievementOverlay::Update(const ControllerInput* pInput, float fDelta, BOOL bFullScreen, BOOL bPaused)
{
    auto& pLeaderboardManager = ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>();

    const int nAchCount = (const int)(g_pActiveAchievements->NumAchievements());
    const int nNumFriends = (const int)(RAUsers::LocalUser().NumFriends());
    const int nNumLBs = (const int)(pLeaderboardManager.Count());
    //const int nMsgCount = (const int)( RAUsers::LocalUser().MessageCount() );
    const int nMsgCount = 0;
    auto pnScrollOffset = GetActiveScrollOffset();	//	Dirty!
    auto pnSelectedItem = GetActiveSelectedItem();

    const ControllerInput& input = *pInput;

    BOOL bCloseOverlay = FALSE;	//	False==close overlay

    //	FS fix: this thrashes horribly when both are running :S
    if (bFullScreen)
    {
        if (m_nTransitionState == TransitionState::Out && !bPaused)
        {
            //	Skip to 'out' if we are full-screen
            m_fTransitionTimer = PAGE_TRANSITION_OUT;
        }
    }

    if (m_nTransitionState == TransitionState::In)
    {
        m_fTransitionTimer += fDelta;

        if (m_fTransitionTimer >= 0.0f)
        {
            m_fTransitionTimer = 0.0f;
            m_nTransitionState = TransitionState::Hold;
        }
    }
    else if (m_nTransitionState == TransitionState::Out)
    {
        m_fTransitionTimer += fDelta;
        if (m_fTransitionTimer >= PAGE_TRANSITION_OUT)
        {
            m_fTransitionTimer = PAGE_TRANSITION_OUT;

            if (bPaused)
            {
                //	???
                //SelectNextTopLevelPage( TRUE );
// 				//	Still paused, just transition to another page!
// 				m_nCurrentPage = (OverlayPage)((int)(m_nCurrentPage)+1);
// 				if( m_nCurrentPage == OP__MAX )
// 					m_nCurrentPage = OP_ACHIEVEMENTS;

                m_nTransitionState = TransitionState::In;
                m_fTransitionTimer = PAGE_TRANSITION_IN;
            }
            else
            {
                m_nTransitionState = TransitionState::Off;

                m_hOverlayBackground.Release();
                m_hUserImage.Release();

                m_mAchievementBadges.clear(); // will release each item as they're destroyed
            }
        }
    }

    if (m_nTransitionState == TransitionState::Off)
        return FALSE;

    //	Inputs! Restrict to ABCULDR+Start
    if (!m_bInputLock)
    {
        switch (m_Pages.at(m_nPageStackPointer))
        {
            case Page::Achievements:
            {
                if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) < (nAchCount - 1))
                    {
                        (*pnSelectedItem)++;
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bUpPressed)
                {
                    if ((*pnSelectedItem) > 0)
                    {
                        (*pnSelectedItem)--;
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bConfirmPressed)
                {
                    if ((*pnSelectedItem) < nAchCount)
                    {
                        AddPage(Page::Achievement_Examine);
                        g_AchExamine.Initialize(&g_pActiveAchievements->GetAchievement((*pnSelectedItem)));
                    }
                }

                //	Move page to match selection
                if ((*pnScrollOffset) > (*pnSelectedItem))
                    (*pnScrollOffset) = (*pnSelectedItem);
                else if ((*pnSelectedItem) > (*pnScrollOffset) + (m_nNumAchievementsBeingRendered - 1))
                    (*pnScrollOffset) = (*pnSelectedItem) - (m_nNumAchievementsBeingRendered - 1);

            }
            break;
            case Page::Achievement_Examine:
            {
                //	Overload:
                pnScrollOffset = &m_nAchievementsScrollOffset;
                pnSelectedItem = &m_nAchievementsSelectedItem;

                if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) < (nAchCount - 1))
                    {
                        (*pnSelectedItem)++;
                        g_AchExamine.Initialize(&g_pActiveAchievements->GetAchievement((*pnSelectedItem)));
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bUpPressed)
                {
                    if ((*pnSelectedItem) > 0)
                    {
                        (*pnSelectedItem)--;
                        g_AchExamine.Initialize(&g_pActiveAchievements->GetAchievement((*pnSelectedItem)));
                        m_bInputLock = TRUE;
                    }
                }

                //	Move page to match selection
                if ((*pnScrollOffset) > (*pnSelectedItem))
                    (*pnScrollOffset) = (*pnSelectedItem);
                else if ((*pnSelectedItem) > (*pnScrollOffset) + (m_nNumAchievementsBeingRendered - 1))
                    (*pnScrollOffset) = (*pnSelectedItem) - (m_nNumAchievementsBeingRendered - 1);
            }
            break;
            case Page::Friends:
            {
                if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) < (nNumFriends - 1))
                    {
                        (*pnSelectedItem)++;
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bUpPressed)
                {
                    if ((*pnSelectedItem) > 0)
                    {
                        (*pnSelectedItem)--;
                        m_bInputLock = TRUE;
                    }
                }

                //	Move page to match selection
                if ((*pnScrollOffset) > (*pnSelectedItem))
                    (*pnScrollOffset) = (*pnSelectedItem);
                else if ((*pnSelectedItem) > (*pnScrollOffset) + (m_nNumFriendsBeingRendered - 1))
                    (*pnScrollOffset) = (*pnSelectedItem) - (m_nNumFriendsBeingRendered - 1);

                // 				//	Lim the selected item to a valid range
                // 				while( nSelectedItem > nNumElements )
                // 					nSelectedItem--;
                // 
                // 				if( nSelectedItem < nScrollOffset )
                // 					nScrollOffset--;
                // 				if( nSelectedItem >= nScrollOffset+m_nNumFriendsBeingRendered )
                // 					nScrollOffset++;
            }
            break;
            case Page::Messages:
            {
                //	Select message
                if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) < (nAchCount - 1))
                    {
                        (*pnSelectedItem)++;
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) > 0)
                    {
                        (*pnSelectedItem)--;
                        m_bInputLock = TRUE;
                    }
                }

                //	Move page to match selection
                //if( (*pnScrollOffset) > (*pnSelectedItem) )
                //	(*pnScrollOffset) = (*pnSelectedItem);
                //else if( (*pnSelectedItem) > (*pnScrollOffset)+(NUM_MESSAGES_TO_DRAW) )
                //	(*pnScrollOffset) = (*pnSelectedItem) - (NUM_MESSAGES_TO_DRAW);
            }
            break;
            case Page::Message_Viewer:
            {
                //RAMessage Msg = RAUsers::LocalUser().GetMessage( m_nMessagesSelectedItem );

                break;
            }
            case Page::News:
                //	Scroll news
                if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) < static_cast<int>(m_LatestNews.size()))
                    {
                        (*pnSelectedItem)++;
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bUpPressed)
                {
                    if ((*pnSelectedItem) > 0)
                    {
                        (*pnSelectedItem)--;
                        m_bInputLock = TRUE;
                    }
                }
                break;
            case Page::Leaderboards:
                //	Scroll news
                if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) < (nNumLBs - 1))
                    {
                        (*pnSelectedItem)++;
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bUpPressed)
                {
                    if ((*pnSelectedItem) > 0)
                    {
                        (*pnSelectedItem)--;
                        m_bInputLock = TRUE;
                    }
                }
                if (input.m_bConfirmPressed)
                {
                    if ((*pnSelectedItem) < nNumLBs)
                    {
                        AddPage(Page::Leaderboard_Examine);
                        g_LBExamine.Initialize(pLeaderboardManager.GetLB((*pnSelectedItem)).ID());
                    }
                }
                //	Move page to match selection
                if ((*pnScrollOffset) > (*pnSelectedItem))
                    (*pnScrollOffset) = (*pnSelectedItem);
                else if ((*pnSelectedItem) > (*pnScrollOffset) + (m_nNumLeaderboardsBeingRendered - 1))
                    (*pnScrollOffset) = (*pnSelectedItem) - (m_nNumLeaderboardsBeingRendered - 1);
                break;
            case Page::Leaderboard_Examine:
                //	Overload from previous
                //	Overload:
                pnScrollOffset = &m_nLeaderboardScrollOffset;
                pnSelectedItem = &m_nLeaderboardSelectedItem;

                if (input.m_bDownPressed)
                {
                    if ((*pnSelectedItem) < (nNumLBs - 1))
                    {
                        (*pnSelectedItem)++;
                        g_LBExamine.Initialize(pLeaderboardManager.GetLB((*pnSelectedItem)).ID());
                        m_bInputLock = TRUE;
                    }
                }
                else if (input.m_bUpPressed)
                {
                    if ((*pnSelectedItem) > 0)
                    {
                        (*pnSelectedItem)--;
                        g_LBExamine.Initialize(pLeaderboardManager.GetLB((*pnSelectedItem)).ID());
                        m_bInputLock = TRUE;
                    }
                }

                //	Move page to match selection
                //if( (*pnScrollOffset) > (*pnSelectedItem) )
                //	(*pnScrollOffset) = (*pnSelectedItem);
                //else if( (*pnSelectedItem) > (*pnScrollOffset) + (m_nNumAchievementsBeingRendered-1) )
                //	(*pnScrollOffset) = (*pnSelectedItem) - (m_nNumAchievementsBeingRendered-1);

                break;
            default:
                assert(0);	//	Unknown page!
                break;
        }


        if (input.m_bCancelPressed)
        {
            //	If TRUE: Close overlay
            bCloseOverlay = GoBack();
            m_bInputLock = TRUE;
        }

        if (input.m_bLeftPressed || input.m_bRightPressed)
        {
            if (m_nTransitionState == TransitionState::Hold)
            {
                SelectNextTopLevelPage(input.m_bRightPressed);
                m_bInputLock = TRUE;
            }
        }
    }
    else
    {
        if (!input.m_bUpPressed && !input.m_bDownPressed && !input.m_bLeftPressed &&
            !input.m_bRightPressed && !input.m_bConfirmPressed && !input.m_bCancelPressed)
        {
            m_bInputLock = FALSE;
        }
    }

    if (input.m_bQuitPressed)
    {
        Deactivate();
        bCloseOverlay = TRUE;
    }

    return bCloseOverlay;
}

void AchievementOverlay::DrawAchievementsPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const
{
    const int nGameTitleX = 12;
    const int nGameTitleY = 80;
    const int nGameSubTitleY = nGameTitleY + 36;

    const size_t nNumberOfAchievements = g_pActiveAchievements->NumAchievements();

    unsigned int nMaxPts = 0;
    unsigned int nUserPts = 0;
    unsigned int nUserCompleted = 0;

    const int nAchTopEdge = 160;//80;
    const int nAchSpacing = 64 + 8;//52;
    const int nAchImageOffset = 28;
    int nAchIdx = 0;

    const int* pnScrollOffset = GetActiveScrollOffset();
    const int* pnSelectedItem = GetActiveSelectedItem();

    const int nWidth = rcTarget.right - rcTarget.left;
    const int nHeight = rcTarget.bottom - rcTarget.top;

    auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto& sGameTitle = pGameContext.GameTitle();
    if (!sGameTitle.empty())
    {
        SelectObject(hDC, g_hFontTitle);
        SetTextColor(hDC, COL_TEXT);
        TextOutW(hDC, nGameTitleX + nDX + 8, nGameTitleY + nDY, sGameTitle.c_str(), sGameTitle.length());
    }

    SelectObject(hDC, g_hFontDesc);
    if (g_nActiveAchievementSet == AchievementSet::Type::Core)
    {
        for (size_t i = 0; i < nNumberOfAchievements; ++i)
        {
            const Achievement* pAch = &g_pActiveAchievements->GetAchievement(i);
            nMaxPts += pAch->Points();
            if (!pAch->Active())
            {
                nUserPts += pAch->Points();
                nUserCompleted++;
            }
        }

        if (nNumberOfAchievements > 0)
        {
            SetTextColor(hDC, COL_TEXT_LOCKED);
            char buffer[256];
            sprintf_s(buffer, 256, " %u of %u won (%u/%u) ",
                nUserCompleted, nNumberOfAchievements,
                nUserPts, nMaxPts);
            TextOut(hDC, nDX + nGameTitleX, nGameSubTitleY, NativeStr(buffer).c_str(), strlen(buffer));
        }
    }

    const int nAchievementsToDraw = ((rcTarget.bottom - rcTarget.top) - 160) / nAchSpacing;
    if (nAchievementsToDraw > 0 && nNumberOfAchievements > 0)
    {
        for (int i = 0; i < nAchievementsToDraw; ++i)
        {
            nAchIdx = (*pnScrollOffset) + i;
            if (nAchIdx < static_cast<int>(nNumberOfAchievements))
            {
                const BOOL bSelected = ((*pnSelectedItem) - (*pnScrollOffset) == i);
                if (bSelected)
                {
                    //	Draw bounding box around text
                    const int nSelBoxXOffs = 28 + 64;
                    const int nSelBoxWidth = nWidth - nAchSpacing - 24;
                    const int nSelBoxHeight = nAchSpacing - 8;

                    RECT rcSelected ={ (nDX + nSelBoxXOffs),
                                        (nAchTopEdge + (i * nAchSpacing)),
                                        (nDX + nSelBoxXOffs + nSelBoxWidth),
                                        (nAchTopEdge + (i * nAchSpacing)) + nSelBoxHeight };
                    FillRect(hDC, &rcSelected, g_hBrushSelectedBG);
                }

                DrawAchievement(hDC,
                                &g_pActiveAchievements->GetAchievement(nAchIdx), // pAch
                                nDX,                                             // X
                                (nAchTopEdge + (i*nAchSpacing)),                 // Y
                                bSelected,                                       // Selected
                                TRUE);
            }
        }

        if (nNumberOfAchievements > static_cast<size_t>(nAchievementsToDraw - 1))
        {
            DrawBar(hDC,
                nDX + 8,
                nAchTopEdge,
                12,
                nAchSpacing*nAchievementsToDraw,
                nNumberOfAchievements - (nAchievementsToDraw - 1),
                (*pnScrollOffset));
        }
    }
    else
    {

        if (!RA_GameIsActive())
        {
            const std::string sMsg(" No achievements present... ");
            TextOut(hDC, nDX + nGameTitleX, nGameSubTitleY, NativeStr(sMsg).c_str(), sMsg.length());
        }
        else if (nNumberOfAchievements == 0)
        {
            const std::string sMsg(" No achievements present... ");
            TextOut(hDC, nDX + nGameTitleX, nGameSubTitleY, NativeStr(sMsg).c_str(), sMsg.length());
        }
    }

    m_nNumAchievementsBeingRendered = nAchievementsToDraw;
}

void AchievementOverlay::DrawMessagesPage(_UNUSED HDC, _UNUSED int, _UNUSED int, _UNUSED const RECT&) const
{

    // for( size_t i = 0; i < 256; ++i )
    //      buffer[i] = (char)(i);
    // 
    //  SelectObject( hDC, hFontDesc );
    //  TextOut( hDC, nDX+8, 40, buffer, 32 );
    //  TextOut( hDC, nDX+8, 60, buffer+32, 32 );
    //  TextOut( hDC, nDX+8, 80, buffer+64, 32 );
    //  TextOut( hDC, nDX+8, 100, buffer+96, 32 );
    //  TextOut( hDC, nDX+8, 120, buffer+128, 32 );
    //  TextOut( hDC, nDX+8, 140, buffer+160, 32 );
    //  TextOut( hDC, nDX+8, 160, buffer+192, 32 );
    //  TextOut( hDC, nDX+8, 180, buffer+224, 32 );

}

void AchievementOverlay::DrawFriendsPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const
{
    const int* pnScrollOffset = GetActiveScrollOffset();

    const unsigned int nFriendSpacing = 64 + 8;//80;
    const unsigned int nFriendsToDraw = ((rcTarget.bottom - rcTarget.top) - 140) / nFriendSpacing;

    const unsigned int nFriendLeftOffsetImage = 32;//16;
    const unsigned int nFriendLeftOffsetText = 84;//64;

    const unsigned int nFriendTopEdge = 120;//44;
    const unsigned int nFriendSubtitleYOffs = 24;
    const unsigned int nFriendSpacingText = 36;//18;

    char buffer[256];

    const unsigned int nOffset = m_nFriendsScrollOffset;

    const unsigned int nNumFriends = RAUsers::LocalUser().NumFriends();

    // TODO: friends list/activity is captured at time of login. eliminate that call as most
    // people don't care and fetch the data when switching to the friends page in the overlay
    for (unsigned int i = 0; i < nFriendsToDraw; ++i)
    {
        const int nXOffs = nDX + (rcTarget.left + nFriendLeftOffsetImage);
        const int nYOffs = nFriendTopEdge + nFriendSpacing * i;

        if (i > nNumFriends)
            break;

        if ((i + nOffset) < nNumFriends)
        {
            const RAUser* pFriend = RAUsers::LocalUser().GetFriendByIter((i + nOffset));
            if (pFriend == nullptr)
                continue;

            ra::services::ImageReference friendImage(ra::services::ImageType::UserPic, pFriend->Username());
            HBITMAP hBitmap = friendImage.GetHBitmap();
            if (hBitmap != nullptr)
                DrawImage(hDC, hBitmap, nXOffs, nYOffs, 64, 64);

            if ((m_nFriendsSelectedItem - m_nFriendsScrollOffset) == ra::to_signed(i))
                SetTextColor(hDC, COL_SELECTED);
            else
                SetTextColor(hDC, COL_TEXT);

            HANDLE hOldObj = SelectObject(hDC, g_hFontDesc);

            sprintf_s(buffer, 256, " %s (%u) ", pFriend->Username().c_str(), pFriend->GetScore());
            TextOut(hDC, nXOffs + nFriendLeftOffsetText, nYOffs, NativeStr(buffer).c_str(), strlen(buffer));

            SelectObject(hDC, g_hFontTiny);
            sprintf_s(buffer, 256, " %s ", pFriend->Activity().c_str());
            //RARect rcDest( nXOffs+nFriendLeftOffsetText, nYOffs+nFriendSubtitleYOffs )
            RECT rcDest;
            SetRect(&rcDest,
                nXOffs + nFriendLeftOffsetText + 16,
                nYOffs + nFriendSubtitleYOffs,
                nDX + rcTarget.right - 40,
                nYOffs + nFriendSubtitleYOffs + 46);
            DrawText(hDC, NativeStr(pFriend->Activity()).c_str(), -1, &rcDest, DT_LEFT | DT_WORDBREAK);

            //	Restore
            SelectObject(hDC, hOldObj);

            //sprintf_s( buffer, 256, " %d Points ", Friend.Score() );
            //TextOut( hDC, nXOffs+nFriendLeftOffsetText, nYOffs+nFriendSpacingText, buffer, strlen( buffer ) );
        }
    }

    if (nNumFriends > (nFriendsToDraw))
    {
        DrawBar(hDC,
            nDX + 8,
            nFriendTopEdge,
            12,
            nFriendSpacing*nFriendsToDraw,
            nNumFriends - (nFriendsToDraw - 1),
            (*pnScrollOffset));
    }

    m_nNumFriendsBeingRendered = nFriendsToDraw;
}

void AchievementOverlay::DrawAchievementExaminePage(HDC hDC, int nDX, _UNUSED int, _UNUSED const RECT&) const
{
    char buffer[256];

    const unsigned int nNumAchievements = g_pActiveAchievements->NumAchievements();

    const int nAchievementStartX = 0;
    const int nAchievementStartY = 80;

    const int nLoadingMessageX = 120;
    const int nLoadingMessageY = 300;

    const int nCoreDetailsY = 150;
    const int nCoreDetailsSpacing = 32;

    const int nWonByPlayerYOffs = 300;
    const int nWonByPlayerYSpacing = 28;

    const int nRecentWinnersSubtitleX = 20;
    const int nRecentWinnersSubtitleY = 250;

    const int nWonByPlayerNameX = 20;
    const int nWonByPlayerDateX = 220;

    char bufTime[256];

    const Achievement* pAch = &g_pActiveAchievements->GetAchievement(m_nAchievementsSelectedItem);

    const time_t tCreated = pAch->CreatedDate();
    const time_t tModified = pAch->ModifiedDate();

    DrawAchievement(hDC,
        pAch,
        nDX + nAchievementStartX,
        nAchievementStartY,
        TRUE,
        FALSE);

    if (m_nAchievementsSelectedItem >= (int)nNumAchievements)
        return;

    ctime_s(bufTime, 256, &tCreated);
    bufTime[strlen(bufTime) - 1] = '\0';	//	Remove pesky newline
    sprintf_s(buffer, 256, " Created: %s ", bufTime);
    TextOut(hDC, nDX + 20, nCoreDetailsY, NativeStr(buffer).c_str(), strlen(buffer));

    ctime_s(bufTime, 256, &tModified);
    bufTime[strlen(bufTime) - 1] = '\0';	//	Remove pesky newline
    sprintf_s(buffer, 256, " Modified: %s ", bufTime);
    TextOut(hDC, nDX + 20, nCoreDetailsY + nCoreDetailsSpacing, NativeStr(buffer).c_str(), strlen(buffer));

    if (g_AchExamine.HasData())
    {
        sprintf_s(buffer, 256, " Won by %u of %u (%1.0f%%)",
            g_AchExamine.TotalWinners(),
            g_AchExamine.PossibleWinners(),
            static_cast<float>(g_AchExamine.TotalWinners() * 100) / static_cast<float>(g_AchExamine.PossibleWinners()));
        TextOut(hDC, nDX + 20, nCoreDetailsY + (nCoreDetailsSpacing * 2), NativeStr(buffer).c_str(), strlen(buffer));

        if (g_AchExamine.NumRecentWinners() > 0)
        {
            sprintf_s(buffer, 256, " Recent winners: ");
            TextOut(hDC, nDX + nRecentWinnersSubtitleX, nRecentWinnersSubtitleY, NativeStr(buffer).c_str(), strlen(buffer));
        }

        auto i{ 0U };
        for (const auto& data : g_AchExamine)
        {
            {
                std::string sUser{ " " };
                sUser += data.User();
                sUser += "      ";

                //	Draw/Fetch user image? //TBD
                TextOut(hDC,
                    nDX + nWonByPlayerNameX,
                    nWonByPlayerYOffs + (ra::to_signed(i)*nWonByPlayerYSpacing),
                    NativeStr(sUser).c_str(), ra::narrow_cast<int>(sUser.length()));
            }

            std::string sWonAt{ "       " };
            sWonAt += data.WonAt();
            sWonAt += " ";

            TextOut(hDC,
                nDX + nWonByPlayerDateX,
                nWonByPlayerYOffs + (ra::to_signed(i)*nWonByPlayerYSpacing),
                NativeStr(sWonAt).c_str(), ra::narrow_cast<int>(sWonAt.length()));
            i++;
        }
    }
    else
    {
        static int nDots = 0;
        nDots++;
        if (nDots > 100)
            nDots = 0;

        const int nDotCount = nDots / 25;
        sprintf_s(buffer, 256, " Loading.%c%c%c ",
            nDotCount >= 1 ? '.' : ' ',
            nDotCount >= 2 ? '.' : ' ',
            nDotCount >= 3 ? '.' : ' ');

        TextOut(hDC, nDX + nLoadingMessageX, nLoadingMessageY, NativeStr(buffer).c_str(), strlen(buffer));
    }
}

void AchievementOverlay::DrawNewsPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const
{
    unsigned int nYOffset = 90;

    const unsigned int nHeaderGap = 4;
    const unsigned int nArticleGap = 10;

    const unsigned int nBorder = 32;

    const unsigned int nLeftAlign = nDX + nBorder;
    const unsigned int nRightAlign = nDX + (rcTarget.right - nBorder);

    const unsigned int nArticleIndent = 20;

    const int nWidth = rcTarget.right - rcTarget.left;
    const int nHeight = rcTarget.bottom - rcTarget.top;

    HGDIOBJ hOldObject = SelectObject(hDC, g_hFontDesc2);

    for (int i = m_nNewsSelectedItem; i < static_cast<int>(m_LatestNews.size()); ++i)
    {
        const char* sTitle = m_LatestNews[i].m_sTitle.c_str();
        const char* sPayload = m_LatestNews[i].m_sPayload.c_str();


        SelectObject(hDC, g_hFontDesc2);

        //	Setup initial variables for the rect
        RECT rcNews;
        SetRect(&rcNews, nLeftAlign, nYOffset, nRightAlign, nYOffset);

        //	Calculate height of rect, fetch bottom for next rect:
        DrawText(hDC, NativeStr(sTitle).c_str(), strlen(sTitle), &rcNews, DT_CALCRECT | DT_WORDBREAK);
        nYOffset = rcNews.bottom;

        if (rcNews.bottom > nHeight)
            rcNews.bottom = nHeight;

        //	Draw title:
        DrawText(hDC, NativeStr(sTitle).c_str(), strlen(sTitle), &rcNews, DT_TOP | DT_LEFT | DT_WORDBREAK);

        //	Offset header
        nYOffset += nHeaderGap;

        SelectObject(hDC, g_hFontTiny);

        //	Setup initial variables for the rect
        SetRect(&rcNews, nLeftAlign + nArticleIndent, nYOffset, nRightAlign - nArticleIndent, nYOffset);
        //	Calculate height of rect, fetch and inset bottom:
        DrawText(hDC, NativeStr(sPayload).c_str(), strlen(sPayload), &rcNews, DT_CALCRECT | DT_WORDBREAK);
        nYOffset = rcNews.bottom;

        if (rcNews.bottom > nHeight)
            rcNews.bottom = nHeight;

        //	Draw payload:
        DrawText(hDC, NativeStr(sPayload).c_str(), strlen(sPayload), &rcNews, DT_TOP | DT_LEFT | DT_WORDBREAK);

        nYOffset += nArticleGap;
    }

    //	Restore object:
    SelectObject(hDC, hOldObject);
}

void AchievementOverlay::DrawLeaderboardPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const
{
    const unsigned int nYOffsetTop = 90;
    unsigned int nYOffset = nYOffsetTop;

    const unsigned int nHeaderGap = 4;
    const unsigned int nArticleGap = 10;

    const unsigned int nBorder = 32;

    const unsigned int nLeftAlign = nDX + nBorder;
    const unsigned int nRightAlign = nDX + (rcTarget.right - nBorder);

    const unsigned int nArticleIndent = 20;

    const int nWidth = rcTarget.right - rcTarget.left;
    const int nHeight = rcTarget.bottom - rcTarget.top;

    const int* pnScrollOffset = GetActiveScrollOffset();
    const int* pnSelectedItem = GetActiveSelectedItem();

    const int nLBSpacing = 52;

    const int nItemSpacing = 48;

    HGDIOBJ hOldObject = SelectObject(hDC, g_hFontDesc2);

    m_nNumLeaderboardsBeingRendered = 0;

    auto& pLeaderboardManager = ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>();
    unsigned int nNumLBsToDraw = ((rcTarget.bottom - rcTarget.top) - 160) / nItemSpacing;
    const unsigned int nNumLBs = pLeaderboardManager.Count();

    if (nNumLBsToDraw > nNumLBs)
        nNumLBsToDraw = nNumLBs;

    if (nNumLBs > 0 && nNumLBsToDraw > 0)
    {
        for (unsigned int i = m_nLeaderboardScrollOffset; i < m_nLeaderboardScrollOffset + nNumLBsToDraw; ++i)
        {
            if (i >= nNumLBs)
                continue;

            const RA_Leaderboard& nextLB = pLeaderboardManager.GetLB(i);

            std::string sTitle(" " + nextLB.Title() + " ");
            const std::string& sPayload = nextLB.Description();

            const BOOL bSelected = ((*pnSelectedItem) == ra::to_signed(i));
            if (bSelected)
            {
                //	Draw bounding box around text
                const int nSelBoxXOffs = nLeftAlign - 4;
                const int nSelBoxWidth = nWidth - 8;
                const int nSelBoxHeight = nItemSpacing;

                RECT rcSelected ={ nDX + nSelBoxXOffs,
                                    static_cast<LONG>(nYOffset),
                                    nDX + nSelBoxXOffs + nSelBoxWidth,
                                    static_cast<LONG>(nYOffset + nSelBoxHeight) };

                FillRect(hDC, &rcSelected, g_hBrushSelectedBG);
            }

            SelectObject(hDC, g_hFontDesc2);

            //	Setup initial variables for the rect
            RECT rcNews;
            SetRect(&rcNews, nLeftAlign, nYOffset, nRightAlign, nYOffset);
            //	Calculate height of rect, fetch bottom for next rect:
            DrawText(hDC, NativeStr(sTitle).c_str(), sTitle.length(), &rcNews, DT_CALCRECT | DT_WORDBREAK);
            nYOffset = rcNews.bottom;

            if (rcNews.bottom > nHeight)
                rcNews.bottom = nHeight;

            SetTextColor(hDC, bSelected ? COL_SELECTED : COL_TEXT);

            //	Draw title:
            DrawText(hDC, NativeStr(sTitle).c_str(), sTitle.length(), &rcNews, DT_TOP | DT_LEFT | DT_WORDBREAK);

            //	Offset header
            nYOffset += nHeaderGap;

            SelectObject(hDC, g_hFontTiny);

            //	Setup initial variables for the rect
            SetRect(&rcNews, nLeftAlign + nArticleIndent, nYOffset, nRightAlign - nArticleIndent, nYOffset);
            //	Calculate height of rect, fetch and inset bottom:
            DrawText(hDC, NativeStr(sPayload).c_str(), sPayload.length(), &rcNews, DT_CALCRECT | DT_WORDBREAK);
            nYOffset = rcNews.bottom;

            if (rcNews.bottom > nHeight)
                rcNews.bottom = nHeight;

            SetTextColor(hDC, COL_SELECTED_LOCKED);

            //	Draw payload:
            DrawText(hDC, NativeStr(sPayload).c_str(), sPayload.length(), &rcNews, DT_TOP | DT_LEFT | DT_WORDBREAK);

            nYOffset += nArticleGap;

            m_nNumLeaderboardsBeingRendered++;
        }

        if (nNumLBs > (nNumLBsToDraw - 1))
        {
            DrawBar(hDC,
                nDX + 8,
                nYOffsetTop,
                12,
                nLBSpacing * m_nNumLeaderboardsBeingRendered,
                nNumLBs - (nNumLBsToDraw - 1),
                (*pnScrollOffset));
        }
    }

    //	Restore object:
    SelectObject(hDC, hOldObject);
}

void AchievementOverlay::DrawLeaderboardExaminePage(HDC hDC, int nDX, _UNUSED int, _UNUSED const RECT&) const
{
    const int nLBStartX = 0;
    const int nLBStartY = 80;

    const int nLoadingMessageX = 120;
    const int nLoadingMessageY = 300;

    const int nCoreDetailsY = 100;
    const int nCoreDetailsSpacing = 28;

    const int nLeaderboardYOffs = 160;
    const int nLeaderboardYSpacing = 26;

    const int nRecentWinnersSubtitleX = 20;
    const int nRecentWinnersSubtitleY = 272;

    const int nWonByPlayerRankX = 20;
    const int nWonByPlayerUserX = 100;
    const int nWonByPlayerScoreX = 320;

    auto& pLeaderboardManager = ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>();
    const RA_Leaderboard* pLB = pLeaderboardManager.FindLB(g_LBExamine.m_nLBID);
    if (pLB == nullptr)
    {
        const std::string sMsg(" No leaderboard found ");
        TextOut(hDC, 120, 120, NativeStr(sMsg).c_str(), sMsg.length());
    }
    else
    {
        const std::string sTitle(" " + pLB->Title() + " ");
        TextOut(hDC, nDX + 20, nCoreDetailsY, NativeStr(sTitle).c_str(), sTitle.length());

        SetTextColor(hDC, COL_SELECTED_LOCKED);
        const std::string sDesc(" " + pLB->Description() + " ");
        TextOut(hDC, nDX + 20, nCoreDetailsY + nCoreDetailsSpacing, NativeStr(sDesc).c_str(), sDesc.length());

        SetTextColor(hDC, COL_TEXT);

        if (g_LBExamine.m_bHasData)
        {
            for (size_t i = 0; i < pLB->GetRankInfoCount(); ++i)
            {
                const RA_Leaderboard::Entry& rEntry = pLB->GetRankInfo(i);
                std::string sScoreFormatted = pLB->FormatScore(rEntry.m_nScore);

                char sRankText[256];
                sprintf_s(sRankText, 256, " %u ", rEntry.m_nRank);

                char sNameText[256];
                sprintf_s(sNameText, 256, " %s ", rEntry.m_sUsername.c_str());

                char sScoreText[256];
                sprintf_s(sScoreText, 256, " %s ", sScoreFormatted.c_str());

                //	Draw/Fetch user image? //TBD
                TextOut(hDC,
                    nDX + nWonByPlayerRankX,
                    nLeaderboardYOffs + (i * nLeaderboardYSpacing),
                    NativeStr(sRankText).c_str(), strlen(sRankText));

                TextOut(hDC,
                    nDX + nWonByPlayerUserX,
                    nLeaderboardYOffs + (i * nLeaderboardYSpacing),
                    NativeStr(sNameText).c_str(), strlen(sNameText));

                TextOut(hDC,
                    nDX + nWonByPlayerScoreX,
                    nLeaderboardYOffs + (i * nLeaderboardYSpacing),
                    NativeStr(sScoreText).c_str(), strlen(sScoreText));
            }
        }
        else
        {
            int nDotCount = 0;
            static int nDots = 0;
            nDots++;
            if (nDots > 100)
                nDots = 0;

            nDotCount = nDots / 25;

            char buffer[256];
            sprintf_s(buffer, 256, " Loading.%c%c%c ",
                nDotCount > 1 ? '.' : ' ',
                nDotCount > 2 ? '.' : ' ',
                nDotCount > 3 ? '.' : ' ');

            TextOut(hDC, nDX + nLoadingMessageX, nLoadingMessageY, NativeStr(buffer).c_str(), strlen(buffer));
        }
    }
}

_Use_decl_annotations_
void AchievementOverlay::Render(HDC hRealDC, const RECT* rcDest) const
{
    //	Rendering:
    if (!RAUsers::LocalUser().IsLoggedIn())
        return;	//	Not available!

    if (m_nTransitionState == TransitionState::Off)
        return;

    const RECT rcTarget{ *rcDest };
    const auto lHeight{ rcTarget.bottom - rcTarget.top };
    {
        _CONSTANT_LOC nFontSize1{ 32 };
        _CONSTANT_LOC nFontSize2{ 26 };
        _CONSTANT_LOC nFontSize3{ 22 };
        _CONSTANT_LOC nFontSize4{ 16 };
        _CONSTANT_LOC FONT_TO_USE{ _T("Tahoma") };

        g_hFontTitle = CreateFont(nFontSize1, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, FONT_TO_USE);

        g_hFontDesc = CreateFont(nFontSize2, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, FONT_TO_USE);

        g_hFontDesc2 = CreateFont(nFontSize3, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, FONT_TO_USE);

        g_hFontTiny = CreateFont(nFontSize4, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, FONT_TO_USE);
    }

    const auto fPctOffScreen{ (m_nTransitionState == TransitionState::In) ?
        (m_fTransitionTimer / PAGE_TRANSITION_IN) :
        (m_fTransitionTimer / PAGE_TRANSITION_OUT)
    };

    const auto nDX{ ra::ftol(0 - (fPctOffScreen * (rcTarget.right - rcTarget.left))) };
    const auto nDY{ rcTarget.top };

    const auto  nRightPx{ ra::ftol(rcTarget.right - (fPctOffScreen * rcTarget.right)) };

    RECT rc{ nDX, nDY, nDX + rcTarget.right, rcTarget.bottom };

    // create backbuffer
    auto hDC{ ::CreateCompatibleDC(hRealDC) };
    auto hBitmap{ ::CreateCompatibleBitmap(hRealDC, rc.right, lHeight) };
    SelectBitmap(hDC, hBitmap);

    //	Draw background:
    SetBkMode(hDC, TRANSPARENT);
    {
        auto hBackground{ m_hOverlayBackground.GetHBitmap() };
        if (hBackground)
        {
            RECT rcBGSize{ 0L, 0L, ra::to_signed(OVERLAY_WIDTH), ra::to_signed(OVERLAY_HEIGHT) };
            OffsetRect(&rcBGSize, -(ra::to_signed(OVERLAY_WIDTH) - rc.right), 0);
            DrawImageTiled(hDC, hBackground, rcBGSize, rc);
        }
    }

    g_hBrushBG = CreateSolidBrush(COL_USER_FRAME_BG);
    g_hBrushSelectedBG = CreateSolidBrush(COL_SELECTED_BOX_BG);

    ::SetTextColor(hDC, COL_TEXT);
    SelectFont(hDC, g_hFontDesc);

    _CONSTANT_LOC nBorder{ 8 };
    {
        _CONSTANT_LOC uMinUserFrameWidth{ 300U };
        _CONSTANT_LOC uMinUserFrameHeight{ 64U + 4U + 4U };

        //	Draw user info
        if (rcTarget.right > 360)
        {
            DrawUserFrame(hDC,
                &RAUsers::LocalUser(),
                (nDX + (rcTarget.right - ra::to_signed(uMinUserFrameWidth))) - 4,
                4 + nBorder,
                ra::to_signed(uMinUserFrameWidth),
                ra::to_signed(uMinUserFrameHeight));
        }
    }

    //	Draw the bulk of the page:
    {
        const auto nCurrentPage{ m_Pages.at(m_nPageStackPointer) };
        switch (nCurrentPage)
        {
            case Page::Achievements:
                DrawAchievementsPage(hDC, nDX, nDY, rcTarget);
                break;

            case Page::Messages:
                DrawMessagesPage(hDC, nDX, nDY, rcTarget);
                break;

            case Page::Friends:
                DrawFriendsPage(hDC, nDX, nDY, rcTarget);
                break;

            case Page::Achievement_Examine:
                DrawAchievementExaminePage(hDC, nDX, nDY, rcTarget);
                break;

            case Page::News:
                DrawNewsPage(hDC, nDX, nDY, rcTarget);
                break;

            case Page::Leaderboards:
                DrawLeaderboardPage(hDC, nDX, nDY, rcTarget);
                break;

            case Page::Leaderboard_Examine:
                DrawLeaderboardExaminePage(hDC, nDX, nDY, rcTarget);
                break;

            default:
                //	Not implemented!
                ASSERT(!"Attempting to render an undefined overlay page!");
        }
    
    //	Title:
    SelectFont(hDC, g_hFontTitle);
    SetTextColor(hDC, COL_TEXT);

    const auto& sTitle{ ra::PAGE_TITLES.at(ra::etoi(nCurrentPage)) };
    TextOut(hDC, nDX + nBorder, 4 + nBorder, sTitle, ra::narrow_cast<int>(ra::tcslen_s(sTitle)));

    }

    //	Render controls:
    SelectFont(hDC, g_hFontDesc2);
    {
        const int nControlsX1 = 80 + 80 + 4;
        const int nControlsX2 = 80;
        const int nControlsY1 = rcTarget.bottom - 30 - 30 - 4;
        const int nControlsY2 = rcTarget.bottom - 30 - 4;

        //	Fill again:
        SetRect(&rc, nRightPx - nControlsX1 - 4, nControlsY1 - 4, nRightPx, lHeight);
        FillRect(hDC, &rc, g_hBrushBG);

        //	Draw control text:
        {
            ra::tstring stNext{ _T(" ->:") };
            stNext += _T("Next ");
            TextOut(hDC, nRightPx - nControlsX1, nControlsY1, stNext.c_str(), ra::narrow_cast<int>(stNext.length()));
        }
        {
            ra::tstring stPrev{ _T(" <-:") };
            stPrev += _T("Prev ");
            TextOut(hDC, nRightPx - nControlsX1, nControlsY2, stPrev.c_str(), ra::narrow_cast<int>(stPrev.length()));
        }

        _CONSTANT_LOC ctBackChar{ _T('B') };
        auto ctSelectChar{ _T('A') };

        //	Genesis wouldn't use 'A' for select
        if (g_EmulatorID == RA_Gens)
            ctSelectChar = _T('C');

        {
            ra::tstring stBack{ _T(" ") };
            stBack += ctBackChar;
            stBack += _T(":");
            stBack += _T("Back ");
            TextOut(hDC, nRightPx - nControlsX2, nControlsY1, stBack.c_str(), ra::narrow_cast<int>(stBack.length()));
        }
        {
            ra::tstring stSelect{ _T(" ") };
            stSelect += ctSelectChar;
            stSelect += _T(":");
            stSelect += _T("Select ");
            TextOut(hDC, nRightPx - nControlsX2, nControlsY2, stSelect.c_str(), ra::narrow_cast<int>(stSelect.length()));
        }
    }

    DeleteBrush(g_hBrushBG);
    DeleteBrush(g_hBrushSelectedBG);
    DeleteFont(g_hFontTitle);
    DeleteFont(g_hFontDesc);
    DeleteFont(g_hFontDesc2);
    DeleteFont(g_hFontTiny);

    // render backbuffer to real DC
    ::BitBlt(hRealDC, 0, 0, rcDest->right - nDX, lHeight, hDC, 0, 0, SRCCOPY);
    DeleteBitmap(hBitmap);
    ::DeleteDC(hDC);
}

void AchievementOverlay::DrawBar(HDC hDC, int nX, int nY, int nW, int nH, int nMax, int nSel) const
{
    HBRUSH hBarBack = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
    HBRUSH hBarFront = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
    float fNumMax = (float)(nMax);
    const float fInnerBarMaxSizePx = (float)nH - 4.0f;

    const float fInnerBarSizePx = (fInnerBarMaxSizePx / fNumMax);
    const float fInnerBarOffsetY = fInnerBarSizePx * nSel;

    const int nInnerBarAbsY = (int)(nY + 2.0f + fInnerBarOffsetY);

    //	Draw bar:
    SetTextColor(hDC, COL_BAR);
    SetBkColor(hDC, COL_BAR_BG);

    RECT rc;
    SetRect(&rc, nX, nY, nX + nW, nY + nH);
    if (FillRect(hDC, &rc, hBarBack))
    {
        SetTextColor(hDC, COL_TEXT);

        if (fNumMax <= 0.0f)
            fNumMax = 1.0f;

        SetRect(&rc, nX + 2, nInnerBarAbsY, nX + (nW - 2), nInnerBarAbsY + (int)(fInnerBarSizePx));
        FillRect(hDC, &rc, hBarFront);
    }

    DeleteObject(hBarFront);
    DeleteObject(hBarBack);
}

void AchievementOverlay::DrawAchievement(HDC hDC, const Achievement* pAch, int nX, int nY, BOOL bSelected, BOOL bCanLock) const
{
    const int nAchImageOffset = 28;
    const int nAchLeftOffset1 = 28 + 64 + 6;
    const int nAchLeftOffset2 = 28 + 64 + 6 + 4;
    const int nAchSpacingDesc = 24;
    BOOL bLocked = FALSE;
    char buffer[1024];

    if (bCanLock)
    {
        if (g_nActiveAchievementSet == AchievementSet::Type::Core)
            bLocked = pAch->Active();
    }

    if (bSelected)
        SetTextColor(hDC, bLocked ? COL_SELECTED_LOCKED : COL_SELECTED);
    else
        SetTextColor(hDC, bLocked ? COL_TEXT_LOCKED : COL_TEXT);

    HBITMAP hBitmap = nullptr;
    std::string sBadgeName = pAch->BadgeImageURI();
    if (bLocked)
        sBadgeName += "_lock";

    auto iter = m_mAchievementBadges.find(sBadgeName);
    if (iter != m_mAchievementBadges.end())
    {
        hBitmap = iter->second.GetHBitmap();
    }
    else
    {
        auto& imageRef = m_mAchievementBadges[sBadgeName];
        imageRef.ChangeReference(ra::services::ImageType::Badge, sBadgeName);
        hBitmap = imageRef.GetHBitmap();
    }

    if (hBitmap != nullptr)
        DrawImage(hDC, hBitmap, nX + nAchImageOffset, nY, 64, 64);

    sprintf_s(buffer, 1024, " %s ", pAch->Description().c_str());
    SelectObject(hDC, g_hFontDesc2);
    TextOut(hDC, nX + nAchLeftOffset2, nY + nAchSpacingDesc, NativeStr(buffer).c_str(), strlen(buffer));

    sprintf_s(buffer, 1024, " %s (%u Points) ", pAch->Title().c_str(), pAch->Points());
    SelectObject(hDC, g_hFontDesc);
    TextOut(hDC, nX + nAchLeftOffset1, nY, NativeStr(buffer).c_str(), strlen(buffer));
}

_Use_decl_annotations_
void AchievementOverlay::DrawUserFrame(HDC hDC, const RAUser* pUser, int nX, int nY, int nW, int nH) const
{
    char buffer[256];
    HBRUSH hBrush2 = CreateSolidBrush(COL_USER_FRAME_BG);
    RECT rcUserFrame;

    const int nTextX = nX + 4;
    const int nTextY1 = nY + 4;
    const int nTextY2 = nTextY1 + 36;

    SetRect(&rcUserFrame, nX, nY, nX + nW, nY + nH);
    FillRect(hDC, &rcUserFrame, hBrush2);

    HBITMAP hBitmap = m_hUserImage.GetHBitmap();
    if (hBitmap != nullptr)
    {
        DrawImage(hDC,
            hBitmap,
            nX + ((nW - 64) - 4),
            nY + 4,
            64, 64);
    }

    SetTextColor(hDC, COL_TEXT);
    SelectObject(hDC, g_hFontDesc);

    sprintf_s(buffer, 256, " %s ", pUser->Username().c_str());
    TextOut(hDC, nTextX, nTextY1, NativeStr(buffer).c_str(), strlen(buffer));

    sprintf_s(buffer, 256, " %u Points ", pUser->GetScore());
    TextOut(hDC, nTextX, nTextY2, NativeStr(buffer).c_str(), strlen(buffer));

    if (_RA_HardcoreModeIsActive())
    {
        const COLORREF nLastColor = SetTextColor(hDC, COL_WARNING);
        const COLORREF nLastColorBk = SetBkColor(hDC, COL_WARNING_BG);

        sprintf_s(buffer, 256, " HARDCORE ");
        TextOut(hDC, nX + 180, nY + 70, NativeStr(buffer).c_str(), strlen(buffer));

        SetTextColor(hDC, nLastColor);
        SetBkColor(hDC, nLastColorBk);
    }

    DeleteObject(hBrush2);
}

int* AchievementOverlay::GetActiveScrollOffset() const
{
    switch (m_Pages.at(m_nPageStackPointer))
    {
        case Page::Achievements:
            return &m_nAchievementsScrollOffset;
        case Page::Friends:
            return &m_nFriendsScrollOffset;
        case Page::Messages:
            return &m_nMessagesScrollOffset;
        case Page::News:
            return &m_nNewsScrollOffset;
        case Page::Leaderboards:
            return &m_nLeaderboardScrollOffset;

        case Page::Leaderboard_Examine:
            _FALLTHROUGH;
        case Page::Achievement_Examine:
            return nullptr;

        default:
            ASSERT(!"Unknown page");
            return &m_nAchievementsScrollOffset;
    }
}

int* AchievementOverlay::GetActiveSelectedItem() const
{
    switch (m_Pages.at(m_nPageStackPointer))
    {
        case Page::Achievements:
            return &m_nAchievementsSelectedItem;	//	?
        case Page::Friends:
            return &m_nFriendsSelectedItem;
        case Page::Messages:
            return &m_nMessagesSelectedItem;
        case Page::News:
            return &m_nNewsSelectedItem;
        case Page::Leaderboards:
            return &m_nLeaderboardSelectedItem;

        case Page::Achievement_Examine:
            _FALLTHROUGH;
        case Page::Leaderboard_Examine:
            return nullptr;

        default:
            ASSERT(!"Unknown page");
            return &m_nAchievementsSelectedItem;
    }
}

void AchievementOverlay::OnLoad_NewRom()
{
    m_nAchievementsSelectedItem = 0;
    m_nAchievementsScrollOffset = 0;
    m_nLeaderboardSelectedItem = 0;
    m_nLeaderboardScrollOffset = 0;

    if (IsActive())
        Deactivate();
}

void AchievementOverlay::InstallNewsArticlesFromFile()
{
    m_LatestNews.clear();
    auto sNewsFile{g_sHomeDir};
    sNewsFile += RA_NEWS_FILENAME;

    std::ifstream ifile{ sNewsFile };
    if (!ifile.is_open())
        return;

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw{ ifile };
    doc.ParseStream(isw);

    if (doc.HasMember("Success") && doc["Success"].GetBool())
    {
        const auto& News{ doc["News"] };
        for (const auto& NextNewsArticle : News.GetArray())
        {
            NewsItem nNewsItem
            {
                NextNewsArticle["ID"].GetUint(),
                NextNewsArticle["Title"].GetString(),
                NextNewsArticle["Payload"].GetString(),
                NextNewsArticle["TimePosted"].GetUint(),
                "", /* this value is assigned properly in the scope below */
                NextNewsArticle["Author"].GetString(),
                NextNewsArticle["Link"].GetString(),
                NextNewsArticle["Image"].GetString(),
            };

            {
                std::tm destTime;
                localtime_s(&destTime, &nNewsItem.m_nPostedAt);

                char buffer[256U]{};
                strftime(buffer, 256U, "%b %d", &destTime);
                nNewsItem.m_sPostedAt = buffer;
            }

            m_LatestNews.push_back(std::move(nNewsItem));
        }
    }
}

void AchievementOverlay::UpdateImages() noexcept
{
    using ra::services::ImageReference;
    m_hOverlayBackground = ImageReference{ ra::services::ImageType::Local, "Overlay\\overlayBG.png" };
    m_hUserImage         = ImageReference{ ra::services::ImageType::UserPic, RAUsers::LocalUser().Username() };
}


AchievementExamine::AchievementExamine() :
    m_pSelectedAchievement(nullptr),
    m_bHasData(false),
    m_nTotalWinners(0),
    m_nPossibleWinners(0)
{
}

void AchievementExamine::Clear()
{
    m_pSelectedAchievement = nullptr;
    m_CreatedDate.clear();
    m_LastModifiedDate.clear();
    m_bHasData = false;
    m_nTotalWinners = 0;
    m_nPossibleWinners = 0;
    RecentWinners.clear();
}

void AchievementExamine::Initialize(const Achievement* pAch)
{
    Clear();
    m_pSelectedAchievement = pAch;

    if (pAch == nullptr)
    {
        //	Do nothing.
    }
    else if (m_pSelectedAchievement->ID() == 0)
    {
        //	Uncommitted/Exempt ID
        //	NB. Don't attempt to get anything
        m_bHasData = true;
    }
    else
    {
        //	Go fetch data:
        m_CreatedDate = _TimeStampToString(pAch->CreatedDate());
        m_LastModifiedDate = _TimeStampToString(pAch->ModifiedDate());

        PostArgs args;
        args['u'] = RAUsers::LocalUser().Username();
        args['t'] = RAUsers::LocalUser().Token();
        args['a'] = std::to_string(m_pSelectedAchievement->ID());
        args['f'] = true;	//	Friends only?
        RAWeb::CreateThreadedHTTPRequest(RequestAchievementInfo, args);
    }
}

void AchievementExamine::OnReceiveData(rapidjson::Document& doc)
{
    ASSERT(doc["Success"].GetBool());
    const auto nOffset{ doc["Offset"].GetUint() };
    const auto nCount{ doc["Count"].GetUint() };
    const auto nFriendsOnly{ doc["FriendsOnly"].GetUint() };
    const auto nAchievementID{ doc["AchievementID"].GetUint() };
    const auto& ResponseData{ doc["Response"] };

    const auto nGameID{ ResponseData["GameID"].GetUint() };

    m_nTotalWinners    = ResponseData["NumEarned"].GetUint();
    m_nPossibleWinners = ResponseData["TotalPlayers"].GetUint();

    const auto& RecentWinnerData{ ResponseData["RecentWinner"] };
    ASSERT(RecentWinnerData.IsArray());
    for (auto& NextWinner : RecentWinnerData.GetArray())
    {
        const auto nDateAwarded{ static_cast<time_t>(ra::to_signed(NextWinner["DateAwarded"].GetUint())) };
        std::ostringstream oss;
        oss << NextWinner["User"].GetString() << " (" << NextWinner["RAPoints"].GetUint() << ")";
        RecentWinners.push_back({ oss.str(), _TimeStampToString(nDateAwarded) });
    }

    m_bHasData = true;
}

void LeaderboardExamine::Initialize(const unsigned int nLBIDIn)
{
    m_bHasData = false;

    if (m_nLBID == nLBIDIn)
    {
        //	Same ID again: keep existing data
        m_bHasData = true;
        return;
    }

    m_nLBID = nLBIDIn;

    const unsigned int nOffset = 0;		//	TBD
    const unsigned int nCount = 10;

    PostArgs args;
    args['i'] = std::to_string(m_nLBID);
    args['o'] = std::to_string(nOffset);
    args['c'] = std::to_string(nCount);

    RAWeb::CreateThreadedHTTPRequest(RequestLeaderboardInfo, args);
}

//static 
void LeaderboardExamine::OnReceiveData(const rapidjson::Document& doc)
{
    ASSERT(doc.HasMember("LeaderboardData"));
    const auto& LBData{ doc["LeaderboardData"] };

    const auto nLBID{ LBData["LBID"].GetUint() };
    const auto nGameID{ LBData["GameID"].GetUint() };
    const auto sConsoleID{ LBData["ConsoleID"].GetUint() };
    const auto nLowerIsBetter{ LBData["LowerIsBetter"].GetUint() };

    auto& pLeaderboardManager = ra::services::ServiceLocator::GetMutable<ra::services::ILeaderboardManager>();
    RA_Leaderboard* pLB = pLeaderboardManager.FindLB(nLBID);
    if (!pLB)
        return;

    const auto& Entries{ LBData["Entries"] };
    ASSERT(Entries.IsArray());
    for (auto& NextLBData : Entries.GetArray())
    {
        const auto nRank{ NextLBData["Rank"].GetUint() };
        const std::string& sUser{ NextLBData["User"].GetString() };
        const auto nScore{ NextLBData["Score"].GetInt() };
        const auto nDate{ NextLBData["DateSubmitted"].GetUint() };

        RA_LOG("LB Entry: %u: %s earned %d at %u\n", nRank, sUser.c_str(), nScore, nDate);
        pLB->SubmitRankInfo(nRank, sUser.c_str(), nScore, static_cast<time_t>(ra::to_signed(nDate)));
    }

    m_bHasData = true;
}

//	Stubs for non-class based, indirect calling of these functions.
_Use_decl_annotations_
API int _RA_UpdateOverlay(ControllerInput* pInput, float fDTime, bool Full_Screen, bool Paused)
{
    return g_AchievementOverlay.Update(pInput, fDTime, Full_Screen, Paused);
}

_Use_decl_annotations_
API void _RA_RenderOverlay(HDC hDC, RECT* rcSize)
{
    g_AchievementOverlay.Render(hDC, rcSize);
}

API bool _RA_IsOverlayFullyVisible()
{
    return g_AchievementOverlay.IsFullyVisible();
}
