#ifndef RA_UI_OVERLAY_ACHIEVEMENTS_VIEWMODEL_H
#define RA_UI_OVERLAY_ACHIEVEMENTS_VIEWMODEL_H

#include "OverlayListPageViewModel.hh"

#include "data/Types.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayAchievementsPageViewModel : public OverlayListPageViewModel
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for the user's unlocked achievements.
    /// </summary>
    static const StringModelProperty UnlockedAchievementsProperty;

    /// <summary>
    /// Gets the user's unlocked achievements.
    /// </summary>
    const std::wstring& GetUnlockedAchievements() const { return GetValue(UnlockedAchievementsProperty); }

    /// <summary>
    /// Sets the user's unlocked achievements.
    /// </summary>
    void SetUnlockedAchievements(const std::wstring& sValue) { SetValue(UnlockedAchievementsProperty, sValue); }

    void Refresh() override;
    bool Update(double fElapsed) override;

    const wchar_t* GetPrevButtonText() const noexcept override;
    const wchar_t* GetNextButtonText() const noexcept override;

    class AchievementViewModel : public ViewModelBase
    {
    public:
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

        ViewModelCollection<ItemViewModel> RecentWinners;
    };

protected:
    bool OnHeaderClicked(ItemViewModel& vmItem) override;
    void FetchItemDetail(ItemViewModel& vmItem) override;
    std::map<ra::AchievementID, AchievementViewModel> m_vAchievementDetails;
    std::map<uint32_t, bool> m_mCollapseState;

private:
    void RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, _UNUSED int nWidth, int nHeight) const override;

    std::wstring m_sSummary;
    std::map<gsl::index, uint32_t> m_mHeaderKeys;

    std::unique_ptr<ra::ui::drawing::ISurface> m_pMissableSurface;
    std::unique_ptr<ra::ui::drawing::ISurface> m_pProgressionSurface;
    std::unique_ptr<ra::ui::drawing::ISurface> m_pWinConditionSurface;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_ACHIEVEMENTS_VIEWMODEL_H
