#ifndef IRA_DIALOG_H
#define IRA_DIALOG_H
#pragma once

#include <WTypes.h>
#include <yvals.h>
#include "IRA_WndClass.h"

// Credits
// Most respect for hernandp for is Message Cracker Wizard
// Most respect to Microsoft for their documentation

#ifndef _NORETURN
#define _NORETURN [[noreturn]]
#endif // !_NORETURN


namespace ra {



class IRA_Dialog
{
public:
    IRA_Dialog(_In_ int ResID, _In_ const IRA_WndClass& myWndClass, _In_ HWND hParent = nullptr) noexcept;
    // use WM_NCDESTROY for most things
    virtual ~IRA_Dialog() noexcept = default;;
    IRA_Dialog(const IRA_Dialog&) noexcept = default;
    IRA_Dialog& operator=(const IRA_Dialog&) noexcept = default;
    IRA_Dialog(IRA_Dialog&&) noexcept = default;
    IRA_Dialog& operator=(IRA_Dialog&&) noexcept = default;
    IRA_Dialog() noexcept = default;

public:
#pragma region Event Handlers
    // HANDLE_WM_ACTIVATE
/// <summary>Activates the specified HWND.</summary>
/// <param name="hwnd">The handle to this window.</param>
/// <param name="state">
///   Specifies whether the window is being activated or deactivated. Maps to
///   the low-order word of <c>wParam</c>
/// </param>
/// <param name="hwndActDeact">
///   A handle to the window being activated or deactivated, depending on the
///   value of the <paramref name="state " /> parameter.
/// </param>
/// <param name="fMinimized">
///   Specifies the minimized state of the window being activated or
///   deactivated. Maps to the high-order word of <c>wParam</c>.
/// </param>
/// <returns>If an application processes this message, it should return zero.</returns>
    _NORETURN virtual void
        Activate(_In_ HWND hwnd, _In_ UINT state, _In_ HWND hwndActDeact, _In_ BOOL fMinimized) noexcept;

    // HANDLE_WM_ACTIVATEAPP
    /// <summary>Activates the application.</summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fActivate">
    ///   Indicates whether the window is being activated or deactivated.
    /// </param>
    /// <param name="dwThreadId">
    ///   The thread identifier. If the <paramref name="fActivate" /> parameter
    ///   is <c>TRUE</c>, this is the identifier of the thread that owns the
    ///   window being deactivated. If <paramref name="fActivate" /> is
    ///   <c>FALSE</c>, this is the identifier of the thread that owns the window
    ///   being activated.
    /// </param>
    /// <returns>
    ///   If an application processes this message, it should return zero.
    /// </returns>
    _NORETURN virtual void
        ActivateApp(_In_ HWND hwnd, _In_ BOOL fActivate, _In_ DWORD dwThreadId) noexcept;

    // HANDLE_WM_ASKCBFORMATNAME
    /// <summary>
    ///   Sent to the clipboard owner by a clipboard viewer window to request the
    ///   name of a <c>CF_OWNERDISPLAY</c> clipboard format.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="cchMax">
    ///   The size, in characters, of the buffer pointed to by the
    ///   <paramref name="rgchName" /> parameter.
    /// </param>
    /// <param name="rgchName">
    ///   A pointer to the buffer that is to receive the clipboard format name.
    /// </param>
    /// <returns>
    ///   If an application processes this message, it should return zero.
    /// </returns>
    _NORETURN virtual void
        AskCBFormatName(_In_ HWND hwnd, _In_ int cchMax, _In_ LPTSTR rgchName) noexcept;

    // HANDLE_WM_CANCELMODE
    /// <summary>Sent to cancel certain modes, such as mouse capture.</summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns>
    ///   If an application processes this message, it should return zero.
    /// </returns>
    _NORETURN virtual void
        CancelMode(_In_ HWND hwnd) noexcept;

    // Didn't find documentation for this
    // HANDLE_WM_CHANGECBCHAIN
    /// <summary>Alters the combobox chain</summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndRemove">The hwnd to remove ( <c>wParam</c>).</param>
    /// <param name="hwndNext">
    ///   The next ComboBox in the chain ( <c>lParam</c>).
    /// </param>
    /// <returns>
    ///   If an application processes this message, it should return zero.
    /// </returns>
    _NORETURN virtual void
        ChangeCBChain(_In_ HWND hwnd, _In_ HWND hwndRemove, _In_ HWND hwndNext) noexcept;

    // HANDLE_WM_CHAR
    /// <summary>Called on WM_CHAR.</summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="ch">The character code of the key.</param>
    /// <param name="cRepeat">The repeat count.</param>
    /// <returns>
    ///   An application should return zero if it processes this message.
    /// </returns>
    _NORETURN virtual void
        OnChar(_In_ HWND hwnd, _In_ TCHAR ch, _In_ int cRepeat) noexcept;

    // HANDLE_WM_CHARTOITEM
    /// <summary>
    ///   Called when a list box with the LBS_WANTKEYBOARDINPUT style to its
    ///   owner in response to a WM_CHAR message..
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="ch">
    ///   specifies the character code of the key the user pressed. (LOWORD of
    ///   <c>wParam</c>)
    /// </param>
    /// <param name="hwndListbox">Handle to the list box.</param>
    /// <param name="iCaret">
    ///   Current position of the caret.(HIWORD of <c>wParam</c>)
    /// </param>
    /// <returns>
    ///   The return value specifies the action that the application performed in
    ///   response to the message. A return value of –1 or –2 indicates that the
    ///   application handled all aspects of selecting the item and requires no
    ///   further action by the list box. A return value of 0 or greater
    ///   specifies the zero-based index of an item in the list box and indicates
    ///   that the list box should perform the default action for the keystroke
    ///   on the specified item.
    /// </returns>
    _NODISCARD virtual int
        OnCharToItem(_In_ HWND hwnd, _In_ UINT ch, _In_ HWND hwndListbox, _In_ int iCaret) noexcept;


    /// <summary>
    /// Called when [child activate].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnChildActivate(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Clears the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Clear(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Closes the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Close(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Called when [command].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="id">The identifier.</param>
    /// <param name="hwndCtl">The HWND control.</param>
    /// <param name="codeNotify">The code notify.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnCommand(_In_ HWND hwnd, _In_ int id, _In_ HWND hwndCtl, _In_ UINT codeNotify) noexcept;


    /// <summary>
    /// Comms the notify.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="cid">The cid.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        CommNotify(_In_ HWND hwnd, _In_ int cid, _In_ UINT flags) noexcept;


    /// <summary>
    /// Called when [compacting].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="compactRatio">The compact ratio.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnCompacting(_In_ HWND hwnd, _In_ UINT compactRatio) noexcept;


    /// <summary>
    /// Compares the item.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpCompareItem">The lp compare item.</param>
    /// <returns></returns>
    _NODISCARD virtual int
        CompareItem(_In_ HWND hwnd, _In_ const COMPAREITEMSTRUCT* lpCompareItem) noexcept;


    /// <summary>
    /// Contexts the menu.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndContext">The HWND context.</param>
    /// <param name="xPos">The x position.</param>
    /// <param name="yPos">The y position.</param>
    /// <returns></returns>
    _NORETURN virtual void
        ContextMenu(_In_ HWND hwnd, _In_ HWND hwndContext, _In_ UINT xPos, _In_ UINT yPos) noexcept;


    /// <summary>
    /// Copies the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Copy(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Copies the data.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndFrom">The HWND from.</param>
    /// <param name="pcds">The PCDS.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        CopyData(_In_ HWND hwnd, _In_ HWND hwndFrom, _In_ PCOPYDATASTRUCT pcds) noexcept;


    /// <summary>
    /// Creates the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpCreateStruct">The lp create structure.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        OnCreate(_In_ HWND hwnd, _In_ LPCREATESTRUCT lpCreateStruct) noexcept;

    /// <summary>
    /// Called when [control color BTN].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="type">The type.</param>
    /// <returns></returns>
    _NODISCARD virtual HBRUSH
        OnCtlColorBtn(_In_ HWND hwnd, _In_ HDC hdc, _In_ HWND hwndChild, _In_ int type) noexcept;


    /// <summary>
    /// Called when [control color dialog].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="type">The type.</param>
    /// <returns></returns>
    _NODISCARD virtual HBRUSH
        OnCtlColorDlg(_In_ HWND hwnd, _In_ HDC hdc, _In_ HWND hwndChild, _In_ int type) noexcept;

    /// <summary>
    /// Called when [control color edit].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="type">The type.</param>
    /// <returns></returns>
    _NODISCARD virtual HBRUSH
        OnCtlColorEdit(_In_ HWND hwnd, _In_ HDC hdc, _In_ HWND hwndChild, _In_ int type) noexcept;


    /// <summary>
    /// Called when [control color listbox].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="type">The type.</param>
    /// <returns></returns>
    _NODISCARD virtual HBRUSH
        OnCtlColorListbox(_In_ HWND hwnd, _In_ HDC hdc, _In_ HWND hwndChild, _In_ int type) noexcept;


    /// <summary>
    /// Called when [control color msgbox].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="type">The type.</param>
    /// <returns></returns>
    _NODISCARD virtual HBRUSH
        OnCtlColorMsgbox(_In_ HWND hwnd, _In_ HDC hdc, _In_ HWND hwndChild, _In_ int type) noexcept;


    /// <summary>
    /// Called when [control color scrollbar].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="type">The type.</param>
    /// <returns></returns>
    _NODISCARD virtual HBRUSH
        OnCtlColorScrollbar(_In_ HWND hwnd, _In_ HDC hdc, _In_ HWND hwndChild, _In_ int type) noexcept;


    /// <summary>
    /// Called when [control color static].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="type">The type.</param>
    /// <returns></returns>
    _NODISCARD virtual HBRUSH
        OnCtlColorStatic(_In_ HWND hwnd, _In_ HDC hdc, _In_ HWND hwndChild, _In_ int type) noexcept;


    /// <summary>
    /// Cuts the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Cut(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Deads the character.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="ch">The ch.</param>
    /// <param name="cRepeat">The c repeat.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DeadChar(_In_ HWND hwnd, _In_ TCHAR ch, _In_ int cRepeat) noexcept;


    /// <summary>
    /// Deletes the item.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpDeleteItem">The lp delete item.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DeleteItem(_In_ HWND hwnd, _In_ const DELETEITEMSTRUCT* lpDeleteItem) noexcept;


    /// <summary>
    /// Destroys the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Destroy(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Destroys the clipboard.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DestroyClipboard(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Devices the change.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="uEvent">The u event.</param>
    /// <param name="dwEventData">The dw event data.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        DeviceChange(_In_ HWND hwnd, _In_ UINT uEvent, _In_ DWORD dwEventData) noexcept;


    /// <summary>
    /// Devs the mode change.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpszDeviceName">Name of the LPSZ device.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DevModeChange(_In_ HWND hwnd, _In_ LPCTSTR lpszDeviceName) noexcept;


    /// <summary>
    /// Displays the change.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="bitsPerPixel">The bits per pixel.</param>
    /// <param name="cxScreen">The cx screen.</param>
    /// <param name="cyScreen">The cy screen.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DisplayChange(_In_ HWND hwnd, _In_ UINT bitsPerPixel, _In_ UINT cxScreen, _In_ UINT cyScreen) noexcept;


    /// <summary>
    /// Draws the clipboard.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DrawClipboard(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Draws the item.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpDrawItem">The lp draw item.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DrawItem(_In_ HWND hwnd, _In_ const DRAWITEMSTRUCT* lpDrawItem) noexcept;


    /// <summary>
    /// Drops the files.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdrop">The hdrop.</param>
    /// <returns></returns>
    _NORETURN virtual void
        DropFiles(_In_ HWND hwnd, _In_ HDROP hdrop) noexcept;


    /// <summary>
    /// Enables the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fEnable">The f enable.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Enable(_In_ HWND hwnd, _In_ BOOL fEnable = TRUE) noexcept;


    /// <summary>
    /// Ends the session.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fEnding">The f ending.</param>
    /// <returns></returns>
    _NORETURN virtual void
        EndSession(_In_ HWND hwnd, _In_ BOOL fEnding) noexcept;


    /// <summary>
    /// Enters the idle.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="source">The source.</param>
    /// <param name="hwndSource">The HWND source.</param>
    /// <returns></returns>
    _NORETURN virtual void
        EnterIdle(_In_ HWND hwnd, _In_ UINT source, _In_ HWND hwndSource) noexcept;


    /// <summary>
    /// Erases the BKGND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        EraseBkgnd(_In_ HWND hwnd, _In_ HDC hdc) noexcept;


    /// <summary>
    /// Fonts the change.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        FontChange(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Gets the dialog code.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpmsg">The LPMSG.</param>
    /// <returns></returns>
    _NODISCARD virtual UINT
        GetDlgCode(_In_ HWND hwnd, _In_ LPMSG lpmsg) noexcept;


    /// <summary>
    /// Gets the font.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NODISCARD virtual HFONT
        GetFont(_In_ HWND hwnd);


    /// <summary>
    /// Gets the minimum maximum information.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpMinMaxInfo">The lp minimum maximum information.</param>
    /// <returns></returns>
    _NORETURN virtual void
        GetMinMaxInfo(_In_ HWND hwnd, _In_ LPMINMAXINFO lpMinMaxInfo) noexcept;


    /// <summary>
    /// Gets the text.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="cchTextMax">The CCH text maximum.</param>
    /// <param name="lpszText">The LPSZ text.</param>
    /// <returns></returns>
    _NODISCARD virtual INT
        GetText(_In_ HWND hwnd, _In_ int cchTextMax, _In_ LPTSTR lpszText) noexcept;

    /// <summary>
    /// Gets the length of the text.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NODISCARD virtual INT
        GetTextLength(_In_ HWND hwnd) noexcept;

    /// <summary>
    /// Hots the key.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="idHotKey">The identifier hot key.</param>
    /// <param name="fuModifiers">The fu modifiers.</param>
    /// <param name="vk">The vk.</param>
    /// <returns></returns>
    _NORETURN virtual void
        HotKey(_In_ HWND hwnd, _In_ int idHotKey, _In_ UINT fuModifiers, _In_ UINT vk) noexcept;


    /// <summary>
    /// hes the scroll.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCtl">The HWND control.</param>
    /// <param name="code">The code.</param>
    /// <param name="pos">The position.</param>
    /// <returns></returns>
    _NORETURN virtual void
        HScroll(_In_ HWND hwnd, _In_ HWND hwndCtl, _In_ UINT code, _In_ int pos) noexcept;


    /// <summary>
    /// hes the scroll clipboard.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCBViewer">The HWND cb viewer.</param>
    /// <param name="code">The code.</param>
    /// <param name="pos">The position.</param>
    /// <returns></returns>
    _NORETURN virtual void
        HScrollClipboard(_In_ HWND hwnd, _In_ HWND hwndCBViewer, _In_ UINT code, _In_ int pos) noexcept;


    /// <summary>
    /// Icons the erase BKGND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hdc">The HDC.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        IconEraseBkgnd(_In_ HWND hwnd, _In_ HDC hdc) noexcept;


    /// <summary>
    /// Initializes the dialog.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndFocus">The HWND focus.</param>
    /// <param name="lParam">The l parameter.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        InitDialog(_In_ HWND hwnd, _In_ HWND hwndFocus, _In_ LPARAM lParam) noexcept;


    /// <summary>
    /// Initializes the menu.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hMenu">The h menu.</param>
    /// <returns></returns>
    _NORETURN virtual void
        InitMenu(_In_ HWND hwnd, _In_ HMENU hMenu) noexcept;

    /// <summary>
    /// Initializes the menu popup.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hMenu">The h menu.</param>
    /// <param name="item">The item.</param>
    /// <param name="fSystemMenu">The f system menu.</param>
    /// <returns></returns>
    _NORETURN virtual void
        InitMenuPopup(_In_ HWND hwnd, _In_ HMENU hMenu, _In_ UINT item, _In_ BOOL fSystemMenu) noexcept;


    /// <summary>
    /// Keys down.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="vk">The vk.</param>
    /// <param name="fDown">The f down.</param>
    /// <param name="cRepeat">The c repeat.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        KeyDown(_In_ HWND hwnd, _In_ UINT vk, _In_ BOOL fDown, _In_ int cRepeat, _In_ UINT flags) noexcept;


    /// <summary>
    /// Keys up.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="vk">The vk.</param>
    /// <param name="fDown">The f down.</param>
    /// <param name="cRepeat">The c repeat.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        KeyUp(_In_ HWND hwnd, _In_ UINT vk, _In_ BOOL fDown, _In_ int cRepeat, _In_ UINT flags) noexcept;


    /// <summary>
    /// Kills the focus.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndNewFocus">The HWND new focus.</param>
    /// <returns></returns>
    _NORETURN virtual void
        KillFocus(_In_ HWND hwnd, _In_ HWND hwndNewFocus) noexcept;


    /// <summary>
    /// ls the button double CLK.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        LButtonDblClk(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;


    /// <summary>
    /// ls the button down.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        LButtonDown(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;


    /// <summary>
    /// ls the button up.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        LButtonUp(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;


    /// <summary>
    /// ms the button DBLCLK.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MButtonDblclk(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;


    /// <summary>
    /// ms the button down.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MButtonDown(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;


    /// <summary>
    /// ms the button up.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MButtonUp(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT flags) noexcept;


    /// <summary>
    /// MDIs the activate.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fActive">The f active.</param>
    /// <param name="hwndActivate">The HWND activate.</param>
    /// <param name="hwndDeactivate">The HWND deactivate.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MDIActivate(_In_ HWND hwnd, _In_ BOOL fActive, _In_ HWND hwndActivate, _In_ HWND hwndDeactivate) noexcept;


    /// <summary>
    /// MDIs the cascade.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="cmd">The command.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        MDICascade(_In_ HWND hwnd, _In_ UINT cmd) noexcept;


    /// <summary>
    /// MDIs the create.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpmcs">The LPMCS.</param>
    /// <returns></returns>
    _NODISCARD virtual HWND
        MDICreate(_In_ HWND hwnd, _In_ const LPMDICREATESTRUCT lpmcs) noexcept;


    /// <summary>
    /// MDIs the destroy.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndDestroy">The HWND destroy.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MDIDestroy(_In_ HWND hwnd, _In_ HWND hwndDestroy) noexcept;


    /// <summary>
    /// MDIs the get active.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NODISCARD virtual HWND
        MDIGetActive(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// MDIs the icon arrange.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MDIIconArrange(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// MDIs the maximize.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndMaximize">The HWND maximize.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MDIMaximize(_In_ HWND hwnd, _In_ HWND hwndMaximize) noexcept;


    /// <summary>
    /// MDIs the next.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCur">The HWND current.</param>
    /// <param name="fPrev">The f previous.</param>
    /// <returns></returns>
    _NODISCARD virtual HWND
        MDINext(_In_ HWND hwnd, _In_ HWND hwndCur, _In_ BOOL fPrev) noexcept;


    /// <summary>
    /// MDIs the restore.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndRestore">The HWND restore.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MDIRestore(_In_ HWND hwnd, _In_ HWND hwndRestore) noexcept;


    /// <summary>
    /// MDIs the set menu.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fRefresh">The f refresh.</param>
    /// <param name="hmenuFrame">The hmenu frame.</param>
    /// <param name="hmenuWindow">The hmenu window.</param>
    /// <returns></returns>
    _NODISCARD virtual HMENU
        MDISetMenu(_In_ HWND hwnd, _In_ BOOL fRefresh, _In_ HMENU hmenuFrame, _In_ HMENU hmenuWindow) noexcept;


    /// <summary>
    /// Called when [MDI tile].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="cmd">The command.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        OnMDITile(_In_ HWND hwnd, _In_ UINT cmd) noexcept;


    /// <summary>
    /// Measures the item.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpMeasureItem">The lp measure item.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MeasureItem(_In_ HWND hwnd, _In_ MEASUREITEMSTRUCT * lpMeasureItem) noexcept;


    /// <summary>
    /// Menus the character.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="ch">The ch.</param>
    /// <param name="flags">The flags.</param>
    /// <param name="hmenu">The hmenu.</param>
    /// <returns></returns>
    _NODISCARD virtual DWORD
        MenuChar(_In_ HWND hwnd, _In_ UINT ch, _In_ UINT flags, _In_ HMENU hmenu) noexcept;

    /// <summary>
    /// Menus the select.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hmenu">The hmenu.</param>
    /// <param name="item">The item.</param>
    /// <param name="hmenuPopup">The hmenu popup.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MenuSelect(_In_ HWND hwnd, _In_ HMENU hmenu, _In_ int item, _In_ HMENU hmenuPopup,
            _In_ UINT flags) noexcept;


    /// <summary>
    /// Mouses the activate.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndTopLevel">The HWND top level.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <param name="msg">The MSG.</param>
    /// <returns></returns>
    _NODISCARD virtual int
        MouseActivate(_In_ HWND hwnd, _In_ HWND hwndTopLevel, _In_ UINT codeHitTest, _In_ UINT msg) noexcept;


    /// <summary>
    /// Mouses the move.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MouseMove(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;


    /// <summary>
    /// Mouses the wheel.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="xPos">The x position.</param>
    /// <param name="yPos">The y position.</param>
    /// <param name="zDelta">The z delta.</param>
    /// <param name="fwKeys">The fw keys.</param>
    /// <returns></returns>
    _NORETURN virtual void
        MouseWheel(_In_ HWND hwnd, _In_ int xPos, _In_ int yPos, _In_ int zDelta, _In_ UINT fwKeys) noexcept;

    /// <summary>
    /// Moves the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Move(_In_ HWND hwnd, _In_ int x, _In_ int y) noexcept;


    /// <summary>
    /// Ncs the activate.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fActive">The f active.</param>
    /// <param name="hwndActDeact">The HWND act deact.</param>
    /// <param name="fMinimized">The f minimized.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        NCActivate(_In_ HWND hwnd, _In_ BOOL fActive, _In_ HWND hwndActDeact, _In_ BOOL fMinimized) noexcept;


    _NODISCARD virtual UINT
        NCCalcSize(_In_ HWND hwnd, _In_ BOOL fCalcValidRects, _In_ NCCALCSIZE_PARAMS* lpcsp) noexcept;


    /// <summary>
    /// Ncs the create.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpCreateStruct">The lp create structure.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        NCCreate(_In_ HWND hwnd, _In_ LPCREATESTRUCT lpCreateStruct) noexcept;


    /// <summary>
    /// Ncs the destroy.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCDestroy(_In_ HWND hwnd) noexcept = 0;

    /// <summary>
    /// Ncs the hit test.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <returns></returns>
    _NODISCARD virtual UINT
        NCHitTest(_In_ HWND hwnd, _In_ int x, _In_ int y) noexcept;


    /// <summary>
    /// NCLs the button double CLK.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCLButtonDblClk(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y,
            _In_ UINT codeHitTest) noexcept;

    /// <summary>
    /// NCLs the button down.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCLButtonDown(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y, _In_ UINT codeHitTest) noexcept;

    /// <summary>
    /// NCLs the button up.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCLButtonUp(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT codeHitTest) noexcept;

    /// <summary>
    /// NCMs the button double CLK.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCMButtonDblClk(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y,
            _In_ UINT codeHitTest) noexcept;

    /// <summary>
    /// NCMs the button down.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN  virtual void
        NCMButtonDown(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y,
            _In_ UINT codeHitTest) noexcept;


    /// <summary>
    /// NCMs the button up.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCMButtonUp(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT codeHitTest) noexcept;


    /// <summary>
    /// Posts the nc mouse move.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCMouseMove(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT codeHitTest) noexcept;

    /// <summary>
    /// Ncs the paint.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hrgn">The HRGN.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCPaint(_In_ HWND hwnd, _In_ HRGN hrgn) noexcept;


    /// <summary>
    /// NCRs the button double CLK.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCRButtonDblClk(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y,
            _In_ UINT codeHitTest) noexcept;


    /// <summary>
    /// NCRs the button down.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN virtual void
        NCRButtonDown(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y,
            _In_ UINT codeHitTest) noexcept;


    /// <summary>
    /// NCRs the button up.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <returns></returns>
    _NORETURN  virtual void
        NCRButtonUp(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT codeHitTest) noexcept;


    /// <summary>
    /// Nexts the dialog control.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndSetFocus">The HWND set focus.</param>
    /// <param name="fNext">The f next.</param>
    /// <returns></returns>
    _NODISCARD virtual HWND
        NextDlgCtl(_In_ HWND hwnd, _In_ HWND hwndSetFocus, _In_ BOOL fNext) noexcept;

    /// <summary>
    /// Paints the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Paint(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Paints the clipboard.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCBViewer">The HWND cb viewer.</param>
    /// <param name="lpPaintStruct">The lp paint structure.</param>
    /// <returns></returns>
    _NORETURN virtual void
        PaintClipboard(_In_ HWND hwnd, _In_ HWND hwndCBViewer, _In_ const LPPAINTSTRUCT lpPaintStruct) noexcept;

    /// <summary>
    /// Palettes the changed.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndPaletteChange">The HWND palette change.</param>
    /// <returns></returns>
    _NORETURN virtual void
        PaletteChanged(_In_ HWND hwnd, _In_ HWND hwndPaletteChange) noexcept;


    /// <summary>
    /// Palettes the is changing.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndPaletteChange">The HWND palette change.</param>
    /// <returns></returns>
    _NORETURN virtual void
        PaletteIsChanging(_In_ HWND hwnd, _In_ HWND hwndPaletteChange) noexcept;


    /// <summary>
    /// Parents the notify.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="msg">The MSG.</param>
    /// <param name="hwndChild">The HWND child.</param>
    /// <param name="idChild">The identifier child.</param>
    /// <returns></returns>
    _NORETURN virtual void
        ParentNotify(_In_ HWND hwnd, _In_ UINT msg, _In_ HWND hwndChild, _In_ int idChild) noexcept;


    /// <summary>
    /// Pastes the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Paste(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Powers the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="code">The code.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Power(_In_ HWND hwnd, _In_ int code) noexcept;

    /// <summary>
    /// Queries the drag icon.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NODISCARD virtual HICON
        QueryDragIcon(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Queries the end session.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        QueryEndSession(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Queries the new palette.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        QueryNewPalette(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Queries the open.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        QueryOpen(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Queues the synchronize.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        QueueSync(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Quits the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="exitCode">The exit code.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Quit(_In_ HWND hwnd, _In_ int exitCode) noexcept;

    /// <summary>
    /// rs the button double CLK.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        RButtonDblClk(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;

    /// <summary>
    /// rs the button down.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fDoubleClick">The f double click.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="keyFlags">The key flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        RButtonDown(_In_ HWND hwnd, _In_ BOOL fDoubleClick, _In_ int x, _In_ int y, _In_ UINT keyFlags) noexcept;


    /// <summary>
    /// rs the button up.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        RButtonUp(_In_ HWND hwnd, _In_ int x, _In_ int y, _In_ UINT flags) noexcept;


    /// <summary>
    /// Renders all formats.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        RenderAllFormats(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Renders the format.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fmt">The FMT.</param>
    /// <returns></returns>
    _NODISCARD virtual HANDLE
        RenderFormat(_In_ HWND hwnd, _In_ UINT fmt) noexcept;


    /// <summary>
    /// Sets the cursor.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCursor">The HWND cursor.</param>
    /// <param name="codeHitTest">The code hit test.</param>
    /// <param name="msg">The MSG.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        SetCursor(_In_ HWND hwnd, _In_ HWND hwndCursor, _In_ UINT codeHitTest, _In_ UINT msg) noexcept;


    /// <summary>
    /// Sets the focus.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndOldFocus">The HWND old focus.</param>
    /// <returns></returns>
    _NORETURN virtual void
        SetFocus(_In_ HWND hwnd, _In_ HWND hwndOldFocus) noexcept;


    /// <summary>
    /// Sets the font.
    /// </summary>
    /// <param name="hwndCtl">The HWND control.</param>
    /// <param name="hfont">The hfont.</param>
    /// <param name="fRedraw">The f redraw.</param>
    /// <returns></returns>
    _NORETURN virtual void
        SetFont(_In_ HWND hwndCtl, _In_ HFONT hfont, _In_ BOOL fRedraw) noexcept;


    /// <summary>
    /// Sets the redraw.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fRedraw">The f redraw.</param>
    /// <returns></returns>
    _NORETURN virtual void
        SetRedraw(_In_ HWND hwnd, _In_ BOOL fRedraw) noexcept;


    /// <summary>
    /// Sets the text.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpszText">The LPSZ text.</param>
    /// <returns></returns>
    _NORETURN virtual void
        SetText(_In_ HWND hwnd, _In_ LPCTSTR lpszText) noexcept;


    /// <summary>
    /// Shows the window.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="fShow">The f show.</param>
    /// <param name="status">The status.</param>
    /// <returns></returns>
    _NORETURN virtual void
        ShowWindow(_In_ HWND hwnd, _In_ BOOL fShow, _In_ UINT status) noexcept;


    /// <summary>
    /// Resizes the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="state">The state.</param>
    /// <param name="cx">The cx.</param>
    /// <param name="cy">The cy.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Resize(_In_ HWND hwnd, _In_ UINT state, _In_ int cx, _In_ int cy) noexcept;


    /// <summary>
    /// Called when [size clipboard].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCBViewer">The HWND cb viewer.</param>
    /// <param name="lprc">The LPRC.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSizeClipboard(_In_ HWND hwnd, _In_ HWND hwndCBViewer, _In_ const LPRECT lprc) noexcept;


    /// <summary>
    /// Called when [spooler status].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="status">The status.</param>
    /// <param name="cJobInQueue">The c job in queue.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSpoolerStatus(_In_ HWND hwnd, _In_ UINT status, _In_ int cJobInQueue) noexcept;


    /// <summary>
    /// Called when [system character].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="ch">The ch.</param>
    /// <param name="cRepeat">The c repeat.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSysChar(_In_ HWND hwnd, _In_ TCHAR ch, _In_ int cRepeat) noexcept;


    /// <summary>
    /// Called when [system color change].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSysColorChange(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Called when [system command].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="cmd">The command.</param>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSysCommand(_In_ HWND hwnd, _In_ UINT cmd, _In_ int x, _In_ int y) noexcept;


    /// <summary>
    /// Called when [system dead character].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="ch">The ch.</param>
    /// <param name="cRepeat">The c repeat.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSysDeadChar(_In_ HWND hwnd, _In_ TCHAR ch, _In_ int cRepeat) noexcept;


    /// <summary>
    /// Called when [system key down].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="vk">The vk.</param>
    /// <param name="fDown">The f down.</param>
    /// <param name="cRepeat">The c repeat.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSysKeyDown(_In_ HWND hwnd, _In_ UINT vk, _In_ BOOL fDown, _In_ int cRepeat, _In_ UINT flags) noexcept;

    /// <summary>
    /// Called when [system key up].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="vk">The vk.</param>
    /// <param name="fDown">The f down.</param>
    /// <param name="cRepeat">The c repeat.</param>
    /// <param name="flags">The flags.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSysKeyUp(_In_ HWND hwnd, _In_ UINT vk, _In_ BOOL fDown, _In_ int cRepeat, _In_ UINT flags) noexcept;


    /// <summary>
    /// Called when [system error].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="errCode">The error code.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnSystemError(_In_ HWND hwnd, _In_ int errCode) noexcept;


    /// <summary>
    /// Called when [time change].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnTimeChange(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Called when [timer].
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="id">The identifier.</param>
    /// <returns></returns>
    _NORETURN virtual void
        OnTimer(_In_ HWND hwnd, _In_ UINT id) noexcept;


    /// <summary>
    /// Undoes the specified HWND.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <returns></returns>
    _NORETURN virtual void
        Undo(_In_ HWND hwnd) noexcept;


    /// <summary>
    /// Vkeys to item.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="vk">The vk.</param>
    /// <param name="hwndListbox">The HWND listbox.</param>
    /// <param name="iCaret">The i caret.</param>
    /// <returns></returns>
    _NODISCARD virtual int
        VkeyToItem(_In_ HWND hwnd, _In_ UINT vk, _In_ HWND hwndListbox, _In_ int iCaret) noexcept;


    /// <summary>
    /// vs the scroll.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCtl">The HWND control.</param>
    /// <param name="code">The code.</param>
    /// <param name="pos">The position.</param>
    /// <returns></returns>
    _NORETURN virtual void
        VScroll(_In_ HWND hwnd, _In_ HWND hwndCtl, _In_ UINT code, _In_ int pos) noexcept;


    /// <summary>
    /// vs the scroll clipboard.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="hwndCBViewer">The HWND cb viewer.</param>
    /// <param name="code">The code.</param>
    /// <param name="pos">The position.</param>
    /// <returns></returns>
    _NORETURN virtual void
        VScrollClipboard(_In_ HWND hwnd, _In_ HWND hwndCBViewer, _In_ UINT code, _In_ int pos) noexcept;


    /// <summary>
    /// Windows the position changed.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpwpos">The lpwpos.</param>
    /// <returns></returns>
    _NORETURN virtual void
        WindowPosChanged(_In_ HWND hwnd, _In_ const LPWINDOWPOS lpwpos) noexcept;


    /// <summary>
    /// Windows the position changing.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpwpos">The lpwpos.</param>
    /// <returns></returns>
    _NODISCARD virtual BOOL
        WindowPosChanging(_In_ HWND hwnd, _In_ LPWINDOWPOS lpwpos) noexcept;


    /// <summary>
    /// Wins the ini change.
    /// </summary>
    /// <param name="hwnd">The handle to this window.</param>
    /// <param name="lpszSectionName">Name of the LPSZ section.</param>
    /// <returns></returns>
    _NORETURN virtual void
        WinIniChange(_In_ HWND hwnd, _In_ LPCTSTR lpszSectionName) noexcept;
#pragma endregion

    virtual INT_PTR DoModal(_In_ HWND hParent = nullptr) noexcept;
    virtual HWND Create(_In_ HWND hParent = nullptr) noexcept;

public:
    virtual void Show(_In_ HWND hDlg) noexcept { ::ShowWindow(hDlg, SW_SHOW); }
    virtual void Hide(_In_ HWND hDlg) noexcept { ::ShowWindow(hDlg, SW_HIDE); }
    virtual void Minimize(_In_ HWND hDlg) noexcept { ::ShowWindow(hDlg, SW_MINIMIZE); }
    virtual void Maximize(_In_ HWND hDlg) noexcept { ::ShowWindow(hDlg, SW_MAXIMIZE); }

protected:
    // Event handlers for common command IDs
    virtual void OnOK(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDOK); }
    virtual void OnCancel(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDCANCEL); }
    virtual void OnClose(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDCLOSE); }
    virtual void OnAbort(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDABORT); }
    virtual void OnRetry(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDRETRY); }
    virtual void OnIgnore(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDIGNORE); }
    virtual void OnYes(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDYES); }
    virtual void OnNo(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDNO); }
    virtual void OnTryAgain(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDTRYAGAIN); }
    virtual void OnHelp(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDHELP); }
    virtual void OnContinue(_In_ HWND hDlg) noexcept { ::EndDialog(hDlg, IDCONTINUE); }

    virtual INT_PTR DialogProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam,
        _In_ LPARAM lParam) noexcept = 0;

protected:
    LPCTSTR m_lpCaption{ nullptr };

private:
    bool m_bIsModal{ false };
    int m_nResId{ 0 };
    IRA_WndClass m_Wndclass{ TEXT("Dialog") };

    friend STDMETHODIMP_(VOID) TimerProc(_In_ HWND hwnd, _In_ UINT uMsg = WM_TIMER, _In_ UINT_PTR idEvent = 1U,
        _In_ DWORD dwTime = ::GetTickCount()) noexcept;
    friend STDMETHODIMP_(INT_PTR) DialogProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam,
        _In_ LPARAM lParam) noexcept;

};




} // namespace ra

#endif // !IRA_DIALOG_H

