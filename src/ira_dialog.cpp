#include "ira_dialog.h"

#include "RA_Core.h"

namespace ra {

// Instead of guessing we're just going to put _UNUSED
// You don't have to put [[maybe_unused]] in the declaration but if you do it has to be in the def too

// DO NOT USE DEFWINDOWPROC WITH THIS CLASS!

IRA_Dialog::IRA_Dialog(int ResID, const IRA_WndClass& myWndClass, _UNUSED HWND hParent) noexcept :
    m_nResId{ResID},
    m_Wndclass{ myWndClass },
    m_lpCaption{ myWndClass.className() }
{
}


void IRA_Dialog::Activate(_UNUSED HWND hwnd, _UNUSED UINT state, _UNUSED HWND hwndActDeact,
    _UNUSED BOOL fMinimized) noexcept
{
}

void IRA_Dialog::ActivateApp(_UNUSED HWND hwnd, _UNUSED BOOL fActivate, _UNUSED DWORD dwThreadId) noexcept
{
}

// HANDLE_WM_ASKCBFORMATNAME
void IRA_Dialog::AskCBFormatName(_UNUSED HWND hwnd, int cchMax, LPTSTR rgchName) noexcept
{
    // https://docs.microsoft.com/en-us/windows/desktop/dataxchg/wm-askcbformatname
    GetClipboardFormatName(CF_OWNERDISPLAY, rgchName, cchMax);
}

void IRA_Dialog::CancelMode(HWND hwnd) noexcept
{
    Enable(hwnd, FALSE);
}

void IRA_Dialog::ChangeCBChain(_UNUSED HWND hwnd, HWND hwndRemove, HWND hwndNext) noexcept
{
    // The only one that seems the same
    ::ChangeClipboardChain(hwndRemove, hwndNext);
}

void IRA_Dialog::OnChar(_UNUSED HWND hwnd, _UNUSED TCHAR ch, _UNUSED int cRepeat) noexcept
{
    // To many possibilities for a default
}

int IRA_Dialog::OnCharToItem(_UNUSED HWND hwnd, _UNUSED UINT ch, _UNUSED HWND hwndListbox,
    _UNUSED int iCaret) noexcept
{
    return 0;
}

void IRA_Dialog::OnChildActivate(_UNUSED HWND hwnd) noexcept
{
}

void IRA_Dialog::Clear(_UNUSED HWND hwnd) noexcept
{
    // This function really depends, it could be used for combo box, edit, or list box
}

void IRA_Dialog::Close(HWND hwnd) noexcept
{
    if (auto nResult = ::GetWindowLongPtr(hwnd, DWLP_MSGRESULT); m_bIsModal)
        ::EndDialog(hwnd, nResult);
    else
    {
        ::DestroyWindow(hwnd);
        hwnd = nullptr; // if we don't do this windows can't respawn
    }

}

void IRA_Dialog::OnCommand(_UNUSED HWND hwnd, _UNUSED int id, _UNUSED HWND hwndCtl,
    _UNUSED UINT codeNotify) noexcept
{
    // it's always different
}

void IRA_Dialog::CommNotify(_UNUSED HWND hwnd, _UNUSED int cid, _UNUSED UINT flags) noexcept
{
}

void IRA_Dialog::OnCompacting(_UNUSED HWND hwnd, _UNUSED UINT compactRatio) noexcept
{
    // Mainly for Win16 (DOS, Windows 3.2 and less) but we could use it here
}

int IRA_Dialog::CompareItem(_UNUSED HWND hwnd, _UNUSED const COMPAREITEMSTRUCT* lpCompareItem) noexcept
{
    return 0;
}

void IRA_Dialog::ContextMenu(_UNUSED HWND hwnd, _UNUSED HWND hwndContext, _UNUSED UINT xPos,
    _UNUSED UINT yPos) noexcept
{
}

// FORWARD_WM_COPY
void IRA_Dialog::Copy(_UNUSED HWND hwnd) noexcept
{
    // This depends, it can't be abstracted
}

BOOL IRA_Dialog::CopyData(HWND hwnd, HWND hwndFrom, PCOPYDATASTRUCT pcds) noexcept
{
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms649009(v=vs.85).aspx
    return FORWARD_WM_COPYDATA(hwnd, hwndFrom, pcds, SendMessage);
}

BOOL IRA_Dialog::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) noexcept
{
    if (hwnd = CreateWindowEx(static_cast<DWORD>(lpCreateStruct->style), lpCreateStruct->lpszClass,
        lpCreateStruct->lpszName, lpCreateStruct->dwExStyle, lpCreateStruct->x, lpCreateStruct->y,
        lpCreateStruct->cx, lpCreateStruct->cy, lpCreateStruct->hwndParent, lpCreateStruct->hMenu,
        lpCreateStruct->hInstance, lpCreateStruct->lpCreateParams); hwnd != nullptr)
    {
        return TRUE;
    }
    return FALSE; // throw?
}

#pragma region Control Colors
HBRUSH IRA_Dialog::OnCtlColorBtn(_UNUSED HWND hwnd, _UNUSED HDC hdc, _UNUSED HWND hwndChild,
    _UNUSED int type) noexcept
{
    return HBRUSH{};
}

HBRUSH IRA_Dialog::OnCtlColorDlg(_UNUSED HWND hwnd, _UNUSED HDC hdc, _UNUSED HWND hwndChild,
    _UNUSED int type) noexcept
{
    return HBRUSH{};
}

HBRUSH IRA_Dialog::OnCtlColorEdit(_UNUSED HWND hwnd, _UNUSED HDC hdc, _UNUSED HWND hwndChild,
    _UNUSED int type) noexcept
{
    return HBRUSH{};
}

HBRUSH IRA_Dialog::OnCtlColorListbox(_UNUSED HWND hwnd, _UNUSED HDC hdc, _UNUSED HWND hwndChild,
    _UNUSED int type) noexcept
{
    return HBRUSH{};
}

HBRUSH IRA_Dialog::OnCtlColorMsgbox(_UNUSED HWND hwnd, _UNUSED HDC hdc, _UNUSED HWND hwndChild,
    _UNUSED int type) noexcept
{
    return HBRUSH{};
}

HBRUSH IRA_Dialog::OnCtlColorScrollbar(_UNUSED HWND hwnd, _UNUSED HDC hdc, _UNUSED HWND hwndChild,
    _UNUSED int type) noexcept
{
    return HBRUSH{};
}

HBRUSH IRA_Dialog::OnCtlColorStatic(_UNUSED HWND hwnd, _UNUSED HDC hdc, _UNUSED HWND hwndChild,
    _UNUSED int type) noexcept
{
    return HBRUSH{};
}
#pragma endregion


void IRA_Dialog::Cut(_UNUSED HWND hwnd) noexcept
{
    // depends
}

void IRA_Dialog::DeadChar(_UNUSED HWND hwnd, _UNUSED TCHAR ch, _UNUSED int cRepeat) noexcept
{
}

void IRA_Dialog::DeleteItem(_UNUSED HWND hwnd, _UNUSED const DELETEITEMSTRUCT * lpDeleteItem) noexcept
{
    // depends on what type of window you're sub classing
    /*ListView_DeleteItem(hwnd, lpDeleteItem->itemID);*/
    /*ListBox_DeleteString(hwnd, lpDeleteItem->itemID);*/
    /*ComboBox_DeleteString(hwnd, lpDeleteItem->itemID);*/
}

void IRA_Dialog::Destroy(_UNUSED HWND hwnd) noexcept
{
    ::PostQuitMessage(0);
}

void IRA_Dialog::DestroyClipboard(HWND hwnd) noexcept
{
    ::OpenClipboard(hwnd);
    ::EmptyClipboard();
}

BOOL IRA_Dialog::DeviceChange(_UNUSED HWND hwnd, _UNUSED UINT uEvent, _UNUSED DWORD dwEventData) noexcept
{
    return 0;
}

void IRA_Dialog::DevModeChange(_UNUSED HWND hwnd, _UNUSED LPCTSTR lpszDeviceName) noexcept
{
}

void IRA_Dialog::DisplayChange(_UNUSED HWND hwnd, _UNUSED UINT bitsPerPixel, _UNUSED UINT cxScreen,
    _UNUSED UINT cyScreen) noexcept
{
    
}

void IRA_Dialog::DrawClipboard(_UNUSED HWND hwnd) noexcept
{
}

void IRA_Dialog::DrawItem(_UNUSED HWND hwnd, _UNUSED const DRAWITEMSTRUCT * lpDrawItem) noexcept
{
    // It really depends, leaving these here for "copy and pasting"
    /*DrawCaption(hwnd, lpDrawItem->hDC, &lpDrawItem->rcItem, lpDrawItem->itemState);*/
    /*DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem);*/
    /*DrawInsert(::GetParent(hwnd), lpDrawItem->hwndItem, lpDrawItem->itemID);*/
    /*DrawMenuBar(hwnd);*/
}

void IRA_Dialog::DropFiles(_UNUSED HWND hwnd, _UNUSED HDROP hdrop) noexcept
{
}

void IRA_Dialog::Enable(HWND hwnd, BOOL fEnable) noexcept
{
    EnableWindow(hwnd, fEnable);
}

void IRA_Dialog::EndSession(_UNUSED HWND hwnd, _UNUSED BOOL fEnding) noexcept
{
    // This function could be important (crashes)
}

void IRA_Dialog::EnterIdle(_UNUSED HWND hwnd, _UNUSED UINT source, _UNUSED HWND hwndSource) noexcept
{
    // This function could be important *cough* RASnes9x *cough*
}

BOOL IRA_Dialog::EraseBkgnd(_UNUSED HWND hwnd, _UNUSED HDC hdc) noexcept
{
    return 0;
}

void IRA_Dialog::FontChange(_UNUSED HWND hwnd) noexcept
{
    
}

UINT IRA_Dialog::GetDlgCode(_UNUSED HWND hwnd, _UNUSED LPMSG lpmsg) noexcept
{
    // This is not the same as GetDlgID or w/e it's called
    return UINT{};
}

HFONT IRA_Dialog::GetFont(HWND hwnd)
{
    return GetWindowFont(hwnd);
}

void IRA_Dialog::GetMinMaxInfo(_UNUSED HWND hwnd, _UNUSED LPMINMAXINFO lpMinMaxInfo) noexcept
{
    
}

INT IRA_Dialog::GetText(HWND hwnd, int cchTextMax, LPTSTR lpszText) noexcept
{
    return GetWindowText(hwnd, lpszText, cchTextMax);
}

INT IRA_Dialog::GetTextLength(HWND hwnd) noexcept
{
    return ::GetWindowTextLength(hwnd) + 1;
}

void IRA_Dialog::HotKey(HWND hwnd, int idHotKey, UINT fuModifiers, UINT vk) noexcept
{
    // Docs were kind of confusing, this could be used somewhat incase it was registered already
    if (::RegisterHotKey(hwnd, idHotKey, fuModifiers, vk) == 0)
        ::UnregisterHotKey(hwnd, idHotKey);
}

void IRA_Dialog::HScroll(_UNUSED HWND hwnd, _UNUSED HWND hwndCtl, _UNUSED UINT code, _UNUSED int pos) noexcept
{
    // no idea
}

void IRA_Dialog::HScrollClipboard(_UNUSED HWND hwnd, _UNUSED HWND hwndCBViewer, _UNUSED UINT code, _UNUSED int pos) noexcept
{
}

BOOL IRA_Dialog::IconEraseBkgnd(HWND hwnd, HDC hdc) noexcept
{
    // The DefWindowProc function fills the icon background with the class background brush of the parent window.
    // But we can't use that so let's try to do it ourselves
    auto lpWindowInfo = std::make_unique<WINDOWINFO>();

    GetWindowInfo(hwnd, lpWindowInfo.get());

    // yes we don't need these later since it should already be stored in the window class
    std::unique_ptr<HBRUSH__, decltype(&::DeleteObject)> myBrush{ nullptr, DeleteObject };
    myBrush.reset(reinterpret_cast<HBRUSH>(GetClassLongPtr(GetParent(hwnd), GCLP_HBRBACKGROUND)));

    std::unique_ptr<HICON__, decltype(&::DestroyIcon)> myIcon{ nullptr, DestroyIcon };
    myIcon.reset(reinterpret_cast<HCURSOR>(GetClassLongPtr(hwnd, GCLP_HICON)));

    // I'd have to check whether it's the window rect or client rect
    // We needed the one with an brush param, DrawIcon didn't have it
    return DrawIconEx(hdc, lpWindowInfo->rcClient.left, lpWindowInfo->rcClient.top, myIcon.get(),
        lpWindowInfo->cxWindowBorders, lpWindowInfo->cyWindowBorders, 0U, myBrush.get(),
        DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);;
}

BOOL IRA_Dialog::InitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) noexcept
{
    std::unique_ptr<IRA_Dialog> myDlg;
    myDlg.reset(reinterpret_cast<IRA_Dialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)));

    if (myDlg != nullptr)
        return myDlg->InitDialog(hwnd, hwndFocus, reinterpret_cast<LPARAM>(this));
    return GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

void IRA_Dialog::InitMenu(_UNUSED HWND hwnd, _UNUSED HMENU hMenu) noexcept
{
}

void IRA_Dialog::InitMenuPopup(_UNUSED HWND hwnd, _UNUSED HMENU hMenu, _UNUSED UINT item,
    _UNUSED BOOL fSystemMenu) noexcept
{
}

void IRA_Dialog::KeyDown(_UNUSED HWND hwnd, _UNUSED UINT vk, _UNUSED BOOL fDown, _UNUSED int cRepeat,
    _UNUSED UINT flags) noexcept
{
}

void IRA_Dialog::KeyUp(_UNUSED HWND hwnd, _UNUSED UINT vk, _UNUSED BOOL fDown, _UNUSED int cRepeat,
    _UNUSED UINT flags) noexcept
{
}

void IRA_Dialog::KillFocus(_UNUSED HWND hwnd, HWND hwndNewFocus) noexcept
{
    ::SetFocus(hwndNewFocus);
}

void IRA_Dialog::LButtonDblClk(_UNUSED HWND hwnd, _UNUSED BOOL fDoubleClick, _UNUSED int x, _UNUSED int y,
    _UNUSED UINT keyFlags) noexcept
{
}

void IRA_Dialog::LButtonDown(_UNUSED HWND hwnd, _UNUSED BOOL fDoubleClick, _UNUSED int x, _UNUSED int y,
    _UNUSED UINT keyFlags) noexcept
{
}

void IRA_Dialog::LButtonUp(_UNUSED HWND hwnd, _UNUSED int x, _UNUSED int y, _UNUSED UINT keyFlags) noexcept
{
}

void IRA_Dialog::MButtonDblclk(_UNUSED HWND hwnd, _UNUSED BOOL fDoubleClick, _UNUSED int x, _UNUSED int y,
    _UNUSED UINT keyFlags) noexcept
{
}

void IRA_Dialog::MButtonDown(_UNUSED HWND hwnd, _UNUSED BOOL fDoubleClick, _UNUSED int x, _UNUSED int y,
    _UNUSED UINT keyFlags) noexcept
{
}

void IRA_Dialog::MButtonUp(_UNUSED HWND hwnd, _UNUSED int x, _UNUSED int y, _UNUSED UINT flags) noexcept
{
}

void IRA_Dialog::MDIActivate(_UNUSED HWND hwnd, _UNUSED BOOL fActive, _UNUSED HWND hwndActivate,
    _UNUSED HWND hwndDeactivate) noexcept
{
}

BOOL IRA_Dialog::MDICascade(HWND hwnd, UINT cmd) noexcept
{
    return 0;
}

HWND IRA_Dialog::MDICreate(HWND hwnd, const LPMDICREATESTRUCT lpmcs) noexcept
{
    if (hwnd = CreateMDIWindow(lpmcs->szClass, lpmcs->szTitle, lpmcs->style, lpmcs->x, lpmcs->y, lpmcs->cx,
        lpmcs->cy, ::GetParent(hwnd), GetWindowInstance(hwnd), lpmcs->lParam); hwnd != nullptr)
    {
        return hwnd;
    }

    return HWND{}; // throw?
}

void IRA_Dialog::MDIDestroy(_UNUSED HWND hwnd, HWND hwndDestroy) noexcept
{
    ::DestroyWindow(hwndDestroy);
    hwndDestroy = nullptr;
}

HWND IRA_Dialog::MDIGetActive(_UNUSED HWND hwnd) noexcept
{
    return GetActiveWindow();
}

void IRA_Dialog::MDIIconArrange(_UNUSED HWND hwnd) noexcept
{
}

void IRA_Dialog::MDIMaximize(HWND hwnd, HWND hwndMaximize) noexcept
{
}

HWND IRA_Dialog::MDINext(_UNUSED HWND hwnd, HWND hwndCur, BOOL fPrev) noexcept
{
    return ::GetNextWindow(hwndCur, fPrev);
}

void IRA_Dialog::MDIRestore(HWND hwnd, HWND hwndRestore) noexcept
{
    ::ShowWindow(hwndRestore, SW_RESTORE);
}

HMENU IRA_Dialog::MDISetMenu(_UNUSED HWND hwnd, _UNUSED BOOL fRefresh, _UNUSED HMENU hmenuFrame,
    _UNUSED HMENU hmenuWindow) noexcept
{
    return HMENU();
}

BOOL IRA_Dialog::OnMDITile(_UNUSED HWND hwnd, _UNUSED UINT cmd) noexcept
{
    return BOOL();
}

void IRA_Dialog::MeasureItem(_UNUSED HWND hwnd, _UNUSED MEASUREITEMSTRUCT* lpMeasureItem) noexcept
{
}

DWORD IRA_Dialog::MenuChar(_UNUSED HWND hwnd, _UNUSED UINT ch, _UNUSED  UINT flags, _UNUSED HMENU hmenu) noexcept
{
    return DWORD();
}

void IRA_Dialog::MenuSelect(_UNUSED HWND hwnd, _UNUSED HMENU hmenu, _UNUSED int item,
    _UNUSED HMENU hmenuPopup, _UNUSED UINT flags) noexcept
{
}

int IRA_Dialog::MouseActivate(_UNUSED HWND hwnd, _UNUSED HWND hwndTopLevel, _UNUSED UINT codeHitTest,
    _UNUSED UINT msg) noexcept
{
    return 0;
}

void IRA_Dialog::MouseMove(_UNUSED HWND hwnd, _UNUSED int x, _UNUSED int y, _UNUSED UINT keyFlags) noexcept
{
}

void IRA_Dialog::MouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys) noexcept
{
}

void IRA_Dialog::Move(HWND hwnd, int x, int y) noexcept
{
}

BOOL IRA_Dialog::NCActivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized) noexcept
{
    return 0;
}

UINT IRA_Dialog::NCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp) noexcept
{
    return 0;
}

BOOL IRA_Dialog::NCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) noexcept
{
    return ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(lpCreateStruct->lpCreateParams));
}

void IRA_Dialog::NCDestroy(HWND hwnd) noexcept
{
    ::DestroyWindow(hwnd);
    hwnd = nullptr;
}

UINT IRA_Dialog::NCHitTest(HWND hwnd, int x, int y) noexcept
{
    return 0;
}

void IRA_Dialog::NCLButtonDblClk(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCMButtonDblClk(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCMButtonUp(HWND hwnd, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCPaint(HWND hwnd, HRGN hrgn) noexcept
{
}

void IRA_Dialog::NCRButtonDblClk(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) noexcept
{
}

void IRA_Dialog::NCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest) noexcept
{
}

HWND IRA_Dialog::NextDlgCtl(HWND hwnd, HWND hwndSetFocus, BOOL fNext) noexcept
{
    return HWND();
}

void IRA_Dialog::Paint(HWND hwnd) noexcept
{
}

void IRA_Dialog::PaintClipboard(HWND hwnd, HWND hwndCBViewer, const LPPAINTSTRUCT lpPaintStruct) noexcept
{
}

void IRA_Dialog::PaletteChanged(HWND hwnd, HWND hwndPaletteChange) noexcept
{
}

void IRA_Dialog::PaletteIsChanging(HWND hwnd, HWND hwndPaletteChange) noexcept
{
}

void IRA_Dialog::ParentNotify(HWND hwnd, UINT msg, HWND hwndChild, int idChild) noexcept
{
}

void IRA_Dialog::Paste(HWND hwnd) noexcept
{
}

void IRA_Dialog::Power(HWND hwnd, int code) noexcept
{
}

HICON IRA_Dialog::QueryDragIcon(HWND hwnd) noexcept
{
    return HICON();
}

BOOL IRA_Dialog::QueryEndSession(HWND hwnd) noexcept
{
    return 0;
}

BOOL IRA_Dialog::QueryNewPalette(HWND hwnd) noexcept
{
    return 0;
}

BOOL IRA_Dialog::QueryOpen(HWND hwnd) noexcept
{
    return 0;
}

void IRA_Dialog::QueueSync(HWND hwnd) noexcept
{
}

void IRA_Dialog::Quit(_UNUSED HWND hwnd, int exitCode) noexcept
{
    ::PostQuitMessage(exitCode);
}

void IRA_Dialog::RButtonDblClk(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) noexcept
{
}

void IRA_Dialog::RButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) noexcept
{
}

void IRA_Dialog::RButtonUp(HWND hwnd, int x, int y, UINT flags) noexcept
{
}

void IRA_Dialog::RenderAllFormats(HWND hwnd) noexcept
{
}

HANDLE IRA_Dialog::RenderFormat(HWND hwnd, UINT fmt) noexcept
{
    return HANDLE();
}

BOOL IRA_Dialog::SetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg) noexcept
{
    return 0;
}

void IRA_Dialog::SetFocus(HWND hwnd, HWND hwndOldFocus) noexcept
{
    KillFocus(hwndOldFocus, hwnd);
}

void IRA_Dialog::SetFont(HWND hwndCtl, HFONT hfont, BOOL fRedraw) noexcept
{
    SetWindowFont(hwndCtl, hfont, fRedraw);
}

void IRA_Dialog::SetRedraw(HWND hwnd, BOOL fRedraw) noexcept
{
    SetWindowRedraw(hwnd, fRedraw);
}

void IRA_Dialog::SetText(HWND hwnd, LPCTSTR lpszText) noexcept
{
    SetWindowText(hwnd, lpszText);
}

void IRA_Dialog::ShowWindow(HWND hwnd, BOOL fShow, UINT status) noexcept
{
    ::ShowWindow(hwnd, fShow | status);
}

void IRA_Dialog::Resize(HWND hwnd, UINT state, int cx, int cy) noexcept
{
}

void IRA_Dialog::OnSizeClipboard(HWND hwnd, HWND hwndCBViewer, const LPRECT lprc) noexcept
{
}

void IRA_Dialog::OnSpoolerStatus(HWND hwnd, UINT status, int cJobInQueue) noexcept
{
}

void IRA_Dialog::OnSysChar(HWND hwnd, TCHAR ch, int cRepeat) noexcept
{
}

void IRA_Dialog::OnSysColorChange(HWND hwnd) noexcept
{
}

void IRA_Dialog::OnSysCommand(HWND hwnd, UINT cmd, int x, int y) noexcept
{
}

void IRA_Dialog::OnSysDeadChar(HWND hwnd, TCHAR ch, int cRepeat) noexcept
{
}

void IRA_Dialog::OnSysKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) noexcept
{
}

void IRA_Dialog::OnSysKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) noexcept
{
}

void IRA_Dialog::OnSystemError(HWND hwnd, int errCode) noexcept
{
}

void IRA_Dialog::OnTimeChange(HWND hwnd) noexcept
{
}

void IRA_Dialog::OnTimer(HWND hwnd, UINT id) noexcept
{
    SetTimer(hwnd, id, GetTickCount(), ra::TimerProc);
}

void IRA_Dialog::Undo(HWND hwnd) noexcept
{
    Edit_Undo(hwnd);
}

int IRA_Dialog::VkeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret) noexcept
{
    return 0;
}

void IRA_Dialog::VScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos) noexcept
{
}

void IRA_Dialog::VScrollClipboard(HWND hwnd, HWND hwndCBViewer, UINT code, int pos) noexcept
{
}

void IRA_Dialog::WindowPosChanged(HWND hwnd, const LPWINDOWPOS lpwpos) noexcept
{
}

BOOL IRA_Dialog::WindowPosChanging(HWND hwnd, LPWINDOWPOS lpwpos) noexcept
{
    return 0;
}

void IRA_Dialog::WinIniChange(HWND hwnd, LPCTSTR lpszSectionName) noexcept
{
}

INT_PTR IRA_Dialog::DoModal(HWND hParent) noexcept
{
    m_bIsModal = true;

    if (!hParent)
        hParent = g_RAMainWnd;
    // Does g_RAMainWnd have a window class? (for find window)
    return ::DialogBoxParam(::GetModuleHandle("RA_Integration.dll"), MAKEINTRESOURCE(m_nResId), hParent,
        ra::DialogProc, reinterpret_cast<LPARAM>(this));
}

_Use_decl_annotations_
HWND IRA_Dialog::Create(HWND hParent) noexcept
{
    if (!hParent)
        hParent = g_RAMainWnd;

    // Does g_RAMainWnd have a window class? (for find window)
    return CreateDialogParam(::GetModuleHandle("RA_Integration.dll"), MAKEINTRESOURCE(m_nResId), hParent,
        ra::DialogProc, reinterpret_cast<LPARAM>(this));
}

_Use_decl_annotations_
STDMETHODIMP_(VOID) TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) noexcept
{
    /*SetTimer(hwnd, idEvent, dwTime, ra::TimerProc);*/
}

// This is the "main" function so to speak, this is more of a message loop
STDMETHODIMP_(INT_PTR) DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    if (uMsg == WM_INITDIALOG)
        ::SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);

    // It takes a while for it to get here
    // You have to release it since it's an interface, we just want to make sure it doesn't leak
    if (std::unique_ptr<IRA_Dialog> myDlg{
        reinterpret_cast<IRA_Dialog*>(GetWindowLongPtr(hwndDlg, DWLP_USER))
        }; myDlg != nullptr)
    {
        auto myPtr{ myDlg.release() };
        return myPtr->DialogProc(hwndDlg, uMsg, wParam, lParam);
    }
    else
    {
        auto myPtr{ myDlg.release() };
        myPtr = nullptr;
        return INT_PTR{}; /* This should actually throw but were just return 0 for now */
    }
}

} // namespace ra

