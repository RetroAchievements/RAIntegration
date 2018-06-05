#pragma once

#include "RA_Defs.h"
#include <memory>

class Dlg_RichPresence
{
    // Jumping the gun a little but this is so we don't need constructors defined
    using WindowH         = std::unique_ptr<std::remove_pointer_t<HWND>, decltype(&::DestroyWindow)>;
    using FontDeleterType = BOOL(CALLBACK*)(HFONT);
    using FontDeleter     = decltype(reinterpret_cast<FontDeleterType>(&::DeleteObject));
    using FontH           = std::unique_ptr<std::remove_pointer_t<HFONT>, FontDeleter>;

public:
    static INT_PTR CALLBACK s_RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR CALLBACK RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);

    void InstallHWND(HWND hWnd) {
        WindowH myWindow{ hWnd, &::DestroyWindow };
        m_hRichPresenceDialog.swap(myWindow);
    }
    HWND GetHWND() const { return m_hRichPresenceDialog.get(); }

    void StartMonitoring();

private:
    void StartTimer();
    void StopTimer();


    FontH m_hFont{ nullptr, reinterpret_cast<FontDeleterType>(&::DeleteObject) };  
    WindowH m_hRichPresenceDialog{ nullptr, &::DestroyWindow };
    WindowH m_ctlFont{ nullptr, &::DestroyWindow };

    // Gave a misalignment error
#pragma pack(push, 1)
    bool m_bTimerActive{ false };
#pragma pack(pop)
};

extern Dlg_RichPresence g_RichPresenceDialog;


