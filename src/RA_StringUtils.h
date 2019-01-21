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
        if constexpr (std::is_same_v<T, wchar_t>)
        {
            int status = 0;
            std::string mb(MB_CUR_MAX, '\0');
            Expects(wctomb_s(&status, mb.data(), MB_CUR_MAX, value) == 0);
            Ensures(to_unsigned(status) != to_unsigned(-1));
            return mb;
        }
        else if constexpr (std::is_same_v<T, char>)
            return value;
        else
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
        if constexpr (std::is_same_v<T, char>)
        {
            std::wstring wc{value};
            std::string mb(MB_CUR_MAX, '\0');
            Ensures(std::mbtowc(wc.data(), mb.data(), 8) != -1);
            return wc;
        }
        else if constexpr (std::is_same_v<T, wchar_t>)
            return std::wstring(1, value);
        else
            return std::to_wstring(value);
    }
    else if constexpr (std::is_enum_v<T>)
    {
        return std::to_wstring(etoi(value));
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
        if (m_bPrepareWide)
            m_vPending.emplace_back(std::wstring{ra::ToWString(arg)});
        else
            m_vPending.emplace_back(std::string{ra::ToString(arg)});
    }

    template<>
    void Append(const std::string& arg)
    {
        m_vPending.emplace_back(arg.c_str(), arg.length());
    }

    template<>
    void Append(const std::wstring& arg)
    {
        m_vPending.emplace_back(arg.c_str(), arg.length());
    }

    void Append(std::string&& arg)
    {
        m_vPending.emplace_back(std::move(arg));
    }

    void Append(std::wstring&& arg)
    {
        m_vPending.emplace_back(std::move(arg));
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

        m_vPending.emplace_back(pStart, nLength);
    }

    void AppendSubString(const wchar_t* pStart, size_t nLength)
    {
        if (nLength == 0)
            return;

        m_vPending.emplace_back(pStart, nLength);
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
            if(sFormat.back() == 'c')
                oss << arg;
            else
            {
                if(std::isdigit(ra::to_unsigned(sFormat.front())))
                {
                    if (sFormat.front() == '0')
                        oss << std::setfill('0');

                    const int nDigits = std::stoi(sFormat);
                    if (nDigits > 0)
                        oss << std::setw(nDigits);
                }
                if (sFormat.back() == 'X')
                    oss << std::uppercase << std::hex;
                else if (sFormat.back() == 'x')
                    oss << std::hex;

                if (is_char_v<T>)
                    oss << gsl::narrow<int>(arg);
                else
                    oss << arg;
            }
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

        m_vPending.emplace_back(oss.str());
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
                m_vPending.emplace_back(std::string(nPadding, ' '));

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
                    if (isalpha(to_unsigned(c)))
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
    void AppendPrintfParameterizedFormat(const CharT* const pFormat, const std::string& sFormat, const T& value, Ts&&... args)
    {
        AppendFormat(value, sFormat);
        AppendPrintf(pFormat, std::forward<Ts>(args)...);
    }

    bool m_bPrepareWide;

    class PendingString
    {
    public:
        explicit PendingString(std::string&& arg) noexcept : String{std::move(arg)}, DataType{Type::String} {}
        explicit PendingString(std::wstring&& arg) noexcept : WString{std::move(arg)}, DataType{Type::WString} {}
        explicit PendingString(_In_z_ const char* const restrict ptr, std::size_t len) noexcept :
            Ref{std::string_view{ptr, len}},
            DataType{Type::CharRef}
        {}
        explicit PendingString(_In_z_ const wchar_t* const restrict ptr, std::size_t len) noexcept :
            Ref{std::wstring_view{ptr, len}},
            DataType{Type::WCharRef}
        {}
        PendingString() noexcept = delete;
        ~PendingString() noexcept = default;

        // To prevent copying moving
        PendingString(const PendingString&) = delete;
        PendingString& operator=(const PendingString&) = delete;
        PendingString& operator=(PendingString&&) noexcept = delete;
        
        // This is needed by m_vPending's allocator
        PendingString(PendingString&&) noexcept = default;

    private:
        using RefType = std::variant<std::string_view, std::wstring_view>;
        RefType Ref;

        std::string String;
        std::wstring WString;

        enum class Type
        {
            String,
            WString,
            CharRef,
            WCharRef,
        };
        Type DataType{Type::String};

        friend class StringBuilder;
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

// ----- string parsing -----

class Tokenizer
{
public:
    Tokenizer(const std::string& sString) noexcept : m_sString(sString) {}

    /// <summary>
    /// Returns <c>true</c> if the entire string has been processed.
    /// </summary>
    _NODISCARD bool EndOfString() const noexcept { return m_nPosition >= m_sString.length(); }

    /// <summary>
    /// Returns the next character without advancing the position.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD char PeekChar() const noexcept { return m_nPosition < m_sString.length() ? m_sString.at(m_nPosition) : '\0'; }

    /// <summary>
    /// Get the current position of the cursor within the string.
    /// </summary>
    _NODISCARD size_t CurrentPosition() const noexcept { return m_nPosition; }

    /// <summary>
    /// Sets the cursor position.
    /// </summary>
    void Seek(size_t nPosition) noexcept { m_nPosition = std::min(nPosition, m_sString.length()); }

    /// <summary>
    /// Advances the cursor one character.
    /// </summary>
    void Advance() noexcept
    {
        if (m_nPosition < m_sString.length())
            ++m_nPosition;
    }

    /// <summary>
    /// Advances the cursor the specified number of characters.
    /// </summary>
    void Advance(size_t nCount) noexcept
    {
        m_nPosition += nCount;
        if (m_nPosition > m_sString.length())
            m_nPosition = m_sString.length();
    }

    /// <summary>
    /// Advances the cursor to the next occurrence of the specified charater, or the end of the string if no
    /// occurances are found.
    /// </summary>
    void AdvanceTo(char cStop)
    {
        while (m_nPosition < m_sString.length() && m_sString.at(m_nPosition) != cStop)
            ++m_nPosition;
    }

    /// <summary>
    /// Advances the cursor to the next occurrance of the specified charater, or the end of the string if no
    /// occurances are found and returns a string containing all of the characters advanced over.
    /// </summary>
    _NODISCARD std::string ReadTo(char cStop)
    {
        const size_t nStart = m_nPosition;
        AdvanceTo(cStop);
        return std::string(m_sString, nStart, m_nPosition - nStart);
    }

    /// <summary>
    /// Reads from the current quote to the next unescaped quote, unescaping any other characters along the way.
    /// </summary>
    _NODISCARD std::string ReadQuotedString();

    /// <summary>
    /// Advances the cursor over digits and returns the number they represent.
    /// </summary>
    _NODISCARD unsigned int ReadNumber()
    {
        if (EndOfString())
            return 0;

        char* pEnd;
        const auto nResult = strtoul(&m_sString.at(m_nPosition), &pEnd, 10);
        m_nPosition = pEnd - m_sString.c_str();
        return nResult;
    }

    /// <summary>
    /// Returns the number represented by the next series of digits.
    /// </summary>
    _NODISCARD unsigned int PeekNumber()
    {
        if (EndOfString())
            return 0;

        char* pEnd;
        return strtoul(&m_sString.at(m_nPosition), &pEnd, 10);
    }

    /// <summary>
    /// If the next character is the specified character, advance the cursor over it.
    /// </summary>
    /// <returns><c>true</c> if the next character matched and was skipped over, <c>false</c> if not.
    bool Consume(char c)
    {
        if (EndOfString())
            return false;
        if (m_sString.at(m_nPosition) != c)
            return false;
        ++m_nPosition;
        return true;
    }

    /// <summary>
    /// Gets the raw pointer to the specified offset within the string.
    /// </summary>
    GSL_SUPPRESS_F6 const char* GetPointer(size_t nPosition) const noexcept
    {
        if (nPosition >= m_sString.length())
            return &m_sString.back() + 1;

        return &m_sString.at(nPosition);
    }

private:
    const std::string& m_sString;
    size_t m_nPosition = 0;
};

// ----- other -----

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
///   the character type specified
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
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
GSL_SUPPRESS_F6 _NODISCARD bool StringStartsWith(_In_ const std::basic_string<CharT>& sString,
                                                 _In_ const std::basic_string<CharT>& sMatch) noexcept
{
    if (sMatch.length() > sString.length())
        return false;

    return (sString.compare(0, sMatch.length(), sMatch) == 0);
}

/// <summary>
/// Determines if <paramref name="sString" /> starts with <paramref name="sMatch" />.
/// </summary>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
GSL_SUPPRESS_F6 _NODISCARD bool StringStartsWith(_In_ const std::basic_string<CharT>& sString,
                                                 _In_ const CharT* restrict sMatch) noexcept
{
    const auto sMatchLen = tcslen_s(sMatch);
    if (sMatchLen > sString.length())
        return false;

    return (sString.compare(0, sMatchLen, sMatch) == 0);
}

/// <summary>
/// Determines if <paramref name="sString" /> starts with <paramref name="sMatch" />.
/// </summary>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
GSL_SUPPRESS_F6 _NODISCARD bool StringStartsWith(_In_ const CharT* restrict sString,
                                                 _In_ const CharT* restrict sMatch) noexcept
{
    const auto sMatchLen = tcslen_s(sMatch);
    if (sMatchLen > tcslen_s(sString))
        return false;

    return (std::basic_string<CharT>{sString}.compare(0, sMatchLen, sMatch) == 0);
}

/// <summary>
/// Determines if <paramref name="sString" /> ends with <paramref name="sMatch" />.
/// </summary>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
GSL_SUPPRESS_F6 _NODISCARD bool StringEndsWith(_In_ const std::basic_string<CharT>& sString,
                                               _In_ const std::basic_string<CharT>& sMatch) noexcept
{
    if (sMatch.length() > sString.length())
        return false;

    return (sString.compare(sString.length() - sMatch.length(), sMatch.length(), sMatch) == 0);
}

/// <summary>
/// Determines if <paramref name="sString" /> ends with <paramref name="sMatch" />.
/// </summary>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
GSL_SUPPRESS_F6 _NODISCARD bool StringEndsWith(_In_ const std::basic_string<CharT>& sString,
                                               _In_ const CharT* restrict sMatch) noexcept
{
    const auto sMatchLen = tcslen_s(sMatch);
    if (sMatchLen > sString.length())
        return false;

    return (sString.compare(sString.length() - sMatchLen, sMatchLen, sMatch) == 0);
}

/// <summary>
/// Determines if <paramref name="sString" /> ends with <paramref name="sMatch" />.
/// </summary>
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
GSL_SUPPRESS_F6 _NODISCARD bool StringEndsWith(_In_ const CharT* restrict sString,
                                               _In_ const CharT* restrict sMatch) noexcept
{
    const auto sMatchLen = tcslen_s(sMatch);
    const auto sStringLen = tcslen_s(sString);
    if (sMatchLen > sStringLen)
        return false;

    return (std::basic_string<CharT>{sString}.compare(sStringLen - sMatchLen, sMatchLen, sMatch) == 0);
}

} // namespace ra

#ifdef UNICODE
#define NativeStr(x) ra::Widen(x)
#define NativeStrType std::wstring
#else
#define NativeStr(x) ra::Narrow(x)
#define NativeStrType std::string
#endif

#endif // !RA_STRINGUTILS_H
