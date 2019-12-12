#include "OverlayListPageViewModel.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "services\IThreadPool.hh"

#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayListPageViewModel::ListTitleProperty("OverlayListPageViewModel", "ListTitle", L"");
const StringModelProperty OverlayListPageViewModel::SummaryProperty("OverlayListPageViewModel", "Summary", L"");
const IntModelProperty OverlayListPageViewModel::SelectedItemIndexProperty("OverlayListPageViewModel", "SelectedItemIndex", 0);

const StringModelProperty OverlayListPageViewModel::ItemViewModel::DetailProperty("ItemViewModel", "Detail", L"");
const BoolModelProperty OverlayListPageViewModel::ItemViewModel::IsDisabledProperty("ItemViewModel", "IsDisabled", false);
const IntModelProperty OverlayListPageViewModel::ItemViewModel::ProgressMaximumProperty("ItemViewModel", "ProgressMaximum", 0);
const IntModelProperty OverlayListPageViewModel::ItemViewModel::ProgressValueProperty("ItemViewModel", "ProgressValue", 0);

void OverlayListPageViewModel::Refresh()
{
    m_bDetail = false;
    SetTitle(m_sTitle);
    m_nImagesPending = 99;
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

    // title
    const auto& sGameTitle = GetListTitle();
    if (!sGameTitle.empty())
    {
        const auto nTitleFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayTitle(), ra::ui::FontStyles::Normal);
        pSurface.WriteText(nX, nY, nTitleFont, pTheme.ColorOverlayText(), sGameTitle);
        nY += 34;
        nHeight -= 34;
    }

    // subtitle
    const auto nFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayHeader(), ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);
    const auto& sSummary = GetSummary();
    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), sSummary);
    nY += 30;
    nHeight -= 30;

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

    // items list
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

        const ra::ui::Color nSubTextColor = pItem->IsDisabled() ? pTheme.ColorOverlayDisabledSubText() : pTheme.ColorOverlaySubText();
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

        pSurface.WriteText(nTextX + 12, nY + 1, nFont, nTextColor, pItem->GetLabel());
        pSurface.WriteText(nTextX + 12, nY + 1 + 26, nSubFont, nSubTextColor, pItem->GetDetail());

        const auto nTarget = pItem->GetProgressMaximum();
        if (nTarget != 0)
        {
            const auto nProgressBarWidth = (nWidth - nTextX - 12) * 2 / 3;
            const auto nValue = std::max(pItem->GetProgressValue(), nTarget);
            const auto nProgressBarFillWidth = gsl::narrow_cast<int>(((static_cast<long>(nProgressBarWidth) - 2) * nValue) / nTarget);
            const auto nProgressBarPercent = gsl::narrow_cast<int>(static_cast<long>(nValue) * 100 / nTarget);

            pSurface.FillRectangle(nTextX + 12, nY + 1 + 26 + 25, nProgressBarWidth, 8, pTheme.ColorOverlayScrollBar());
            pSurface.FillRectangle(nTextX + 12 + 2, nY + 1 + 26 + 25 + 2, nProgressBarFillWidth - 4, 8 - 4, pTheme.ColorOverlayScrollBarGripper());

            const auto nProgressFont = pSurface.LoadFont(pTheme.FontOverlay(), 14, ra::ui::FontStyles::Normal);
            const std::wstring sProgressPercent = ra::StringPrintf(L"%d%%", nProgressBarPercent);
            const auto szProgressPercent = pSurface.MeasureText(nProgressFont, sProgressPercent);
            pSurface.WriteText(nTextX + 12 + nProgressBarWidth + 6, nY + 1 + 26 + 25 + 4 - (szProgressPercent.Height / 2) - 1, nProgressFont, nSubTextColor, sProgressPercent);
        }

        nIndex++;
        nY += nItemSize + nItemSpacing;
        nHeight -= nItemSize + nItemSpacing;
    }
}

bool OverlayListPageViewModel::ProcessInput(const ControllerInput& pInput)
{
    if (pInput.m_bDownPressed)
    {
        const auto nSelectedItemIndex = GetSelectedItemIndex();
        if (nSelectedItemIndex < ra::to_signed(m_vItems.Count()) - 1)
        {
            SetSelectedItemIndex(nSelectedItemIndex + 1);

            if (gsl::narrow_cast<size_t>(nSelectedItemIndex) + 1 - m_nScrollOffset == gsl::narrow_cast<size_t>(m_nVisibleItems))
                ++m_nScrollOffset;

            if (m_bDetail)
            {
                auto* vmItem = m_vItems.GetItemAt(gsl::narrow_cast<size_t>(nSelectedItemIndex) + 1);
                if (vmItem != nullptr)
                    FetchItemDetail(*vmItem);
            }

            return true;
        }
    }
    else if (pInput.m_bUpPressed)
    {
        const auto nSelectedItemIndex = GetSelectedItemIndex();
        if (nSelectedItemIndex > 0)
        {
            SetSelectedItemIndex(nSelectedItemIndex - 1);

            if (nSelectedItemIndex == m_nScrollOffset)
                --m_nScrollOffset;

            if (m_bDetail)
            {
                auto* vmItem = m_vItems.GetItemAt(gsl::narrow_cast<size_t>(nSelectedItemIndex) - 1);
                if (vmItem != nullptr)
                    FetchItemDetail(*vmItem);
            }

            return true;
        }
    }
    else if (pInput.m_bConfirmPressed)
    {
        return SetDetail(true);
    }
    else if (pInput.m_bCancelPressed)
    {
        return SetDetail(false);
    }

    return false;
}

bool OverlayListPageViewModel::SetDetail(bool bDetail)
{
    if (bDetail)
    {
        if (!m_bDetail && !m_sDetailTitle.empty())
        {
            auto* vmItem = m_vItems.GetItemAt(GetSelectedItemIndex());
            if (vmItem != nullptr)
            {
                FetchItemDetail(*vmItem);
                m_bDetail = true;
                SetTitle(m_sDetailTitle);
                return true;
            }
        }
    }
    else
    {
        if (m_bDetail)
        {
            m_bDetail = false;
            SetTitle(m_sTitle);
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
