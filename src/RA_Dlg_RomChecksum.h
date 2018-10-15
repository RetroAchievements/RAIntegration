#ifndef RA_DLG_ROMCHECKSUM_H
#define RA_DLG_ROMCHECKSUM_H
#pragma once

class RA_Dlg_RomChecksum
{
public:
    static INT_PTR DoModalDialog() noexcept;

public:
    static INT_PTR CALLBACK RA_Dlg_RomChecksumProc(HWND, UINT, WPARAM, [[maybe_unused]] LPARAM);
};


#endif // !RA_DLG_ROMCHECKSUM_H
