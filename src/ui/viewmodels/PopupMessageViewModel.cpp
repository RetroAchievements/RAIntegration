#include "PopupMessageViewModel.hh"

#include "ra_math.h"

namespace ra {
namespace ui {
namespace viewmodels {

static _CONSTANT_VAR FONT_TO_USE = "Tahoma";
static _CONSTANT_VAR FONT_SIZE_TITLE = 32;
static _CONSTANT_VAR FONT_SIZE_SUBTITLE = 28;

const StringModelProperty PopupMessageViewModel::TitleProperty("PopupMessageViewModel", "Title", L"");
const StringModelProperty PopupMessageViewModel::DescriptionProperty("PopupMessageViewModel", "Description", L"");
const StringModelProperty PopupMessageViewModel::DetailProperty("PopupMessageViewModel", "Detail", L"");

void PopupMessageViewModel::UpdateRenderImage(double fElapsed)
{
    m_fAnimationProgress += fElapsed;

    if (m_fAnimationProgress < INOUT_TIME)
    {
        // fading in
        const auto fPercentage = (INOUT_TIME - m_fAnimationProgress) / INOUT_TIME;
        Expects(m_nTargetY > m_nInitialY);
        const auto nY = to_unsigned(m_nTargetY - m_nInitialY) * (fPercentage * fPercentage);
        SetRenderLocationY(ftol(m_nTargetY - nY));
    }
    else if (m_fAnimationProgress < TOTAL_ANIMATION_TIME - INOUT_TIME)
    {
        // faded in - hold position
        SetRenderLocationY(m_nTargetY);
    }
    else if (m_fAnimationProgress < TOTAL_ANIMATION_TIME)
    {
        // fading out
        const auto fPercentage = (TOTAL_ANIMATION_TIME - INOUT_TIME - m_fAnimationProgress) / INOUT_TIME;
        Expects(m_nTargetY > m_nInitialY);
        const auto nY = to_unsigned(m_nTargetY - m_nInitialY) * (fPercentage * fPercentage);
        SetRenderLocationY(ftoi(m_nTargetY - nY));
    }
    else
    {
        // faded out
        SetRenderLocationY(m_nInitialY);
    }

    if (m_bSurfaceStale)
    {
        m_bSurfaceStale = false;
        CreateRenderImage();
    }
    else if (m_pSurface == nullptr)
    {
        CreateRenderImage();
    }
    else
    {
        const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
        if (pImageRepository.HasReferencedImageChanged(m_hImage))
            CreateRenderImage();
    }
}

void PopupMessageViewModel::CreateRenderImage()
{
    // create a temporary surface so we can determine the size required for the actual surface
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pSurface = pSurfaceFactory.CreateSurface(1, 1);

    auto nFontTitle = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
    const auto sTitle = GetTitle();
    const auto szTitle = pSurface->MeasureText(nFontTitle, sTitle);

    auto nFontSubtitle = pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_SUBTITLE, ra::ui::FontStyles::Normal);
    const auto sSubTitle = GetDescription();
    const auto szSubTitle = pSurface->MeasureText(nFontSubtitle, sSubTitle);

    // create the actual surface
    const int nWidth = 64 + 6 + std::max(szTitle.Width, szSubTitle.Width) + 8 + 2;
    constexpr int nHeight = 64 + 2;

    pSurface = pSurfaceFactory.CreateTransparentSurface(nWidth, nHeight);
    m_pSurface = std::move(pSurface);

    int nX = 0;
    int nY = 0;
    const ra::ui::Color nColorBlack(0, 0, 0);
    const ra::ui::Color nColorPopup(251, 102, 0);
    const ra::ui::Color nColorBackground(0, 255, 0, 255);
    constexpr int nShadowOffset = 2;

    // background
    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), nColorBackground);

    // image
    if (m_hImage.Type() != ra::ui::ImageType::None)
    {
        m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, 64, 64, nColorBlack);
        m_pSurface->DrawImage(nX, nY, 64, 64, m_hImage);
        nX += 64 + 6;
    }

    // title
    m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szTitle.Width + 8, szTitle.Height, nColorBlack);
    m_pSurface->FillRectangle(nX, nY, szTitle.Width + 8, szTitle.Height, nColorPopup);
    m_pSurface->WriteText(nX + 4, nY - 1, nFontTitle, nColorBlack, sTitle);

    // subtitle
    if (!sSubTitle.empty())
    {
        nY += 32 + 2;
        m_pSurface->FillRectangle(nX + nShadowOffset, nY + nShadowOffset, szSubTitle.Width + 8, szSubTitle.Height, nColorBlack);
        m_pSurface->FillRectangle(nX, nY, szSubTitle.Width + 8, szSubTitle.Height, nColorPopup);
        m_pSurface->WriteText(nX + 4, nY - 1, nFontSubtitle, nColorBlack, sSubTitle);
    }

    m_pSurface->SetOpacity(0.85);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
