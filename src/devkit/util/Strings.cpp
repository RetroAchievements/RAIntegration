#include "Strings.hh"

#include "TypeCasts.hh"

namespace ra {
namespace util {

_Use_decl_annotations_
std::string& String::TrimLineEnding(std::string& str) noexcept
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
std::wstring& String::Trim(std::wstring& str)
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
std::wstring& String::NormalizeLineEndings(std::wstring& str)
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
const std::wstring String::FormatDateTime(time_t when)
{
    struct tm tm;
    if (localtime_s(&tm, &when) == 0)
    {
        wchar_t buffer[64];
        if (std::wcsftime(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%a %e %b %Y %H:%M:%S", &tm) > 0)
            return std::wstring(buffer);
    }

    return std::to_wstring(when);
}

_Use_decl_annotations_
const std::wstring String::FormatDate(time_t when)
{
    struct tm tm;
    if (localtime_s(&tm, &when) == 0)
    {
        wchar_t buffer[64];
        if (std::wcsftime(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%a %e %b %Y", &tm) > 0)
            return std::wstring(buffer);
    }

    return std::to_wstring(when);
}

_Use_decl_annotations_
const std::wstring String::FormatDateRecent(time_t when)
{
    auto now = std::time(nullptr);

    struct tm tm;
    if (localtime_s(&tm, &now) == 0)
        now = now - (static_cast<time_t>(tm.tm_hour) * 60 * 60) - (static_cast<time_t>(tm.tm_min) * 60) - tm.tm_sec; // round to midnight

    const auto days = (now + (60 * 60 * 24) - when) / (60 * 60 * 24);

    if (days < 1)
        return L"Today";
    if (days < 2)
        return L"Yesterday";
    if (days <= 30)
        return Printf(L"%u days ago", days);

    struct tm tm_when;
    if (localtime_s(&tm_when, &when) != 0)
        return Printf(L"%u days ago", days);

    const auto months = (tm.tm_mon + 12 * tm.tm_year) - (tm_when.tm_mon + 12 * tm_when.tm_year);
    if (months < 1)
        return L"This month";
    if (months < 2)
        return L"Last month";
    if (months < 12)
        return Printf(L"%u months ago", months);

    const auto years = tm.tm_year - tm_when.tm_year;
    if (years < 2)
        return L"Last year";

    return Printf(L"%u years ago", years);
}

_Use_decl_annotations_
void String::MakeLowercase(std::wstring& sString)
{
    std::transform(sString.begin(), sString.end(), sString.begin(), [](wchar_t c) noexcept {
        return gsl::narrow_cast<wchar_t>(std::tolower(c));
    });
}

_Use_decl_annotations_
void String::MakeLowercase(std::string& sString)
{
    std::transform(sString.begin(), sString.end(), sString.begin(), [](wchar_t c) noexcept {
        return gsl::narrow_cast<char>(std::tolower(c));
    });
}

_Use_decl_annotations_
bool String::ContainsCaseInsensitive(std::wstring_view sString, std::wstring_view sMatch) noexcept
{
    auto iter = std::search(
        sString.begin(), sString.end(),
        sMatch.begin(), sMatch.end(),
        [](wchar_t c1, wchar_t c2) noexcept {
            return std::tolower(c1) == std::tolower(c2);
        }
    );

    return (iter != sString.end());
}

std::string String::EncodeBase64(const std::string& sString)
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

} // namespace util
} // namespace ra
