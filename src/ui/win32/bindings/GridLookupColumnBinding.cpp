#include "GridLookupColumnBinding.hh"

#include "GridBinding.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

static LRESULT NotifySelection(HWND hwnd, GridColumnBinding::InPlaceEditorInfo* pInfo)
{
    Expects(pInfo != nullptr);

    const auto* pColumnBinding = dynamic_cast<GridLookupColumnBinding*>(pInfo->pColumnBinding);
    Expects(pColumnBinding != nullptr);

    const gsl::index nIndex = ComboBox_GetCurSel(hwnd);
    const auto nValue = pColumnBinding->GetValueFromIndex(nIndex);

    GridBinding* gridBinding = static_cast<GridBinding*>(pInfo->pGridBinding);
    Expects(gridBinding != nullptr);
    if (gridBinding->UpdateSelected(pColumnBinding->GetBoundProperty(), nValue) == 0)
        gridBinding->GetItems().SetItemValue(pInfo->nItemIndex, pColumnBinding->GetBoundProperty(), nValue);

    return GridBinding::CloseIPE(hwnd, pInfo);
}

static LRESULT CALLBACK DropDownProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    GridColumnBinding::InPlaceEditorInfo* pInfo = GridBinding::GetIPEInfo(hwnd);

    switch (nMsg)
    {
        case WM_DESTROY:
        {
            SubclassWindow(hwnd, pInfo->pOriginalWndProc);
            std::unique_ptr<GridColumnBinding::InPlaceEditorInfo> pPointer(pInfo);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            return 0;
        }

        case WM_KILLFOCUS:
            if (pInfo->bIgnoreLostFocus)
                return 0;

            return NotifySelection(hwnd, pInfo);

        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                PostMessage(hwnd, WM_KILLFOCUS, 0, 0);
            break;

        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
                return NotifySelection(hwnd, pInfo);

            if (wParam == VK_ESCAPE)
            {
                // Undo changes: i.e. simply destroy the window!
                return GridBinding::CloseIPE(hwnd, pInfo);
            }

            break;
    }

    return CallWindowProc(pInfo->pOriginalWndProc, hwnd, nMsg, wParam, lParam);
}


HWND GridLookupColumnBinding::CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo)
{
    HWND hInPlaceEditor =
        CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("ComboBox"), TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST,
            pInfo.rcSubItem.left, pInfo.rcSubItem.top, pInfo.rcSubItem.right - pInfo.rcSubItem.left,
            ra::ftol(1.6F * (pInfo.rcSubItem.bottom - pInfo.rcSubItem.top) * m_vmItems.Count()),
            hParent, nullptr, GetModuleHandle(nullptr), nullptr);

    if (hInPlaceEditor == nullptr)
    {
        assert(!"Could not create combo box!");
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not create combo box.");
        return nullptr;
    }

    const auto& pItems = static_cast<GridBinding*>(pInfo.pGridBinding)->GetItems();
    const auto nValue = pItems.GetItemValue(pInfo.nItemIndex, *m_pBoundProperty);

    const ViewModelBase* vmItem = nullptr;
    if (m_fIsVisible)
        vmItem = pItems.GetViewModelAt(pInfo.nItemIndex);

    m_vVisibleItems.clear();

    for (size_t i = 0; i < m_vmItems.Count(); ++i)
    {
        const auto* pItem = m_vmItems.GetItemAt(i);
        Expects(pItem != nullptr);
        const auto nItemValue = pItem->GetId();

        if (vmItem != nullptr)
        {
            if (nItemValue != nValue && !m_fIsVisible(*vmItem, nItemValue))
                continue;

            m_vVisibleItems.push_back(nItemValue);
        }

        const auto nIndex = ComboBox_AddString(hInPlaceEditor, NativeStr(pItem->GetLabel()).c_str());

        if (nItemValue == nValue)
            ComboBox_SetCurSel(hInPlaceEditor, nIndex);
    }

    if (m_nDropDownWidth > 0)
        SendMessage(hInPlaceEditor, CB_SETDROPPEDWIDTH, m_nDropDownWidth, 0);

    SetWindowFont(hInPlaceEditor, GetStockObject(DEFAULT_GUI_FONT), TRUE);
    ComboBox_ShowDropdown(hInPlaceEditor, TRUE);

    pInfo.pOriginalWndProc = SubclassWindow(hInPlaceEditor, DropDownProc);

    return hInPlaceEditor;
}

int GridLookupColumnBinding::GetValueFromIndex(gsl::index nIndex) const
{
    if (m_fIsVisible != nullptr)
        return m_vVisibleItems.at(nIndex);

    const auto* pItem = m_vmItems.GetItemAt(nIndex);
    return (pItem != nullptr) ? pItem->GetId() : 0;        
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
