#ifndef RA_SERVICES_WINDOWS_CLIPBOARD_HH
#define RA_SERVICES_WINDOWS_CLIPBOARD_HH
#pragma once

#include "services\IClipboard.hh"

namespace ra {
namespace services {
namespace impl {

class WindowsClipboard : public ra::services::IClipboard
{
public:
    void SetText(const std::wstring& sValue) const noexcept override
    {
        // allocate memory to be managed by the clipboard
        const SIZE_T nSize = (sValue.length() + 1) * 2; // 16-bit characters, with null terminator
        const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, nSize);
        if (hMem)
        {
            const LPVOID pMem = GlobalLock(hMem);
            if (pMem)
            {
                memcpy(pMem, sValue.c_str(), nSize);
                GlobalUnlock(hMem);
            }

            // assign the memory to the clipboard
            if (OpenClipboard(nullptr))
            {
                EmptyClipboard();
                SetClipboardData(CF_UNICODETEXT, hMem);
                CloseClipboard();
            }
            else
            {
                GlobalFree(hMem);
            }
        }
    }

    std::wstring GetText() const override
    {
        std::wstring sText;

        if (OpenClipboard(nullptr))
        {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData != nullptr)
            {
                const auto* pszText = static_cast<wchar_t*>(GlobalLock(hData));
                if (pszText != nullptr)
                {
                    sText = pszText;
                    GlobalUnlock(hData);
                }
            }
            else
            {
                hData = GetClipboardData(CF_TEXT);
                if (hData != nullptr)
                {
                    const auto* pszText = static_cast<char*>(GlobalLock(hData));
                    if (pszText != nullptr)
                    {
                        sText = ra::util::String::Widen(pszText);
                        GlobalUnlock(hData);
                    }
                }
            }

            CloseClipboard();
        }

        return sText;
    }
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_WINDOWS_CLIPBOARD_HH
