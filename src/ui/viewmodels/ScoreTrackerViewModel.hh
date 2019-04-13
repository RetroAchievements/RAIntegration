#ifndef RA_UI_SCORETRACKER_VIEWMODEL_H
#define RA_UI_SCORETRACKER_VIEWMODEL_H
#pragma once

#include "PopupViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class ScoreTrackerViewModel : public PopupViewModelBase
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for the title message.
    /// </summary>
    static const StringModelProperty DisplayTextProperty;

    /// <summary>
    /// Gets the title message to display.
    /// </summary>
    const std::wstring& GetDisplayText() const { return GetValue(DisplayTextProperty); }

    /// <summary>
    /// Sets the title message to display, empty string for no title message.
    /// </summary>
    void SetDisplayText(const std::wstring& sValue);

    /// <summary>
    /// Updates the image to render.
    /// </summary>
    bool UpdateRenderImage(_UNUSED double fElapsed) override;

    void BeginAnimation() noexcept override {}
    bool IsAnimationStarted() const noexcept override { return true; }
    bool IsAnimationComplete() const noexcept override { return false; }

    bool SetOffset(int nValue) noexcept
    {
        if (m_nOffset == nValue)
            return false;

        m_nOffset = nValue;
        return true;
    }

private:
    bool m_bSurfaceStale = false;

    int m_nOffset = 10;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_SCORETRACKER_VIEWMODEL_H
