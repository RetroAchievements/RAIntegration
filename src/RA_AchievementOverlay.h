#ifndef RA_ACHIEVEMENTOVERLAY_H
#define RA_ACHIEVEMENTOVERLAY_H
#pragma once

#include "RA_Achievement.h"
#include "RA_Core.h" // RA_Interface
#include "RA_User.h"

#include "ui\ImageReference.hh"

class LeaderboardExamine
{
public:
    void Initialize(const unsigned int nLBIDIn);
    // static void CB_OnReceiveData( void* pRequestObject );
    void OnReceiveData(const rapidjson::Document& doc);

public:
    unsigned int m_nLBID;
    //	Refer to RA_Leaderboard entry rank info via g_LeaderboardManager.FindLB(m_nLBID)
    bool m_bHasData;
};
extern LeaderboardExamine g_LBExamine;

class AchievementExamine
{
public:
    class RecentWinnerData
    {
    public:
        explicit RecentWinnerData(const std::string& sUser, const std::string& sWonAt) :
            m_sUser{sUser},
            m_sWonAt{sWonAt}
        {}

        const std::string& User() const noexcept { return m_sUser; }
        const std::string& WonAt() const noexcept { return m_sWonAt; }

    private:
        const std::string m_sUser;
        const std::string m_sWonAt;
    };

public:
    void Initialize(const Achievement* pAchIn);
    void Clear() noexcept;
    void OnReceiveData(rapidjson::Document& doc);

    bool HasData() const noexcept { return m_bHasData; }
    const std::string& CreatedDate() const noexcept { return m_CreatedDate; }
    const std::string& ModifiedDate() const noexcept { return m_LastModifiedDate; }
    size_t NumRecentWinners() const noexcept { return RecentWinners.size(); }
    const RecentWinnerData& GetRecentWinner(size_t nOffs) const { return RecentWinners.at(nOffs); }

    unsigned int TotalWinners() const noexcept { return m_nTotalWinners; }
    unsigned int PossibleWinners() const noexcept { return m_nPossibleWinners; }

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
};
extern AchievementExamine g_AchExamine;

class AchievementOverlay
{
    enum class Page
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

    enum class TransitionState
    {
        Off,
        In,
        Hold,
        Out
    };

public:
    void Activate() noexcept;
    void Deactivate() noexcept;

    void Render(_In_ HDC hDC, _In_ const RECT* rcDest) const;
    _NODISCARD BOOL Update(_In_ gsl::not_null<const ControllerInput*> input, _In_ float fDelta, _In_ BOOL bFullScreen,
                           _In_ BOOL bPaused);

    _NODISCARD _CONSTANT_FN IsActive() const noexcept { return (m_nTransitionState != TransitionState::Off); }
    _NODISCARD _CONSTANT_FN IsFullyVisible() const noexcept { return (m_nTransitionState == TransitionState::Hold); }

    int* GetActiveScrollOffset() const noexcept;
    int* GetActiveSelectedItem() const noexcept;

    void OnLoad_NewRom() noexcept;

    void DrawAchievementsPage(HDC hDC, int nDX, int nDY, const RECT& rcTarget) const;
    void DrawAchievementExaminePage(HDC hDC, int nDX, _UNUSED int, _UNUSED const RECT&) const;
    void DrawMessagesPage(_UNUSED HDC, _UNUSED int, _UNUSED int, _UNUSED const RECT&) const noexcept;
    void DrawFriendsPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const;
    void DrawNewsPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const;
    void DrawLeaderboardPage(HDC hDC, int nDX, _UNUSED int, const RECT& rcTarget) const;
    void DrawLeaderboardExaminePage(HDC hDC, int nDX, _UNUSED int, _UNUSED const RECT&) const;

    void DrawBar(HDC hDC, int nX, int nY, int nW, int nH, int nMax, int nSel) const noexcept;
    void DrawUserFrame(_In_ HDC hDC, _In_ int nX, _In_ int nY, _In_ int nW, _In_ int nH) const;
    void DrawAchievement(HDC hDC, gsl::not_null<const Achievement*> Ach, int nX, int nY, BOOL bSelected,
                         BOOL bCanLock) const;

    _NODISCARD _CONSTANT_FN CurrentPage() const noexcept { return m_Pages.at(m_nPageStackPointer); }
    _CONSTANT_FN AddPage(_In_ Page NewPage) noexcept
    {
        m_nPageStackPointer++;
        m_Pages.at(m_nPageStackPointer) = NewPage;
    }

    BOOL GoBack() noexcept;
    void SelectNextTopLevelPage(BOOL bPressedRight) noexcept;
    void InstallNewsArticlesFromFile();

    [[gsl::suppress(f .6)]] void UpdateImages() noexcept;

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
    inline static constexpr auto PAGE_TRANSITION_IN{-0.200F};
    inline static constexpr auto PAGE_TRANSITION_OUT{0.2F};

    mutable int m_nAchievementsScrollOffset{};
    mutable int m_nFriendsScrollOffset{};
    mutable int m_nMessagesScrollOffset{};
    mutable int m_nNewsScrollOffset{};
    mutable int m_nLeaderboardScrollOffset{};

    mutable int m_nAchievementsSelectedItem{};
    mutable int m_nFriendsSelectedItem{};
    mutable int m_nMessagesSelectedItem{};
    mutable int m_nNewsSelectedItem{};
    mutable int m_nLeaderboardSelectedItem{};

    mutable int m_nNumAchievementsBeingRendered{};
    mutable int m_nNumFriendsBeingRendered{};
    mutable int m_nNumLeaderboardsBeingRendered{};

    BOOL m_bInputLock{}; // Waiting for pad release
    std::vector<NewsItem> m_LatestNews{};
    TransitionState m_nTransitionState{};
    float m_fTransitionTimer{PAGE_TRANSITION_IN};

    std::array<Page, 5> m_Pages{Page::Achievements};
    unsigned int m_nPageStackPointer{};

    mutable ra::ui::ImageReference m_hOverlayBackground;
    mutable ra::ui::ImageReference m_hUserImage;
    mutable std::map<std::string, ra::ui::ImageReference> m_mAchievementBadges;
};
extern AchievementOverlay g_AchievementOverlay;

// Exposed to DLL
extern "C" {
[[gsl::suppress(con.3)]] API int _RA_UpdateOverlay(_In_ ControllerInput* pInput, _In_ float fDTime,
                                                   _In_ bool Full_Screen, _In_ bool Paused);
[[gsl::suppress(con.3)]] API void _RA_RenderOverlay(_In_ HDC hDC, _In_ RECT* rcSize);
API bool _RA_IsOverlayFullyVisible();
}

_CONSTANT_VAR COL_TEXT           = RGB(17, 102, 221);
_CONSTANT_VAR COL_TEXT_HIGHLIGHT = RGB(251, 102, 0);
_CONSTANT_VAR COL_SELECTED       = RGB(255, 255, 255);
_CONSTANT_VAR COL_BLACK          = RGB(0, 0, 0);
_CONSTANT_VAR COL_WHITE          = RGB(255, 255, 255);
_CONSTANT_VAR COL_POPUP          = RGB(0, 0, 40);
_CONSTANT_VAR COL_POPUP_BG       = RGB(212, 212, 212);
_CONSTANT_VAR COL_POPUP_SHADOW   = RGB(0, 0, 0);

#endif // !RA_ACHIEVEMENTOVERLAY_H
