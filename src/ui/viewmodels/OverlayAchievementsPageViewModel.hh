#ifndef RA_UI_OVERLAY_ACHIEVEMENTS_VIEWMODEL_H
#define RA_UI_OVERLAY_ACHIEVEMENTS_VIEWMODEL_H

#include "OverlayViewModel.hh"

#include "ui\ViewModelCollection.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayAchievementsPageViewModel : public OverlayViewModel::PageViewModel
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for the game title.
    /// </summary>
    static const StringModelProperty GameTitleProperty;

    /// <summary>
    /// Gets the game title.
    /// </summary>
    const std::wstring& GetGameTitle() const { return GetValue(GameTitleProperty); }

    /// <summary>
    /// Sets the game title.
    /// </summary>
    void SetGameTitle(const std::wstring& sValue) { SetValue(GameTitleProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the summary information.
    /// </summary>
    static const StringModelProperty SummaryProperty;

    /// <summary>
    /// Gets the summary information.
    /// </summary>
    const std::wstring& GetSummary() const { return GetValue(SummaryProperty); }

    /// <summary>
    /// Sets the summary information.
    /// </summary>
    void SetSummary(const std::wstring& sValue) { SetValue(SummaryProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the user's play time.
    /// </summary>
    static const StringModelProperty PlayTimeProperty;

    /// <summary>
    /// Gets the user's play time.
    /// </summary>
    const std::wstring& GetPlayTime() const { return GetValue(PlayTimeProperty); }

    /// <summary>
    /// Sets the user's play time.
    /// </summary>
    void SetPlayTime(const std::wstring& sValue) { SetValue(PlayTimeProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the index of the selected achievement.
    /// </summary>
    static const IntModelProperty SelectedAchievementIndexProperty;

    /// <summary>
    /// Gets the index of the selected achievement.
    /// </summary>
    int GetSelectedAchievementIndex() const { return GetValue(SelectedAchievementIndexProperty); }

    /// <summary>
    /// Sets the index of the selected achievement.
    /// </summary>
    void SetSelectedAchievementIndex(int nValue) { SetValue(SelectedAchievementIndexProperty, nValue); }

    void Refresh() override;
    bool Update(double fElapsed) override;
    void Render(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const override;
    bool ProcessInput(const ControllerInput& pInput) override;

    class WinnerViewModel : public ViewModelBase
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the user name.
        /// </summary>
        static const StringModelProperty UsernameProperty;

        /// <summary>
        /// Gets the user name.
        /// </summary>
        const std::wstring& GetUsername() const { return GetValue(UsernameProperty); }

        /// <summary>
        /// Sets the user name.
        /// </summary>
        void SetUsername(const std::wstring& sValue) { SetValue(UsernameProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the unlock date.
        /// </summary>
        static const StringModelProperty UnlockDateProperty;

        /// <summary>
        /// Gets the unlock date.
        /// </summary>
        const std::wstring& GetUnlockDate() const { return GetValue(UnlockDateProperty); }

        /// <summary>
        /// Sets the unlock date.
        /// </summary>
        void SetUnlockDate(const std::wstring& sValue) { SetValue(UnlockDateProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether or not the winner is the user.
        /// </summary>
        static const BoolModelProperty IsUserProperty;

        /// <summary>
        /// Gets whether or not the winner is the user.
        /// </summary>
        bool IsUser() const { return GetValue(IsUserProperty); }

        /// <summary>
        /// Sets whether the winner is the user.
        /// </summary>
        void SetUser(bool bValue) { SetValue(IsUserProperty, bValue); }
    };

    class AchievementViewModel : public ViewModelBase
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the achievement ID.
        /// </summary>
        static const IntModelProperty AchievementIdProperty;

        /// <summary>
        /// Gets the unique identifier of the achievement.
        /// </summary>
        int GetAchievementId() const { return GetValue(AchievementIdProperty); }

        /// <summary>
        /// Sets the unique identifier of the achievement.
        /// </summary>
        void SetAchievementId(int nValue) { SetValue(AchievementIdProperty, nValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the achievement title.
        /// </summary>
        static const StringModelProperty TitleProperty;

        /// <summary>
        /// Gets the achievement title.
        /// </summary>
        const std::wstring& GetTitle() const { return GetValue(TitleProperty); }

        /// <summary>
        /// Sets the achievement title.
        /// </summary>
        void SetTitle(const std::wstring& sValue) { SetValue(TitleProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the achievement description.
        /// </summary>
        static const StringModelProperty DescriptionProperty;

        /// <summary>
        /// Gets the achievement description.
        /// </summary>
        const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

        /// <summary>
        /// Sets the achievement description.
        /// </summary>
        void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether or not the achievement is locked.
        /// </summary>
        static const BoolModelProperty IsLockedProperty;

        /// <summary>
        /// Gets whether or not the achievement is locked.
        /// </summary>
        bool IsLocked() const { return GetValue(IsLockedProperty); }

        /// <summary>
        /// Sets whether the achievement is locked.
        /// </summary>
        void SetLocked(bool bValue) { SetValue(IsLockedProperty, bValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the achievement creation date.
        /// </summary>
        static const StringModelProperty CreatedDateProperty;

        /// <summary>
        /// Gets the achievement creation date.
        /// </summary>
        const std::wstring& GetCreatedDate() const { return GetValue(CreatedDateProperty); }

        /// <summary>
        /// Sets the achievement creation date.
        /// </summary>
        void SetCreatedDate(const std::wstring& sValue) { SetValue(CreatedDateProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the achievement last modified date.
        /// </summary>
        static const StringModelProperty ModifiedDateProperty;

        /// <summary>
        /// Gets the achievement last modified date.
        /// </summary>
        const std::wstring& GetModifiedDate() const { return GetValue(ModifiedDateProperty); }

        /// <summary>
        /// Sets the achievement last modified date.
        /// </summary>
        void SetModifiedDate(const std::wstring& sValue) { SetValue(ModifiedDateProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the won by ration.
        /// </summary>
        static const StringModelProperty WonByProperty;

        /// <summary>
        /// Gets the won by ratio.
        /// </summary>
        const std::wstring& GetWonBy() const { return GetValue(WonByProperty); }

        /// <summary>
        /// Sets the won by ratio.
        /// </summary>
        void SetWonBy(const std::wstring& sValue) { SetValue(WonByProperty, sValue); }

        ImageReference Image{};

        ViewModelCollection<WinnerViewModel> RecentWinners;
    };

protected:
    gsl::index m_nScrollOffset = 0;
    mutable unsigned int m_nVisibleAchievements = 0;

private:
    void RenderList(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const;
    void RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const;
    void FetchAchievementDetail(AchievementViewModel& vmAchievement);

    double m_fElapsed = 0.0;
    bool m_bImagesPending = false;
    bool m_bAchievementDetail = false;
    bool m_bRedraw = false;

    ra::ui::ViewModelCollection<AchievementViewModel> m_vAchievements;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_ACHIEVEMENTS_VIEWMODEL_H
