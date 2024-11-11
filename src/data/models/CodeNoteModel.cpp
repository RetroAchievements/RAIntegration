#include "CodeNoteModel.hh"

#include "data\context\ConsoleContext.hh"
#include "data\context\EmulatorContext.hh"

#include "services\ServiceLocator.hh"

#include "RA_StringUtils.h"
#include "ra_utility.h"

namespace ra {
namespace data {
namespace models {

struct CodeNoteModel::PointerData
{
    uint32_t RawPointerValue = 0xFFFFFFFF;       // last raw value of pointer captured
    ra::ByteAddress PointerAddress = 0xFFFFFFFF; // raw pointer value converted to RA address
    unsigned int OffsetRange = 0;                // highest offset captured within pointer block
    bool HasPointers = false;

    enum OffsetType
    {
        None = 0,
        Converted, // PointerAddress will contain a converted address
        Overflow,  // offset exceeds RA address space, apply to RawPointerValue
    };
    OffsetType OffsetType = OffsetType::None;

    struct OffsetCodeNote
    {
        CodeNoteModel CodeNote;
        int Offset = 0;
    };
    std::vector<OffsetCodeNote> OffsetNotes;
};

// these must be defined here because of forward declaration of PointerData in header file.
CodeNoteModel::CodeNoteModel() noexcept {}
CodeNoteModel::~CodeNoteModel() {}
CodeNoteModel::CodeNoteModel(CodeNoteModel&& pOther) noexcept
    : m_sAuthor(std::move(pOther.m_sAuthor)),
      m_sNote(std::move(pOther.m_sNote)),
      m_nBytes(pOther.m_nBytes),
      m_nMemSize(pOther.m_nMemSize),
      m_pPointerData(std::move(pOther.m_pPointerData))
{
}
CodeNoteModel& CodeNoteModel::operator=(CodeNoteModel&& pOther) noexcept
{
    m_sAuthor = std::move(pOther.m_sAuthor);
    m_sNote = std::move(pOther.m_sNote);
    m_nBytes = pOther.m_nBytes;
    m_nMemSize = pOther.m_nMemSize;
    m_pPointerData = std::move(pOther.m_pPointerData);
    return *this;
}

ra::ByteAddress CodeNoteModel::GetPointerAddress() const noexcept
{
    return m_pPointerData != nullptr ? m_pPointerData->PointerAddress : 0xFFFFFFFF;
}

uint32_t CodeNoteModel::GetRawPointerValue() const noexcept
{
    return m_pPointerData != nullptr ? m_pPointerData->RawPointerValue : 0xFFFFFFFF;
}

static ra::ByteAddress ConvertPointer(ra::ByteAddress nAddress)
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
    const auto nConvertedAddress = pConsoleContext.ByteAddressFromRealAddress(nAddress);
    if (nConvertedAddress != 0xFFFFFFFF)
        nAddress = nConvertedAddress;

    return nAddress;
}

void CodeNoteModel::UpdateRawPointerValue(ra::ByteAddress nAddress, const ra::data::context::EmulatorContext& pEmulatorContext,
                                          NoteMovedFunction fNoteMovedCallback)
{
    if (m_pPointerData == nullptr)
        return;

    const uint32_t nValue = pEmulatorContext.ReadMemory(nAddress, GetMemSize());
    if (nValue != m_pPointerData->RawPointerValue)
    {
        m_pPointerData->RawPointerValue = nValue;

        const auto nNewAddress = (m_pPointerData->OffsetType == PointerData::OffsetType::Converted)
            ? ConvertPointer(nValue) : nValue;

        const auto nOldAddress = m_pPointerData->PointerAddress;
        if (nNewAddress != nOldAddress)
        {
            m_pPointerData->PointerAddress = nNewAddress;
            if (fNoteMovedCallback)
            {
                fNoteMovedCallback(nOldAddress, nNewAddress, *this);

                for (const auto& pNote : m_pPointerData->OffsetNotes)
                {
                    if (!pNote.CodeNote.IsPointer())
                        fNoteMovedCallback(nOldAddress + pNote.Offset, nNewAddress + pNote.Offset, pNote.CodeNote);
                }
            }
        }
    }

    if (m_pPointerData->HasPointers)
    {
        for (auto& pNote : m_pPointerData->OffsetNotes)
        {
            if (pNote.CodeNote.IsPointer())
            {
                pNote.CodeNote.UpdateRawPointerValue(m_pPointerData->PointerAddress + pNote.Offset,
                                                     pEmulatorContext, fNoteMovedCallback);
            }
        }
    }
}

const CodeNoteModel* CodeNoteModel::GetPointerNoteAtOffset(int nOffset) const
{
    if (m_pPointerData == nullptr)
        return nullptr;

    // look for explicit offset match
    for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
    {
        if (pOffsetNote.Offset == nOffset)
            return &pOffsetNote.CodeNote;
    }

    if (m_pPointerData->OffsetType == PointerData::OffsetType::Overflow)
    {
        // direct offset not found, look for converted offset
        const auto nConvertedAddress = ConvertPointer(m_pPointerData->RawPointerValue);
        nOffset += nConvertedAddress - m_pPointerData->RawPointerValue;

        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (pOffsetNote.Offset == nOffset)
                return &pOffsetNote.CodeNote;
        }
    }

    return nullptr;
}

std::pair<ra::ByteAddress, const CodeNoteModel*> CodeNoteModel::GetPointerNoteAtAddress(ra::ByteAddress nAddress) const
{
    if (m_pPointerData == nullptr)
        return {0, nullptr};

    const auto nPointerAddress = m_pPointerData->PointerAddress;
    const auto nConvertedAddress = (m_pPointerData->OffsetType == PointerData::OffsetType::Overflow)
        ? ConvertPointer(nPointerAddress) : nPointerAddress;

    // if address is in the struct, look for a matching field
    if (nAddress >= nConvertedAddress && nAddress < nConvertedAddress + m_pPointerData->OffsetRange)
    {
        auto nOffset = ra::to_signed(nAddress - nPointerAddress);

        // check for exact matches first
        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (nOffset == pOffsetNote.Offset)
                return {pOffsetNote.Offset + nPointerAddress, &pOffsetNote.CodeNote};
        }

        // check for trailing bytes in a multi-byte note
        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (nOffset > pOffsetNote.Offset)
            {
                const auto nBytes = ra::to_signed(pOffsetNote.CodeNote.GetBytes());
                if (nBytes > 1 && nOffset < pOffsetNote.Offset + nBytes)
                    return {pOffsetNote.Offset + nPointerAddress, &pOffsetNote.CodeNote};
            }
        }
    }

    // check pointer chains
    if (m_pPointerData->HasPointers)
    {
        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (pOffsetNote.CodeNote.IsPointer())
            {
                auto pNestedObject = pOffsetNote.CodeNote.GetPointerNoteAtAddress(nAddress);
                if (pNestedObject.second)
                    return pNestedObject;
            }
        }
    }

    // not found
    return {0, nullptr};
}

bool CodeNoteModel::GetPreviousAddress(ra::ByteAddress nBeforeAddress, ra::ByteAddress& nPreviousAddress) const
{
    if (m_pPointerData == nullptr)
        return false;

    const auto nPointerAddress = m_pPointerData->PointerAddress;
    const auto nConvertedAddress = (m_pPointerData->OffsetType == PointerData::OffsetType::Overflow)
        ? ConvertPointer(nPointerAddress) : nPointerAddress;

    if (nConvertedAddress > nBeforeAddress)
        return false;

    bool bResult = false;
    nPreviousAddress = 0;
    for (const auto& pOffset : m_pPointerData->OffsetNotes)
    {
        const auto nOffsetAddress = nPointerAddress + pOffset.Offset;
        if (nOffsetAddress < nBeforeAddress && nOffsetAddress > nPreviousAddress)
        {
            nPreviousAddress = nOffsetAddress;
            bResult = true;
        }
    }

    return bResult;
}

bool CodeNoteModel::GetNextAddress(ra::ByteAddress nAfterAddress, ra::ByteAddress& nNextAddress) const
{
    if (m_pPointerData == nullptr)
        return false;

    const auto nPointerAddress = m_pPointerData->PointerAddress;
    const auto nConvertedAddress = (m_pPointerData->OffsetType == PointerData::OffsetType::Overflow)
        ? ConvertPointer(nPointerAddress) : nPointerAddress;

    if (nConvertedAddress + m_pPointerData->OffsetRange < nAfterAddress)
        return false;

    bool bResult = false;
    nNextAddress = 0xFFFFFFFF;
    for (const auto& pOffset : m_pPointerData->OffsetNotes)
    {
        const auto nOffsetAddress = nPointerAddress + pOffset.Offset;
        if (nOffsetAddress > nAfterAddress && nOffsetAddress < nNextAddress)
        {
            nNextAddress = nOffsetAddress;
            bResult = true;
        }
    }

    return bResult;
}

void CodeNoteModel::SetNote(const std::wstring& sNote)
{
    if (m_sNote == sNote)
        return;

    m_sNote = sNote;

    auto nIndex = sNote.find(L'\n');
    auto sFirstLine = (nIndex == std::string::npos) ? sNote : sNote.substr(0, nIndex);
    StringMakeLowercase(sFirstLine);
    ExtractSize(sFirstLine);

    if (sFirstLine.find(L"pointer") != std::string::npos)
    {
        if (m_nMemSize == MemSize::Unknown)
        {
            m_nBytes = 4;
            m_nMemSize = MemSize::ThirtyTwoBit;
        }

        // if there are any lines starting with a plus sign, extract the indirect code notes
        nIndex = sNote.find(L"\n+");
        if (nIndex != std::string::npos)
            ProcessIndirectNotes(sNote, nIndex);
    }
}

void CodeNoteModel::ExtractSize(const std::wstring& sNote)
{
    // provide defaults in case no matches are found
    m_nBytes = 1;
    m_nMemSize = MemSize::Unknown;

    // "Nbit" smallest possible note - and that's just the size annotation
    if (sNote.length() < 4)
        return;

    bool bBytesFromBits = false;
    bool bFoundSize = false;
    bool bLastWordIsSize = false;
    bool bLastWordIsNumber = false;
    bool bWordIsNumber = false;

    std::wstring sPreviousWord, sWord;
    const size_t nLength = sNote.length();
    for (size_t nIndex = 0; nIndex <= nLength; ++nIndex)
    {
        // support reading null terminator so we process the last word in the string
        const wchar_t c = (nIndex == nLength) ? 0 : sNote.at(nIndex);

        // find the next word
        if (c > 255)
        {
            // ignore unicode characters - isalpha with the default locale would return false,
            // but also likes to pop up asserts when in a debug build.
        }
        else if (isalpha(c))
        {
            if (sWord.empty())
            {
                sWord.push_back(gsl::narrow_cast<wchar_t>(tolower(c)));
                bWordIsNumber = false;
                continue;
            }

            if (!bWordIsNumber)
            {
                sWord.push_back(gsl::narrow_cast<wchar_t>(tolower(c)));
                continue;
            }
        }
        else if (isdigit(c))
        {
            if (sWord.empty())
            {
                sWord.push_back(c);
                bWordIsNumber = true;
                continue;
            }

            if (bWordIsNumber)
            {
                sWord.push_back(c);
                continue;
            }
        }

        if (sWord.empty())
            continue;

        // process the word
        bool bWordIsSize = false;
        if (bWordIsNumber)
        {
            if (sPreviousWord == L"mbf")
            {
                const auto nBits = _wtoi(sWord.c_str());
                if (nBits == 32)
                {
                    m_nBytes = 4;
                    m_nMemSize = MemSize::MBF32;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
                else if (nBits == 40)
                {
                    m_nBytes = 5;
                    m_nMemSize = MemSize::MBF32;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
            else if (sPreviousWord == L"double" && sWord == L"32")
            {
                m_nBytes = 4;
                m_nMemSize = MemSize::Double32;
                bWordIsSize = true;
                bFoundSize = true;
            }
        }
        else if (bLastWordIsSize)
        {
            if (sWord == L"float")
            {
                if (m_nMemSize == MemSize::ThirtyTwoBit)
                {
                    m_nMemSize = MemSize::Float;
                    bWordIsSize = true; // allow trailing be/bigendian
                }
            }
            else if (sWord == L"double")
            {
                if (m_nMemSize == MemSize::ThirtyTwoBit || m_nBytes == 8)
                {
                    m_nMemSize = MemSize::Double32;
                    bWordIsSize = true; // allow trailing be/bigendian
                }
            }
            else if (sWord == L"be" || sWord == L"bigendian")
            {
                switch (m_nMemSize)
                {
                    case MemSize::SixteenBit: m_nMemSize = MemSize::SixteenBitBigEndian; break;
                    case MemSize::TwentyFourBit: m_nMemSize = MemSize::TwentyFourBitBigEndian; break;
                    case MemSize::ThirtyTwoBit: m_nMemSize = MemSize::ThirtyTwoBitBigEndian; break;
                    case MemSize::Float: m_nMemSize = MemSize::FloatBigEndian; break;
                    case MemSize::Double32: m_nMemSize = MemSize::Double32BigEndian; break;
                    default: break;
                }
            }
            else if (sWord == L"le")
            {
                if (m_nMemSize == MemSize::MBF32)
                    m_nMemSize = MemSize::MBF32LE;
            }
            else if (sWord == L"mbf")
            {
                if (m_nBytes == 4 || m_nBytes == 5)
                    m_nMemSize = MemSize::MBF32;
            }
        }
        else if (bLastWordIsNumber)
        {
            if (sWord == L"bit" || sWord == L"bits")
            {
                if (!bFoundSize)
                {
                    const auto nBits = _wtoi(sPreviousWord.c_str());
                    m_nBytes = (nBits + 7) / 8;
                    m_nMemSize = MemSize::Unknown;
                    bBytesFromBits = true;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
            else if (sWord == L"byte" || sWord == L"bytes")
            {
                if (!bFoundSize || bBytesFromBits)
                {
                    m_nBytes = _wtoi(sPreviousWord.c_str());
                    m_nMemSize = MemSize::Unknown;
                    bBytesFromBits = false;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }

            if (bWordIsSize)
            {
                switch (m_nBytes)
                {
                    case 0: m_nBytes = 1; break; // Unexpected size, reset to defaults (1 byte, Unknown)
                    case 1: m_nMemSize = MemSize::EightBit; break;
                    case 2: m_nMemSize = MemSize::SixteenBit; break;
                    case 3: m_nMemSize = MemSize::TwentyFourBit; break;
                    case 4: m_nMemSize = MemSize::ThirtyTwoBit; break;
                    default: m_nMemSize = MemSize::Array; break;
                }
            }
        }
        else if (sWord == L"float")
        {
            if (!bFoundSize)
            {
                m_nBytes = 4;
                m_nMemSize = MemSize::Float;
                bWordIsSize = true; // allow trailing be/bigendian

                if (sPreviousWord == L"be" || sPreviousWord == L"bigendian")
                    m_nMemSize = MemSize::FloatBigEndian;
            }
        }
        else if (sWord == L"double")
        {
            if (!bFoundSize)
            {
                m_nBytes = 8;
                m_nMemSize = MemSize::Double32;
                bWordIsSize = true; // allow trailing be/bigendian

                if (sPreviousWord == L"be" || sPreviousWord == L"bigendian")
                    m_nMemSize = MemSize::Double32BigEndian;
            }
        }

        // store information about the word for later
        bLastWordIsSize = bWordIsSize;
        bLastWordIsNumber = bWordIsNumber;

        if (c < 256 && isalnum(c))
        {
            std::swap(sPreviousWord, sWord);
            sWord.clear();

            sWord.push_back(gsl::narrow_cast<wchar_t>(tolower(c)));
            bWordIsNumber = isdigit(c);
        }
        else
        {
            // only join words with spaces or hyphens.
            if (c == L' ' || c == L'-')
                std::swap(sPreviousWord, sWord);
            else
                sPreviousWord.clear();

            sWord.clear();
        }
    }
}

static void RemoveIndentPrefix(std::wstring& sNote)
{
    auto nLineIndex = sNote.find('\n');
    if (nLineIndex == std::wstring::npos)
        return;

    for (size_t nIndent = nLineIndex + 1; nIndent + 1 < sNote.length(); ++nIndent)
    {
        auto c = sNote.at(nIndent);
        if (c != '+')
        {
            if (c == '\n')
                nLineIndex = nIndent;

            continue;
        }

        c = sNote.at(nIndent + 1);
        if (isdigit(c)) // found +N
        {
            if (nIndent > nLineIndex + 1)
            {
                const auto sPrefix = sNote.substr(nLineIndex, nIndent - nLineIndex); // capture "\n" + prefix
                auto nIndex = nLineIndex;
                do
                {
                    sNote.erase(nIndex + 1, sPrefix.length() - 1);
                    nIndex = sNote.find(sPrefix, nIndex + 1);
                } while (nIndex != std::wstring::npos);
            }
            break;
        }
    }
}

void CodeNoteModel::ProcessIndirectNotes(const std::wstring& sNote, size_t nIndex)
{
    nIndex += 2;

    auto pointerData = std::make_unique<PointerData>();
    do
    {
        PointerData::OffsetCodeNote offsetNote;
        offsetNote.CodeNote.SetAuthor(m_sAuthor);

        // the next note starts when we find a '+' at the start of a line.
        auto nNextIndex = sNote.find(L"\n+", nIndex);
        auto nStopIndex = nNextIndex;

        if (nNextIndex != std::wstring::npos)
        {
            // a chain of plusses indicates an indented nested note. include them
            //
            //   [32-bit pointer] global data
            //   +0x20 [32-bit pointer] user data
            //   ++0x08 [16-bit] points
            //
            while (nNextIndex + 2 < sNote.length() && !isdigit(sNote.at(nNextIndex + 2)))
            {
                nNextIndex = nStopIndex = sNote.find(L"\n+", nNextIndex + 2);
                if (nNextIndex == std::wstring::npos)
                    break;
            }

            // remove trailing whitespace
            if (nStopIndex != std::wstring::npos)
            {
                while (nStopIndex > 0 && isspace(sNote.at(nStopIndex - 1)))
                    nStopIndex--;
            }
        }

        auto sNextNote = sNote.substr(nIndex, nStopIndex - nIndex);
        RemoveIndentPrefix(sNextNote);

        // extract the offset
        wchar_t* pEnd = nullptr;

        try
        {
            if (sNextNote.length() > 2 && sNextNote.at(1) == 'x')
                offsetNote.Offset = gsl::narrow_cast<int>(std::wcstoll(sNextNote.c_str() + 2, &pEnd, 16));
            else
                offsetNote.Offset = gsl::narrow_cast<int>(std::wcstoll(sNextNote.c_str(), &pEnd, 10));
        } catch (const std::exception&)
        {
            break;
        }

        // if there are any error processing offsets, don't treat this as a pointer note
        if (!pEnd || isalnum(*pEnd))
            return;

        // skip over [whitespace] [optional separator] [whitespace]
        const wchar_t* pStop = sNextNote.c_str() + sNextNote.length();
        while (pEnd < pStop && isspace(*pEnd))
            pEnd++;
        if (pEnd < pStop && !isalnum(*pEnd))
        {
            pEnd++;
            while (pEnd < pStop && isspace(*pEnd))
                pEnd++;
        }

        offsetNote.CodeNote.SetNote(sNextNote.substr(pEnd - sNextNote.c_str()));
        pointerData->HasPointers |= offsetNote.CodeNote.IsPointer();

        const auto nRangeOffset = offsetNote.Offset + offsetNote.CodeNote.GetBytes();
        pointerData->OffsetRange = std::max(pointerData->OffsetRange, nRangeOffset);

        pointerData->OffsetNotes.push_back(std::move(offsetNote));

        if (nNextIndex == std::string::npos)
            break;

        nIndex = nNextIndex + 2;
    } while (true);

    // assume anything annotated as a 32-bit pointer will read a real (non-translated) address and
    // flag it to be converted to an RA address when evaluating indirect notes in DoFrame()
    if (m_nMemSize == MemSize::ThirtyTwoBit || m_nMemSize == MemSize::ThirtyTwoBitBigEndian)
    {
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        const auto nMaxAddress = pEmulatorContext.TotalMemorySize();

        pointerData->OffsetType = PointerData::OffsetType::Converted;

        // if any offset exceeds the available memory for the system, assume the user is leveraging
        // overflow math instead of masking, and don't attempt to translate the addresses.
        for (const auto& pNote : pointerData->OffsetNotes)
        {
            if (ra::to_unsigned(pNote.Offset) >= nMaxAddress)
            {
                pointerData->OffsetType = PointerData::OffsetType::Overflow;
                break;
            }
        }
    }

    m_pPointerData = std::move(pointerData);
}

void CodeNoteModel::EnumeratePointerNotes(
    std::function<bool(ra::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const
{
    if (m_pPointerData == nullptr)
        return;

    if (m_pPointerData->OffsetType == PointerData::OffsetType::Overflow)
        EnumeratePointerNotes(m_pPointerData->RawPointerValue, fCallback);
    else
        EnumeratePointerNotes(m_pPointerData->PointerAddress, fCallback);
}

void CodeNoteModel::EnumeratePointerNotes(ra::ByteAddress nPointerAddress,
    std::function<bool(ra::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const
{
    if (m_pPointerData == nullptr)
        return;

    for (const auto& pNote : m_pPointerData->OffsetNotes)
    {
        if (!fCallback(nPointerAddress + pNote.Offset, pNote.CodeNote))
            break;
    }
}

} // namespace models
} // namespace data
} // namespace ra
