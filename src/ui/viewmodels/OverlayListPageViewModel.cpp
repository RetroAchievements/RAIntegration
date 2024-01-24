#include "OverlayListPageViewModel.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "services\IThreadPool.hh"

#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const BoolModelProperty OverlayListPageViewModel::CanCollapseHeadersProperty("OverlayListPageViewModel", "CanCollapseHeadersProperty", true);
const IntModelProperty OverlayListPageViewModel::SelectedItemIndexProperty("OverlayListPageViewModel", "SelectedItemIndex", 0);

const StringModelProperty OverlayListPageViewModel::ItemViewModel::DetailProperty("ItemViewModel", "Detail", L"");
const BoolModelProperty OverlayListPageViewModel::ItemViewModel::IsDisabledProperty("ItemViewModel", "IsDisabled", false);
const BoolModelProperty OverlayListPageViewModel::ItemViewModel::IsCollapsedProperty("ItemViewModel", "IsCollapsed", false);
const StringModelProperty OverlayListPageViewModel::ItemViewModel::ProgressValueProperty("ItemViewModel", "ProgressValue", L"");
const IntModelProperty OverlayListPageViewModel::ItemViewModel::ProgressPercentageProperty("ItemViewModel", "IsProgressPercentage", 0);

void OverlayListPageViewModel::Refresh()
{
    m_bDetail = false;
    m_nImagesPending = 99;
}

bool OverlayListPageViewModel::AssetAppearsInFilter(const ra::data::models::AssetModelBase& pAsset)
{
    const auto nType = pAsset.GetType();
    const auto nId = ra::to_signed(pAsset.GetID());

    const auto& pAssetList = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().AssetList;
    const auto& vFilteredAssets = pAssetList.FilteredAssets();
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(vFilteredAssets.Count()); ++nIndex)
    {
        const auto* vmAsset = vFilteredAssets.GetItemAt(nIndex);
        if (vmAsset && vmAsset->GetId() == nId && vmAsset->GetType() == nType)
            return true;
    }

    return false;
}

OverlayListPageViewModel::ItemViewModel& OverlayListPageViewModel::GetNextItem(size_t* nIndex)
{
    Expects(nIndex != nullptr);

    ItemViewModel* pvmItem = m_vItems.GetItemAt((*nIndex)++);
    if (pvmItem == nullptr)
    {
        pvmItem = &m_vItems.Add();
        Ensures(pvmItem != nullptr);
    }

    return *pvmItem;
}

void OverlayListPageViewModel::SetHeader(OverlayListPageViewModel::ItemViewModel& vmItem, const std::wstring& sHeader)
{
    vmItem.SetId(0);
    vmItem.SetLabel(sHeader);
    vmItem.SetDetail(L"");
    vmItem.SetDisabled(false);
    vmItem.Image.ChangeReference(ra::ui::ImageType::None, "");
    vmItem.SetProgressString(L"");
    vmItem.SetProgressPercentage(0);
}

void OverlayListPageViewModel::EnsureSelectedItemIndexValid()
{
    auto nSelectedIndex = GetSelectedItemIndex();
    const auto* vmItem = m_vItems.GetItemAt(nSelectedIndex);

    if (!vmItem)
    {
        nSelectedIndex = 0;
        m_nScrollOffset = 0;
    }

    if (nSelectedIndex < ra::to_signed(m_vItems.Count()))
        SetSelectedItemIndex(nSelectedIndex);
}

bool OverlayListPageViewModel::Update(double fElapsed)
{
    m_fElapsed += fElapsed;

    bool bUpdated = m_bRedraw;
    m_bRedraw = false;

    if (m_nImagesPending)
    {
        const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();

        unsigned int nImagesPending = 0;
        for (gsl::index nIndex = m_vItems.Count() - 1; nIndex >= 0; --nIndex)
        {
            const auto* pItem = m_vItems.GetItemAt(nIndex);
            if (pItem && pItem->Image.Type() != ra::ui::ImageType::None &&
                !pImageRepository.IsImageAvailable(pItem->Image.Type(), pItem->Image.Name()))
            {
                ++nImagesPending;
            }
        }

        if (m_nImagesPending != nImagesPending)
        {
            m_nImagesPending = nImagesPending;
            bUpdated = true;
        }
    }

    return bUpdated;
}

std::unique_ptr<ra::ui::drawing::ISurface> OverlayListPageViewModel::LoadDecorator(
     const uint8_t* pixels, int nWidth, int nHeight)
{
    uint32_t* argb_pixels = static_cast<uint32_t*>(malloc(gsl::narrow_cast<size_t>(nWidth) * nHeight * sizeof(uint32_t)));
    if (argb_pixels == nullptr)
        return nullptr;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    ra::ui::Color nTextColor = pTheme.ColorOverlayDisabledText();

    auto& pSurfaceFactory = ra::services::ServiceLocator::GetMutable<ra::ui::drawing::ISurfaceFactory>();
    auto pDecorator = pSurfaceFactory.CreateTransparentSurface(nWidth, nHeight);
    GSL_SUPPRESS_F23 uint32_t* dst = argb_pixels;
    GSL_SUPPRESS_F23 const uint8_t* src = pixels;
    const uint8_t* stop = pixels + gsl::narrow_cast<size_t>(nWidth) * nHeight;

    while (src < stop)
    {
        nTextColor.Channel.A = *src++;

        if (nTextColor.Channel.A == 0)
            *dst++ = 0;
        else
            *dst++ = nTextColor.ARGB;
    }

    pDecorator->SetPixels(0, 0, nWidth, nHeight, argb_pixels);
    free(argb_pixels);

    return pDecorator;
}


void OverlayListPageViewModel::Render(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    if (m_bDetail)
        RenderDetail(pSurface, nX, nY, nWidth, nHeight);
    else
        RenderList(pSurface, nX, nY, nWidth, nHeight);
}

void OverlayListPageViewModel::RenderList(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayHeader(), ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);

    // scrollbar
    const auto nSelectedIndex = ra::to_unsigned(GetSelectedItemIndex());
    gsl::index nIndex = m_nScrollOffset;
    constexpr auto nItemSize = 64;
    constexpr auto nItemSpacing = 8;
    m_nVisibleItems = (nHeight + nItemSpacing) / (nItemSize + nItemSpacing);

    if (m_nVisibleItems < m_vItems.Count())
    {
        pSurface.FillRectangle(nX, nY, 12, nHeight, pTheme.ColorOverlayScrollBar());

        const auto nItemHeight = (static_cast<double>(nHeight) - 4) / m_vItems.Count();
        const auto nGripperTop = ra::ftoi(nItemHeight * nIndex);
        const auto nGripperBottom = ra::ftoi(nItemHeight * (static_cast<double>(nIndex) + m_nVisibleItems));
        pSurface.FillRectangle(nX + 2, nY + 2 + nGripperTop, 8, nGripperBottom - nGripperTop, pTheme.ColorOverlayScrollBarGripper());

        nX += 20;
    }
    else
    {
        // fewer items than fit on screen, force scroll offset to 0.
        nIndex = 0;
    }

    // items list
    const bool bCanCollapseHeaders = GetCanCollapseHeaders();
    while (nHeight >= nItemSize)
    {
        const auto* pItem = m_vItems.GetItemAt(nIndex);
        if (!pItem)
            break;

        auto nTextX = nX;
        if (pItem->Image.Type() != ra::ui::ImageType::None)
        {
            pSurface.DrawImage(nX, nY, nItemSize, nItemSize, pItem->Image);
            nTextX += nItemSize;
        }

        const ra::ui::Color nSubTextColor =
            pItem->IsDisabled() ? pTheme.ColorOverlayDisabledSubText() : pTheme.ColorOverlaySubText();
        ra::ui::Color nTextColor = pTheme.ColorOverlayText();

        const bool bSelected = (nIndex == ra::to_signed(nSelectedIndex));
        if (bSelected)
        {
            pSurface.FillRectangle(nTextX, nY, nWidth - nTextX, nItemSize, pTheme.ColorOverlaySelectionBackground());
            nTextColor = pItem->IsDisabled() ?
                pTheme.ColorOverlaySelectionDisabledText() : pTheme.ColorOverlaySelectionText();
        }
        else if (pItem->IsDisabled())
        {
            nTextColor = pTheme.ColorOverlayDisabledText();
        }

        if (pItem->IsHeader())
        {
            const auto szHeaderText = pSurface.MeasureText(nFont, pItem->GetLabel());

            pSurface.FillRectangle(nTextX + 2, nY + nItemSize - 6, nWidth - nTextX - 4, 1, nTextColor);
            pSurface.WriteText(nTextX + 4, nY + nItemSize - 6 - szHeaderText.Height, nFont, nTextColor, pItem->GetLabel());

            if (bCanCollapseHeaders)
            {
                pSurface.WriteText(nWidth - 26, nY + nItemSize - 6 - szHeaderText.Height, nFont, nTextColor,
                                   pItem->IsCollapsed() ? L"\x25BC" : L"\x25B2");
            }
        }
        else
        {
            pSurface.WriteText(nTextX + 12, nY + 1, nFont, nTextColor, pItem->GetLabel());
            pSurface.WriteText(nTextX + 12, nY + 1 + 26, nSubFont, nSubTextColor, pItem->GetDetail());

            const auto& sProgress = pItem->GetProgressString();
            if (!sProgress.empty())
            {
                const auto nProgressBarWidth = (nWidth - nTextX - 12) * 2 / 3;

                const auto nValue = pItem->GetProgressPercentage();
                if (nValue >= 0.0)
                {
                    const auto nProgressBarFillWidth = gsl::narrow_cast<int>((nProgressBarWidth - 2) * nValue);

                    pSurface.FillRectangle(nTextX + 12, nY + 1 + 26 + 25, nProgressBarWidth, 8, pTheme.ColorOverlayScrollBar());
                    pSurface.FillRectangle(nTextX + 12 + 2, nY + 1 + 26 + 25 + 2, nProgressBarFillWidth - 4, 8 - 4, pTheme.ColorOverlayScrollBarGripper());
                }

                const auto nProgressFont = pSurface.LoadFont(pTheme.FontOverlay(), 14, ra::ui::FontStyles::Normal);
                const auto szProgress = pSurface.MeasureText(nProgressFont, sProgress);
                pSurface.WriteText(nTextX + 12 + nProgressBarWidth + 6, nY + 1 + 26 + 25 + 4 - (szProgress.Height / 2) - 1, nProgressFont, nSubTextColor, sProgress);
            }

            const auto* pDecorator = pItem->GetDecorator();
            if (pDecorator != nullptr)
                pSurface.DrawSurface(nWidth - 5 - pDecorator->GetWidth(), nY + 6, *pDecorator);
        }

        nIndex++;
        nY += nItemSize + nItemSpacing;
        nHeight -= nItemSize + nItemSpacing;
    }
}

const wchar_t* OverlayListPageViewModel::GetAcceptButtonText() const
{
    if (m_bDetail)
        return L"";

    if (GetCanCollapseHeaders())
    {
        const auto* vmItem = m_vItems.GetItemAt(GetSelectedItemIndex());
        if (vmItem && vmItem->IsHeader())
            return vmItem->IsCollapsed() ? L"Expand" : L"Collapse";
    }

    return PageViewModel::GetAcceptButtonText();
}

const wchar_t* OverlayListPageViewModel::GetCancelButtonText() const
{
    if (m_bDetail)
        return L"Back";

    return PageViewModel::GetCancelButtonText();
}

const wchar_t* OverlayListPageViewModel::GetPrevButtonText() const
{
    if (m_bDetail)
        return L"";

    return PageViewModel::GetPrevButtonText();
}

const wchar_t* OverlayListPageViewModel::GetNextButtonText() const
{
    if (m_bDetail)
        return L"";

    return PageViewModel::GetNextButtonText();
}

bool OverlayListPageViewModel::ProcessInput(const ControllerInput& pInput)
{
    if (pInput.m_bDownPressed)
    {
        size_t nSelectedItemIndex = gsl::narrow_cast<size_t>(GetSelectedItemIndex());
        if (nSelectedItemIndex < m_vItems.Count() - 1)
        {
            ItemViewModel* vmItem = nullptr;
            do
            {
                ++nSelectedItemIndex;
                vmItem = m_vItems.GetItemAt(nSelectedItemIndex);
                if (!vmItem)
                    return false;
            } while (m_bDetail && vmItem->IsHeader());

            SetSelectedItemIndex(gsl::narrow_cast<int>(nSelectedItemIndex));

            if (nSelectedItemIndex - m_nScrollOffset >= gsl::narrow_cast<size_t>(m_nVisibleItems))
                m_nScrollOffset = nSelectedItemIndex - m_nVisibleItems + 1;

            if (m_bDetail)
                FetchItemDetail(*vmItem);

            return true;
        }
    }
    else if (pInput.m_bUpPressed)
    {
        auto nSelectedItemIndex = GetSelectedItemIndex();
        if (nSelectedItemIndex > 0)
        {
            ItemViewModel* vmItem = nullptr;
            do
            {
                vmItem = m_vItems.GetItemAt(gsl::narrow_cast<size_t>(--nSelectedItemIndex));
                const auto nScrollOfffset = m_nScrollOffset;
                if (nSelectedItemIndex < m_nScrollOffset)
                    m_nScrollOffset = std::max(nSelectedItemIndex, 0);

                if (!vmItem)
                    return (m_nScrollOffset != nScrollOfffset);
            } while (m_bDetail && vmItem->IsHeader());

            SetSelectedItemIndex(nSelectedItemIndex);

            if (m_bDetail)
                FetchItemDetail(*vmItem);

            return true;
        }
    }
    else if (m_bDetail)
    {
        if (pInput.m_bCancelPressed)
            return SetDetail(false);

        // swallow all other buttons on the detail page
        return true;
    }
    else if (pInput.m_bConfirmPressed)
    {
        return SetDetail(true);
    }

    return false;
}

bool OverlayListPageViewModel::SetDetail(bool bDetail)
{
    if (bDetail)
    {
        if (!m_bDetail)
        {
            auto* vmItem = m_vItems.GetItemAt(GetSelectedItemIndex());
            if (vmItem != nullptr)
            {
                if (vmItem->IsHeader())
                    return OnHeaderClicked(*vmItem);

                if (m_bHasDetail)
                {
                    FetchItemDetail(*vmItem);
                    m_bDetail = true;
                    return true;
                }
            }
        }
    }
    else
    {
        if (m_bDetail)
        {
            m_bDetail = false;

            return true;
        }
    }

    return false;
}

void OverlayListPageViewModel::ForceRedraw()
{
    m_bRedraw = true;
    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().RequestRender();
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
