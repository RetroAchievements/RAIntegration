#ifndef RA_UI_SCOREBOARD_VIEWMODEL_H
#define RA_UI_SCOREBOARD_VIEWMODEL_H
#pragma once

#include "PopupViewModelBase.hh"

#include "ui\ViewModelCollection.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class ScoreboardViewModel : public PopupViewModelBase
{
public:
    ScoreboardViewModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the header text.
    /// </summary>
    static const StringModelProperty HeaderTextProperty;

    /// <summary>
    /// Gets the header text to display.
    /// </summary>
    const std::wstring& GetHeaderText() const { return GetValue(HeaderTextProperty); }

    /// <summary>
    /// Sets the header text to display.
    /// </summary>
    void SetHeaderText(const std::wstring& sValue) { SetValue(HeaderTextProperty, sValue); }

    /// <summary>
    /// Updates the image to render.
    /// </summary>
    bool UpdateRenderImage(double fElapsed) override;

    /// <summary>
    /// Begins the animation cycle.
    /// </summary>
    void BeginAnimation() override;
    
    /// <summary>
    /// Determines whether the animation cycle has started.
    /// </summary>
    bool IsAnimationStarted() const noexcept override
    {
        return (m_fAnimationProgress >= 0.0);
    }

    /// <summary>
    /// Determines whether the animation cycle has completed.
    /// </summary>
    bool IsAnimationComplete() const noexcept override
    {
        return (m_fAnimationProgress >= TOTAL_ANIMATION_TIME);
    }

    class EntryViewModel : public ViewModelBase
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the entry rank.
        /// </summary>
        static const IntModelProperty RankProperty;

        /// <summary>
        /// Gets the rank for the entry.
        /// </summary>
        int GetRank() const { return GetValue(RankProperty); }

        /// <summary>
        /// Sets the rank for the entry.
        /// </summary>
        void SetRank(int nValue) { SetValue(RankProperty, nValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the entry user name.
        /// </summary>
        static const StringModelProperty UserNameProperty;

        /// <summary>
        /// Gets the user name for the entry.
        /// </summary>
#undef GetUserName
        const std::wstring& GetUserName() const { return GetValue(UserNameProperty); }

        /// <summary>
        /// Sets the user name for the entry.
        /// </summary>
        void SetUserName(const std::wstring& sValue) { SetValue(UserNameProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the entry score.
        /// </summary>
        static const StringModelProperty ScoreProperty;

        /// <summary>
        /// Gets the header text to display.
        /// </summary>
        const std::wstring& GetScore() const { return GetValue(ScoreProperty); }

        /// <summary>
        /// Sets the header text to display.
        /// </summary>
        void SetScore(const std::wstring& sValue) { SetValue(ScoreProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the entry highlight state.
        /// </summary>
        static const BoolModelProperty IsHighlightedProperty;

        /// <summary>
        /// Gets whether the entry should be highlighted.
        /// </summary>
        bool IsHighlighted() const { return GetValue(IsHighlightedProperty); }

        /// <summary>
        /// Sets whether the entry should be highlighted.
        /// </summary>
        void SetHighlighted(bool bValue) { SetValue(IsHighlightedProperty, bValue); }
    };

    ViewModelCollection<EntryViewModel>& Entries() noexcept { return m_vEntries; }
    const ViewModelCollection<EntryViewModel>& Entries() const noexcept { return m_vEntries; }

private:
    ViewModelCollection<EntryViewModel> m_vEntries;

    double m_fAnimationProgress = -1.0;
    int m_nInitialX = 0;
    int m_nTargetX = 0;

    static _CONSTANT_VAR TOTAL_ANIMATION_TIME = 7.0;
    static _CONSTANT_VAR INOUT_TIME = 0.8;
    static _CONSTANT_VAR HOLD_TIME = TOTAL_ANIMATION_TIME - INOUT_TIME * 2;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_SCOREBOARD_VIEWMODEL_H
