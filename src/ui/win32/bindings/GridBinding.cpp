#include "GridBinding.hh"

#include "RA_StringUtils.h"

#include "GridCheckBoxColumnBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

void GridBinding::BindColumn(gsl::index nColumn, std::unique_ptr<GridColumnBinding> pColumnBinding)
{
    if (dynamic_cast<GridCheckBoxColumnBinding*>(pColumnBinding.get()) != nullptr)
    {
        if (nColumn != 0)
            return;
    }

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

        if (m_vmItems)
        {
            DisableBinding();
            UpdateItems(nColumn);
            EnableBinding();
        }
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
    if (m_vColumns.empty())
        return;

    if (dynamic_cast<GridCheckBoxColumnBinding*>(m_vColumns.at(0).get()) != nullptr)
        ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_CHECKBOXES);

    // calculate the column widths
    RECT rcList;
    GetClientRect(m_hWnd, &rcList);

    const int nWidth = rcList.right - rcList.left;
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
    const auto nColumns = m_nColumnsCreated;
    for (gsl::index i = 0; ra::to_unsigned(i) < m_vColumns.size(); ++i)
    {
        const auto& pColumn = *m_vColumns.at(i);
        const auto sHeader = NativeStr(pColumn.GetHeader());

        LV_COLUMN col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH;
        col.cx = vWidths.at(i);
        GSL_SUPPRESS_TYPE3 col.pszText = const_cast<LPSTR>(sHeader.data());
        col.iSubItem = i;

        if (i < ra::to_signed(nColumns))
            ListView_SetColumn(m_hWnd, i, &col);
        else
            ListView_InsertColumn(m_hWnd, i, &col);
    }

    // remove any extra columns
    if (nColumns > m_vColumns.size())
    {
        for (gsl::index i = nColumns - 1; ra::to_unsigned(i) >= m_vColumns.size(); --i)
            ListView_DeleteColumn(m_hWnd, i);
    }

    // store the number of columns we know to be present since the suggested way to
    // retrieve that number doesn't always work.
    m_nColumnsCreated = m_vColumns.size();
}

void GridBinding::UpdateAllItems()
{
    RECT rcClientBefore{};
    if (m_hWnd)
        ::GetClientRect(m_hWnd, &rcClientBefore);

    DisableBinding();

    for (gsl::index nColumn = 0; ra::to_unsigned(nColumn) < m_vColumns.size(); ++nColumn)
        UpdateItems(nColumn);

    EnableBinding();

    if (m_hWnd)
    {
        // if a scrollbar is added or removed, the client size will change
        RECT rcClientAfter;
        ::GetClientRect(m_hWnd, &rcClientAfter);
        if (rcClientAfter.right != rcClientBefore.right)
            UpdateLayout();
    }
}

void GridBinding::UpdateItems(gsl::index nColumn)
{
    const auto& pColumn = *m_vColumns.at(nColumn);

    const auto nItems = ListView_GetItemCount(m_hWnd);

    const auto* pCheckBoxColumn = dynamic_cast<const GridCheckBoxColumnBinding*>(&pColumn);
    if (pCheckBoxColumn != nullptr)
    {
        LV_ITEM item{};
        item.mask = LVIF_TEXT;
        item.iSubItem = nColumn;
        GSL_SUPPRESS_TYPE3 item.pszText = const_cast<LPSTR>("");

        const auto& pBoundProperty = pCheckBoxColumn->GetBoundProperty();

        for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
        {
            if (i >= nItems)
            {
                item.iItem = i;
                ListView_InsertItem(m_hWnd, &item);
            }

            ListView_SetCheckState(m_hWnd, i, m_vmItems->GetItemValue(i, pBoundProperty));
        }
    }
    else
    {
        std::string sText;

        LV_ITEM item{};
        item.mask = LVIF_TEXT;
        item.iSubItem = nColumn;

        for (gsl::index i = 0; ra::to_unsigned(i) < m_vmItems->Count(); ++i)
        {
            sText = NativeStr(pColumn.GetText(*m_vmItems, i));
            item.pszText = sText.data();
            item.iItem = i;

            if (i < nItems)
                ListView_SetItem(m_hWnd, &item);
            else
                ListView_InsertItem(m_hWnd, &item);
        }
    }

    if (ra::to_unsigned(nItems) > m_vmItems->Count())
    {
        for (gsl::index i = nItems - 1; ra::to_unsigned(i) >= m_vmItems->Count(); --i)
            ListView_DeleteItem(m_hWnd, i);
    }
}

void GridBinding::OnLvnItemChanged(const LPNMLISTVIEW pnmListView)
{
    switch (pnmListView->uNewState)
    {
        case 0x2000: // LVIS_CHECKED
        {
            const auto* pCheckBoxBinding = dynamic_cast<GridCheckBoxColumnBinding*>(m_vColumns.at(0).get());
            if (pCheckBoxBinding != nullptr)
            {
                m_vmItems->RemoveNotifyTarget(*this);
                m_vmItems->SetItemValue(pnmListView->iItem, pCheckBoxBinding->GetBoundProperty(), true);
                m_vmItems->AddNotifyTarget(*this);
            }
        }
        break;

        case 0x1000: // LVIS_UNCHECKED
        {
            const auto* pCheckBoxBinding = dynamic_cast<GridCheckBoxColumnBinding*>(m_vColumns.at(0).get());
            if (pCheckBoxBinding != nullptr)
            {
                m_vmItems->RemoveNotifyTarget(*this);
                m_vmItems->SetItemValue(pnmListView->iItem, pCheckBoxBinding->GetBoundProperty(), false);
                m_vmItems->AddNotifyTarget(*this);
            }
        }
        break;
    }
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
