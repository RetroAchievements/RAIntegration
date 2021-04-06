#include "PopupMessageViewModel.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty PopupMessageViewModel::TitleProperty("PopupMessageViewModel", "Title", L"");
const StringModelProperty PopupMessageViewModel::DescriptionProperty("PopupMessageViewModel", "Description", L"");
const StringModelProperty PopupMessageViewModel::DetailProperty("PopupMessageViewModel", "Detail", L"");
const BoolModelProperty PopupMessageViewModel::IsDetailErrorProperty("PopupMessageViewModel", "IsDetailError", false);

PopupMessageViewModel::PopupMessageViewModel() noexcept
{
    GSL_SUPPRESS_F6 SetPopupType(Popup::Message);
}

void PopupMessageViewModel::BeginAnimation()
{
    m_fAnimationProgress = 0.0;

    // left margin 10px
    SetHorizontalOffset(10);

    // animate to bottom margin 10px.
    m_nTargetY = 10;

    const auto& pOverlayTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nShadowOffset = pOverlayTheme.ShadowOffset();
    constexpr int nImageSize = 64;
    const int nHeight = 4 + nImageSize + 4 + nShadowOffset;
    m_nInitialY = -nHeight;
    SetVerticalOffset(m_nInitialY);

    UpdateRenderImage(0.0);
}

bool PopupMessageViewModel::UpdateRenderImage(double fElapsed)
{
    const int nOldY = GetVerticalOffset();

    m_fAnimationProgress += fElapsed;
    const int nNewY = GetFadeOffset(m_fAnimationProgress, TOTAL_ANIMATION_TIME, INOUT_TIME, m_nInitialY, m_nTargetY);

    bool bUpdated = false;
    if (nNewY != nOldY)
    {
        SetVerticalOffset(nNewY);
        bUpdated = true;
    }

    RenderImageLock lock(*this);

    if (m_bSurfaceStale)
    {
        m_bSurfaceStale = false;
        CreateRenderImage();
        bUpdated = true;
    }
    else if (m_pSurface == nullptr)
    {
        CreateRenderImage();
        bUpdated = true;
    }
    else
    {
        const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
        if (pImageRepository.HasReferencedImageChanged(m_hImage))
        {
            CreateRenderImage();
            bUpdated = true;
        }
    }

    return bUpdated;
}

void PopupMessageViewModel::CreateRenderImage()
{
    const auto& pOverlayTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();

    // create a temporary surface so we can determine the size required for the actual surface
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pSurface = pSurfaceFactory.CreateSurface(1, 1);

    const auto nFontTitle = pSurface->LoadFont(pOverlayTheme.FontPopup(), pOverlayTheme.FontSizePopupTitle(), ra::ui::FontStyles::Normal);
    const auto sTitle = GetTitle();
    const auto szTitle = pSurface->MeasureText(nFontTitle, sTitle);

    const auto nFontSubtitle = pSurface->LoadFont(pOverlayTheme.FontPopup(), pOverlayTheme.FontSizePopupSubtitle(), ra::ui::FontStyles::Normal);
    const auto sSubTitle = GetDescription();
    const auto szSubTitle = pSurface->MeasureText(nFontSubtitle, sSubTitle);

    const auto nFontDetail = pSurface->LoadFont(pOverlayTheme.FontPopup(), pOverlayTheme.FontSizePopupDetail(), ra::ui::FontStyles::Normal);
    const auto sDetail = GetDetail();
    const auto szDetail = pSurface->MeasureText(nFontDetail, sDetail);

    const auto nShadowOffset = pOverlayTheme.ShadowOffset();

    // create the actual surface
    constexpr int nImageSize = 64;
    constexpr int nSubTitleIndent = 4;
    const int nWidth = 4 + ((m_hImage.Type() != ra::ui::ImageType::None) ? nImageSize : 0) + 6 +
        std::max(szTitle.Width, std::max(szSubTitle.Width, szDetail.Width) + nSubTitleIndent) + 6 + 4 + nShadowOffset;
    const int nHeight = 4 + nImageSize + 4 + nShadowOffset;

    pSurface = pSurfaceFactory.CreateSurface(nWidth, nHeight);

    int nX = 4;
    int nY = 4;

    // background
    pSurface->FillRectangle(0, 0, nWidth, nHeight, ra::ui::Color::Transparent);
    pSurface->FillRectangle(nShadowOffset, nShadowOffset, nWidth - nShadowOffset, nHeight - nShadowOffset, pOverlayTheme.ColorShadow());

    // frame
    const auto nColorBackground = (GetPopupType() == Popup::Mastery) ? pOverlayTheme.ColorMasteryBackground() : GetBackgroundColor();
    pSurface->FillRectangle(0, 0, nWidth - nShadowOffset, nHeight - nShadowOffset, nColorBackground);
    pSurface->FillRectangle(1, 1, nWidth - nShadowOffset - 2, nHeight - nShadowOffset - 2, pOverlayTheme.ColorBorder());
    pSurface->FillRectangle(2, 2, nWidth - nShadowOffset - 4, nHeight - nShadowOffset - 4, nColorBackground);

    // image
    if (m_hImage.Type() != ra::ui::ImageType::None)
    {
        pSurface->DrawImage(nX, nY, nImageSize, nImageSize, m_hImage);
        nX += nImageSize;
    }
    nX += 6;

    // title
    nY -= 1;
    pSurface->WriteText(nX + 2, nY + 2, nFontTitle, pOverlayTheme.ColorTextShadow(), sTitle);
    pSurface->WriteText(nX, nY, nFontTitle, pOverlayTheme.ColorTitle(), sTitle);
    nX += nSubTitleIndent;

    // subtitle
    nY += pOverlayTheme.FontSizePopupTitle() - 1;
    if (!sSubTitle.empty())
    {
        pSurface->WriteText(nX + 2, nY + 2, nFontSubtitle, pOverlayTheme.ColorTextShadow(), sSubTitle);
        pSurface->WriteText(nX, nY, nFontSubtitle, pOverlayTheme.ColorDescription(), sSubTitle);
    }

    // detail
    nY += pOverlayTheme.FontSizePopupSubtitle();
    if (!sDetail.empty())
    {
        pSurface->WriteText(nX + 2, nY + 2, nFontDetail, pOverlayTheme.ColorTextShadow(), sDetail);
        pSurface->WriteText(nX, nY, nFontDetail, IsDetailError() ? pOverlayTheme.ColorError() : pOverlayTheme.ColorDetail(), sDetail);
    }

    {
        RenderImageLock lock(*this);

        m_pSurface = std::move(pSurface);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
