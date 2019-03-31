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

    class AchievementViewModel : public ViewModelBase
    {
    public:
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

        ImageReference Image{};
    };

private:
    double m_fElapsed = 0.0;
    bool m_bImagesPending = false;

    ra::ui::ViewModelCollection<AchievementViewModel> m_vAchievements;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_ACHIEVEMENTS_VIEWMODEL_H
