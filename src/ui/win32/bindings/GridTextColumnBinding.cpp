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

    // if SetText pops up an error message, it will cause a Lost Focus event - ignore it
    // until after the focus has been set back to the IPE.
    pInfo->bIgnoreLostFocus = true;

    auto* pColumnBinding = dynamic_cast<GridTextColumnBindingBase*>(pInfo->pColumnBinding);
    Expects(pColumnBinding != nullptr);

    const auto nLength = Edit_GetTextLength(hwnd);
    std::wstring sValue;
    sValue.resize(gsl::narrow_cast<size_t>(nLength) + 1);
    GetWindowTextW(hwnd, sValue.data(), nLength + 1);
    sValue.resize(nLength);

    GridBinding* gridBinding = static_cast<GridBinding*>(pInfo->pGridBinding);
    Expects(gridBinding != nullptr);
    if (pColumnBinding->SetText(gridBinding->GetItems(), pInfo->nItemIndex, sValue))
        return GridBinding::CloseIPE(hwnd, pInfo);

    SetFocus(hwnd);

    pInfo->bIgnoreLostFocus = false;
    return 0;
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
            if (pInfo->bIgnoreLostFocus)
                return 0;

            return NotifySelection(hwnd, pInfo);

        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
                return NotifySelection(hwnd, pInfo);

            if (wParam == VK_ESCAPE)
            {
                // Undo changes: i.e. simply destroy the window!
                return GridBinding::CloseIPE(hwnd, pInfo);
            }

            // BizHawk specific hack (possibly affects other emus?), seems BizHawk does not have WM_CHAR messages sent,
            // so text can't be typed. It's unknown why this occurs, it's likely something to do with it using
            // IsDialogMessage more properly in its message loop and something is likely wrong with the way edit windows
            // are handled in RAIntegration. Until that is figured out, this hack seems to suffice
            if (pInfo->bForceWmChar)
            {
                MSG msg = {hwnd, nMsg, wParam, lParam};
                TranslateMessage(&msg);
                PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE);
                return CallWindowProc(pInfo->pOriginalWndProc, msg.hwnd, msg.message, msg.wParam, msg.lParam);
            }

            break;

        case WM_GETDLGCODE:
            // only reached if IsDialogMessage is used (also used as a signal to activate the above hack)
            pInfo->bForceWmChar = true;
            return DLGC_WANTALLKEYS;
    }

    return CallWindowProc(pInfo->pOriginalWndProc, hwnd, nMsg, wParam, lParam);
}

HWND GridTextColumnBindingBase::CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo)
{
    HWND hInPlaceEditor =
        CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | ES_WANTRETURN | ES_AUTOHSCROLL,
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
