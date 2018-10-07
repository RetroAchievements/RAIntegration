#ifndef RA_DLG_ROMCHECKSUM_H
#define RA_DLG_ROMCHECKSUM_H
#pragma once

#ifndef PCH_H
#include "windows_nodefines.h"
#include <wtypes.h>
#endif /* !PCH_H */

class RA_Dlg_RomChecksum
{
public:
    static BOOL DoModalDialog();

public:
    static INT_PTR CALLBACK RA_Dlg_RomChecksumProc(HWND, UINT, WPARAM, [[maybe_unused]] LPARAM);
};


#endif // !RA_DLG_ROMCHECKSUM_H
