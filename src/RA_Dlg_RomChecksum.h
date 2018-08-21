#ifndef RA_DLG_ROMCHECKSUM_H
#define RA_DLG_ROMCHECKSUM_H
#pragma once

#include <wtypes.h>

namespace ra {
namespace ui {

class Dlg_RomChecksum
{
public:
#pragma region Constructors
    inline constexpr Dlg_RomChecksum() noexcept = default;
    ~Dlg_RomChecksum() noexcept = default;
    Dlg_RomChecksum(const Dlg_RomChecksum&) = delete;
    Dlg_RomChecksum& operator=(const Dlg_RomChecksum&) = delete;
    inline constexpr Dlg_RomChecksum(Dlg_RomChecksum&&) noexcept = default;
    inline constexpr Dlg_RomChecksum& operator=(Dlg_RomChecksum&&) noexcept = default;
#pragma endregion

    INT_PTR DoModal();

private:
    friend INT_PTR CALLBACK Dlg_RomChecksumProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
};

} // namespace ui
} // namespace ra


#endif // !RA_DLG_ROMCHECKSUM_H
