#ifndef RA_UI_POPUPVIEWMODELBASE_H
#define RA_UI_POPUPVIEWMODELBASE_H
#pragma once

#include "ui\WindowViewModelBase.hh"
#include "ui\drawing\ISurface.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class PopupViewModelBase : public ViewModelBase
{
public:
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
    /// The <see cref="ModelProperty" /> for the parent relative X location to render the popup at.
    /// </summary>
    static const IntModelProperty RenderLocationXRelativePositionProperty;

    /// <summary>
    /// Gets the parent relative X location to render the popup at.
    /// </summary>
    RelativePosition GetRenderLocationXRelativePosition() const
    {
        return ra::itoe<RelativePosition>(GetValue(RenderLocationXRelativePositionProperty));
    }

    /// <summary>
    /// Sets the parent relative X location to render the popup at.
    /// </summary>
    void SetRenderLocationXRelativePosition(RelativePosition nValue)
    {
        SetValue(RenderLocationXRelativePositionProperty, ra::etoi(nValue));
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the Y location to render the popup at.
    /// </summary>
    static const IntModelProperty RenderLocationYProperty;

    /// <summary>
    /// Gets the Y location to render the popup at.
    /// </summary>
    int GetRenderLocationY() const { return GetValue(RenderLocationYProperty); }

    /// <summary>
    /// Sets the Y location to render the popup at.
    /// </summary>
    void SetRenderLocationY(int nValue) { SetValue(RenderLocationYProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the parent relative Y location to render the popup at.
    /// </summary>
    static const IntModelProperty RenderLocationYRelativePositionProperty;

    /// <summary>
    /// Gets the parent relative Y location to render the popup at.
    /// </summary>
    RelativePosition GetRenderLocationYRelativePosition() const
    {
        return ra::itoe<RelativePosition>(GetValue(RenderLocationYRelativePositionProperty));
    }

    /// <summary>
    /// Sets the parent relative Y location to render the popup at.
    /// </summary>
    void SetRenderLocationYRelativePosition(RelativePosition nValue)
    {
        SetValue(RenderLocationYRelativePositionProperty, ra::etoi(nValue));
    }

    /// <summary>
    /// Gets the image to render.
    /// </summary>
    const ra::ui::drawing::ISurface& GetRenderImage() const { return *m_pSurface; }

    /// <summary>
    /// Updates the image to render.
    /// </summary>
    virtual void UpdateRenderImage(double fElapsed) = 0;
    
    /// <summary>
    /// Gets the unique identifier of the popup.
    /// </summary>
    int GetPopupId() const noexcept { return m_nPopupId; }

    /// <summary>
    /// Sets the unique identifier of the popup.
    /// </summary>
    void SetPopupId(int nPopupId) noexcept { m_nPopupId = nPopupId; }

protected:
    std::unique_ptr<ra::ui::drawing::ISurface> m_pSurface;
    int m_nPopupId{ 0 };
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POPUPVIEWMODELBASE_H
