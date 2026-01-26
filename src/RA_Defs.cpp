#include "RA_Defs.h"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {

_Use_decl_annotations_
std::string ByteAddressToString(ra::data::ByteAddress nAddr)
{
#ifndef RA_UTEST
    const auto& pEmulatorMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    return pEmulatorMemoryContext.FormatAddress(nAddr);
#else
    return ra::util::String::Printf("0x%04x", nAddr);
#endif
}

ra::data::ByteAddress ByteAddressFromString(_In_ const std::string_view sByteAddress)
{
    ra::data::ByteAddress address{};

    if (!ra::util::String::StartsWith(sByteAddress, "-")) // negative addresses not supported
    {
        char* pEnd;
        address = std::strtoul(sByteAddress.data(), &pEnd, 16);
        Ensures(pEnd != nullptr);
        if (gsl::narrow_cast<size_t>(pEnd - sByteAddress.data()) < sByteAddress.length())
        {
            // hex parse failed
            address = {};
        }
    }

    return address;
}

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

namespace data {

const char* ValueFormatToString(ValueFormat nFormat) noexcept
{
    switch (nFormat)
    {
        case ValueFormat::Centiseconds: return "MILLISECS";
        case ValueFormat::Frames: return "TIME";
        case ValueFormat::Minutes: return "MINUTES";
        case ValueFormat::Score: return "SCORE";
        case ValueFormat::Seconds: return "TIMESECS";
        case ValueFormat::SecondsAsMinutes: return "SECS_AS_MINS";
        case ValueFormat::Value: return "VALUE";
        case ValueFormat::Float1: return "FLOAT1";
        case ValueFormat::Float2: return "FLOAT2";
        case ValueFormat::Float3: return "FLOAT3";
        case ValueFormat::Float4: return "FLOAT4";
        case ValueFormat::Float5: return "FLOAT5";
        case ValueFormat::Float6: return "FLOAT6";
        case ValueFormat::Fixed1: return "FIXED1";
        case ValueFormat::Fixed2: return "FIXED2";
        case ValueFormat::Fixed3: return "FIXED3";
        case ValueFormat::Tens: return "TENS";
        case ValueFormat::Hundreds: return "HUNDREDS";
        case ValueFormat::Thousands: return "THOUSANDS";
        case ValueFormat::UnsignedValue: return "UNSIGNED";
        default: return "UNKNOWN";
    }
}

ValueFormat ValueFormatFromString(const std::string& sFormat) noexcept
{
    return ra::itoe<ValueFormat>(rc_parse_format(sFormat.c_str()));
}

} /* namespace data */
} /* namespace ra */
