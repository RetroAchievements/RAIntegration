#include "Strings.hh"

#include "TypeCasts.hh"

// TODO: not rely on Windows API
// for ::MultiByteToWideChar and ::WideCharToMultiByte
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace ra {

_Use_decl_annotations_
std::string Narrow(const std::wstring& wstr)
{
    return Narrow(wstr.c_str());
}

std::string Narrow(std::wstring&& wstr) noexcept
{
    const auto wwstr{ std::move_if_noexcept(wstr) };
    return Narrow(wwstr);
}

_Use_decl_annotations_
std::string Narrow(const wchar_t* wstr)
{
    const auto len = gsl::narrow_cast<int>(std::wcslen(wstr));
    const auto needed = ::WideCharToMultiByte(CP_UTF8, 0, wstr, len + 1, nullptr, 0, nullptr, nullptr);

    std::string str(ra::to_unsigned(needed), '\000'); // allocate required space (including terminator)
    ::WideCharToMultiByte(CP_UTF8, 0, wstr, len + 1, str.data(), gsl::narrow_cast<unsigned int>(str.capacity()),
        nullptr, nullptr);
    str.resize(ra::to_unsigned(needed - 1)); // terminator is not actually part of the string
    return str;
}

_Use_decl_annotations_
std::wstring Widen(const std::string& str)
{
    return Widen(str.c_str());
}

std::wstring Widen(std::string&& str) noexcept
{
    const auto sstr{ std::move_if_noexcept(str) };
    return Widen(sstr);
}

_Use_decl_annotations_
std::wstring Widen(const char* str)
{
    const auto len = gsl::narrow_cast<int>(std::strlen(str));
    const auto needed = ::MultiByteToWideChar(CP_UTF8, 0, str, len + 1, nullptr, 0);
    // doesn't seem wchar_t is treated like a character type by default
    std::wstring wstr(ra::to_unsigned(needed), L'\x0');
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, len + 1, wstr.data(),
        gsl::narrow_cast<unsigned int>(wstr.capacity()));
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
            sError = ra::StringPrintf(L"Value cannot exceed %lu", nMaximumValue);
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
        sError = ra::StringPrintf(L"Value cannot exceed %lu", nMaximumValue);
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
            sError = ra::StringPrintf(L"Value cannot exceed 0x%02X", nMaximumValue);
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
        sError = ra::StringPrintf(L"Value cannot exceed 0x%02X", nMaximumValue);
    }

    return false;
}

_Use_decl_annotations_
bool ParseNumeric(const std::wstring& sValue, _Out_ unsigned int& nValue, _Out_ std::wstring& sError)
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
std::wstring& Trim(std::wstring& str)
{
    while (!str.empty() && iswspace(str.back()))
        str.pop_back();

    size_t nIndex = 0;
    while (nIndex < str.length() && iswspace(str.at(nIndex)))
        ++nIndex;

    if (nIndex > 0)
        str.erase(0, nIndex);

    return str;
}

_Use_decl_annotations_
std::wstring& NormalizeLineEndings(std::wstring& str)
{
    size_t nIndex = 0;
    do
    {
        nIndex = str.find('\n', nIndex);
        if (nIndex == std::wstring::npos)
            break;

        if (nIndex == 0 || str.at(nIndex - 1) != '\r')
            str.insert(str.begin() + nIndex++, '\r');

        ++nIndex;
    } while (true);

    return str;
}

_Use_decl_annotations_
const std::string FormatDateTime(time_t when)
{
    struct tm tm;
    if (localtime_s(&tm, &when) == 0)
    {
        char buffer[64];
        if (std::strftime(buffer, sizeof(buffer), "%a %e %b %Y %H:%M:%S", &tm) > 0)
            return std::string(buffer);
    }

    return std::to_string(when);
}

_Use_decl_annotations_
const std::string FormatDate(time_t when)
{
    struct tm tm;
    if (localtime_s(&tm, &when) == 0)
    {
        char buffer[64];
        if (std::strftime(buffer, sizeof(buffer), "%a %e %b %Y", &tm) > 0)
            return std::string(buffer);
    }

    return std::to_string(when);
}

_Use_decl_annotations_
const std::string FormatDateRecent(time_t when)
{
    auto now = std::time(nullptr);

    struct tm tm;
    if (localtime_s(&tm, &now) == 0)
        now = now - (static_cast<time_t>(tm.tm_hour) * 60 * 60) - (static_cast<time_t>(tm.tm_min) * 60) - tm.tm_sec; // round to midnight

    const auto days = (now + (60 * 60 * 24) - when) / (60 * 60 * 24);

    if (days < 1)
        return "Today";
    if (days < 2)
        return "Yesterday";
    if (days <= 30)
        return ra::StringPrintf("%u days ago", days);

    struct tm tm_when;
    if (localtime_s(&tm_when, &when) != 0)
        return ra::StringPrintf("%u days ago", days);

    const auto months = (tm.tm_mon + 12 * tm.tm_year) - (tm_when.tm_mon + 12 * tm_when.tm_year);
    if (months < 1)
        return "This month";
    if (months < 2)
        return "Last month";
    if (months < 12)
        return ra::StringPrintf("%u months ago", months);

    const auto years = tm.tm_year - tm_when.tm_year;
    if (years < 2)
        return "Last year";

    return ra::StringPrintf("%u years ago", years);
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

            case PendingString::Type::CharRef:
                nNeeded += std::get<std::string_view>(pPending.Ref).length();
                break;

            case PendingString::Type::WCharRef:
                pPending.String = ra::Narrow(std::wstring{std::get<std::wstring_view>(pPending.Ref)});
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

            case PendingString::Type::CharRef:
                sResult.append(std::get<std::string_view>(pPending.Ref));
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

            case PendingString::Type::WCharRef:
                nNeeded += std::get<std::wstring_view>(pPending.Ref).length();
                break;

            case PendingString::Type::CharRef:
                pPending.WString = ra::Widen(std::string{std::get<std::string_view>(pPending.Ref)});
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

            case PendingString::Type::WCharRef:
                sResult.append(std::wstring{std::get<std::wstring_view>(pPending.Ref)});
                break;
        }
    }
}

std::string Tokenizer::ReadQuotedString()
{
    std::string sString;
    if (PeekChar() != '"')
        return sString;

    ++m_nPosition;
    while (m_nPosition < m_sString.length())
    {
        const auto c = m_sString.at(m_nPosition++);
        if (c == '"')
            break;

        if (c != '\\')
        {
            sString.push_back(c);
        }
        else
        {
            if (m_nPosition == m_sString.length())
                break;

            const auto c2 = m_sString.at(m_nPosition++);
            switch (c2)
            {
                case 'n':
                    sString.push_back('\n');
                    break;
                case 'r':
                    sString.push_back('\r');
                    break;
                case 't':
                    sString.push_back('\t');
                    break;
                default:
                    sString.push_back(c2);
                    break;
            }
        }
    }

    return sString;
}

_Use_decl_annotations_
void StringMakeLowercase(std::wstring& sString)
{
    std::transform(sString.begin(), sString.end(), sString.begin(), [](wchar_t c) noexcept {
        return gsl::narrow_cast<wchar_t>(std::tolower(c));
    });
}

_Use_decl_annotations_
void StringMakeLowercase(std::string& sString)
{
    std::transform(sString.begin(), sString.end(), sString.begin(), [](wchar_t c) noexcept {
        return gsl::narrow_cast<char>(std::tolower(c));
    });
}

#pragma warning(push)
#pragma warning(disable : 5045)

GSL_SUPPRESS_F6 _NODISCARD bool StringContainsCaseInsensitive(_In_ const std::wstring& sString, _In_ const std::wstring& sMatch, bool bMatchIsLowerCased) noexcept
{
    if (sString.length() < sMatch.length())
        return false;

    std::wstring sMatchLower;
    const wchar_t* pMatch = sMatch.c_str();
    if (!bMatchIsLowerCased)
    {
        sMatchLower = sMatch;
        StringMakeLowercase(sMatchLower);

        pMatch = sMatchLower.data();
    }
    Ensures(pMatch != nullptr);

    const wchar_t* pScan = sString.c_str();
    Ensures(pScan != nullptr);

    const wchar_t* pEnd = pScan + sString.length() - sMatch.length() + 1;
    while (pScan < pEnd)
    {
        if (std::tolower(*pScan) == *pMatch)
        {
            bool bMatch = true;
            for (size_t i = 1; i < sMatch.length(); i++)
            {
                if (std::tolower(pScan[i]) != pMatch[i])
                {
                    bMatch = false;
                    break;
                }
            }

            if (bMatch)
                return true;
        }

        ++pScan;
    }

    return false;
}

std::string Base64(const std::string& sString)
{
    const char base64EncTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string sEncoded;
    sEncoded.reserve(((sString.length() + 2) / 3) * 4 + 1);

    const size_t nLength = sString.length();
    size_t i = 0;
    while (i < nLength)
    {
        GSL_SUPPRESS_BOUNDS4 const uint8_t a = sString[i++];
        GSL_SUPPRESS_BOUNDS4 const uint8_t b = (i < nLength) ? sString[i] : 0; i++;
        GSL_SUPPRESS_BOUNDS4 const uint8_t c = (i < nLength) ? sString[i] : 0; i++;
        const uint32_t merged = (a << 16 | b << 8 | c);

        GSL_SUPPRESS_BOUNDS4 sEncoded.push_back(base64EncTable[(merged >> 18) & 0x3F]);
        GSL_SUPPRESS_BOUNDS4 sEncoded.push_back(base64EncTable[(merged >> 12) & 0x3F]);
        GSL_SUPPRESS_BOUNDS4 sEncoded.push_back(base64EncTable[(merged >> 6) & 0x3F]);
        GSL_SUPPRESS_BOUNDS4 sEncoded.push_back(base64EncTable[(merged) & 0x3F]);
    }

    if (i > nLength)
    {
        sEncoded.pop_back();
        if (i - 1 > nLength)
        {
            sEncoded.pop_back();
            sEncoded.push_back('=');
        }
        sEncoded.push_back('=');
    }

    return sEncoded;
}

#pragma warning(pop)

} /* namespace ra */
