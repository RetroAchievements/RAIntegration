#ifndef RA_ACHIEVEMENTOVERLAY_H
#define RA_ACHIEVEMENTOVERLAY_H
#pragma once

struct IUnknown;
#include "RA_Achievement.h"
#include "RA_User.h"
#include "RA_Core.h"

#ifndef RA_INTERFACE_H
#include "RA_Interface.h"  
#endif /* !RA_INTERFACE_H */

#include "services/ImageRepository.h"

#ifndef _MAP_
#include <map>
#endif /* !_MAP_ */

#ifndef _STRING_
#include <string>
#endif /* !_STRING_ */

namespace ra {

enum class OverlayPage
{
    Achievements,
    Friends,
    Messages,
    News,
    Leaderboards,

    Achievement_Examine,
    Achievement_Compare,
    Friend_Examine,
    Friend_Add,
    Leaderboard_Examine,
    Message_Viewer
};

enum class TransitionState { Off, In, Hold, Out };

namespace enum_sizes {

_CONSTANT_VAR NUM_OVERLAY_PAGES{ 11U };
_CONSTANT_VAR TS__MAX{ 4U };

} // namespace enum_sizes
} // namespace ra

class LeaderboardExamine
{
public:
    void Initialize(const unsigned int nLBIDIn);
    //static void CB_OnReceiveData( void* pRequestObject );
    void OnReceiveData(const Document& doc);

public:
    unsigned int m_nLBID{};
    //	Refer to RA_Leaderboard entry rank info via g_LeaderboardManager.FindLB(m_nLBID)
    bool m_bHasData{};
};
extern LeaderboardExamine g_LBExamine;


class AchievementExamine
{
public:
    class RecentWinnerData
    {
    public:
        RecentWinnerData(const std::string& sUser, const std::string& sWonAt) :
            m_sUser(sUser), m_sWonAt(sWonAt)
        {
        }
        const std::string& User() const { return m_sUser; }
        const std::string& WonAt() const { return m_sWonAt; }

    private:
        const std::string m_sUser;
        const std::string m_sWonAt;
    };

public:
    void Initialize(const Achievement* pAchIn);
    void Clear();
    void OnReceiveData(Document& doc);

    bool HasData() const { return m_bHasData; }
    const std::string& CreatedDate() const { return m_CreatedDate; }
    const std::string& ModifiedDate() const { return m_LastModifiedDate; }
    size_t NumRecentWinners() const { return RecentWinners.size(); }
    const RecentWinnerData& GetRecentWinner(size_t nOffs) const { return RecentWinners.at(nOffs); }

    unsigned int TotalWinners() const { return m_nTotalWinners; }
    unsigned int PossibleWinners() const { return m_nPossibleWinners; }

private:
    const Achievement* m_pSelectedAchievement{};
    std::string m_CreatedDate;
    std::string m_LastModifiedDate;

    bool m_bHasData{};

    //	Data found:
    unsigned int m_nTotalWinners{};
    unsigned int m_nPossibleWinners{};

    std::vector<RecentWinnerData> RecentWinners;
};
extern AchievementExamine g_AchExamine;


class AchievementOverlay
{
public:
    void Initialize(HINSTANCE hInst);

    void Activate();
    void Deactivate();

    void Render(HDC hDC, RECT* rcDest) const;
    BOOL Update(ControllerInput* input, float fDelta, BOOL bFullScreen, BOOL bPaused);

    BOOL IsActive() const { return(m_nTransitionState != ra::TransitionState::Off); }
    BOOL IsFullyVisible() const { return (m_nTransitionState == ra::TransitionState::Hold); }

    const int* GetActiveScrollOffset() const;
    const int* GetActiveSelectedItem() const;

    void OnLoad_NewRom();

    void DrawAchievementsPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawAchievementExaminePage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawMessagesPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawFriendsPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawNewsPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawLeaderboardPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawLeaderboardExaminePage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;

    void DrawBar(HDC hDC, int nX, int nY, int nW, int nH, int nMax, int nSel) const;
    void DrawUserFrame(HDC hDC, RAUser* pUser, int nX, int nY, int nW, int nH) const;
    void DrawAchievement(HDC hDC, const Achievement* Ach, int nX, int nY, BOOL bSelected, BOOL bCanLock) const;

    ra::OverlayPage CurrentPage() { return m_Pages[m_nPageStackPointer]; }
    void AddPage(ra::OverlayPage NewPage);
    BOOL GoBack();

    void SelectNextTopLevelPage(BOOL bPressedRight);

    void InstallNewsArticlesFromFile();

public:
    struct NewsItem
    {
        unsigned int m_nID{};
        std::string m_sTitle;
        std::string m_sPayload;
        time_t m_nPostedAt{};
        std::string m_sPostedAt;
        std::string m_sAuthor;
        std::string m_sLink;
        std::string m_sImage;
    };

private:
    int m_nAchievementsScrollOffset{};
    int m_nFriendsScrollOffset{};
    int m_nMessagesScrollOffset{};
    int m_nNewsScrollOffset{};
    int m_nLeaderboardScrollOffset{};

    int m_nAchievementsSelectedItem{};
    int m_nFriendsSelectedItem{};
    int m_nMessagesSelectedItem{};
    int m_nNewsSelectedItem{};
    int m_nLeaderboardSelectedItem{};

    mutable int m_nNumAchievementsBeingRendered{};
    mutable int m_nNumFriendsBeingRendered{};
    mutable int m_nNumLeaderboardsBeingRendered{};

    BOOL                  m_bInputLock{};	//	Waiting for pad release
    std::vector<NewsItem> m_LatestNews;
    ra::TransitionState   m_nTransitionState{};
    float                 m_fTransitionTimer{};

    ra::OverlayPage m_Pages[5]{};
    unsigned int    m_nPageStackPointer{};

    ra::services::ImageReference m_hOverlayBackground{};
    ra::services::ImageReference m_hUserImage{};
    mutable std::map<std::string, ra::services::ImageReference> m_mAchievementBadges;
};
extern AchievementOverlay g_AchievementOverlay;

//	Exposed to DLL
_EXTERN_C
API extern int _RA_UpdateOverlay(ControllerInput* pInput, float fDTime, bool Full_Screen, bool Paused);
API extern void _RA_RenderOverlay(HDC hDC, RECT* rcSize);
API extern bool _RA_IsOverlayFullyVisible();
_END_EXTERN_C

extern const COLORREF COL_TEXT;
extern const COLORREF COL_TEXT_HIGHLIGHT;
extern const COLORREF COL_SELECTED;
extern const COLORREF COL_WHITE;
extern const COLORREF COL_BLACK;
extern const COLORREF COL_POPUP;
extern const COLORREF COL_POPUP_BG;
extern const COLORREF COL_POPUP_SHADOW;


#endif // !RA_ACHIEVEMENTOVERLAY_H
