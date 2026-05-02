#include "RA_Defs.h"

#include "util/Strings.hh"

namespace ra {

_Use_decl_annotations_
bool ParseUnsignedInt(const std::wstring& sValue, unsigned int nMaximumValue, unsigned int& nValue, std::wstring& sError)
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

        if (ra::to_unsigned(nVal) > nMaximumValue)
        {
            sError = ra::util::String::Printf(L"Value cannot exceed %lu", nMaximumValue);
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
        sError = ra::util::String::Printf(L"Value cannot exceed %lu", nMaximumValue);
    }

    return false;
}

_Use_decl_annotations_
bool ParseHex(const std::wstring& sValue, unsigned int nMaximumValue, unsigned int& nValue, std::wstring& sError)
{
    nValue = 0U;
    sError.clear();

    try
    {
        wchar_t* pEnd = nullptr;
        const auto nVal = std::wcstoll(sValue.c_str(), &pEnd, 16);
        if (pEnd && *pEnd != '\0')
        {
            sError = L"Only values that can be represented as hexadecimal are allowed";
            return false;
        }

        if (nVal < 0)
        {
            sError = L"Negative values are not allowed";
            return false;
        }

        if (nVal > nMaximumValue)
        {
            sError = ra::util::String::Printf(L"Value cannot exceed 0x%02X", nMaximumValue);
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
        sError = ra::util::String::Printf(L"Value cannot exceed 0x%02X", nMaximumValue);
    }

    return false;
}

_Use_decl_annotations_
bool ParseNumeric(const std::wstring& sValue, unsigned int& nValue, std::wstring& sError)
{
    if (sValue.length() > 2 && sValue.at(1) == 'x')
        return ra::ParseHex(sValue, 0xFFFFFFFF, nValue, sError);

    return ra::ParseUnsignedInt(sValue, 0xFFFFFFFF, nValue, sError);
}

_Use_decl_annotations_
bool ParseFloat(const std::wstring& sValue, float& fValue, std::wstring& sError)
{
    fValue = 0.0f;
    sError.clear();

    wchar_t* pEnd = nullptr;
    const auto fVal = std::wcstof(sValue.c_str(), &pEnd);
    if (pEnd && *pEnd != '\0')
    {
        sError = L"Only values that can be represented as floating point numbers are allowed";
        return false;
    }

    fValue = fVal;
    return true;
}

} /* namespace ra */
