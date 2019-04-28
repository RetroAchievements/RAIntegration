#ifndef RA_UI_BROKENACHIEVEMENTSVIEWMODEL_H
#define RA_UI_BROKENACHIEVEMENTSVIEWMODEL_H
#pragma once

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class BrokenAchievementsViewModel : public WindowViewModelBase
{
public:
    GSL_SUPPRESS_F6 BrokenAchievementsViewModel() = default;
    ~BrokenAchievementsViewModel() = default;

    BrokenAchievementsViewModel(const BrokenAchievementsViewModel&) noexcept = delete;
    BrokenAchievementsViewModel& operator=(const BrokenAchievementsViewModel&) noexcept = delete;
    BrokenAchievementsViewModel(BrokenAchievementsViewModel&&) noexcept = delete;
    BrokenAchievementsViewModel& operator=(BrokenAchievementsViewModel&&) noexcept = delete;
    
    /// <summary>
    /// Initializes the <see cref="Achievements"/> collection (asynchronously).
    /// </summary>
    bool InitializeAchievements(); // shows error and returns false for Local achievements
    
    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected problem.
    /// </summary>
    static const IntModelProperty SelectedProblemIdProperty;

    /// <summary>
    /// Gets the selected problem ID.
    /// </summary>
    int GetSelectedProblemId() const { return GetValue(SelectedProblemIdProperty); }

    /// <summary>
    /// Sets the selected problem ID.
    /// </summary>
    void SetSelectedProblemId(int nValue) { SetValue(SelectedProblemIdProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the comment.
    /// </summary>
    static const StringModelProperty CommentProperty;

    /// <summary>
    /// Gets the comment.
    /// </summary>
    const std::wstring& GetComment() const { return GetValue(CommentProperty); }

    /// <summary>
    /// Sets the comment.
    /// </summary>
    void SetComment(const std::wstring& sValue) { SetValue(CommentProperty, sValue); }

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
        /// The <see cref="ModelProperty" /> for the whether the achievement is selected.
        /// </summary>
        static const BoolModelProperty IsSelectedProperty;

        /// <summary>
        /// Gets whether the achievement is selected.
        /// </summary>
        bool IsSelected() const { return GetValue(IsSelectedProperty); }

        /// <summary>
        /// Sets whether the achievement is selected.
        /// </summary>
        void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }
        
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

private:
    ViewModelCollection<BrokenAchievementViewModel> m_vAchievements;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_BROKENACHIEVEMENTSVIEWMODEL_H
