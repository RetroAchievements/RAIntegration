#include "Strings.hh"

#include "TypeCasts.hh"

namespace ra {

static size_t CalculateUtf8Length(const wchar_t* sText, size_t nTextLength, bool& bAsciiOnlyOut)
{
    bool bAsciiOnly = true;
    size_t nUtf8Length = 0;

    const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(sText);
    const uint16_t* pStop = pSrc + nTextLength;
    while (pSrc < pStop)
    {
        const auto c = *pSrc++;
        if (c < 0x80)
        {
            ++nUtf8Length;
            continue;
        }

        bAsciiOnly = false;

        if (c < 0x800)
        {
            nUtf8Length += 2;
        }
        else if (c >= 0xD800 && c <= 0xDBFF && *pSrc >= 0xDC00 && *pSrc <= 0xDFFF)
        {
            // two surrogate pairs will take four bytes total
            nUtf8Length += 4;
            ++pSrc;
        }
        else
        {
            nUtf8Length += 3;
        }
    }

    bAsciiOnlyOut = bAsciiOnly;
    return nUtf8Length;
}

static std::string Narrow(const wchar_t* sText, size_t sTextLength)
{
    bool bAsciiOnly = true;
    const auto nUtf8Length = CalculateUtf8Length(sText, sTextLength, bAsciiOnly);

    std::string sResult;
    sResult.resize(nUtf8Length);
    uint8_t* pOut = reinterpret_cast<uint8_t*>(sResult.data());

    const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(sText);
    const uint16_t* pStop = pSrc + sTextLength;

    if (bAsciiOnly)
    {
        while (pSrc < pStop)
            *pOut++ = gsl::narrow_cast<uint8_t>(*pSrc++);
        *pOut = '\0';
        return sResult;
    }

    while (pSrc < pStop)
    {
        uint16_t c = *pSrc++;
        if (c < 0x80)
        {
            *pOut++ = gsl::narrow_cast<uint8_t>(c);
            continue;
        }

        if (c < 0x800)
        {
            *pOut++ = 0xC0 | (c >> 6);
            *pOut++ = 0x80 | (c & 0x3F);
            continue;
        }

        if (c >= 0xD800 && c <= 0xDFFF)
        {
            if (c >= 0xDC00)
            {
                // second part of surrogate pair after first
                c = 0xFFFD;
            }
            else
            {
                const uint16_t c2 = *pSrc;
                if (c2 < 0xDC00 || c2 > 0xDFFF)
                {
                    // not second part of surrogate pair
                    c = 0xFFFD;
                }
                else
                {
                    // decode surrogate pair
                    uint32_t cx = (((c & 0x03FF) << 10) | (c2 & 0x3FF)) + 0x10000;
                    *pOut++ = 0xF0 | (cx >> 18);
                    *pOut++ = 0x80 | ((cx >> 12) & 0x3F);
                    *pOut++ = 0x80 | ((cx >> 6) & 0x3F);
                    *pOut++ = 0x80 | (cx & 0x3F);
                    ++pSrc;
                    continue;
                }
            }
        }

        *pOut++ = 0xE0 | (c >> 12);
        *pOut++ = 0x80 | ((c >> 6) & 0x3F);
        *pOut++ = 0x80 | (c & 0x3F);
    }

    *pOut = '\0';
    const auto nActualSize = pOut - reinterpret_cast<uint8_t*>(sResult.data());
    sResult.resize(nActualSize);
    return sResult;
}

_Use_decl_annotations_
std::string Narrow(const std::wstring& wstr)
{
    return Narrow(wstr.c_str(), wstr.length());
}

std::string Narrow(std::wstring&& wstr) noexcept
{
    const auto wwstr{ std::move_if_noexcept(wstr) };
    return Narrow(wwstr);
}

_Use_decl_annotations_
std::string Narrow(const wchar_t* wstr)
{
    return Narrow(wstr, wcslen(wstr));
}

// https://en.wikipedia.org/wiki/UTF-8#Byte_map
static const uint8_t UTF_NUM_TRAIL_BYTES[128] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x80-0x8f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x90-0x9f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xa0-0xaf
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xb0-0xbf
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0xc0-0xcf
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0xd0-0xdf
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // 0xe0-0xef
    3,3,3,3,3,3,3,3,4,4,4,4,5,5,0,0  // 0xf0-0xff
};

static size_t CalculateUnicodeLength(const char* sText, size_t nTextLength, bool& bAsciiOnlyOut)
{
    bool bAsciiOnly = true;

    size_t nUnicodeLength = 0;
    size_t nRemaining = nTextLength;
    const uint64_t* p64 = reinterpret_cast<const uint64_t*>(sText);
    while (nRemaining > 8) {
        // if any byte is not an ASCII character, switch to the slower algorithm
        if (*p64 & 0x8080808080808080)
            break;

        ++p64;
        nUnicodeLength += 8;
        nRemaining -= 8;
    }

    const uint8_t* p8 = reinterpret_cast<const uint8_t*>(p64);
    while (nRemaining) {
        --nRemaining;
        ++nUnicodeLength;

        const uint8_t c = *p8++;
        if ((c & 0x80) == 0)
        {
            // single byte character
            continue;
        }

        bAsciiOnly = false;

        if ((c & 0x40) == 0)
        {
            // trail byte without lead
            continue;
        }

        auto nAdditional = UTF_NUM_TRAIL_BYTES[c & 0x7F];
        if (nAdditional > nRemaining) // not enough remaining
            break;

        if (nAdditional > 3)
        {
            // 5/6 byte UTF-8 sequences not supported in UTF-16 - will be replaced with xFFFD
        }
        else if (nAdditional == 3)
        {
            // extra space for surrogate pair
            ++nUnicodeLength;
        }

        nRemaining -= nAdditional;
    }

    bAsciiOnlyOut = bAsciiOnly;
    return nUnicodeLength;
}

static std::wstring Widen(const char* sText, size_t nTextLength)
{
    bool bAsciiOnly = true;
    const auto nUnicodeLength = CalculateUnicodeLength(sText, nTextLength, bAsciiOnly);

    std::wstring sResult;
    sResult.resize(nUnicodeLength);
    uint16_t* pOut = reinterpret_cast<uint16_t*>(sResult.data());

    const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(sText);
    const uint8_t* pStop = pSrc + nTextLength;

    if (bAsciiOnly)
    {
        while (pSrc < pStop)
            *pOut++ = gsl::narrow_cast<uint16_t>(*pSrc++);
        *pOut = '\0';
        return sResult;
    }

    while (pSrc < pStop)
    {
        const uint8_t c = *pSrc++;
        if ((c & 0x80) == 0)
        {
            *pOut++ = gsl::narrow_cast<uint16_t>(c);
            continue;
        }

        if ((c & 0xC0) == 0x80)
        {
            // trail byte
            *pOut++ = 0xFFFD;
            continue;
        }

        auto nAdditional = UTF_NUM_TRAIL_BYTES[c & 0x7F];
        if (pSrc + nAdditional > pStop)
        {
            // not enough data
            *pOut++ = 0xFFFD;
            break;
        }

        uint32_t nAccumulator = gsl::narrow_cast<uint32_t>(c);

        // 1 additional -> 0x1F, 2 -> 0x0F, 3 -> 0x07, 4 -> 0x03, 5 -> 0x01
        nAccumulator &= (1 << (6 - nAdditional)) - 1;

        bool bInvalid = false;
        while (nAdditional)
        {
            const char c2 = *pSrc++;
            bInvalid |= ((c2 & 0xC0) != 0x80);
            nAccumulator <<= 6;
            nAccumulator |= (c2 & 0x3F);
            --nAdditional;
        }

        if (bInvalid)
        {
            *pOut++ = 0xFFFD;
        }
        else if (nAccumulator < 0xFFFF)
        {
            *pOut++ = gsl::narrow_cast<uint16_t>(nAccumulator & 0xFFFF);
        }
        else if (nAccumulator < 0x110000)
        {
            // convert to surrogate pair
            nAccumulator -= 0x10000;
            *pOut++ = 0xD800 | gsl::narrow_cast<uint16_t>(nAccumulator >> 10);
            *pOut++ = 0xDC00 | gsl::narrow_cast<uint16_t>(nAccumulator & 0x03FF);
        }
        else
        {
            // 5/6 byte UTF-8 characters not supported by UTF-16.
            *pOut++ = 0xFFFD;
        }
    }

    *pOut = '\0';
    const auto nActualSize = pOut - reinterpret_cast<uint16_t*>(sResult.data());
    sResult.resize(nActualSize);
    return sResult;
}

_Use_decl_annotations_
std::wstring Widen(const std::string& str)
{
    return Widen(str.c_str(), str.length());
}

std::wstring Widen(std::string&& str) noexcept
{
    const auto sstr{ std::move_if_noexcept(str) };
    return Widen(sstr);
}

_Use_decl_annotations_
std::wstring Widen(const char* str)
{
    return Widen(str, strlen(str));
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
