#include "ProgressTrackerViewModel.hh"

#include "ra_math.h"

#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

ProgressTrackerViewModel::ProgressTrackerViewModel() noexcept
{
    GSL_SUPPRESS_F6 SetPopupType(ra::ui::viewmodels::Popup::Progress);

    GSL_SUPPRESS_F6 SetHorizontalOffset(10);
    GSL_SUPPRESS_F6 SetVerticalOffset(10);
}

void ProgressTrackerViewModel::SetProgress(unsigned nValue, unsigned nTarget, bool bAsPercent)
{
    if (bAsPercent)
    {
        const auto nProgressPercent = gsl::narrow_cast<int>(static_cast<long long>(nValue) * 100 / nTarget);
        m_sProgress = ra::StringPrintf(L"%d%%", nProgressPercent);
    }
    else
    {
        m_sProgress = ra::StringPrintf(L"%u/%u", nValue, nTarget);
    }
}

bool ProgressTrackerViewModel::UpdateRenderImage(double fElapsed)
{
    if (m_fAnimationProgress >= TOTAL_ANIMATION_TIME)
        return false;

    m_fAnimationProgress += fElapsed;

    bool bUpdated = false;

    if (m_pSurface == nullptr)
    {
        const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
        const auto nShadowOffset = pTheme.ShadowOffset();
        constexpr int nImageSize = 32;

        // create a temporary surface so we can determine the size required for the actual surface
        const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
        auto pSurface = pSurfaceFactory.CreateSurface(1, 1);

        const auto& pOverlayTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();

        const auto nFontSubtitle = pSurface->LoadFont(pOverlayTheme.FontPopup(), pOverlayTheme.FontSizePopupDetail(), ra::ui::FontStyles::Normal);
        const auto szText = pSurface->MeasureText(nFontSubtitle, m_sProgress);

        const auto nWidth = 4 + nImageSize + 6 + szText.Width + nShadowOffset + 4 + nShadowOffset;
        const auto nHeight = 4 + nImageSize + 4 + nShadowOffset;

        pSurface = pSurfaceFactory.CreateSurface(nWidth, nHeight);

        // background
        pSurface->FillRectangle(0, 0, nWidth, nHeight, Color::Transparent);
        pSurface->FillRectangle(nShadowOffset, nShadowOffset, nWidth - nShadowOffset, nHeight - nShadowOffset, pTheme.ColorShadow());

        // frame
        const auto nColorBackground = GetBackgroundColor();
        pSurface->FillRectangle(0, 0, nWidth - nShadowOffset, nHeight - nShadowOffset, nColorBackground);
        pSurface->FillRectangle(1, 1, nWidth - nShadowOffset - 2, nHeight - nShadowOffset - 2, pOverlayTheme.ColorBorder());
        pSurface->FillRectangle(2, 2, nWidth - nShadowOffset - 4, nHeight - nShadowOffset - 4, nColorBackground);

        // image
        if (m_hImage.Type() != ra::ui::ImageType::None)
            pSurface->DrawImageStretched(4, 4, nImageSize, nImageSize, m_hImage);

        // text
        const auto constexpr nX = 4 + nImageSize + 6;
        const auto nY = (nHeight - nShadowOffset - szText.Height - nShadowOffset) / 2;
        pSurface->WriteText(nX + 2, nY + 2, nFontSubtitle, pOverlayTheme.ColorTextShadow(), m_sProgress);
        pSurface->WriteText(nX, nY, nFontSubtitle, pOverlayTheme.ColorDetail(), m_sProgress);

        m_pSurface = std::move(pSurface);

        bUpdated = true;
    }

    return bUpdated;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
