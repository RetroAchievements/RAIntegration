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
        m_pSurface.reset();
        SetRenderLocationY(-1); // force redraw when position calculated
    }

    /// <summary>
    /// Sets the percentage of the image to highlight
    /// </summary>
    void SetProgress(unsigned nValue, unsigned nTarget, bool bAsPercent);

    /// <summary>
    /// Gets the image to display.
    /// </summary>
    const ra::ui::ImageReference& GetImage() const noexcept { return m_hImage; }

    /// <summary>
    /// Gets the text to display.
    /// </summary>
    const std::wstring& GetText() const noexcept { return m_sProgress; }

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
    std::wstring m_sProgress;

    double m_fAnimationProgress = TOTAL_ANIMATION_TIME;

    static _CONSTANT_VAR TOTAL_ANIMATION_TIME = 2.0;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_PROGRESSTRACKER_VIEWMODEL_H
