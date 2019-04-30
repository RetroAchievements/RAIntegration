#include "GridBinding.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

void GridBinding::BindColumn(gsl::index nColumn, std::unique_ptr<GridColumnBinding> pColumnBinding)
{
    if (m_vColumns.size() <= ra::to_unsigned(nColumn))
    {
        if (m_vColumns.capacity() == 0)
            m_vColumns.reserve(4U);

        m_vColumns.resize(nColumn + 1);
    }

    m_vColumns.at(nColumn) = std::move(pColumnBinding);

    if (m_hWnd)
    {
        UpdateLayout();

        if (!m_vmItems)
            UpdateItems(nColumn);
    }
}

void GridBinding::BindItems(ViewModelCollectionBase& vmItems)
{
    if (m_vmItems != nullptr)
    {
        m_vmItems->RemoveNotifyTarget(*this);
    }

    m_vmItems = &vmItems;
    m_vmItems->AddNotifyTarget(*this);

    if (m_hWnd && !m_vColumns.empty())
        UpdateAllItems();
}

void GridBinding::UpdateLayout()
{
    // calculate the column widths
    RECT rcList;
    GetClientRect(m_hWnd, &rcList);

    const auto nScrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
    const int nWidth = rcList.right - rcList.left - nScrollbarWidth;
    int nRemaining = nWidth;
    int nFillParts = 0;
    std::vector<int> vWidths;
    vWidths.resize(m_vColumns.size());

    for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        const auto& pColumn = *m_vColumns.at(i);
        switch (pColumn.GetWidthType())
        {
        case GridColumnBinding::WidthType::Pixels:
            vWidths.at(i) = pColumn.GetWidth();
            break;

        case GridColumnBinding::WidthType::Percentage:
            vWidths.at(i) = nWidth * pColumn.GetWidth() / 100;
            break;

        default:
            nFillParts += pColumn.GetWidth();
            vWidths.at(i) = 0;
            break;
        }

        nRemaining -= vWidths.at(i);
    }

    if (nFillParts > 0)
    {
        for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
        {
            const auto& pColumn = *m_vColumns.at(i);
            if (pColumn.GetWidthType() == GridColumnBinding::WidthType::Fill)
                vWidths.at(i) = nRemaining * pColumn.GetWidth() / nFillParts;
        }
    }

    // update or insert the columns
    const auto nColumns = ListView_GetItemCount(ListView_GetHeader(m_hWnd));
    for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        const auto& pColumn = *m_vColumns.at(i);
        const auto sHeader = NativeStr(pColumn.GetHeader());

        LV_COLUMN col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH;
        col.cx = vWidths.at(i);
        col.pszText = const_cast<LPTSTR>(sHeader.c_str());
        col.iSubItem = i;

        if (i < nColumns)
            ListView_SetColumn(m_hWnd, i, &col);
        else
            ListView_InsertColumn(m_hWnd, i, &col);
    }

    // remove any extra columns
    if (ra::to_unsigned(nColumns) > m_vColumns.size())
    {
        for (gsl::index i = nColumns - 1; ra::to_unsigned(i) >= m_vColumns.size(); --i)
            ListView_DeleteColumn(m_hWnd, i);
    }
}

void GridBinding::UpdateAllItems()
{
    for (gsl::index nColumn = 0; ra::to_unsigned(nColumn) < m_vColumns.size(); ++nColumn)
        UpdateItems(nColumn);
}

void GridBinding::UpdateItems(gsl::index nColumn)
{
    const auto& pColumn = *m_vColumns.at(nColumn);

    const auto nItems = ListView_GetItemCount(m_hWnd);
    std::string sText;

    LV_ITEM item{};
    item.mask = LVIF_TEXT;
    item.iSubItem = nColumn;
    
    for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
    {
        sText = NativeStr(pColumn.GetText(*m_vmItems, i));
        item.pszText = const_cast<LPTSTR>(sText.c_str());
        item.iItem = i;

        if (i < nItems)
            ListView_SetItem(m_hWnd, &item);
        else
            ListView_InsertItem(m_hWnd, &item);
    }

    if (ra::to_unsigned(nItems) > m_vmItems->Count())
    {
        for (gsl::index i = nItems - 1; ra::to_unsigned(i) >= m_vmItems->Count(); --i)
            ListView_DeleteItem(m_hWnd, i);
    }
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
