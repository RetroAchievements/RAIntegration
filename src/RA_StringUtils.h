#ifndef RA_STRINGUTILS_H
#define RA_STRINGUTILS_H
#pragma once

#include "ra_utility.h"

namespace ra {

_NODISCARD std::string Narrow(_In_ const std::wstring& wstr);
_NODISCARD std::string Narrow(_In_z_ const wchar_t* wstr);
_NODISCARD std::wstring Widen(_In_ const std::string& str);
_NODISCARD std::wstring Widen(_In_z_ const char* str);

GSL_SUPPRESS_F6 _NODISCARD std::string Narrow(std::wstring&& wstr) noexcept;
GSL_SUPPRESS_F6 _NODISCARD std::wstring Widen(std::string&& str) noexcept;

//	No-ops to help convert:
_NODISCARD std::wstring Widen(_In_z_ const wchar_t* wstr);
_NODISCARD std::wstring Widen(_In_ const std::wstring& wstr);
_NODISCARD std::string Narrow(_In_z_ const char* str);
_NODISCARD std::string Narrow(_In_ const std::string& wstr);

/// <summary>
/// Removes one "\r", "\n", or "\r\n" from the end of a string.
/// </summary>
/// <returns>Reference to <paramref name="str" /> for chaining.</returns>
GSL_SUPPRESS_F6 std::string& TrimLineEnding(_Inout_ std::string& str) noexcept;

// ----- ToString -----

template<typename T>
_NODISCARD inline const std::string ToString(_In_ const T& value)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        return std::to_string(value);
    }
    else if constexpr (std::is_enum_v<T>)
    {
        return std::to_string(etoi(value));
    }
    else
    {
        static_assert(false, "No ToString implementation for type");
        return std::string();
    }
}

template<>
_NODISCARD inline const std::string ToString(_In_ const std::string& value)
{
    return value;
}

template<>
_NODISCARD inline const std::string ToString(_In_ const std::wstring& value)
{
    return ra::Narrow(value);
}

template<>
_NODISCARD inline const std::string ToString(_In_ const wchar_t* const& value)
{
    return ra::Narrow(value);
}

template<>
_NODISCARD inline const std::string ToString(_In_ const char* const& value)
{
    return std::string(value);
}

template<>
_NODISCARD inline const std::string ToString(_In_ char* const& value)
{
    return std::string(value);
}

template<>
_NODISCARD inline const std::string ToString(_In_ const char& value)
{
    return std::string(1, value);
}

// literal strings can't be passed by reference, so won't call the templated methods
_NODISCARD inline const std::string ToString(_In_ const char* value) { return std::string(value); }
_NODISCARD inline const std::string ToString(_In_ const wchar_t* value) { return ra::Narrow(value); }

// ----- ToWString -----

template<typename T>
_NODISCARD inline const std::wstring ToWString(_In_ const T& value)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        return std::to_wstring(value);
    }
    else if constexpr (std::is_enum_v<T>)
    {
        return std::to_wstring(static_cast<std::underlying_type_t<T>>(value));
    }
    else
    {
        static_assert(false, "No ToWString implementation for type");
        return std::wstring();
    }
}

template<>
_NODISCARD inline const std::wstring ToWString(_In_ const std::wstring& value)
{
    return value;
}

template<>
_NODISCARD inline const std::wstring ToWString(_In_ const std::string& value)
{
    return ra::Widen(value);
}

template<>
_NODISCARD inline const std::wstring ToWString(_In_ const wchar_t* const& value)
{
    return std::wstring(value);
}

template<>
_NODISCARD inline const std::wstring ToWString(_In_ const char* const& value)
{
    return ra::Widen(value);
}

template<>
_NODISCARD inline const std::wstring ToWString(_In_ char* const& value)
{
    return ra::Widen(value);
}

template<>
_NODISCARD inline const std::wstring ToWString(_In_ const char& value)
{
    return std::wstring(1, value);
}

// literal strings can't be passed by reference, so won't call the templated methods
_NODISCARD inline const std::wstring ToWString(_In_ const char* value) { return ra::Widen(value); }
_NODISCARD inline const std::wstring ToWString(_In_ const wchar_t* value) { return std::wstring(value); }

// ----- string building -----

class StringBuilder
{
public:
    explicit StringBuilder(bool bPrepareWide = false) noexcept : m_bPrepareWide(bPrepareWide) {}

    template<typename T>
    void Append(const T& arg)
    {
        PendingString pPending;
        if (m_bPrepareWide)
        {
            pPending.WString = ra::ToWString(arg);
            pPending.DataType = PendingString::Type::WString;
        }
        else
        {
            pPending.String = ra::ToString(arg);
            pPending.DataType = PendingString::Type::String;
        }
        m_vPending.emplace_back(std::move(pPending));
    }

    template<>
    void Append(const std::string& arg)
    {
        PendingString pPending;
        pPending.Ref.String = &arg;
        pPending.DataType = PendingString::Type::StringRef;
        m_vPending.emplace_back(std::move(pPending));
    }

    template<>
    void Append(const std::wstring& arg)
    {
        PendingString pPending;
        pPending.Ref.WString = &arg;
        pPending.DataType = PendingString::Type::WStringRef;
        m_vPending.emplace_back(std::move(pPending));
    }

    void Append(std::string&& arg)
    {
        PendingString pPending;
        pPending.String = std::move(arg);
        pPending.DataType = PendingString::Type::String;
        m_vPending.emplace_back(std::move(pPending));
    }

    void Append(std::wstring&& arg)
    {
        PendingString pPending;
        pPending.WString = std::move(arg);
        pPending.DataType = PendingString::Type::WString;
        m_vPending.emplace_back(std::move(pPending));
    }

    template<>
    void Append(const char* const& arg)
    {
        AppendSubString(arg, strlen(arg));
    }

    template<>
    void Append(const wchar_t* const& arg)
    {
        AppendSubString(arg, wcslen(arg));
    }

    void AppendSubString(const char* pStart, size_t nLength)
    {
        if (nLength == 0)
            return;

        PendingString pPending;
        pPending.Ref.Char.Pointer = pStart;
        pPending.Ref.Char.Length = nLength;
        pPending.DataType = PendingString::Type::CharRef;
        m_vPending.emplace_back(std::move(pPending));
    }

    void AppendSubString(const wchar_t* pStart, size_t nLength)
    {
        if (nLength == 0)
            return;

        PendingString pPending;
        pPending.Ref.WChar.Pointer = pStart;
        pPending.Ref.WChar.Length = nLength;
        pPending.DataType = PendingString::Type::WCharRef;
        m_vPending.emplace_back(std::move(pPending));
    }

    void Append() noexcept
    {
        // just in case one of the variadic methods gets called with 0 parameters
    }

    template<typename T, typename... Ts>
    void Append(const T&& arg, Ts&&... args)
    {
        Append(arg);
        Append(std::forward<Ts>(args)...);
    }

    template<typename T>
    void AppendFormat([[maybe_unused]] const T& arg, [[maybe_unused]] const std::string& sFormat)
    {
        std::ostringstream oss;

        if constexpr (std::is_integral_v<T>)
        {
            if (sFormat.front() == '0')
                oss << std::setfill('0');
            int nDigits = std::stoi(sFormat.c_str());
            if (nDigits > 0)
                oss << std::setw(nDigits);

            if (sFormat.back() == 'X')
                oss << std::uppercase << std::hex;
            else if (sFormat.back() == 'x')
                oss << std::hex;

            oss << arg;
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            if (sFormat.back() == 'f' || sFormat.back() == 'F')
            {
                const int nIndex = sFormat.find('.');
                if (nIndex != std::string::npos)
                {
                    int nPrecision = std::stoi(sFormat.c_str() + nIndex + 1);
                    oss.precision(nPrecision);
                    oss << std::fixed;
                }
            }

            oss << arg;
        }
        else
        {
            // cannot use static_assert here because the code will get generated regardless of if its ever used
            assert(!"Unsupported formatted type");
        }

        PendingString pPending;
        pPending.String = oss.str();
        pPending.DataType = PendingString::Type::String;
        m_vPending.emplace_back(std::move(pPending));
    }

    void AppendFormat(const char* arg, const std::string& sFormat)
    {
        if (sFormat.front() == '.')
        {
            int nCharacters = std::stoi(&sFormat.at(1));
            AppendSubString(arg, nCharacters);
        }
        else
        {
            const int nLength = strlen(arg);
            int nPadding = std::stoi(sFormat.c_str());
            nPadding -= nLength;
            if (nPadding > 0)
            {
                PendingString pPending;
                pPending.String = std::string(nPadding, ' ');
                pPending.DataType = PendingString::Type::String;
                m_vPending.emplace_back(std::move(pPending));
            }

            AppendSubString(arg, nLength);
        }
    }

    template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
    void AppendPrintf(const CharT* pFormat)
    {
        while (*pFormat)
        {
            auto* pScan = pFormat;
            Expects(pScan != nullptr); // can't use pointer arithmetic on not_null
            while (*pScan && *pScan != '%')
                ++pScan;

            if (*pScan == '%')
            {
                ++pScan;
                AppendSubString(pFormat, pScan - pFormat);
                if (*pScan == '%')
                    ++pScan;
                else if (*pScan != '\0')
                    assert(!"not enough parameters provided");
            }
            else if (pScan > pFormat)
            {
                AppendSubString(pFormat, pScan - pFormat);
            }
            pFormat = pScan;
        }
    }

    template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>, typename T, typename... Ts>
    void AppendPrintf(const CharT* const restrict pFormat, const T& restrict value, Ts&&... args)
    {
        auto* pScan = pFormat;
        Expects(pScan != nullptr);

        while (*pScan && *pScan != '%')
            ++pScan;

        if (pScan > pFormat)
            AppendSubString(pFormat, pScan - pFormat);

        if (*pScan == CharT()) // w/e "0" is
            return;
        ++pScan;
        switch (*pScan)
        {
            case '\0':
                AppendSubString(pScan - 1, 1);
                break;
            case '%':
                AppendSubString(pScan - 1, 1);
                AppendPrintf(++pScan, value, std::forward<Ts>(args)...);
                break;
            case 'l': // assume ll, or li
            case 'z': // assume zu
                ++pScan;
                _FALLTHROUGH;
            case 's':
            case 'd':
            case 'u':
            case 'f':
            case 'g':
                Append(value);
                AppendPrintf(++pScan, std::forward<Ts>(args)...);
                break;

            default:
                std::string sFormat;
                const CharT* pStart = pScan;
                while (*pScan)
                {
                    const char c = gsl::narrow<char>(*pScan);
                    sFormat.push_back(c);
                    if (isalpha(c))
                        break;

                    ++pScan;
                }

                if (!*pScan)
                {
                    AppendSubString(pStart, pScan - pStart);
                    break;
                }

                if (sFormat.length() > 2 && sFormat.at(sFormat.length() - 2) == '*')
                {
                    const char c = sFormat.back();
                    sFormat.pop_back(); // remove 's'/'x'
                    sFormat.pop_back(); // remove '*'
                    sFormat.append(ra::ToString(value));
                    sFormat.push_back(c); // replace 's'/'x'

                    if constexpr (sizeof...(args) > 0)
                    {
                        AppendPrintfParameterizedFormat(++pScan, sFormat, std::forward<Ts>(args)...);
                    }
                    else
                    {
                        assert(!"not enough parameters provided");
                        Append(sFormat);
                    }
                }
                else
                {
                    AppendFormat(value, sFormat);
                    AppendPrintf(++pScan, std::forward<Ts>(args)...);
                }
                break;
        }
    }

    std::string ToString() const
    {
        std::string sResult;
        AppendToString(sResult);
        return sResult;
    }

    void AppendToString(_Inout_ std::string& sResult) const;

    std::wstring ToWString() const
    {
        std::wstring sResult;
        AppendToWString(sResult);
        return sResult;
    }

    void AppendToWString(_Inout_ std::wstring& sResult) const;

private:
    template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>, typename T, typename... Ts>
    void AppendPrintfParameterizedFormat(const CharT* const pFormat,
                                         const std::string& sFormat,
                                         const T& value,
                                         Ts&&... args)
    {
        AppendFormat(value, sFormat);
        AppendPrintf(pFormat, std::forward<Ts>(args)...);
    }

    bool m_bPrepareWide;

    struct PendingString
    {
        union {
            const std::string* String;
            const std::wstring* WString;

            struct
            {
                const char* Pointer;
                size_t Length;
            } Char;

            struct
            {
                const wchar_t* Pointer;
                size_t Length;
            } WChar;
        } Ref{};

        std::string String;
        std::wstring WString;

        enum class Type
        {
            String,
            WString,
            StringRef,
            WStringRef,
            CharRef,
            WCharRef,
        };
        Type DataType{Type::String};
    };

    mutable std::vector<PendingString> m_vPending;
}; // namespace ra

template<typename... Ts>
void AppendString(std::string& sString, Ts&&... args)
{
    StringBuilder builder;
    builder.Append(std::forward<Ts>(args)...);
    builder.AppendToString(sString);
}

template<typename... Ts>
void AppendWString(std::wstring& sString, Ts&&... args)
{
    StringBuilder builder(true);
    builder.Append(std::forward<Ts>(args)...);
    builder.AppendToWString(sString);
}

template<typename... Ts>
std::string BuildString(Ts&&... args)
{
    StringBuilder builder;
    builder.Append(std::forward<Ts>(args)...);
    return builder.ToString();
}

template<typename... Ts>
std::wstring BuildWString(Ts&&... args)
{
    StringBuilder builder(true);
    builder.Append(std::forward<Ts>(args)...);
    return builder.ToWString();
}

/// <summary>
/// Constructs a <see cref="std::basic_string" /> from a format string and parameters
/// </summary>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>, typename... Ts>
_NODISCARD inline auto StringPrintf(_In_z_ _Printf_format_string_ const CharT* const __restrict sFormat, Ts&&... args)
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
        static_assert(false, "unsupported character type");
        return std::string();
    }
}

// More functions to be Unicode compatible w/o sacrificing MBCS
/* A wrapper for converting a string to an unsigned long depending on _String */
/* The template parameter does not need to be specified */
template<typename CharT, typename = std::enable_if_t<ra::is_char_v<CharT>>>
_Success_(0 < return < ULONG_MAX) _NODISCARD
    inline auto __cdecl tcstoul(_In_z_ const CharT* __restrict str,
                                _Out_opt_ _Deref_post_z_ CharT** __restrict endptr = nullptr,
                                _In_ int base = 0 /*auto-detect base*/)
{
    auto ret = 0UL;
    if constexpr (std::is_same_v<CharT, char>)
        ret = std::strtoul(str, endptr, base);
    else if constexpr (std::is_same_v<CharT, wchar_t>)
        ret = std::wcstoul(str, endptr, base);

    assert((errno != ERANGE) && (ret != ULONG_MAX)); // out of range error
    assert(ret != 0);                                // invalid argument, no conversion done
    return ret;
} /* end function tcstoul */

/// <summary>
///   Returns the number of characters of a null-terminated byte string or wide-character string depending on
///   the character set specified
/// </summary>
/// <param name="_Str">The string.</param>
/// <returns>Number of characters</returns>
/// <remarks>The template argument does not have to be specified.</remarks>
template<typename CharT, typename = std::enable_if_t<ra::is_char_v<CharT>>>
_NODISCARD _Success_(0 < return < strsz) inline auto __cdecl tcslen_s(_In_reads_or_z_(strsz) const CharT* const str,
                                                                      _In_ std::size_t strsz = UINT16_MAX) noexcept
{
    assert(str != nullptr);
    std::size_t ret = 0U;
    if constexpr (std::is_same_v<CharT, char>)
        ret = strnlen_s(str, strsz);
    else if constexpr (std::is_same_v<CharT, wchar_t>)
        ret = wcsnlen_s(str, strsz);

    assert(ret != 0);     // somehow str was a nullptr
    assert(ret != strsz); // str isn't null-terminated
    return ret;
} /* end function tcslen_s */

/// <summary>
/// Determines if <paramref name="sString" /> starts with <paramref name="sMatch" />.
/// </summary>
GSL_SUPPRESS_F6
_NODISCARD bool StringStartsWith(_In_ const std::wstring& sString, _In_ const std::wstring& sMatch) noexcept;

/// <summary>
/// Determines if <paramref name="sString" /> ends with <paramref name="sMatch" />.
/// </summary>
GSL_SUPPRESS_F6
_NODISCARD bool StringEndsWith(_In_ const std::wstring& sString, _In_ const std::wstring& sMatch) noexcept;

} // namespace ra

#ifdef UNICODE
#define NativeStr(x) ra::Widen(x)
#define NativeStrType std::wstring
#else
#define NativeStr(x) ra::Narrow(x)
#define NativeStrType std::string
#endif

#endif // !RA_STRINGUTILS_H
