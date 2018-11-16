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
    auto sCpy{ str };
    str = TrimLineEnding(std::move_if_noexcept(sCpy));
    return str;
}

std::string TrimLineEnding(std::string&& str) noexcept
{
    auto sRet{ std::move_if_noexcept(str) };

    if (!sRet.empty())
    {
        if (sRet.back() == '\n')
            sRet.pop_back();
        if (sRet.back() == '\r')
            sRet.pop_back();
    }

    return sRet;
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

} /* namespace ra */
