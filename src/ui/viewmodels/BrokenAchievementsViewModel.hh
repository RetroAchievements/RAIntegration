#ifndef RA_UI_BROKENACHIEVEMENTSVIEWMODEL_H
#define RA_UI_BROKENACHIEVEMENTSVIEWMODEL_H
#pragma once

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class BrokenAchievementsViewModel : public WindowViewModelBase, protected ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 BrokenAchievementsViewModel() = default;
    ~BrokenAchievementsViewModel() = default;

    BrokenAchievementsViewModel(const BrokenAchievementsViewModel&) noexcept = delete;
    BrokenAchievementsViewModel& operator=(const BrokenAchievementsViewModel&) noexcept = delete;
    BrokenAchievementsViewModel(BrokenAchievementsViewModel&&) noexcept = delete;
    BrokenAchievementsViewModel& operator=(BrokenAchievementsViewModel&&) noexcept = delete;
    
    /// <summary>
    /// Initializes the <see cref="Achievements"/> collection.
    /// </summary>
    bool InitializeAchievements(); // shows error and returns false for Local achievements
    
    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected achievement index.
    /// </summary>
    static const IntModelProperty SelectedIndexProperty;

    /// <summary>
    /// Gets the index of the selected achievement.
    /// </summary>
    int GetSelectedIndex() const { return GetValue(SelectedIndexProperty); }

    /// <summary>
    /// Sets the index of the selected achievement.
    /// </summary>
    void SetSelectedIndex(int nValue) { SetValue(SelectedIndexProperty, nValue); }

    /// <summary>
    /// Command handler for Submit button.
    /// </summary>    
    /// <returns>
    /// <c>true</c> if the submission was successful and the dialog should be closed, 
    /// <c>false</c> if not.
    /// </returns>
    bool Submit();

    class BrokenAchievementViewModel : public LookupItemViewModel
    {
    public:
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
        /// The <see cref="ModelProperty" /> for the whether the achievement was unlocked.
        /// </summary>
        static const BoolModelProperty IsAchievedProperty;

        /// <summary>
        /// Gets whether the achievement was unlocked.
        /// </summary>
        bool IsAchieved() const { return GetValue(IsAchievedProperty); }

        /// <summary>
        /// Sets whether the achievement was unlocked.
        /// </summary>
        void SetAchieved(bool bValue) { SetValue(IsAchievedProperty, bValue); }
    };

    /// <summary>
    /// Gets the list of achievements and their IDs
    /// </summary>
    ViewModelCollection<BrokenAchievementViewModel>& Achievements() noexcept
    {
        return m_vAchievements;
    }

protected:
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;

private:
    void OnItemSelectedChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args);

    ViewModelCollection<BrokenAchievementViewModel> m_vAchievements;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_BROKENACHIEVEMENTSVIEWMODEL_H
