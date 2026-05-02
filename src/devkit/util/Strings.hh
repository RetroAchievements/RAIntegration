#ifndef RA_UTIL_STRINGS_HH
#define RA_UTIL_STRINGS_HH
#pragma once

#include "GSL.hh"
#include "StringBuilder.hh"
#include "TypeTraits.hh"

#include <string>

namespace ra {
namespace util {

class String {
public:
    /// <summary>
    /// Converts a Unicode string to UTF-8.
    /// </summary>
    _NODISCARD static std::string Narrow(_In_ std::wstring_view wstr) { return StringBuilder::Narrow(wstr); }

    /// <summary>
    /// Converts a Unicode string to UTF-8.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD static std::string Narrow(std::wstring&& wstr) noexcept
    {
        const auto wwstr{ std::move_if_noexcept(wstr) };
        return Narrow(wwstr);
    }

    /// <summary>
    /// Converts a Unicode string to UTF-8.
    /// </summary>
    _NODISCARD static std::string Narrow(_In_z_ const wchar_t* wstr) { return StringBuilder::Narrow(wstr); }

    /// <summary>
    /// Converts a Unicode string to UTF-8.
    /// </summary>
    _NODISCARD static std::string Narrow(_In_z_ const char* str) { return std::string(str); }

    /// <summary>
    /// Converts a Unicode string to UTF-8.
    /// </summary>
    _NODISCARD static std::string Narrow(_In_ const std::string& str) { return std::string(str); }

    /// <summary>
    /// Converts a UTF-8 string to Unicode.
    /// </summary>
    _NODISCARD static std::wstring Widen(_In_ std::string_view str) { return StringBuilder::Widen(str); }

    /// <summary>
    /// Converts a UTF-8 string to Unicode.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD static std::wstring Widen(std::string&& str) noexcept
    {
        const auto sstr{ std::move_if_noexcept(str) };
        return Widen(sstr);
    }

    /// <summary>
    /// Converts a UTF-8 string to Unicode.
    /// </summary>
    _NODISCARD static std::wstring Widen(_In_z_ const char* str) { return StringBuilder::Widen(str); }

    /// <summary>
    /// Converts a UTF-8 string to Unicode.
    /// </summary>
    _NODISCARD static std::wstring Widen(_In_z_ const wchar_t* wstr) { return std::wstring(wstr); }

    /// <summary>
    /// Converts a UTF-8 string to Unicode.
    /// </summary>
    _NODISCARD static std::wstring Widen(_In_ const std::wstring& wstr) { return std::wstring(wstr); }

    /// <summary>
    /// Removes one "\r", "\n", or "\r\n" from the end of a string.
    /// </summary>
    /// <returns>Reference to <paramref name="str" /> for chaining.</returns>
    static std::string& TrimLineEnding(_Inout_ std::string& str) noexcept;

    /// <summary>
    /// Removes all whitespace characters from the front and back of a string.
    /// </summary>
    /// <returns>Reference to <paramref name="str" /> for chaining.</returns>
    static std::wstring& Trim(_Inout_ std::wstring& str);

    /// <summary>
    /// Converts all "\n"s in a string to "\r\n".
    /// </summary>
    /// <returns>Reference to <paramref name="str" /> for chaining.</returns>
    static std::wstring& NormalizeLineEndings(_Inout_ std::wstring& str);

    /// <summary>
    /// Formats a <see cref="time_t" /> as "Tue 31 Mar 2020".
    /// </summary>
    _NODISCARD static const std::wstring FormatDate(_In_ time_t when);

    /// <summary>
    /// Formats a <see cref="time_t" /> as "Tue 31 Mar 2020 14:23:11".
    /// </summary>
    _NODISCARD static const std::wstring FormatDateTime(_In_ time_t when);

    /// <summary>
    /// Formats a <see cref="time_t" /> as "Today", "Yesterday", "3 days ago", etc.
    /// </summary>
    _NODISCARD static const std::wstring FormatDateRecent(_In_ time_t when);

    /// <summary>
    /// Constructs a <see cref="std::basic_string" /> from a format string and parameters.
    /// </summary>
    template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>, typename... Ts>
    _NODISCARD inline static auto Printf(_In_z_ _Printf_format_string_ const CharT* const _RESTRICT sFormat, Ts&&... args)
    {
        Expects(sFormat != nullptr);

        if constexpr (std::is_same_v<CharT, char>)
        {
            StringBuilder builder;
            builder.AppendPrintf(sFormat, std::forward<Ts>(args)...);
            return builder.ToString();
        }
        else if constexpr (std::is_same_v<CharT, wchar_t>)
        {
            StringBuilder builder(true);
            builder.AppendPrintf(sFormat, std::forward<Ts>(args)...);
            return builder.ToWString();
        }
        else
        {
            static_assert(std::is_same_v<CharT, char>, "unsupported character type");
            return std::string();
        }
    }

    /// <summary>
    /// Determines if <paramref name="sString" /> starts with <paramref name="sMatch" />.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD static bool StartsWith(_In_ const std::string_view sString,
                                                      _In_ const std::string_view sMatch) noexcept
    {
        if (sMatch.length() > sString.length())
            return false;

        return (sString.compare(0, sMatch.length(), sMatch) == 0);
    }

    /// <summary>
    /// Determines if <paramref name="sString" /> starts with <paramref name="sMatch" />.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD static bool StartsWith(_In_ const std::wstring_view sString,
                                                      _In_ const std::wstring_view sMatch) noexcept
    {
        if (sMatch.length() > sString.length())
            return false;

        return (sString.compare(0, sMatch.length(), sMatch) == 0);
    }

    /// <summary>
    /// Determines if <paramref name="sString" /> ends with <paramref name="sMatch" />.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD static bool EndsWith(_In_ const std::string_view sString,
                                                    _In_ const std::string_view sMatch) noexcept
    {
        if (sMatch.length() > sString.length())
            return false;

        return (sString.compare(sString.length() - sMatch.length(), sMatch.length(), sMatch) == 0);
    }

    /// <summary>
    /// Determines if <paramref name="sString" /> ends with <paramref name="sMatch" />.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD static bool EndsWith(_In_ const std::wstring_view sString,
                                                    _In_ const std::wstring_view sMatch) noexcept
    {
        if (sMatch.length() > sString.length())
            return false;

        return (sString.compare(sString.length() - sMatch.length(), sMatch.length(), sMatch) == 0);
    }

    /// <summary>
    /// Converts a string to lowercase.
    /// </summary>
    static void MakeLowercase(_Inout_ std::wstring& sString);

    /// <summary>
    /// Converts a string to lowercase.
    /// </summary>
    static void MakeLowercase(_Inout_ std::string& sString);

    /// <summary>
    /// Determines if <paramref name="sString" /> contains <paramref name="sMatch" />.
    /// </summary>
    _NODISCARD static bool ContainsCaseInsensitive(_In_ const std::wstring_view sString,
                                                   _In_ const std::wstring_view sMatch) noexcept;

    /// <summary>
    /// Generates a DJB2 hash of the provided string.
    /// </summary>
    template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
    static constexpr int Hash(const std::basic_string<CharT>& sString) noexcept
    {
        int nHash = 5381;
        for (const auto c : sString)
            nHash = ((nHash << 5) + nHash) + c; /* hash * 33 + c */

        return nHash;
    }

    /// <summary>
    /// Encodes a string to Base64.
    /// </summary>
    static std::string EncodeBase64(const std::string& sString);
};

} // namespace util
} // namespace ra

#endif // !RA_UTIL_STRINGS_HH
