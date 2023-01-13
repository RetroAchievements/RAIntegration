#ifndef RA_UI_PROGRESSTRACKER_VIEWMODEL_H
#define RA_UI_PROGRESSTRACKER_VIEWMODEL_H
#pragma once

#include "PopupViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class ProgressTrackerViewModel : public PopupViewModelBase
{
public:
    ProgressTrackerViewModel() noexcept;

    /// <summary>
    /// Sets the image to display.
    /// </summary>
    void SetImage(ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        m_hImage.ChangeReference(nImageType, sImageName);
        m_hImageGrayscale.ChangeReference(nImageType, sImageName + "_lock");
        m_pSurface.reset();
        SetRenderLocationY(-1); // force redraw when position calculated
    }

    /// <summary>
    /// Sets the percentage of the image to highlight (0.0 - 1.0)
    /// </summary>
    void SetProgress(float fProgress) noexcept { m_fProgress = fProgress; }

    /// <summary>
    /// Updates the image to render.
    /// </summary>
    bool UpdateRenderImage(double fElapsed) override;

    void BeginAnimation() noexcept override { m_fAnimationProgress = 0.0; }
    bool IsAnimationStarted() const noexcept override { return true; }

    bool IsAnimationComplete() const noexcept override 
    { 
        return m_fAnimationProgress >= TOTAL_ANIMATION_TIME;
    }

private:
    ra::ui::ImageReference m_hImage;
    ra::ui::ImageReference m_hImageGrayscale;
    float m_fProgress = 0.0;

    double m_fAnimationProgress = TOTAL_ANIMATION_TIME;
    
    static _CONSTANT_VAR TOTAL_ANIMATION_TIME = 2.0;
    static _CONSTANT_VAR OUT_TIME = 0.8;
    static _CONSTANT_VAR HOLD_TIME = TOTAL_ANIMATION_TIME - OUT_TIME;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_PROGRESSTRACKER_VIEWMODEL_H
