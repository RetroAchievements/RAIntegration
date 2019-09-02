#include "GridTextColumnBinding.hh"

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

    auto* pColumnBinding = dynamic_cast<GridTextColumnBinding*>(pInfo->pColumnBinding);
    Expects(pColumnBinding != nullptr);

    const auto nLength = Edit_GetTextLength(hwnd);
    std::wstring sValue;
    sValue.resize(nLength + 1);
    GetWindowTextW(hwnd, sValue.data(), nLength + 1);
    sValue.resize(nLength);

    GridBinding* gridBinding = static_cast<GridBinding*>(pInfo->pGridBinding);
    Expects(gridBinding != nullptr);
    pColumnBinding->SetText(gridBinding->GetItems(), pInfo->nItemIndex, sValue);

    return GridBinding::CloseIPE(hwnd, pInfo);
}

static LRESULT CALLBACK EditBoxProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
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
            return NotifySelection(hwnd, pInfo);

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

HWND GridTextColumnBinding::CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo)
{
    HWND hInPlaceEditor =
        CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | ES_WANTRETURN,
            pInfo.rcSubItem.left, pInfo.rcSubItem.top, pInfo.rcSubItem.right - pInfo.rcSubItem.left,
            pInfo.rcSubItem.bottom - pInfo.rcSubItem.top,
            hParent, nullptr, GetModuleHandle(nullptr), nullptr);

    if (hInPlaceEditor == nullptr)
    {
        assert(!"Could not create edit box!");
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not create edit box.");
        return nullptr;
    }

    SetWindowFont(hInPlaceEditor, GetStockFont(DEFAULT_GUI_FONT), TRUE);

    const auto& pItems = static_cast<GridBinding*>(pInfo.pGridBinding)->GetItems();
    SetWindowTextW(hInPlaceEditor, GetText(pItems, pInfo.nItemIndex).c_str());

    Edit_SetSel(hInPlaceEditor, 0, -1);

    pInfo.pOriginalWndProc = SubclassWindow(hInPlaceEditor, EditBoxProc);

    return hInPlaceEditor;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
