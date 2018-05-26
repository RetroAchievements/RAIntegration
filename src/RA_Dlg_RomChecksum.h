#ifndef RA_DLG_ROMCHECKSUM_H
#define RA_DLG_ROMCHECKSUM_H
#pragma once

#include <wtypes.h>

class RA_Dlg_RomChecksum
{
public:
    static BOOL DoModalDialog();

public:
    static INT_PTR CALLBACK RA_Dlg_RomChecksumProc(HWND, UINT, WPARAM, LPARAM);
};


#endif // !RA_DLG_ROMCHECKSUM_H
