#ifndef RA_UI_POPUPVIEWMODELBASE_H
#define RA_UI_POPUPVIEWMODELBASE_H
#pragma once

#include "ui\WindowViewModelBase.hh"
#include "ui\drawing\ISurface.hh"

namespace ra {
namespace ui {
namespace viewmodels {

enum class Popup
{
    None = 0,
    Message,
    AchievementTriggered,
    Mastery,
    LeaderboardStarted,
    LeaderboardCanceled,
    LeaderboardTracker,
    LeaderboardScoreboard,
    NumPopups
};

enum class PopupLocation
{
    None,
    TopLeft,
    TopMiddle,
    TopRight,
    BottomLeft,
    BottomMiddle,
    BottomRight
};

class PopupViewModelBase : public ViewModelBase
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for the classification of the popup.
    /// </summary>
    static const IntModelProperty PopupTypeProperty;

    /// <summary>
    /// Gets the classification of the popup.
    /// </summary>
    Popup GetPopupType() const { return ra::itoe<Popup>(GetValue(PopupTypeProperty)); }

    /// <summary>
    /// Sets the classification of the popup.
    /// </summary>
    void SetPopupType(Popup nType) { SetValue(PopupTypeProperty, ra::etoi(nType)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the X location to render the popup at.
    /// </summary>
    static const IntModelProperty HorizontalOffsetProperty;

    /// <summary>
    /// Gets the X location to render the popup at.
    /// </summary>
    int GetHorizontalOffset() const { return GetValue(HorizontalOffsetProperty); }

    /// <summary>
    /// Sets the X location to render the popup at.
    /// </summary>
    void SetHorizontalOffset(int nValue) { SetValue(HorizontalOffsetProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the Y location to render the popup at.
    /// </summary>
    static const IntModelProperty VerticalOffsetProperty;

    /// <summary>
    /// Gets the Y location to render the popup at.
    /// </summary>
    int GetVerticalOffset() const { return GetValue(VerticalOffsetProperty); }

    /// <summary>
    /// Sets the Y location to render the popup at.
    /// </summary>
    void SetVerticalOffset(int nValue) { SetValue(VerticalOffsetProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the X location to render the popup at.
    /// </summary>
    static const IntModelProperty RenderLocationXProperty;

    /// <summary>
    /// Gets the X location to render the popup at.
    /// </summary>
    int GetRenderLocationX() const { return GetValue(RenderLocationXProperty); }

    /// <summary>
    /// Sets the X location to render the popup at.
    /// </summary>
    void SetRenderLocationX(int nValue) { SetValue(RenderLocationXProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the X location to render the popup at.
    /// </summary>
    static const IntModelProperty RenderLocationYProperty;

    /// <summary>
    /// Gets the X location to render the popup at.
    /// </summary>
    int GetRenderLocationY() const { return GetValue(RenderLocationYProperty); }

    /// <summary>
    /// Sets the Y location to render the popup at.
    /// </summary>
    void SetRenderLocationY(int nValue) { SetValue(RenderLocationYProperty, nValue); }

    /// <summary>
    /// Begins the animation cycle.
    /// </summary>
    virtual void BeginAnimation() = 0;

    /// <summary>
    /// Determines whether the animation cycle has started.
    /// </summary>
    virtual bool IsAnimationStarted() const noexcept = 0;

    /// <summary>
    /// Determines whether the animation cycle has completed.
    /// </summary>
    virtual bool IsAnimationComplete() const noexcept = 0;

    /// <summary>
    /// Gets the image to render.
    /// </summary>
    const ra::ui::drawing::ISurface& GetRenderImage() const
    {
        Expects(m_pSurface != nullptr);
        return *m_pSurface;
    }

    /// <summary>
    /// Determines if there is an image to render.
    /// </summary>
    const bool HasRenderImage() const noexcept { return m_pSurface != nullptr; }

    /// <summary>
    /// Updates the image to render.
    /// </summary>
    /// <param name="fElapsed">The fractional of seconds that have passed.</param>
    /// <returns><c>true</c> if the image or location was updated, <c>false</c> if not.</returns>
    virtual bool UpdateRenderImage(double fElapsed) = 0;

    /// <summary>
    /// Gets the unique identifier of the popup.
    /// </summary>
    int GetPopupId() const noexcept { return m_nPopupId; }

    /// <summary>
    /// Sets the unique identifier of the popup.
    /// </summary>
    void SetPopupId(int nPopupId) noexcept { m_nPopupId = nPopupId; }

    /// <summary>
    /// Gets whether this instance has been marked for destruction.
    /// </summary>
    bool IsDestroyPending() const noexcept { return m_bDestroyPending; }

    /// <summary>
    /// Marks this instance to be destroyed.
    /// </summary>
    void SetDestroyPending() noexcept { m_bDestroyPending = true; }

protected:
    static int GetFadeOffset(double fAnimationProgress, double fTotalAnimationTime, double fInOutTime, int nInitialOffset, int nTargetOffset);

    std::unique_ptr<ra::ui::drawing::ISurface> m_pSurface;
    int m_nPopupId = 0;
    bool m_bDestroyPending = false;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POPUPVIEWMODELBASE_H
