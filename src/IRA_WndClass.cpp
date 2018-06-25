
#include "IRA_WndClass.h"

#include "RA_Core.h"
#include "IRA_Dialog.h"


namespace ra {
IRA_WndClass::IRA_WndClass(LPCTSTR className) noexcept
{
    cbSize        = sizeof(WNDCLASSEX);
    style         = 0U;
    lpfnWndProc   = reinterpret_cast<WNDPROC>(ra::DialogProc);
    cbClsExtra    = 0;
    cbWndExtra    = 0;
    hInstance     = ::GetModuleHandle("RA_Integration.dll");
    hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    hCursor       = LoadCursor(nullptr, IDC_ARROW);
    hbrBackground = GetStockBrush(COLOR_BTNFACE);
    lpszMenuName  = nullptr;
    lpszClassName = className;
    hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    Register();
} // end constructor

IRA_WndClass::~IRA_WndClass() noexcept
{
    if (hbrBackground)
        DeleteBrush(hbrBackground);
} // end destructor

bool IRA_WndClass::Register() noexcept
{
    if (RegisterClassEx(this))
        return true;
    else
        return false;
} // end function Register
} // namespace ra
