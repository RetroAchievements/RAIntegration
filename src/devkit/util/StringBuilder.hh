#ifndef RA_UTIL_STRINGBUILDER_HH
#define RA_UTIL_STRINGBUILDER_HH
#pragma once

#include "GSL.hh"
#include "TypeCasts.hh"
#include "TypeTraits.hh"

#include <iomanip>
#include <sstream>
#include <string>
#include <variant>

namespace ra {
namespace util {

class StringBuilder
{
public:
    explicit StringBuilder(bool bPrepareWide = false) noexcept : m_bPrepareWide(bPrepareWide) {}

    /// <summary>
    /// Appends a non-string value.
    /// </summary>
    /// <remarks>A <see cref="ToWString" /> and <see cref="ToAString" /> method must exist for <typeparm name="T" />.
    template<typename T>
    void Append(const T& arg)
    {
        if (m_bPrepareWide)
            m_vPending.emplace_back(std::wstring{ ToWString(arg) });
        else
            m_vPending.emplace_back(std::string{ ToAString(arg) });
    }

    /// <summary>
    /// Appends a UTF-8 string value.
    /// </summary>
    template<>
    void Append(const std::string& arg)
    {
        m_vPending.emplace_back(arg.c_str(), arg.length());
    }

    /// <summary>
    /// Appends a Unicode string value.
    /// </summary>
    template<>
    void Append(const std::wstring& arg)
    {
        m_vPending.emplace_back(arg.c_str(), arg.length());
    }

    /// <summary>
    /// Appends a UTF-8 string value.
    /// </summary>
    template<>
    void Append(const std::string_view& arg)
    {
        m_vPending.emplace_back(arg.data(), arg.length());
    }

    /// <summary>
    /// Appends a Unicode string value.
    /// </summary>
    template<>
    void Append(const std::wstring_view& arg)
    {
        m_vPending.emplace_back(arg.data(), arg.length());
    }

    /// <summary>
    /// Appends a UTF-8 string value.
    /// </summary>
    void Append(std::string&& arg)
    {
        m_vPending.emplace_back(std::move(arg));
    }

    /// <summary>
    /// Appends a Unicode string value.
    /// </summary>
    void Append(std::wstring&& arg)
    {
        m_vPending.emplace_back(std::move(arg));
    }

    /// <summary>
    /// Appends a UTF-8 string value.
    /// </summary>
    template<>
    void Append(const char* const& arg)
    {
        const auto nLength = strlen(arg);
        if (nLength > 0)
            Append(std::string_view(arg, nLength));
    }

    /// <summary>
    /// Appends a Unicode string value.
    /// </summary>
    template<>
    void Append(const wchar_t* const& arg)
    {
        const auto nLength = wcslen(arg);
        if (nLength > 0)
            Append(std::wstring_view(arg, nLength));
    }

    /// <summary>
    /// Appends a non-string value, followed by additional items.
    /// </summary>
    template<typename T, typename... Ts>
    void Append(const T&& arg, Ts&&... args)
    {
        Append(arg);
        Append(std::forward<Ts>(args)...);
    }

    /// <summary>
    /// Appends a UTF-8 string value, followed by additional items.
    /// </summary>
    template<typename... Ts>
    void Append(const char* const& arg, Ts&&... args)
    {
        const auto nLength = strlen(arg);
        if (nLength > 0)
            Append(std::string_view(arg, nLength));

        Append(std::forward<Ts>(args)...);
    }

    /// <summary>
    /// Appends a Unicode string value, followed by additional items.
    /// </summary>
    template<typename... Ts>
    void Append(const wchar_t* const& arg, Ts&&... args)
    {
        const auto nLength = wcslen(arg);
        if (nLength > 0)
            Append(std::wstring_view(arg, nLength));

        Append(std::forward<Ts>(args)...);
    }

    /// <summary>
    /// Appends a value using the specified printf-style format.
    /// </summary>
    /// <param name="arg">The value to append.</param>
    /// <param name="sFormat">The format to apply.</param>
    template<typename T>
    void AppendFormat([[maybe_unused]] const T& arg, [[maybe_unused]] const std::string& sFormat)
    {
        std::ostringstream oss;

        if constexpr (std::is_integral_v<T>)
        {
            if (sFormat.back() == 'c')
                oss << arg;
            else
            {
                if (std::isdigit(ra::to_unsigned(sFormat.front())))
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
                const auto nIndex = sFormat.find('.');
                if (nIndex != std::string::npos)
                {
                    const int nPrecision = std::stoi(sFormat.c_str() + nIndex + 1);
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

    /// <summary>
    /// Appends a value using the specified printf-style format.
    /// </summary>
    /// <param name="arg">The value to append.</param>
    /// <param name="sFormat">The format to apply.</param>
    void AppendFormat(const char* arg, const std::string& sFormat)
    {
        if (sFormat.front() == '.')
        {
            const int nCharacters = std::stoi(&sFormat.at(1));
            Append(std::string_view(arg, nCharacters));
        }
        else
        {
            const int nLength = gsl::narrow_cast<int>(strlen(arg));
            int nPadding = std::stoi(sFormat.c_str());
            nPadding -= nLength;
            if (nPadding > 0)
                m_vPending.emplace_back(std::string(nPadding, ' '));

            Append(std::string_view(arg, nLength));
        }
    }

    /// <summary>
    /// Appends values using a sprintf-style format specifier.
    /// </summary>
    /// <param name="pFormat">The format to apply.</param>
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

    /// <summary>
    /// Appends values using a sprintf-style format specifier.
    /// </summary>
    /// <param name="pFormat">The format to apply.</param>
    template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>, typename T, typename... Ts>
    void AppendPrintf(const CharT* const _RESTRICT pFormat, const T& _RESTRICT value, Ts&&... args)
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
                    sFormat.append(ToAString(value));
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

    /// <summary>
    /// Generates a single UTF-8 string from all of the <see cref="Append" />ed values.
    /// </summary>
    std::string ToString() const
    {
        std::string sResult;
        AppendToString(sResult);
        return sResult;
    }

    /// <summary>
    /// Appends all of the <see cref="Append" />ed values to a single UTF-8 string.
    /// </summary>
    void AppendToString(_Inout_ std::string& sResult) const;

    /// <summary>
    /// Generates a single Unicode string from all of the <see cref="Append" />ed values.
    /// </summary>
    std::wstring ToWString() const
    {
        std::wstring sResult;
        AppendToWString(sResult);
        return sResult;
    }

    /// <summary>
    /// Appends all of the <see cref="Append" />ed values to a single Unicode string.
    /// </summary>
    void AppendToWString(_Inout_ std::wstring& sResult) const;

    _NODISCARD static std::string Narrow(_In_ const std::wstring_view wstr);
    _NODISCARD static std::wstring Widen(_In_ const std::string_view str);

private:
    template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>, typename T, typename... Ts>
    void AppendPrintfParameterizedFormat(const CharT* const pFormat, const std::string& sFormat, const T& value, Ts&&... args)
    {
        AppendFormat(value, sFormat);
        AppendPrintf(pFormat, std::forward<Ts>(args)...);
    }

    void Append() noexcept
    {
        // just in case one of the variadic methods gets called with 0 parameters
    }

    void AppendSubString(const char* pStart, size_t nLength)
    {
        if (nLength > 0)
            m_vPending.emplace_back(pStart, nLength);
    }

    void AppendSubString(const wchar_t* pStart, size_t nLength)
    {
        if (nLength > 0)
            m_vPending.emplace_back(pStart, nLength);
    }

    // ----- ToAString -----

    template<typename T>
    _NODISCARD inline const std::string ToAString(_In_ const T& value)
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
            static_assert(std::is_arithmetic_v<T>, "No ToString implementation for type");
            return std::string();
        }
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ const std::string& value)
    {
        return value;
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ const std::wstring& value)
    {
        return Narrow(value);
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ const std::string_view& value)
    {
        return std::string(value);
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ const std::wstring_view& value)
    {
        return Narrow(std::wstring(value));
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ const wchar_t* const& value)
    {
        return Narrow(value);
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ const char* const& value)
    {
        return std::string(value);
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ char* const& value)
    {
        return std::string(value);
    }

    template<>
    _NODISCARD inline const std::string ToAString(_In_ const char& value)
    {
        return std::string(1, value);
    }

    // literal strings can't be passed by reference, so won't call the templated methods
    _NODISCARD inline const std::string ToAString(_In_ const char* value) { return std::string(value); }
    _NODISCARD inline const std::string ToAString(_In_ const wchar_t* value) { return Narrow(value); }

    // ----- ToWString -----

    template<typename T>
    _NODISCARD inline const std::wstring ToWString(_In_ const T& value)
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            if constexpr (std::is_same_v<T, char>)
            {
                std::wstring wc{ value };
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
            static_assert(std::is_arithmetic_v<T>, "No ToWString implementation for type");
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
        return Widen(value);
    }

    template<>
    _NODISCARD inline const std::wstring ToWString(_In_ const std::wstring_view& value)
    {
        return std::wstring(value);
    }

    template<>
    _NODISCARD inline const std::wstring ToWString(_In_ const std::string_view& value)
    {
        return Widen(std::string(value));
    }

    template<>
    _NODISCARD inline const std::wstring ToWString(_In_ const wchar_t* const& value)
    {
        return std::wstring(value);
    }

    template<>
    _NODISCARD inline const std::wstring ToWString(_In_ const char* const& value)
    {
        return Widen(value);
    }

    template<>
    _NODISCARD inline const std::wstring ToWString(_In_ char* const& value)
    {
        return Widen(value);
    }

    template<>
    _NODISCARD inline const std::wstring ToWString(_In_ const char& value)
    {
        return std::wstring(1, value);
    }

    // literal strings can't be passed by reference, so won't call the templated methods
    _NODISCARD inline const std::wstring ToWString(_In_ const char* value) { return Widen(value); }
    _NODISCARD inline const std::wstring ToWString(_In_ const wchar_t* value) { return std::wstring(value); }

    class PendingString
    {
    public:
        explicit PendingString(std::string&& arg) noexcept : String{std::move(arg)}, DataType{Type::String} {}
        explicit PendingString(std::wstring&& arg) noexcept : WString{std::move(arg)}, DataType{Type::WString} {}
        explicit PendingString(_In_z_ const char* const _RESTRICT ptr, std::size_t len) noexcept :
            Ref{std::string_view{ptr, len}},
            DataType{Type::CharRef}
        {}
        explicit PendingString(_In_z_ const wchar_t* const _RESTRICT ptr, std::size_t len) noexcept :
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
    bool m_bPrepareWide;
};

} // namespace util
} // namespace ra

#endif // !RA_UTIL_STRINGBUILDER_HH
