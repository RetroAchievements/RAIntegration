#ifndef RA_ACHIEVEMENTOVERLAY_H
#define RA_ACHIEVEMENTOVERLAY_H
#pragma once

#include "RA_Achievement.h"
#include "RA_User.h"
#include "RA_Core.h" // for API; RA_Interface.h
#include "services\ImageRepository.h" // map, string

enum OverlayPage
{
    OP_ACHIEVEMENTS,
    OP_FRIENDS,
    OP_MESSAGES,
    OP_NEWS,
    OP_LEADERBOARDS,

    OP_ACHIEVEMENT_EXAMINE,
    OP_ACHIEVEMENT_COMPARE,
    OP_FRIEND_EXAMINE,
    OP_FRIEND_ADD,
    OP_LEADERBOARD_EXAMINE,
    OP_MESSAGE_VIEWER,

    NumOverlayPages
};

enum TransitionState
{
    TS_OFF = 0,
    TS_IN,
    TS_HOLD,
    TS_OUT,

    TS__MAX
};

class LeaderboardExamine
{
public:
    void Initialize(const unsigned int nLBIDIn);
    //static void CB_OnReceiveData( void* pRequestObject );
    void OnReceiveData(const rapidjson::Document& doc);

public:
    unsigned int m_nLBID;
    //	Refer to RA_Leaderboard entry rank info via g_LeaderboardManager.FindLB(m_nLBID)
    bool m_bHasData;
};
extern LeaderboardExamine g_LBExamine;


class AchievementExamine
{
    struct RecentWinnerData
    {
        const std::string m_sUser;
        const std::string m_sWonAt;
        friend class AchievementOverlay;
    };

public:
    void Initialize(const Achievement* pAchIn);
    void Clear();
    void OnReceiveData(rapidjson::Document& doc);

    bool HasData() const { return m_bHasData; }
    const std::string& CreatedDate() const { return m_CreatedDate; }
    const std::string& ModifiedDate() const { return m_LastModifiedDate; }
    size_t NumRecentWinners() const { return RecentWinners.size(); }
    const RecentWinnerData& GetRecentWinner(size_t nOffs) const { return RecentWinners.at(nOffs); }

    unsigned int TotalWinners() const { return m_nTotalWinners; }
    unsigned int PossibleWinners() const { return m_nPossibleWinners; }

    _NODISCARD inline auto begin() noexcept { return RecentWinners.begin(); }
    _NODISCARD inline auto begin() const noexcept { return RecentWinners.begin(); }
    _NODISCARD inline auto end() noexcept { return RecentWinners.end(); }
    _NODISCARD inline auto end() const noexcept { return RecentWinners.end(); }

private:
    const Achievement* m_pSelectedAchievement{};
    std::string m_CreatedDate;
    std::string m_LastModifiedDate;

    bool m_bHasData{};

    //	Data found:
    unsigned int m_nTotalWinners{};
    unsigned int m_nPossibleWinners{};

    std::vector<RecentWinnerData> RecentWinners;
    friend class AchievementOverlay;
};
extern AchievementExamine g_AchExamine;

class AchievementOverlay
{
public:
    void Activate();
    void Deactivate();

    void Render(HDC hDC, RECT* rcDest) const;
    BOOL Update(ControllerInput* input, float fDelta, BOOL bFullScreen, BOOL bPaused);

    BOOL IsActive() const { return(m_nTransitionState != TS_OFF); }
    BOOL IsFullyVisible() const { return (m_nTransitionState == TS_HOLD); }

    const int* GetActiveScrollOffset() const;
    const int* GetActiveSelectedItem() const;

    void OnLoad_NewRom();

    void DrawAchievementsPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawAchievementExaminePage(HDC hDC, int nDX, _UNUSED int, _UNUSED const RECT&) const;
    void DrawMessagesPage(_UNUSED HDC, _UNUSED int, _UNUSED int, _UNUSED const RECT&) const;
    void DrawFriendsPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const;
    void DrawNewsPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const;
    void DrawLeaderboardPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const;
    void DrawLeaderboardExaminePage(HDC hDC, int nDX, _UNUSED int, _UNUSED const RECT&) const;

    void DrawBar(HDC hDC, int nX, int nY, int nW, int nH, int nMax, int nSel) const;
    void DrawUserFrame(HDC hDC, RAUser* pUser, int nX, int nY, int nW, int nH) const;
    void DrawAchievement(HDC hDC, const Achievement* Ach, int nX, int nY, BOOL bSelected, BOOL bCanLock) const;

    OverlayPage CurrentPage() { return m_Pages[m_nPageStackPointer]; }
    void AddPage(OverlayPage NewPage);
    BOOL GoBack();

    void SelectNextTopLevelPage(BOOL bPressedRight);

    void InstallNewsArticlesFromFile();
    void UpdateImages() noexcept;
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
#pragma warning(push)
#pragma warning(disable : 26495) // "variable" uninitialized
    };
#pragma warning(pop)


private:
    int	m_nAchievementsScrollOffset{};
    int	m_nFriendsScrollOffset{};
    int	m_nMessagesScrollOffset{};
    int	m_nNewsScrollOffset{};
    int	m_nLeaderboardScrollOffset{};

    int	m_nAchievementsSelectedItem{};
    int	m_nFriendsSelectedItem{};
    int	m_nMessagesSelectedItem{};
    int	m_nNewsSelectedItem{};
    int	m_nLeaderboardSelectedItem{};

    mutable int m_nNumAchievementsBeingRendered{};
    mutable int m_nNumFriendsBeingRendered{};
    mutable int m_nNumLeaderboardsBeingRendered{};

    BOOL					m_bInputLock{};	//	Waiting for pad release
    std::vector<NewsItem>	m_LatestNews{};
    TransitionState			m_nTransitionState{};
    float                   m_fTransitionTimer{-0.2F};

    OverlayPage	            m_Pages[5]{OverlayPage::OP_ACHIEVEMENTS};
    unsigned int			m_nPageStackPointer{};

    ra::services::ImageReference m_hOverlayBackground;
    ra::services::ImageReference m_hUserImage;
    mutable std::map<std::string, ra::services::ImageReference> m_mAchievementBadges;
};
extern AchievementOverlay g_AchievementOverlay;

//	Exposed to DLL
extern "C"
{
    API extern int _RA_UpdateOverlay(ControllerInput* pInput, float fDTime, bool Full_Screen, bool Paused);
    API extern void _RA_RenderOverlay(HDC hDC, RECT* rcSize);
    API extern bool _RA_IsOverlayFullyVisible();
}

extern const COLORREF COL_TEXT;
extern const COLORREF COL_TEXT_HIGHLIGHT;
extern const COLORREF COL_SELECTED;
extern const COLORREF COL_WHITE;
extern const COLORREF COL_BLACK;
extern const COLORREF COL_POPUP;
extern const COLORREF COL_POPUP_BG;
extern const COLORREF COL_POPUP_SHADOW;


#endif // !RA_ACHIEVEMENTOVERLAY_H
