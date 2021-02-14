#ifndef RA_UI_CHALLENGEINDICATOR_VIEWMODEL_H
#define RA_UI_CHALLENGEINDICATOR_VIEWMODEL_H
#pragma once

#include "PopupViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class ChallengeIndicatorViewModel : public PopupViewModelBase
{
public:
    ChallengeIndicatorViewModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the title message.
    /// </summary>
    static const StringModelProperty DisplayTextProperty;

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
    /// Updates the image to render.
    /// </summary>
    bool UpdateRenderImage(_UNUSED double fElapsed) override;

    void BeginAnimation() noexcept override {}
    bool IsAnimationStarted() const noexcept override { return true; }
    bool IsAnimationComplete() const noexcept override { return false; }

private:
    ra::ui::ImageReference m_hImage;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_CHALLENGEINDICATOR_VIEWMODEL_H
