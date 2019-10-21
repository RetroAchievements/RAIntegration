#include "GridNumberColumnBinding.hh"

#include "GridBinding.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

bool GridNumberColumnBinding::SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue)
{
    std::wstring sError;
    unsigned int nValue = 0U;

    if (!ParseUnsignedInt(sValue, nValue, sError))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
        return false;
    }

    vmItems.SetItemValue(nIndex, *m_pBoundProperty, ra::to_signed(nValue));
    return true;
}

bool GridNumberColumnBinding::ParseUnsignedInt(const std::wstring& sValue, _Out_ unsigned int& nValue, _Out_ std::wstring& sError)
{
    nValue = 0U;
    sError.clear();

    try
    {
        wchar_t* pEnd = nullptr;
        const auto nVal = std::wcstoll(sValue.c_str(), &pEnd, 10);
        if (pEnd && *pEnd != '\0')
        {
            sError = L"Only values that can be represented as decimal are allowed";
            return false;
        }

        if (nVal < 0)
        {
            sError = L"Negative values are not allowed";
            return false;
        }

        if (ra::to_unsigned(nVal) > m_nMaximum)
        {
            sError = ra::StringPrintf(L"Value cannot exceed %lu", m_nMaximum);
            return false;
        }

        nValue = gsl::narrow_cast<unsigned int>(ra::to_unsigned(nVal));
        return true;
    }
    catch (const std::invalid_argument&)
    {
        sError = L"Only values that can be represented as decimal are allowed";
    }
    catch (const std::out_of_range&)
    {
        sError = ra::StringPrintf(L"Value cannot exceed %lu", m_nMaximum);
    }

    return false;
}

bool GridNumberColumnBinding::ParseHex(const std::wstring& sValue, _Out_ unsigned int& nValue, _Out_ std::wstring& sError)
{
    nValue = 0U;
    sError.clear();

    try
    {
        wchar_t* pEnd = nullptr;
        const auto nVal = std::wcstoul(sValue.c_str(), &pEnd, 16);
        if (pEnd && *pEnd != '\0')
        {
            sError = L"Only values that can be represented as hexadecimal are allowed";
            return false;
        }

        if (nVal > m_nMaximum)
        {
            sError = ra::StringPrintf(L"Value cannot exceed 0x%02X", m_nMaximum);
            return false;
        }

        nValue = gsl::narrow_cast<unsigned int>(nVal);
        return true;
    }
    catch (const std::invalid_argument&)
    {
        sError = L"Only values that can be represented as hexadecimal are allowed";
    }
    catch (const std::out_of_range&)
    {
        sError = ra::StringPrintf(L"Value cannot exceed 0x%02X", m_nMaximum);
    }

    return false;
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
