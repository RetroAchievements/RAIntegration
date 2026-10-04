#include "ValueMemoryNoteModel.hh"

#include "context/IConsoleContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

namespace ra {
namespace data {
namespace models {

static uint32_t ConvertNumber(std::wstring_view svValue, bool isHex) noexcept
{
    uint32_t nValue = 0;
    if (isHex)
    {
        for (const auto c : svValue)
        {
            nValue <<= 4;
            if (c <= '9')
                nValue |= (c - '0');
            else if (c <= 'F')
                nValue |= (c - 'A' + 10);
            else
                nValue |= (c - 'a' + 10);
        }
    }
    else
    {
        for (const auto c : svValue)
        {
            nValue *= 10;
            nValue += (c - '0');
        }
    }

    return nValue;
}

static size_t MatchBitPrefix(std::wstring_view svRange)
{
    if (svRange.size() < 2)
        return std::wstring::npos;

    if (svRange.at(0) != 'b' && svRange.at(0) != 'B')
        return std::wstring::npos;

    if (ra::util::String::IsDigit(svRange.at(1)))
        return 1;

    if (svRange.size() < 4)
        return std::wstring::npos;

    if (svRange.at(1) != 'i' && svRange.at(1) != 'I')
        return std::wstring::npos;

    if (svRange.at(2) != 't' && svRange.at(2) != 'T')
        return std::wstring::npos;

    if (ra::util::String::IsDigit(svRange.at(3)))
        return 3;

    if (svRange.size() < 5 || (svRange.at(3) != 's' && svRange.at(3) != 'S'))
        return std::wstring::npos;

    if (ra::util::String::IsDigit(svRange.at(4)))
        return 4;

    return std::wstring::npos;
}

bool ValueMemoryNoteModel::ParseBitRange(std::wstring_view svRange, uint32_t& nLow, uint32_t& nHigh)
{
    size_t nIndex = MatchBitPrefix(svRange);
    if (nIndex == std::wstring::npos)
    {
        if (svRange.substr(0, 4) == L"low4")
        {
            nLow = 4;
            nHigh = 7;
            return true;
        }
        if (svRange.substr(0, 5) == L"high4")
        {
            nLow = 4;
            nHigh = 7;
            return true;
        }
        return false;
    }

    size_t nStart = nIndex;
    while (nIndex < svRange.size() && ra::util::String::IsDigit(svRange.at(nIndex)))
        ++nIndex;

    const auto svLow = svRange.substr(nStart, nIndex - nStart);
    std::wstring_view svHigh;

    while (nIndex < svRange.size() && ra::util::String::IsSpace(svRange.at(nIndex)))
        ++nIndex;

    if (nIndex < svRange.size())
    {
        if (svRange.at(nIndex) != '-')
        {
            if (svRange.substr(nIndex) == L"set")
            {
                nLow = nHigh = ConvertNumber(svLow, false);
                return true;
            }

            return false;
        }

        ++nIndex;
        while (nIndex < svRange.size() && ra::util::String::IsSpace(svRange.at(nIndex)))
            ++nIndex;

        if (nIndex < svRange.size() && !ra::util::String::IsDigit(svRange.at(nIndex)))
        {
            nStart = nIndex;
            nIndex = MatchBitPrefix(svRange.substr(nIndex));
            if (nIndex == std::wstring::npos)
                return false;
            nIndex += nStart;
        }

        nStart = nIndex;
        while (nIndex < svRange.size() && ra::util::String::IsDigit(svRange.at(nIndex)))
            ++nIndex;

        svHigh = svRange.substr(nStart, nIndex - nStart);
    }

    nLow = ConvertNumber(svLow, false);
    nHigh = svHigh.empty() ? nLow : ConvertNumber(svHigh, false);

    return true;
}

static bool ParseRange(std::wstring_view svRange, uint32_t& nLow, uint32_t& nHigh, bool isHex)
{
    if (svRange.size() > 2 && svRange.at(0) == L'0' && svRange.at(1) == L'x')
        svRange = svRange.substr(2);
    else if (svRange.size() > 1 && (svRange.front() == L'h' || svRange.front() == L'H'))
        svRange = svRange.substr(1);
    else if (svRange.size() > 1 && (svRange.back() == L'h' || svRange.back() == L'H'))
        svRange = svRange.substr(0, svRange.size() - 1);

    size_t nIndex = 0;
    while (nIndex < svRange.size() && ra::util::String::IsHexDigit(svRange.at(nIndex)))
        ++nIndex;

    if (nIndex == 0)
        return false;

    const auto svLow = svRange.substr(0, nIndex);
    std::wstring_view svHigh;

    while (nIndex < svRange.size() && ra::util::String::IsSpace(svRange.at(nIndex)))
        ++nIndex;

    if (nIndex < svRange.size())
    {
        if (svRange.at(nIndex++) != '-')
            return false;

        while (nIndex < svRange.size() && ra::util::String::IsSpace(svRange.at(nIndex)))
            ++nIndex;

        const auto nStart = nIndex;
        while (nIndex < svRange.size() && ra::util::String::IsHexDigit(svRange.at(nIndex)))
            ++nIndex;

        if (nIndex == nStart)
            return false;

        if (nIndex < svRange.size() && ra::util::String::IsAlpha(svRange.at(nIndex)))
            return false;

        svHigh = svRange.substr(nStart, nIndex - nStart);
    }

    nLow = ConvertNumber(svLow, isHex);
    nHigh = svHigh.empty() ? nLow : ConvertNumber(svHigh, isHex);

    return true;
}

size_t constexpr ValueMemoryNoteModel::FindValueSplit(const std::wstring_view svLine) noexcept
{
    auto nSplit = svLine.find_first_of(L"=:|");
    if (nSplit == std::wstring::npos)
        nSplit = svLine.find(L"->");

    return nSplit;
}

std::wstring_view ValueMemoryNoteModel::GetValues(const std::wstring_view svLine)
{
    const auto nSplit = FindValueSplit(svLine);
    if (nSplit == std::wstring::npos || nSplit == 0)
        return {};

    const auto nLeftBracket = svLine.find_last_of(L"([{", nSplit);
    if (nLeftBracket != std::wstring::npos)
    {
        // found opening bracket before assignment. look for closing bracket
        size_t nRightBracket = std::wstring::npos;
        switch (svLine.at(nLeftBracket))
        {
            case '(':
                nRightBracket = svLine.find(L')', nSplit);
                break;
            case '[':
                nRightBracket = svLine.find(L']', nSplit);
                break;
            case '{':
                nRightBracket = svLine.find(L'}', nSplit);
                break;
        }

        // no right bracket found. take the rest of the line
        if (nRightBracket == std::wstring::npos)
            return {};

        return svLine.substr(nLeftBracket + 1, nRightBracket - nLeftBracket - 1);
    }

    // skip over any leading non-alphanumeric characters
    if (!ra::util::String::IsAlNum(svLine.at(0)))
    {
        for (size_t nScan = 1; nScan < nSplit; ++nScan)
        {
            if (ra::util::String::IsAlNum(svLine.at(nScan)))
                return GetValues(svLine.substr(nScan));
        }

        // no non-alphanumeric characters before value splitter, abort
        return {};
    }

    // if the line starts with mapped values, return the whole line
    const auto svLeft = svLine.substr(0, nSplit);
    uint32_t nLow, nHigh;
    if (ParseBitRange(svLeft, nLow, nHigh) || ParseRange(svLeft, nLow, nHigh, true))
        return svLine;

    // scan backwards from the splitter to find the first non-alphanumeric non-whitespace character
    auto nIndex = nSplit;
    while (nIndex > 0 && (ra::util::String::IsAlNum(svLine.at(nIndex - 1)) || ra::util::String::IsSpace(svLine.at(nIndex - 1))))
        --nIndex;

    // if nIndex is 0, we should have matched mapped values earlier. must be invalid. ignore.
    if (nIndex == 0 || nIndex == nSplit)
        return {};

    // ignore leading whitespace
    while (ra::util::String::IsSpace(svLine.at(nIndex)))
        ++nIndex;

    // return remainder of line
    return svLine.substr(nIndex);
}

std::wstring_view ValueMemoryNoteModel::MatchSubNote(std::wstring_view svNote, std::function<bool(std::wstring_view)> fMatch)
{
    size_t nStart = 0;
    while (nStart < svNote.size())
    {
        auto nEnd = svNote.find_first_of(L"\n\r", nStart);

        if (nStart != nEnd)
        {
            const auto svLine = nEnd != std::wstring::npos ? svNote.substr(nStart, nEnd - nStart) : svNote.substr(nStart);
            const auto svValues = GetValues(svLine);
            if (!svValues.empty())
            {
                if (svValues == svLine)
                {
                    // If the line starts with mapped values, GetValues() returns the whole line. Try to match it.
                    if (fMatch(svLine))
                        return svLine;
                }
                else
                {
                    // Break the values subclause into individual parts and try to match each.
                    size_t nFront = 0;
                    do {
                        const auto nComma = svValues.find_first_of(L",;", nFront);
                        const auto svValue = (nComma == std::wstring::npos) ? svValues.substr(nFront) : svValues.substr(nFront, nComma - nFront);

                        if (fMatch(svValue))
                            return svValue;

                        if (nComma == std::wstring::npos)
                            break;

                        nFront = nComma + 1;
                        while (nFront < svValues.size() && ra::util::String::IsSpace(svValues.at(nFront)))
                            ++nFront;
                    } while (nFront < svValues.size());
                }
            }
        }

        while (nEnd < svNote.size() && (svNote.at(nEnd) == L'\n' || svNote.at(nEnd) == L'\r'))
            ++nEnd;

        nStart = nEnd;
    }

    return {};
}

MemoryNoteModel::EnumState ValueMemoryNoteModel::DetermineEnumState(std::wstring_view svNote)
{
    EnumState nState = EnumState::None;

    MatchSubNote(svNote, [&nState](std::wstring_view svValue)
    {
        const auto nSplit = FindValueSplit(svValue);
        if (nSplit != std::wstring::npos)
        {
            uint32_t nLow, nHigh;
            const auto svLeft = svValue.substr(0, nSplit);
            if (ParseBitRange(svLeft, nLow, nHigh))
            {
                // if we've already flagged things as dec/hex, don't reclassify to bits (b3 looks like hex)
                if (nState == EnumState::None)
                    nState = EnumState::Bits;
            }
            else if (ParseRange(svLeft, nLow, nHigh, true))
            {
                if (svLeft.find_first_of(L"ABCDEFabcdefHhx") != std::wstring::npos)
                {
                    // hex state is definitive. we can stop scanning
                    nState = EnumState::Hex;
                    return true;
                }

                // a dec value doesn't preclude a hex value appearing later (03, 07, 0E)
                nState = EnumState::Dec;
            }
        }

        return false;
    });

    return nState;
}

bool ValueMemoryNoteModel::MatchEnumText(const std::wstring_view svValue, uint32_t nValue, bool isHex)
{
    const auto nSplit = FindValueSplit(svValue);
    if (nSplit == std::wstring::npos)
        return false;

    const auto svLeft = svValue.substr(0, nSplit);

    uint32_t nLow, nHigh;
    if (ParseRange(svLeft, nLow, nHigh, isHex))
    {
        if (nValue >= nLow && nValue <= nHigh)
            return true;
    }
    else if (svValue.at(nSplit) == L'=')
    {
        auto svRight = svValue.substr(nSplit + 1);
        while (!svRight.empty() && ra::util::String::IsSpace(svRight.at(0)))
            svRight.remove_prefix(1);

        if (ParseRange(svRight, nLow, nHigh, isHex))
        {
            if (nValue >= nLow && nValue <= nHigh)
                return true;
        }
    }

    return false;
}

std::wstring_view ValueMemoryNoteModel::GetEnumText(uint32_t nValue) const
{
    if (m_nEnumState == EnumState::None)
        return {};

    if (m_nEnumState == EnumState::Unknown)
    {
        m_nEnumState = DetermineEnumState(m_svNote);
        if (m_nEnumState == EnumState::None)
            return {};
    }

    const bool isHex = (m_nEnumState == EnumState::Hex || m_nEnumState == EnumState::Bits);
    return MatchSubNote(m_svNote, [nValue, isHex](std::wstring_view svValue) {
        return MatchEnumText(svValue, nValue, isHex);
    });
}

bool ValueMemoryNoteModel::MatchBitsText(const std::wstring_view svValue, ra::data::Memory::Size nBits)
{
    const auto nSplit = FindValueSplit(svValue);
    if (nSplit == std::wstring::npos)
        return false;

    const auto svLeft = svValue.substr(0, nSplit);

    uint32_t nLow, nHigh;
    if (ParseBitRange(svLeft, nLow, nHigh))
    {
        switch (nBits)
        {
            case ra::data::Memory::Size::Bit0: return nLow == 0;
            case ra::data::Memory::Size::Bit1: return nLow <= 1 && nHigh >= 1;
            case ra::data::Memory::Size::Bit2: return nLow <= 2 && nHigh >= 2;
            case ra::data::Memory::Size::Bit3: return nLow <= 3 && nHigh >= 3;
            case ra::data::Memory::Size::Bit4: return nLow <= 4 && nHigh >= 4;
            case ra::data::Memory::Size::Bit5: return nLow <= 5 && nHigh >= 5;
            case ra::data::Memory::Size::Bit6: return nLow <= 6 && nHigh >= 6;
            case ra::data::Memory::Size::Bit7: return nLow <= 7 && nHigh >= 7;
            case ra::data::Memory::Size::NibbleLower: return nLow == 0 && nHigh >= 3;
            case ra::data::Memory::Size::NibbleUpper: return nLow <= 4 && nHigh >= 7;
        }
    }

    return false;
}

std::wstring_view ValueMemoryNoteModel::GetSubNote(ra::data::Memory::Size nBits) const
{
    if (ra::data::Memory::SizeBits(nBits) >= 8 || nBits == ra::data::Memory::Size::BitCount)
        return {};

    if (m_nEnumState != EnumState::Bits && m_nEnumState != EnumState::Unknown)
        return {};

    if (m_nEnumState == EnumState::Unknown)
    {
        m_nEnumState = DetermineEnumState(m_svNote);
        if (m_nEnumState != EnumState::Bits)
            return {};
    }

    return MatchSubNote(m_svNote, [nBits](std::wstring_view svValue) {
        return MatchBitsText(svValue, nBits);
    });
}

static constexpr ra::data::Memory::Format GetNumberFormat(std::wstring_view svValue)
{
    auto nFormat = ra::data::Memory::Format::Dec;

    if (svValue.size() > 2 && svValue.at(0) == L'0' && svValue.at(1) == L'x')
    {
        svValue = svValue.substr(2);
        nFormat = ra::data::Memory::Format::Hex;
    }
    else if (svValue.size() > 1 && (svValue.front() == L'h' || svValue.front() == L'H'))
    {
        svValue = svValue.substr(1);
        nFormat = ra::data::Memory::Format::Hex;
    }
    else if (svValue.size() > 1 && (svValue.back() == L'h' || svValue.back() == L'H'))
    {
        svValue = svValue.substr(0, svValue.size() - 1);
        nFormat = ra::data::Memory::Format::Hex;
    }

    size_t nIndex = 0;
    while (nIndex < svValue.size())
    {
        const auto c = svValue.at(nIndex);
        if (c >= 'a' && c <= 'f')
            nFormat = ra::data::Memory::Format::Hex;
        else if (c >= 'A' && c <= 'F')
            nFormat = ra::data::Memory::Format::Hex;
        else if (c < '0' || c > '9')
            break;

        ++nIndex;
    }

    if (nIndex == 0)
    {
        // did not match any digits
        nFormat = ra::data::Memory::Format::Unknown;
    }
    else if (nIndex < svValue.size() && ra::util::String::IsAlpha(svValue.at(nIndex)))
    {
        // trailing alphabetic characters after matching digits
        nFormat = ra::data::Memory::Format::Unknown;
    }

    return nFormat;
}

void ValueMemoryNoteModel::DeterminePreferredMemFormat()
{
    if (m_nMemFormat != ra::data::Memory::Format::Unknown && m_nMemFormat != ra::data::Memory::Format::Dec)
        return;

    auto nMemFormat = ra::data::Memory::Format::Dec;
    auto bPotentiallyPaddedHex = false;
    auto bAllValuesPotentiallyPaddedHex = true;

    MatchSubNote(m_svNote, [&nMemFormat, &bPotentiallyPaddedHex, &bAllValuesPotentiallyPaddedHex](std::wstring_view svValue)
        {
            const auto nSplit = FindValueSplit(svValue);
            if (nSplit != std::wstring::npos)
            {
                const auto nLeft = svValue.substr(0, nSplit);
                const auto nLeftFormat = GetNumberFormat(nLeft);
                switch (nLeftFormat)
                {
                    case ra::data::Memory::Format::Hex:
                        nMemFormat = ra::data::Memory::Format::Hex;
                        return true;

                    case ra::data::Memory::Format::Dec:
                        nMemFormat = ra::data::Memory::Format::Dec;

                        // if the key has a leading zero and is a multiple of two characters long (i.e. 0027), assume it's hex
                        if (nLeft.length() % 2 != 0)
                            bAllValuesPotentiallyPaddedHex = false;
                        else if (nLeft.at(0) == '0')
                            bPotentiallyPaddedHex = true;
                        break;

                    default:
                    {
                        uint32_t nLow, nHigh;
                        if (ParseBitRange(nLeft, nLow, nHigh))
                        {
                            // found a bit indicator. prefer hex format for value display
                            nMemFormat = ra::data::Memory::Format::Hex;
                            return true;
                        }
                    }
                }
            }

            return false;
        });

    if (nMemFormat == ra::data::Memory::Format::Dec && bPotentiallyPaddedHex && bAllValuesPotentiallyPaddedHex)
        nMemFormat = ra::data::Memory::Format::Hex;

    m_nMemFormat = nMemFormat;
}

std::wstring_view ValueMemoryNoteModel::GetFullSummaryStringView() const
{
    const auto nLineEnd = m_svNote.find_first_of(L"\n\r");
    auto svNote = (nLineEnd != std::string::npos) ? m_svNote.substr(0, nLineEnd) : m_svNote;

    if (m_nEnumState == EnumState::Unknown)
        m_nEnumState = DetermineEnumState(m_svNote);

    if (m_nEnumState != EnumState::None)
    {
        const auto svValues = GetValues(svNote);
        if (!svValues.empty())
        {
            auto nIndex = svNote.find(svValues);

            if (nIndex > 0)
            {
                // ignore bracket and whitespace between summary and enum values
                --nIndex;
                while (nIndex > 0 && ra::util::String::IsSpace(svNote.at(nIndex - 1)))
                    --nIndex;

                if (nIndex > 0 && !ra::util::String::IsAlNum(svNote.at(nIndex - 1)))
                {
                    --nIndex;
                    while (nIndex > 0 && ra::util::String::IsSpace(svNote.at(nIndex - 1)))
                        --nIndex;
                }
            }

            svNote = svNote.substr(0, nIndex);
        }
    }

    while (!svNote.empty() && ra::util::String::IsSpace(svNote.back()))
        svNote = svNote.substr(0, svNote.size() - 1);

    return svNote;
}

} // namespace models
} // namespace data
} // namespace ra
