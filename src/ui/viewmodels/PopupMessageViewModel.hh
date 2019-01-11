#ifndef RA_UI_POPUPMESSAGE_VIEWMODEL_H
#define RA_UI_POPUPMESSAGE_VIEWMODEL_H
#pragma once

#include "PopupViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class PopupMessageViewModel : public PopupViewModelBase
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for the title message.
    /// </summary>
    static const StringModelProperty TitleProperty;

    /// <summary>
    /// Gets the title message to display.
    /// </summary>
    const std::wstring& GetTitle() const { return GetValue(TitleProperty); }

    /// <summary>
    /// Sets the title message to display, empty string for no title message.
    /// </summary>
    void SetTitle(const std::wstring& sValue) { SetValue(TitleProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the description.
    /// </summary>
    static const StringModelProperty DescriptionProperty;

    /// <summary>
    /// Gets the description to display.
    /// </summary>
    const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

    /// <summary>
    /// Sets the description to display.
    /// </summary>
    void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the detail message.
    /// </summary>
    static const StringModelProperty DetailProperty;

    /// <summary>
    /// Gets the detail message to display.
    /// </summary>
    const std::wstring& GetDetail() const { return GetValue(DetailProperty); }

    /// <summary>
    /// Sets the detail message to display.
    /// </summary>
    void SetDetail(const std::wstring& sValue) { SetValue(DetailProperty, sValue); }
    
    /// <summary>
    /// Sets the image to display.
    /// </summary>
    void SetImage(ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        m_hImage.ChangeReference(nImageType, sImageName);
    }

    /// <summary>
    /// Gets the image to display.
    /// </summary>
    const ra::ui::ImageReference& GetImage() const noexcept { return m_hImage; }
    
    /// <summary>
    /// Begins the animation cycle.
    /// </summary>
    void BeginAnimation()
    {
        m_fAnimationProgress = 0.0;

        // left margin 10px
        SetRenderLocationX(10);

        // animate to bottom margin 10px. assume height = 64+2
        m_nInitialY = 0;
        m_nTargetY = 10 + 64 + 2;
        SetRenderLocationY(m_nInitialY);
        SetRenderLocationYRelativePosition(RelativePosition::Far);
    }
    
    /// <summary>
    /// Updates the image to render.
    /// </summary>
    void UpdateRenderImage(double fElapsed) override;
    
    /// <summary>
    /// Causes the cached render image to be rebuilt.
    /// </summary>
    void RebuildRenderImage() noexcept { m_bSurfaceStale = true; }

    /// <summary>
    /// Determines whether the animation cycle has started.
    /// </summary>
    bool IsAnimationStarted() const noexcept
    {
        return (m_fAnimationProgress >= 0.0);
    }

    /// <summary>
    /// Determines whether the animation cycle has completed.
    /// </summary>
    bool IsAnimationComplete() const noexcept
    {
        return (m_fAnimationProgress >= TOTAL_ANIMATION_TIME);
    }

private:
    void CreateRenderImage();
    bool m_bSurfaceStale = false;

    double m_fAnimationProgress = -1.0;
    int m_nInitialY = 0;
    int m_nTargetY = 0;

    static _CONSTANT_VAR TOTAL_ANIMATION_TIME = 5.0;
    static _CONSTANT_VAR INOUT_TIME = 0.8;
    static _CONSTANT_VAR HOLD_TIME = TOTAL_ANIMATION_TIME - INOUT_TIME * 2;

    ra::ui::ImageReference m_hImage;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POPUPMESSAGE_VIEWMODEL_H
