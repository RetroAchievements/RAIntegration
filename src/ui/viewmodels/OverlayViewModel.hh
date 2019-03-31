#ifndef RA_UI_OVERLAY_VIEWMODEL_H
#define RA_UI_OVERLAY_VIEWMODEL_H

#include "PopupViewModelBase.hh"

#include "RA_Interface.h"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayViewModel : public PopupViewModelBase {
public:
    /// <summary>
    /// Begins the animation cycle.
    /// </summary>
    void BeginAnimation() override;

    /// <summary>
    /// Updates the image to render.
    /// </summary>
    bool UpdateRenderImage(double fElapsed) override;

    /// <summary>
    /// Causes the cached render image to be rebuilt.
    /// </summary>
    void RebuildRenderImage() noexcept { m_bSurfaceStale = true; }

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
        return (m_fAnimationProgress >= INOUT_TIME);
    }

    enum class State
    {
        Hidden,
        FadeIn,
        Visible,
        FadeOut,
    };

    State CurrentState() const noexcept { return m_nState; }

    void ProcessInput(const ControllerInput& pInput);

    void Activate();
    void Deactivate();

private:

    void CreateRenderImage();

    State m_nState = State::Hidden;
    bool m_bSurfaceStale = false;

    bool m_bInputLock = false;

    double m_fAnimationProgress = -1.0;
    static _CONSTANT_VAR INOUT_TIME = 0.8;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_VIEWMODEL_H
