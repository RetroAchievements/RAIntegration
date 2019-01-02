#include "RA_StringUtils.h"

namespace ra {

_Use_decl_annotations_
std::string Narrow(const std::wstring& wstr)
{
    return Narrow(wstr.c_str());
}

_Use_decl_annotations_
std::string Narrow(std::wstring&& wstr) noexcept
{
    const auto wwstr{ std::move_if_noexcept(wstr) };
    return Narrow(wwstr);
}

_Use_decl_annotations_
std::string Narrow(const wchar_t* wstr)
{
    const auto len{ ra::to_signed(std::wcslen(wstr)) };

    const auto needed{
        ::WideCharToMultiByte(CP_UTF8, 0, wstr, len + 1, nullptr, 0, nullptr, nullptr)
    };

    std::string str(ra::to_unsigned(needed), '\000'); // allocate required space (including terminator)
    ::WideCharToMultiByte(CP_UTF8, 0, wstr, len + 1, str.data(), ra::to_signed(str.capacity()),
                          nullptr, nullptr);
    str.resize(ra::to_unsigned(needed - 1)); // terminator is not actually part of the string
    return str;
}

_Use_decl_annotations_
std::wstring Widen(const std::string& str)
{
    return Widen(str.c_str());
}

_Use_decl_annotations_
std::wstring Widen(std::string&& str) noexcept
{
    const auto sstr{ std::move_if_noexcept(str) };
    return Widen(sstr);
}

_Use_decl_annotations_
std::wstring Widen(const char* str)
{
    const auto len{ ra::to_signed(std::strlen(str)) };
    const auto needed{ ::MultiByteToWideChar(CP_UTF8, 0, str, len + 1, nullptr, 0) };
    // doesn't seem wchar_t is treated like a character type by default
    std::wstring wstr(ra::to_unsigned(needed), L'\x0');
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, len + 1, wstr.data(),
                          ra::to_signed(wstr.capacity()));
    wstr.resize(ra::to_unsigned(needed - 1));

    return wstr;
}

_Use_decl_annotations_
std::wstring Widen(const wchar_t* wstr)
{
    const std::wstring _wstr{ wstr };
    return _wstr;
}

_Use_decl_annotations_ std::wstring Widen(const std::wstring& wstr) { return wstr; }

_Use_decl_annotations_
std::string Narrow(const char* str)
{
    const std::string _str{ str };
    return _str;
}

_Use_decl_annotations_
std::string Narrow(const std::string& str)
{
    return str;
}

_Use_decl_annotations_
std::string& TrimLineEnding(std::string& str) noexcept
{
    if (!str.empty())
    {
        if (str.back() == '\n')
            str.pop_back();
        if (str.back() == '\r')
            str.pop_back();
    }

    return str;
}
_Use_decl_annotations_
bool StringStartsWith(const std::string& sString, const std::string& sMatch) noexcept
{
    if (sMatch.length() > sString.length())
        return false;

    return (sString.compare(0, sMatch.length(), sMatch) == 0);
}

_Use_decl_annotations_
bool StringEndsWith(const std::string& sString, const std::string& sMatch) noexcept
{
    if (sMatch.length() > sString.length())
        return false;

    return (sString.compare(sString.length() - sMatch.length(), sMatch.length(), sMatch) == 0);
}

_Use_decl_annotations_
bool StringStartsWith(const std::wstring& sString, const std::wstring& sMatch) noexcept
{
    if (sMatch.length() > sString.length())
        return false;

    return (sString.compare(0, sMatch.length(), sMatch) == 0);
}

_Use_decl_annotations_
bool StringEndsWith(const std::wstring& sString, const std::wstring& sMatch) noexcept
{
    if (sMatch.length() > sString.length())
        return false;

    return (sString.compare(sString.length() - sMatch.length(), sMatch.length(), sMatch) == 0);
}

void StringBuilder::AppendToString(_Inout_ std::string& sResult) const
{
    size_t nNeeded = 0;
    for (auto& pPending : m_vPending)
    {
        switch (pPending.DataType)
        {
            case PendingString::Type::String:
                nNeeded += pPending.String.length();
                break;

            case PendingString::Type::WString:
                pPending.String = ra::Narrow(pPending.WString);
                pPending.DataType = PendingString::Type::String;
                nNeeded += pPending.String.length();
                break;

            case PendingString::Type::StringRef:
                nNeeded += std::get<const std::string*>(pPending.Ref)->length();
                break;

            case PendingString::Type::WStringRef:
                pPending.String = ra::Narrow(*std::get<const std::wstring*>(pPending.Ref));
                pPending.DataType = PendingString::Type::String;
                nNeeded += pPending.String.length();
                break;

            case PendingString::Type::CharRef:
                nNeeded += std::get<gsl::cstring_span<>>(pPending.Ref).size();
                break;

            case PendingString::Type::WCharRef:
                pPending.String = ra::Narrow(gsl::to_string(std::get<gsl::cwstring_span<>>(pPending.Ref)));
                pPending.DataType = PendingString::Type::String;
                nNeeded += pPending.String.length();
                break;
        }
    }

    sResult.reserve(sResult.length() + nNeeded + 1);

    for (auto& pPending : m_vPending)
    {
        switch (pPending.DataType)
        {
            case PendingString::Type::String:
                sResult.append(pPending.String);
                break;

            case PendingString::Type::StringRef:
                sResult.append(*std::get<const std::string*>(pPending.Ref));
                break;

            case PendingString::Type::CharRef:
                sResult.append(gsl::to_string(std::get<gsl::cstring_span<>>(pPending.Ref)));
                break;
        }
    }
}

void StringBuilder::AppendToWString(_Inout_ std::wstring& sResult) const
{
    size_t nNeeded = 0;
    for (auto& pPending : m_vPending)
    {
        switch (pPending.DataType)
        {
            case PendingString::Type::WString:
                nNeeded += pPending.WString.length();
                break;

            case PendingString::Type::String:
                pPending.WString = ra::Widen(pPending.String);
                pPending.DataType = PendingString::Type::WString;
                nNeeded += pPending.WString.length();
                break;

            case PendingString::Type::WStringRef:
                nNeeded += std::get<const std::wstring*>(pPending.Ref)->length();
                break;

            case PendingString::Type::StringRef:
                pPending.WString = ra::Widen(*std::get<const std::string*>(pPending.Ref));
                pPending.DataType = PendingString::Type::WString;
                nNeeded += pPending.WString.length();
                break;

            case PendingString::Type::WCharRef:
                nNeeded += std::get<gsl::cwstring_span<>>(pPending.Ref).size();
                break;

            case PendingString::Type::CharRef:
                pPending.WString = ra::Widen(gsl::to_string(std::get<gsl::cstring_span<>>(pPending.Ref)));
                pPending.DataType = PendingString::Type::WString;
                nNeeded += pPending.WString.length();
                break;
        }
    }

    sResult.reserve(sResult.length() + nNeeded + 1);

    for (auto& pPending : m_vPending)
    {
        switch (pPending.DataType)
        {
            case PendingString::Type::WString:
                sResult.append(pPending.WString);
                break;

            case PendingString::Type::WStringRef:
                sResult.append(*std::get<const std::wstring*>(pPending.Ref));
                break;

            case PendingString::Type::WCharRef:
                sResult.append(gsl::to_string(std::get<gsl::cwstring_span<>>(pPending.Ref)));
                break;
        }
    }
}

} /* namespace ra */
