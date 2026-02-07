#include "StringBuilder.hh"

namespace ra {
namespace util {

static std::pair<size_t, bool> CalculateUtf8Length(std::wstring_view str) noexcept
{
    if (str.empty())
        return std::make_pair(0, false);

    bool bAsciiOnly = true;
    size_t nUtf8Length = 0;

    GSL_SUPPRESS_TYPE1 const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(str.data());
    if (pSrc)
    {
        const uint16_t* pStop = pSrc + str.length();
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
    }

    return std::make_pair(nUtf8Length, bAsciiOnly);
}

_Use_decl_annotations_
std::string StringBuilder::Narrow(std::wstring_view str)
{
    if (str.empty())
        return {};

    const auto [nUtf8Length, bAsciiOnly] = CalculateUtf8Length(str);

    std::string sResult;
    sResult.resize(nUtf8Length);
    GSL_SUPPRESS_TYPE1 uint8_t* pOut = reinterpret_cast<uint8_t*>(sResult.data());

    GSL_SUPPRESS_TYPE1 const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(str.data());
    const uint16_t* pStop = pSrc + str.length();

    if (!pOut || !pSrc)
        return sResult;

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
            *pOut++ = 0xC0 | gsl::narrow_cast<uint8_t>(c >> 6);
            *pOut++ = 0x80 | gsl::narrow_cast<uint8_t>(c & 0x3F);
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
                    const uint32_t cx = (((c & 0x03FF) << 10) | (c2 & 0x3FF)) + 0x10000;
                    *pOut++ = 0xF0 | gsl::narrow_cast<uint8_t>(cx >> 18);
                    *pOut++ = 0x80 | gsl::narrow_cast<uint8_t>((cx >> 12) & 0x3F);
                    *pOut++ = 0x80 | gsl::narrow_cast<uint8_t>((cx >> 6) & 0x3F);
                    *pOut++ = 0x80 | gsl::narrow_cast<uint8_t>(cx & 0x3F);
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
    GSL_SUPPRESS_TYPE1 const auto nActualSize = pOut - reinterpret_cast<uint8_t*>(sResult.data());
    sResult.resize(nActualSize);
    return sResult;
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

static std::pair<size_t, bool> CalculateUnicodeLength(std::string_view str) noexcept
{
    if (str.empty())
        return std::make_pair(0, false);

    bool bAsciiOnly = true;

    size_t nUnicodeLength = 0;
    size_t nRemaining = str.length();
    GSL_SUPPRESS_TYPE1 const uint64_t* p64 = reinterpret_cast<const uint64_t*>(str.data());
    if (p64)
    {
        while (nRemaining > 8)
        {
            // if any byte is not an ASCII character, switch to the slower algorithm
            if (*p64 & 0x8080808080808080)
                break;

            ++p64;
            nUnicodeLength += 8;
            nRemaining -= 8;
        }
    }

    GSL_SUPPRESS_TYPE1 const uint8_t* p8 = reinterpret_cast<const uint8_t*>(p64);
    if (p8)
    {
        while (nRemaining)
        {
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

            GSL_SUPPRESS_BOUNDS4 const auto nAdditional = UTF_NUM_TRAIL_BYTES[c & 0x7F];
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
    }

    return std::make_pair(nUnicodeLength, bAsciiOnly);
}

_Use_decl_annotations_
std::wstring StringBuilder::Widen(std::string_view str)
{
    if (str.empty())
        return {};

    const auto [nUnicodeLength, bAsciiOnly] = CalculateUnicodeLength(str);

    std::wstring sResult;
    sResult.resize(nUnicodeLength);
    GSL_SUPPRESS_TYPE1 uint16_t* pOut = reinterpret_cast<uint16_t*>(sResult.data());

    GSL_SUPPRESS_TYPE1 const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(str.data());
    const uint8_t* pStop = pSrc + str.length();

    if (!pOut || !pSrc)
        return sResult;

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

        GSL_SUPPRESS_BOUNDS4 auto nAdditional = UTF_NUM_TRAIL_BYTES[c & 0x7F];
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
            const uint8_t c2 = *pSrc++;
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
    GSL_SUPPRESS_TYPE1 const auto nActualSize = pOut - reinterpret_cast<uint16_t*>(sResult.data());
    sResult.resize(nActualSize);
    return sResult;
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
                pPending.String = Narrow(pPending.WString);
                pPending.DataType = PendingString::Type::String;
                nNeeded += pPending.String.length();
                break;

            case PendingString::Type::CharRef:
                nNeeded += std::get<std::string_view>(pPending.Ref).length();
                break;

            case PendingString::Type::WCharRef:
                pPending.String = Narrow(std::wstring{std::get<std::wstring_view>(pPending.Ref)});
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
                pPending.WString = Widen(pPending.String);
                pPending.DataType = PendingString::Type::WString;
                nNeeded += pPending.WString.length();
                break;

            case PendingString::Type::WCharRef:
                nNeeded += std::get<std::wstring_view>(pPending.Ref).length();
                break;

            case PendingString::Type::CharRef:
                pPending.WString = Widen(std::string{std::get<std::string_view>(pPending.Ref)});
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

} // namespace util
} // namespace ra
