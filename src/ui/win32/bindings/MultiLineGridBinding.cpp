#include "MultiLineGridBinding.hh"

#include "GridCheckBoxColumnBinding.hh"
#include "GridTextColumnBinding.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

static bool SupportsMultipleLines(const GridColumnBinding* pColumn) noexcept
{
    return (dynamic_cast<const GridTextColumnBinding*>(pColumn) != nullptr);
}

gsl::index MultiLineGridBinding::GetIndexForLine(gsl::index nLine) const
{
    // minor optimization: m_nScrollOffset is the number of lines that are not-visible
    // if they're all single-line, it'll also be the first visible item. if some are multi-line,
    // it allows us to start the scan closer to the data we actually care about.
    gsl::index nIndex = m_nScrollOffset;
    while (nIndex < ra::to_signed(m_vItemMetrics.size()))
    {
        const auto& pItemMetrics = m_vItemMetrics.at(nIndex);
        if (pItemMetrics.nFirstLine > ra::to_unsigned(nLine))
        {
            if (nIndex == 0)
                break;
            if (--nIndex == 0)
                break;

            const auto& pItemMetrics2 = m_vItemMetrics.at(nIndex);
            if (gsl::narrow_cast<gsl::index>(pItemMetrics2.nFirstLine) + ra::to_signed(pItemMetrics2.nNumLines) <= nLine)
                break; // unexpected! requested line is not in metrics - bail

        }
        else if (gsl::narrow_cast<gsl::index>(pItemMetrics.nFirstLine) + ra::to_signed(pItemMetrics.nNumLines) <= nLine)
        {
            ++nIndex;
        }
        else
        {
            break;
        }
    }
    return nIndex;
}

void MultiLineGridBinding::OnViewModelAdded(gsl::index nIndex)
{
    m_vItemMetrics.emplace(m_vItemMetrics.begin() + nIndex);

    for (gsl::index nColumn = 0; nColumn < ra::to_signed(m_vColumns.size()); ++nColumn)
    {
        const auto* pColumn = m_vColumns.at(nColumn).get();
        if (SupportsMultipleLines(pColumn))
        {
            const auto nChars = GetMaxCharsForColumn(nColumn);
            UpdateLineBreaks(nIndex, nColumn, pColumn, nChars);
        }
    }

    if (!m_vmItems->IsUpdating())
        OnEndViewModelCollectionUpdate();
    else
        SuspendRedraw();
}

void MultiLineGridBinding::OnViewModelRemoved(gsl::index nIndex)
{
    m_vItemMetrics.erase(m_vItemMetrics.begin() + nIndex);

    if (!m_vmItems->IsUpdating())
        OnEndViewModelCollectionUpdate();
    else
        SuspendRedraw();
}

void MultiLineGridBinding::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    bool bRebuildAll = false;

    auto& pPropertyMapping = GetPropertyColumnMapping(args.Property.GetKey());

    if (pPropertyMapping.nDependentColumns == 0xFFFFFFFF)
    {
        pPropertyMapping.nDependentColumns = 0;

        for (size_t nColumnIndex = 0; nColumnIndex < m_vColumns.size(); ++nColumnIndex)
        {
            const auto& pColumn = *m_vColumns.at(nColumnIndex);
            if (pColumn.DependsOn(args.Property))
                pPropertyMapping.nDependentColumns |= 1 << nColumnIndex;
        }
    }

    if (pPropertyMapping.nDependentColumns)
    {
        auto nDependentColumns = pPropertyMapping.nDependentColumns;
        gsl::index nColumn = 0;
        do
        {
            if (nDependentColumns & 1)
            {
                const auto* pColumn = m_vColumns.at(nColumn).get();
                if (pColumn && SupportsMultipleLines(pColumn))
                {
                    auto& pItemMetrics = m_vItemMetrics.at(nIndex);
                    const auto nChars = GetMaxCharsForColumn(nColumn);
                    std::vector<unsigned int> vLineBreaks;
                    const auto sText = pColumn->GetText(*m_vmItems, nIndex);

                    GetLineBreaks(sText, nChars, vLineBreaks);

                    if (vLineBreaks.empty())
                    {
                        pItemMetrics.mColumnLineOffsets.erase(gsl::narrow_cast<int>(nColumn));
                    }
                    else
                    {
                        auto& pColumnLineBreaks = pItemMetrics.mColumnLineOffsets[gsl::narrow_cast<int>(nColumn)];
                        pColumnLineBreaks.swap(vLineBreaks);
                    }

                    unsigned nLinesNeeded = 1;
                    for (const auto& pPair : pItemMetrics.mColumnLineOffsets)
                    {
                        if (pPair.second.size() + 1 > nLinesNeeded)
                            nLinesNeeded = gsl::narrow_cast<unsigned int>(pPair.second.size()) + 1;
                    }

                    if (!m_vmItems->IsUpdating())
                    {
                        if (nLinesNeeded != pItemMetrics.nNumLines)
                            bRebuildAll = true;
                    }
                }
            }

            ++nColumn;
            nDependentColumns >>= 1;
        } while (nDependentColumns);
    }

    if (bRebuildAll)
        Redraw();
    else
        UpdateDependentColumns(nIndex, pPropertyMapping.nDependentColumns);
}

void MultiLineGridBinding::Redraw()
{
    InvokeOnUIThread([this]() {
        GridBinding::UpdateAllItems();
        GridBinding::Redraw();
    });
}

int MultiLineGridBinding::GetMaxCharsForColumn(gsl::index nColumn) const
{
    return std::max(m_vColumnWidths.at(nColumn) / 6, 32);
}

void MultiLineGridBinding::UpdateLineBreaks(gsl::index nIndex, gsl::index nColumn, const ra::ui::win32::bindings::GridColumnBinding* pColumn, size_t nChars)
{
    std::vector<unsigned int> vLineBreaks;
    Expects(pColumn != nullptr);

    const auto sText = pColumn->GetText(*m_vmItems, nIndex);
    GetLineBreaks(sText, nChars, vLineBreaks);

    if (!vLineBreaks.empty())
    {
        auto& pItemMetrics = m_vItemMetrics.at(nIndex);
        auto& pColumnLineBreaks = pItemMetrics.mColumnLineOffsets[gsl::narrow_cast<int>(nColumn)];
        pColumnLineBreaks.swap(vLineBreaks);
    }
}

void MultiLineGridBinding::GetLineBreaks(const std::wstring& sText, size_t nChars, std::vector<unsigned int>& vLineBreaks)
{
    const auto nNewLine = sText.find('\n');
    if (nNewLine == std::string::npos && sText.length() < nChars)
        return;

    size_t nStart = 0;
    size_t nLastWhitespace = 0;
    for (size_t nOffset = 0; nOffset < sText.length(); ++nOffset)
    {
        const auto c = sText.at(nOffset);
        if (c == '\n')
        {
            nStart = nOffset + 1;
            vLineBreaks.push_back(gsl::narrow_cast<unsigned int>(nStart));
        }
        else if (nOffset - nStart > nChars)
        {
            if (nLastWhitespace > nStart)
            {
                nStart = nLastWhitespace + 1;
                vLineBreaks.push_back(gsl::narrow_cast<unsigned int>(nStart));
            }
            else if (c == ' ' || c == '\t')
            {
                nStart = nOffset + 1;
                vLineBreaks.push_back(gsl::narrow_cast<unsigned int>(nStart));
            }
        }
        else if (c == ' ' || c == '\t')
        {
            nLastWhitespace = nOffset;
        }
    }
}

void MultiLineGridBinding::UpdateAllItems()
{
    m_vItemMetrics.clear();
    m_vItemMetrics.resize(m_vmItems->Count());

    for (gsl::index nColumn = 0; nColumn < ra::to_signed(m_vColumns.size()); ++nColumn)
    {
        const auto* pColumn = m_vColumns.at(nColumn).get();
        if (SupportsMultipleLines(pColumn))
        {
            const auto nChars = GetMaxCharsForColumn(nColumn);
            for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vmItems->Count()); ++nIndex)
                UpdateLineBreaks(nIndex, nColumn, pColumn, nChars);
        }
    }

    GridBinding::UpdateAllItems();
}

void MultiLineGridBinding::UpdateItems(gsl::index nColumn)
{
    const auto& pColumn = *m_vColumns.at(nColumn);
    const unsigned nItems = ra::to_unsigned(ListView_GetItemCount(m_hWnd));
    std::wstring sText;

    unsigned nLine = 0;
    for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
    {
        auto& pItemMetrics = m_vItemMetrics.at(i);

        pItemMetrics.nFirstLine = nLine;
        pItemMetrics.nNumLines = 1;
        for (const auto& pPair : pItemMetrics.mColumnLineOffsets)
        {
            if (pPair.second.size() + 1 > pItemMetrics.nNumLines)
                pItemMetrics.nNumLines = gsl::narrow_cast<unsigned int>(pPair.second.size()) + 1;
        }

        sText = pColumn.GetText(*m_vmItems, i);
        UpdateCellContents(pItemMetrics, nColumn, sText, nItems);
        nLine += pItemMetrics.nNumLines;
    }

    if (nItems > nLine)
    {
        for (int i = ra::to_signed(nItems) - 1; i >= ra::to_signed(nLine); --i)
            ListView_DeleteItem(m_hWnd, i);
    }
}

void MultiLineGridBinding::UpdateCell(gsl::index nIndex, gsl::index nColumnIndex)
{
    const auto& pColumn = *m_vColumns.at(nColumnIndex);
    if (nColumnIndex == 0)
    {
        const auto* pCheckBoxColumn = dynamic_cast<const GridCheckBoxColumnBinding*>(&pColumn);
        if (pCheckBoxColumn != nullptr)
        {
            GridBinding::UpdateCell(nIndex, nColumnIndex);
            return;
        }
    }

    auto sText = pColumn.GetText(*m_vmItems, nIndex);
    const auto& pItemMetrics = m_vItemMetrics.at(nIndex);

    const auto nLines = pItemMetrics.nFirstLine + pItemMetrics.nNumLines;
    UpdateCellContents(pItemMetrics, nColumnIndex, sText, nLines);
}

void MultiLineGridBinding::UpdateCellContents(const ItemMetrics& pItemMetrics,
    gsl::index nColumn, std::wstring& sText, const unsigned int nAvailableLines)
{
    auto nLine = pItemMetrics.nFirstLine;

    LV_ITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = gsl::narrow_cast<int>(nLine);
    item.iSubItem = gsl::narrow_cast<int>(nColumn);
    item.pszText = sText.data();

    if (pItemMetrics.nNumLines > 1)
    {
        unsigned int nItemLine = 1;
        const auto pIter = pItemMetrics.mColumnLineOffsets.find(gsl::narrow_cast<int>(nColumn));
        if (pIter != pItemMetrics.mColumnLineOffsets.end())
        {
            size_t nStop = 0;
            for (auto nIndex : pIter->second)
            {
                auto nIndex2 = gsl::narrow_cast<size_t>(nIndex) - 1;
                wchar_t c = 0; // isspace throws a debug assertion if c > 255
                while (nIndex2 > nStop && (c = sText.at(nIndex2 - 1)) < 255 && isspace(c))
                    --nIndex2;
                sText.at(nIndex2) = '\0';

                GSL_SUPPRESS_TYPE1
                SNDMSG(m_hWnd, (nLine < nAvailableLines) ? LVM_SETITEMW : LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));

                item.pszText = nIndex < sText.length() ? &sText.at(nIndex) : &sText.at(nIndex2);
                item.iItem = gsl::narrow_cast<int>(++nLine);
                ++nItemLine;

                nStop = nIndex;
            }
        }

        for (unsigned int nIndex = nItemLine; nIndex < pItemMetrics.nNumLines; ++nIndex)
        {
            GSL_SUPPRESS_TYPE1
            SNDMSG(m_hWnd, (nLine < nAvailableLines) ? LVM_SETITEMW : LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));

            sText.at(0) = '\0';
            item.pszText = sText.data();
            item.iItem = gsl::narrow_cast<int>(++nLine);
        }
    }

    GSL_SUPPRESS_TYPE1
    SNDMSG(m_hWnd, (nLine < nAvailableLines) ? LVM_SETITEMW : LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
}

LRESULT MultiLineGridBinding::OnLvnItemChanging(const LPNMLISTVIEW pnmListView)
{
    // prevent built-in selection logic - we'll do manual selection in OnNmClick
    if (pnmListView->uChanged & LVIF_STATE && pnmListView->uNewState & LVIS_SELECTED && !(pnmListView->uOldState & LVIS_SELECTED))
        return TRUE;

    return GridBinding::OnLvnItemChanging(pnmListView);
}

void MultiLineGridBinding::OnLvnItemChanged(const LPNMLISTVIEW pnmListView)
{
    NMLISTVIEW nmListView;
    memcpy(&nmListView, pnmListView, sizeof(nmListView));
    nmListView.iItem = gsl::narrow_cast<int>(GetIndexForLine(pnmListView->iItem));
    GridBinding::OnLvnItemChanged(&nmListView);
}

LRESULT MultiLineGridBinding::OnCustomDraw(NMLVCUSTOMDRAW* pCustomDraw)
{
    NMLVCUSTOMDRAW lvCustomDraw;
    memcpy(&lvCustomDraw, pCustomDraw, sizeof(NMLVCUSTOMDRAW));
    lvCustomDraw.nmcd.dwItemSpec = GetIndexForLine(pCustomDraw->nmcd.dwItemSpec);

    switch (pCustomDraw->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
            if (m_pIsSelectedProperty && m_vmItems->GetItemValue(lvCustomDraw.nmcd.dwItemSpec, *m_pIsSelectedProperty))
            {
                pCustomDraw->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
                return CDRF_NOTIFYSUBITEMDRAW;
            }
            break;

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
            if (m_pIsSelectedProperty && m_vmItems->GetItemValue(lvCustomDraw.nmcd.dwItemSpec, *m_pIsSelectedProperty))
            {
                pCustomDraw->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                return CDRF_DODEFAULT;
            }
            break;
    }

    const auto nResult = GridBinding::OnCustomDraw(&lvCustomDraw);
    pCustomDraw->clrText = lvCustomDraw.clrText;
    pCustomDraw->clrTextBk = lvCustomDraw.clrTextBk;
    return nResult;
}

void MultiLineGridBinding::OnNmClick(const NMITEMACTIVATE* pnmItemActivate)
{
    NMITEMACTIVATE nmItemActivate;
    memcpy(&nmItemActivate, pnmItemActivate, sizeof(nmItemActivate));
    nmItemActivate.iItem = gsl::narrow_cast<int>(GetIndexForLine(pnmItemActivate->iItem));

    if (m_pIsSelectedProperty)
    {
        m_vmItems->RemoveNotifyTarget(*this); // ignore IsSelected changes
        m_vmItems->BeginUpdate();

        if (GetAsyncKeyState(VK_LCONTROL) < 0 || GetAsyncKeyState(VK_RCONTROL) < 0)
        {
            // when CTRL+clicking, toggle the clicked item
            m_vmItems->SetItemValue(nmItemActivate.iItem, *m_pIsSelectedProperty,
                !m_vmItems->GetItemValue(nmItemActivate.iItem, *m_pIsSelectedProperty));
        }
        else if (GetAsyncKeyState(VK_LSHIFT) < 0 || GetAsyncKeyState(VK_RSHIFT) < 0)
        {
            // when SHIFT+clicking, select a range
            gsl::index nFirst = nmItemActivate.iItem;
            gsl::index nLast = m_nLastClickedItem;

            if (nLast < nFirst)
            {
                nLast = nmItemActivate.iItem;
                nFirst = m_nLastClickedItem;
            }

            for (auto nIndex = nFirst; nIndex <= nLast; ++nIndex)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, true);
        }
        else
        {
            // when normal clicking, unselect everything but the clicked item
            for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vmItems->Count()); ++nIndex)
                m_vmItems->SetItemValue(nIndex, *m_pIsSelectedProperty, nIndex == nmItemActivate.iItem);
        }

        m_vmItems->EndUpdate();
        m_vmItems->AddNotifyTarget(*this);

        m_nLastClickedItem = nmItemActivate.iItem;

        SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }

    GridBinding::OnNmClick(&nmItemActivate);
}

void MultiLineGridBinding::OnNmDblClick(const NMITEMACTIVATE* pnmItemActivate)
{
    NMITEMACTIVATE nmItemActivate;
    memcpy(&nmItemActivate, pnmItemActivate, sizeof(nmItemActivate));
    nmItemActivate.iItem = gsl::narrow_cast<int>(GetIndexForLine(pnmItemActivate->iItem));
    GridBinding::OnNmDblClick(&nmItemActivate);
}

void MultiLineGridBinding::EnsureVisible(gsl::index nIndex)
{
    if (nIndex >= 0 && nIndex < gsl::narrow_cast<gsl::index>(m_vItemMetrics.size()))
    {
        const auto nLine = m_vItemMetrics.at(nIndex).nFirstLine;
        ListView_EnsureVisible(m_hWnd, nLine, FALSE);
    }
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
