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
    unsigned int HeaderLength = 0;               // length of note text not associated to OffsetNotes
    bool HasPointers = false;                    // true if there are nested pointers
    bool PointerRead = false;                    // true the first time RawPointerValue is updated

    enum OffsetType
    {
        None = 0,
        Converted, // PointerAddress will contain a converted address
        Overflow,  // offset exceeds RA address space, apply to RawPointerValue
    };
    OffsetType OffsetType = OffsetType::None;

    std::vector<CodeNoteModel> OffsetNotes;
};

// these must be defined here because of forward declaration of PointerData in header file.
CodeNoteModel::CodeNoteModel() noexcept {}
CodeNoteModel::~CodeNoteModel() {}
CodeNoteModel::CodeNoteModel(CodeNoteModel&& pOther) noexcept
    : m_sAuthor(std::move(pOther.m_sAuthor)),
      m_sNote(std::move(pOther.m_sNote)),
      m_nBytes(pOther.m_nBytes),
      m_nAddress(pOther.m_nAddress),
      m_nMemSize(pOther.m_nMemSize),
      m_nMemFormat(pOther.m_nMemFormat),
      m_pPointerData(std::move(pOther.m_pPointerData))
{
}
CodeNoteModel& CodeNoteModel::operator=(CodeNoteModel&& pOther) noexcept
{
    m_sAuthor = std::move(pOther.m_sAuthor);
    m_sNote = std::move(pOther.m_sNote);
    m_nBytes = pOther.m_nBytes;
    m_nAddress = pOther.m_nAddress;
    m_nMemSize = pOther.m_nMemSize;
    m_nMemFormat = pOther.m_nMemFormat;
    m_pPointerData = std::move(pOther.m_pPointerData);
    return *this;
}

std::wstring CodeNoteModel::GetPointerDescription() const
{
    return m_pPointerData != nullptr ? m_sNote.substr(0, m_pPointerData->HeaderLength) : std::wstring();
}

ra::ByteAddress CodeNoteModel::GetPointerAddress() const noexcept
{
    return m_pPointerData != nullptr ? m_pPointerData->PointerAddress : 0xFFFFFFFF;
}

bool CodeNoteModel::HasRawPointerValue() const noexcept
{
    return m_pPointerData != nullptr ? m_pPointerData->PointerRead : false;
}

uint32_t CodeNoteModel::GetRawPointerValue() const noexcept
{
    return m_pPointerData != nullptr ? m_pPointerData->RawPointerValue : 0xFFFFFFFF;
}

bool CodeNoteModel::HasNestedPointers() const noexcept
{
    return m_pPointerData != nullptr && m_pPointerData->HasPointers;
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

    m_pPointerData->PointerRead = true;

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
                for (const auto& pNote : m_pPointerData->OffsetNotes)
                {
                    if (!pNote.IsPointer())
                        fNoteMovedCallback(nOldAddress + pNote.GetAddress(), nNewAddress + pNote.GetAddress(), pNote);
                }
            }
        }
    }

    if (m_pPointerData->HasPointers)
    {
        for (auto& pNote : m_pPointerData->OffsetNotes)
        {
            if (pNote.IsPointer())
            {
                pNote.UpdateRawPointerValue(m_pPointerData->PointerAddress + pNote.GetAddress(),
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
        if (ra::to_signed(pOffsetNote.GetAddress()) == nOffset)
            return &pOffsetNote;
    }

    if (m_pPointerData->OffsetType == PointerData::OffsetType::Overflow)
    {
        // direct offset not found, look for converted offset
        const auto nConvertedAddress = ConvertPointer(m_pPointerData->RawPointerValue);
        nOffset += nConvertedAddress - m_pPointerData->RawPointerValue;

        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (ra::to_signed(pOffsetNote.GetAddress()) == nOffset)
                return &pOffsetNote;
        }
    }

    return nullptr;
}

std::pair<ra::ByteAddress, const CodeNoteModel*> CodeNoteModel::GetPointerNoteAtAddress(ra::ByteAddress nAddress) const
{
    if (m_pPointerData == nullptr)
        return {0, nullptr};

    const auto nPointerAddress = m_pPointerData->PointerAddress;
    if (nPointerAddress == 0) // assume null is not a valid pointer address
        return { 0, nullptr };

    bool bAddressValid = true;
    if (m_pPointerData->OffsetType == PointerData::OffsetType::Converted)
    {
        const auto nConvertedAddress = ConvertPointer(nPointerAddress);
        bAddressValid = nAddress >= nConvertedAddress && nAddress < nConvertedAddress + m_pPointerData->OffsetRange;
    }

    // if address is in the struct, look for a matching field
    if (bAddressValid)
    {
        const auto nOffset = nAddress - nPointerAddress;

        // check for exact matches first
        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (nOffset == pOffsetNote.GetAddress())
                return {nPointerAddress + pOffsetNote.GetAddress(), &pOffsetNote};
        }

        // check for trailing bytes in a multi-byte note
        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (nOffset > pOffsetNote.GetAddress())
            {
                const auto nBytes = ra::to_signed(pOffsetNote.GetBytes());
                if (nBytes > 1 && nOffset < pOffsetNote.GetAddress() + nBytes)
                    return {nPointerAddress + pOffsetNote.GetAddress(), &pOffsetNote};
            }
        }
    }

    // check pointer chains
    if (m_pPointerData->HasPointers)
    {
        for (const auto& pOffsetNote : m_pPointerData->OffsetNotes)
        {
            if (pOffsetNote.IsPointer())
            {
                auto pNestedObject = pOffsetNote.GetPointerNoteAtAddress(nAddress);
                if (pNestedObject.second)
                    return pNestedObject;
            }
        }
    }

    // not found
    return {0, nullptr};
}

bool CodeNoteModel::GetPointerChain(std::vector<const CodeNoteModel*>& vChain, const CodeNoteModel& pRootNote) const
{
    if (!pRootNote.IsPointer())
        return false;

    vChain.push_back(&pRootNote);

    if (&pRootNote == this)
        return true;

    return GetPointerChainRecursive(vChain, pRootNote);
}

bool CodeNoteModel::GetPointerChainRecursive(std::vector<const CodeNoteModel*>& vChain,
                                             const CodeNoteModel& pParentNote) const
{
    for (auto& pNote : pParentNote.m_pPointerData->OffsetNotes)
    {
        if (&pNote == this)
        {
            vChain.push_back(this);
            return true;
        }

        if (pNote.IsPointer())
        {
            vChain.push_back(&pNote);

            if (GetPointerChainRecursive(vChain, pNote))
                return true;

            vChain.pop_back();
        }
    }

    return false;
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
        const auto nOffsetAddress = nPointerAddress + pOffset.GetAddress();
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
        const auto nOffsetAddress = nPointerAddress + pOffset.GetAddress();
        if (nOffsetAddress > nAfterAddress && nOffsetAddress < nNextAddress)
        {
            nNextAddress = nOffsetAddress;
            bResult = true;
        }
    }

    return bResult;
}

std::wstring CodeNoteModel::GetPrimaryNote() const
{
    if (m_pPointerData != nullptr)
    {
        auto nIndex = m_sNote.find(L"\n+");
        if (nIndex != std::wstring::npos)
        {
            if (nIndex > 0 && m_sNote.at(nIndex - 1) == '\r')
                --nIndex;

            return m_sNote.substr(0, nIndex);
        }
    }

    return m_sNote;
}

static bool IsValue(const std::wstring& sNote, size_t nIndex, size_t nStopIndex, unsigned& nLength, bool& bHex)
{
    if (nStopIndex == std::string::npos)
        nStopIndex = sNote.length();

    while (nIndex < nStopIndex)
    {
        const wchar_t c = sNote.at(nIndex);
        if (c >= 256 || !isspace(c))
            break;
        ++nIndex;
    }
    while (nStopIndex > nIndex)
    {
        const wchar_t c = sNote.at(nStopIndex - 1);
        if (c >= 256 || !isspace(c))
            break;
        --nStopIndex;
    }

    bHex = false;

    if (nStopIndex - nIndex > 2 && sNote.at(nIndex) == '0' && sNote.at(nIndex + 1) == 'x')
    {
        // explicit hex prefix
        nIndex += 2;
        bHex = true;
    }
    else if (nStopIndex - nIndex > 1 && sNote.at(nStopIndex - 1) == 'h')
    {
        // explicit hex suffix
        nStopIndex--;
        bHex = true;
    }

    nLength = 0;
    while (nIndex < nStopIndex)
    {
        const auto c = sNote.at(nIndex++);
        switch (c)
        {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                nLength++;
                break;

            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                nLength++;
                bHex = true;
                break;

            default:
                return false;
        }
    }

    return true;
}

static bool IsBitX(const std::wstring& sNote, size_t nIndex, size_t nStopIndex)
{
    if (nStopIndex == std::string::npos)
        nStopIndex = sNote.length();

    if (nStopIndex - nIndex < 4)
        return false;

    auto c = sNote.at(nIndex);
    if (c != 'b' && c != 'B')
        return false;

    c = sNote.at(nIndex + 1);
    if (c != 'i' && c != 'I')
        return false;

    c = sNote.at(nIndex + 2);
    if (c != 't' && c != 'T')
        return false;

    c = sNote.at(nIndex + 3);
    if (c < '0' || c > '9')
        return false;

    nIndex += 4;
    while (nIndex < nStopIndex)
    {
        c = sNote.at(nIndex++);
        if (c < 256 && !isspace(c))
            return false;
    }

    return true;
}

void CodeNoteModel::CheckForHexEnum(size_t nNextIndex)
{
    bool bAllValuesPotentialHex = true;
    bool bFoundPotentialHexValueWithLeadingZero = false;
    while (nNextIndex != std::string::npos)
    {
        auto nIndex = nNextIndex + 1;

        // 12=Happy
        // 12-Happy
        // 12|Happy
        // 12: Happy
        const auto nSplitIndex = m_sNote.find_first_of(L"=-|:", nIndex);
        if (nSplitIndex == std::string::npos)
            break;

        do
        {
            nNextIndex = m_sNote.find(L'\n', nIndex);
            if (nNextIndex == std::string::npos || nNextIndex > nSplitIndex)
                break;

            nIndex = nNextIndex + 1;
        } while (true);

        if (m_sNote.at(nIndex) == '*') // bulleted list
            ++nIndex;

        unsigned nLength;
        bool bHex;
        if (IsValue(m_sNote, nIndex, nSplitIndex, nLength, bHex) ||       // 12=Happy
            IsValue(m_sNote, nSplitIndex + 1, nNextIndex, nLength, bHex)) // Happy=12
        {
            if (bHex)
            {
                m_nMemFormat = MemFormat::Hex;
                break;
            }
            else if (bAllValuesPotentialHex)
            {
                if (nLength != m_nBytes * 2)
                    bAllValuesPotentialHex = false;
                else if (m_sNote.at(nIndex) == '0')
                    bFoundPotentialHexValueWithLeadingZero = true;
            }
        }
        else if (IsBitX(m_sNote, nIndex, nSplitIndex)) // bit1=Happy
        {
            m_nMemFormat = MemFormat::Hex;
            break;
        }
    }

    if (bAllValuesPotentialHex && bFoundPotentialHexValueWithLeadingZero)
        m_nMemFormat = MemFormat::Hex;
}

void CodeNoteModel::SetNote(const std::wstring& sNote, bool bImpliedPointer)
{
    if (m_sNote == sNote)
        return;

    m_sNote = sNote;
    m_nMemFormat = MemFormat::Dec;

    std::wstring sLine;
    size_t nIndex = 0;
    size_t nNextIndex;
    do
    {
        nNextIndex = sNote.find(L'\n', nIndex);
        if (nNextIndex == std::string::npos)
            sLine = sNote.substr(nIndex);
        else if (nNextIndex > 0 && sNote.at(nNextIndex - 1) == '\r') // expect data to be normalized for Windows so it will load into the controls correctly
            sLine = sNote.substr(nIndex, nNextIndex - nIndex - 1);
        else
            sLine = sNote.substr(nIndex, nNextIndex - nIndex);

        if (!sLine.empty())
        {
            if (sLine.at(0) == '+' && bImpliedPointer)
            {
                m_nMemSize = GetImpliedPointerSize();
                m_nBytes = ra::data::MemSizeBytes(m_nMemSize);

                // found a line starting with a plus sign, bit no pointer annotation. bImpliedPointer
                // must be true. assume the parent note is not described. pass -1 as the note size
                // because we already skipped over the newline character
                ProcessIndirectNotes(sNote, nIndex - 1);

                // if nIndex is 0, HeaderLength may get set to -1, force it back to 0.
                if (m_pPointerData && nIndex == 0)
                    m_pPointerData->HeaderLength = 0;
                break;
            }

            StringMakeLowercase(sLine);

            const auto nPointerIndex = sLine.find(L"pointer");
            if (nPointerIndex == std::string::npos)
            {
                // "pointer" not found
                ExtractSize(sLine, false);
            }
            else if (sLine.length() > nPointerIndex + 8 && iswalpha(sLine.at(nPointerIndex + 7)))
            {
                // extra trailing letters - assume "pointers"
                ExtractSize(sLine, false);
            }
            else
            {
                // found "pointer"
                ExtractSize(sLine, true);

                m_nMemFormat = MemFormat::Hex;

                if (m_nMemSize == MemSize::Unknown)
                {
                    // pointer size not specified. assume 32-bit
                    m_nMemSize = GetImpliedPointerSize();
                    m_nBytes = ra::data::MemSizeBytes(m_nMemSize);
                }

                // if there are any lines starting with a plus sign, extract the indirect code notes
                nIndex = sNote.find(L"\n+", nIndex + 1);
                if (nIndex != std::string::npos)
                    ProcessIndirectNotes(sNote, nIndex);

                // failed to find nested code notes. create a PointerData object so the note still
                // gets treated as a pointer
                if (!m_pPointerData)
                {
                    m_pPointerData.reset(new PointerData());
                    m_pPointerData->HeaderLength = gsl::narrow_cast<unsigned>(sNote.length());
                }

                break;
            }

            if (m_nMemSize != MemSize::Unknown) // found a size. stop processing.
                break;
        }

        if (nNextIndex == std::string::npos) // end of string
            break;

        nIndex = nNextIndex + 1;
    } while (true);

    if (m_nMemFormat == MemFormat::Dec) // implicitly ignored nested notes as pointers will be flagged as hex
        CheckForHexEnum(nNextIndex);
}

MemSize CodeNoteModel::GetImpliedPointerSize()
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();

    MemSize nSize;
    uint32_t nMask;
    uint32_t nOffset;
    if (pConsoleContext.GetRealAddressConversion(&nSize, &nMask, &nOffset))
        return nSize;

    return MemSize::ThirtyTwoBit;
}

static constexpr bool IsHexDigit(wchar_t c)
{
    if (c >= '0' && c <= '9')
        return true;
    if (c >= 'a' && c <= 'f')
        return true;
    if (c >= 'A' && c <= 'F')
        return true;
    return false;
}

CodeNoteModel::Parser::TokenType CodeNoteModel::Parser::NextToken(std::wstring& sWord) const
{
    wchar_t cFirstLetter = '\0';
    bool bWordIsNumber = false;
    bool bWordIsHexNumber = false;
    sWord.clear();

    for (; m_nIndex < m_nEndIndex; ++m_nIndex)
    {
        const wchar_t c = m_sNote.at(m_nIndex);

        // find the next word
        if (c > 255)
        {
            // ignore unicode characters - isalpha with the default locale would return false,
            // but it also likes to pop up asserts in a debug build.

            // if we've found any alphanumeric characters, process them.
            if (!sWord.empty())
                break;
        }
        else if (isalpha(c))
        {
            if (sWord.empty())
            {
                // start of word
                cFirstLetter = gsl::narrow_cast<wchar_t>(tolower(c));
                sWord.push_back(cFirstLetter);
                bWordIsNumber = false;
            }
            else if (bWordIsHexNumber)
            {
                if (IsHexDigit(c))
                {
                    // continue hex number
                    sWord.push_back(gsl::narrow_cast<wchar_t>(tolower(c)));
                }
                else
                {
                    // transition from numeric to alpha
                    break;
                }
            }
            else if (!bWordIsNumber)
            {
                // continue word
                sWord.push_back(gsl::narrow_cast<wchar_t>(tolower(c)));
            }
            else
            {
                // transition from numeric to alpha
                break;
            }
        }
        else if (isdigit(c))
        {
            if (sWord.empty())
            {
                // start of number
                sWord.push_back(c);

                if (c == '0' && m_nIndex < m_sNote.size() - 2 &&
                    m_sNote.at(m_nIndex + 1) == 'x' &&
                    IsHexDigit(m_sNote.at(m_nIndex + 2)))
                {
                    sWord.push_back(m_sNote.at(++m_nIndex));
                    sWord.push_back(m_sNote.at(++m_nIndex));
                    bWordIsHexNumber = true;
                }
                else
                {
                    bWordIsNumber = true;
                }
            }
            else if (bWordIsNumber || bWordIsHexNumber)
            {
                // continue number
                sWord.push_back(c);
            }
            else
            {
                // transition from alpha to numeric
                break;
            }
        }
        else
        {
            // non alphanumeric character.
            // if we've found any alphanumeric characters, process them.
            if (!sWord.empty())
                break;
        }
    }

    if (sWord.empty()) // end of input
        return TokenType::None;

    if (bWordIsNumber)
        return TokenType::Number;
    if (bWordIsHexNumber)
        return TokenType::HexNumber;

    switch (cFirstLetter)
    {
        case 'a':
            if (sWord == L"ascii")
                return TokenType::ASCII;
            break;

        case 'b':
            if (sWord == L"bit" || sWord == L"bits")
                return TokenType::Bits;
            if (sWord == L"byte" || sWord == L"bytes")
                return TokenType::Bytes;
            if (sWord == L"be" || sWord == L"bigendian")
                return TokenType::BigEndian;
            if (sWord == L"bcd")
                return TokenType::BCD;
            break;

        case 'd':
            if (sWord == L"double")
                return TokenType::Double;
            break;

        case 'f':
            if (sWord == L"float")
                return TokenType::Float;
            break;

        case 'h':
            if (sWord == L"hex")
                return TokenType::Hex;
            break;

        case 'l':
            if (sWord == L"le" || sWord == L"littleendian")
                return TokenType::LittleEndian;
            break;

        case 'm':
            if (sWord == L"mbf")
                return TokenType::MBF;
            break;

        default:
            break;
    }

    return TokenType::Other;
}

void CodeNoteModel::ExtractSize(const std::wstring& sNote, bool bIsPointer)
{
    // provide defaults in case no matches are found
    m_nBytes = 1;
    m_nMemSize = MemSize::Unknown;

    // "Nbit" smallest possible note - and that's just the size annotation
    if (sNote.length() < 4)
        return;

    bool bBytesFromBits = false;
    bool bFoundSize = false;
    bool bFoundASCII = false;
    bool bLastWordIsSize = false;
    Parser::TokenType nLastTokenType = Parser::TokenType::None;

    std::wstring sPreviousWord, sWord;
    const Parser parser(sNote, 0, sNote.length());
    do
    {
        const auto nTokenType = parser.NextToken(sWord);
        if (nTokenType == Parser::TokenType::None)
            break;

        // process the word
        bool bWordIsSize = false;
        if (nTokenType == Parser::TokenType::Number)
        {
            if (nLastTokenType == Parser::TokenType::MBF)
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
            else if (nLastTokenType == Parser::TokenType::Double && sWord == L"32")
            {
                m_nBytes = 4;
                m_nMemSize = MemSize::Double32;
                bWordIsSize = true;
                bFoundSize = true;
            }
        }
        else if (nTokenType == Parser::TokenType::BCD || nTokenType == Parser::TokenType::Hex)
        {
            m_nMemFormat = MemFormat::Hex;
        }
        else if (nTokenType == Parser::TokenType::ASCII)
        {
            bFoundASCII = true;
        }
        else if (bLastWordIsSize)
        {
            if (nTokenType == Parser::TokenType::Float)
            {
                if (m_nMemSize == MemSize::ThirtyTwoBit)
                {
                    m_nMemSize = MemSize::Float;
                    bWordIsSize = true; // allow trailing be/bigendian
                }
            }
            else if (nTokenType == Parser::TokenType::Double)
            {
                if (m_nMemSize == MemSize::ThirtyTwoBit || m_nBytes == 8)
                {
                    m_nMemSize = MemSize::Double32;
                    bWordIsSize = true; // allow trailing be/bigendian
                }
            }
            else if (nTokenType == Parser::TokenType::BigEndian)
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
            else if (nTokenType == Parser::TokenType::LittleEndian)
            {
                if (m_nMemSize == MemSize::MBF32)
                    m_nMemSize = MemSize::MBF32LE;
            }
            else if (nTokenType == Parser::TokenType::MBF)
            {
                if (m_nBytes == 4 || m_nBytes == 5)
                    m_nMemSize = MemSize::MBF32;
            }
        }
        else if (nLastTokenType == Parser::TokenType::Number)
        {
            if (nTokenType == Parser::TokenType::Bits)
            {
                if (!bFoundSize)
                {
                    const auto nBits = _wtoi(sPreviousWord.c_str());
                    m_nBytes = (nBits + 7) / 8;
                    bBytesFromBits = true;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
            else if (nTokenType == Parser::TokenType::Bytes)
            {
                if (!bFoundSize || (bBytesFromBits && !bIsPointer))
                {
                    m_nBytes = _wtoi(sPreviousWord.c_str());
                    bBytesFromBits = false;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }

            if (bWordIsSize &&
                (m_nMemSize == MemSize::Unknown ||      // size not yet determined
                 MemSizeBytes(m_nMemSize) != m_nBytes)) // size mismatch
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
        else if (nTokenType == Parser::TokenType::Float)
        {
            if (!bFoundSize)
            {
                m_nBytes = 4;
                m_nMemSize = MemSize::Float;
                bWordIsSize = true; // allow trailing be/bigendian

                if (nLastTokenType == Parser::TokenType::BigEndian)
                    m_nMemSize = MemSize::FloatBigEndian;
            }
        }
        else if (nTokenType == Parser::TokenType::Double)
        {
            if (!bFoundSize)
            {
                m_nBytes = 8;
                m_nMemSize = MemSize::Double32;
                bWordIsSize = true; // allow trailing be/bigendian

                if (nLastTokenType == Parser::TokenType::BigEndian)
                    m_nMemSize = MemSize::Double32BigEndian;
            }
        }
        else if (nLastTokenType == Parser::TokenType::HexNumber)
        {
            if (nTokenType == Parser::TokenType::Bytes)
            {
                if (!bFoundSize || (bBytesFromBits && !bIsPointer))
                {
                    wchar_t* pEnd;
                    m_nBytes = gsl::narrow_cast<unsigned int>(std::wcstoll(sPreviousWord.c_str(), &pEnd, 16));
                    m_nMemSize = MemSize::Array;
                    bBytesFromBits = false;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
        }

        // store information about the word for later
        bLastWordIsSize = bWordIsSize;
        nLastTokenType = nTokenType;

        const wchar_t c = parser.Peek();
        if (c < 256 && isalnum(c))
        {
            // number next to word [32bit]
            std::swap(sPreviousWord, sWord);
        }
        else if (c == L' ' || c == L'-')
        {
            // spaces or hyphen could be a joined word [32-bit] [32 bit].
            std::swap(sPreviousWord, sWord);
        }
        else
        {
            // everything else starts a new phrase
            sPreviousWord.clear();
            nLastTokenType = Parser::TokenType::None;
        }
    } while (true);

    if (m_nMemSize == MemSize::Array && bFoundASCII)
        m_nMemSize = MemSize::Text;
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
    auto pointerData = std::make_unique<PointerData>();
    pointerData->HeaderLength = gsl::narrow_cast<unsigned int>(nIndex);

    if (nIndex > 0 && nIndex < sNote.length() && sNote.at(nIndex - 1) == '\r')
        --pointerData->HeaderLength;

    nIndex += 2; // "\n+"
    do
    {
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
                while (nStopIndex > 0)
                {
                    const wchar_t c = sNote.at(nStopIndex - 1);
                    if (c >= 256 || !isspace(c))
                        break;
                    --nStopIndex;
                }
            }
        }

        auto sNextNote = sNote.substr(nIndex, nStopIndex - nIndex);
        RemoveIndentPrefix(sNextNote);

        // extract the offset
        wchar_t* pEnd = nullptr;

        int nOffset = 0;
        try
        {
            if (sNextNote.length() > 2 && sNextNote.at(1) == 'x')
                nOffset = gsl::narrow_cast<int>(std::wcstoll(sNextNote.c_str() + 2, &pEnd, 16));
            else
                nOffset = gsl::narrow_cast<int>(std::wcstoll(sNextNote.c_str(), &pEnd, 10));
        } catch (const std::exception&)
        {
            break;
        }

        // if there are any error processing offsets, don't treat this as a pointer note
        if (!pEnd || iswalnum(*pEnd))
            return;

        // skip over [whitespace] [optional separator] [whitespace]
        const wchar_t* pStop = sNextNote.c_str() + sNextNote.length();
        while (pEnd < pStop && *pEnd < 256 && isspace(*pEnd) && *pEnd != '\n')
            ++pEnd;

        if (pEnd < pStop)
        {
            if (*pEnd == '\n')
            {
                // no separator. found an unannotated note
                ++pEnd;
            }
            else if (!iswalnum(*pEnd) && *pEnd != '[' && *pEnd != '(') // assume brackets are not a separator
            {
                // found a separator. skip it and any following whitespace
                ++pEnd;

                while (pEnd < pStop && *pEnd < 256 && isspace(*pEnd))
                    ++pEnd;
            }
        }
        const auto sNoteBody = sNextNote.substr(pEnd - sNextNote.c_str());
        const auto nAddress = gsl::narrow_cast<ra::ByteAddress>(nOffset);

        CodeNoteModel* pExistingNote = nullptr;
        for (auto& pOffsetNote : pointerData->OffsetNotes)
        {
            if (pOffsetNote.GetAddress() == nAddress)
            {
                pExistingNote = &pOffsetNote;
                break;
            }
        }

        if (pExistingNote != nullptr)
        {
            // if the existing note is a pointer, merge the new note data into it.
            // if the existing note is not a pointer, ignore the new note data.
            if (pExistingNote->IsPointer())
                pExistingNote->SetNote(pExistingNote->GetNote() + L"\r\n" + sNoteBody, true);
        }
        else
        {
            CodeNoteModel offsetNote;
            offsetNote.SetAuthor(m_sAuthor);
            offsetNote.SetAddress(nAddress);

            offsetNote.SetNote(sNoteBody, true);
            pointerData->HasPointers |= offsetNote.IsPointer();

            const auto nRangeOffset = nOffset + offsetNote.GetBytes();
            pointerData->OffsetRange = std::max(pointerData->OffsetRange, nRangeOffset);

            pointerData->OffsetNotes.push_back(std::move(offsetNote));
        }

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
        const auto nUnderflowMinAddress = 0xFFFFFFFF - nMaxAddress + 1;

        pointerData->OffsetType = PointerData::OffsetType::Converted;

        // if any offset exceeds the available memory for the system, assume the user is leveraging
        // overflow math instead of masking, and don't attempt to translate the addresses.
        for (const auto& pNote : pointerData->OffsetNotes)
        {
            if (pNote.GetAddress() >= nMaxAddress && pNote.GetAddress() <= nUnderflowMinAddress)
            {
                pointerData->OffsetType = PointerData::OffsetType::Overflow;
                break;
            }
        }
    }

    m_pPointerData = std::move(pointerData);
}

std::wstring CodeNoteModel::TrimSize(const std::wstring& sNote, bool bKeepPointer)
{
    size_t nEndIndex = 0;
    size_t nStartIndex = sNote.find('[');
    if (nStartIndex != std::string::npos)
    {
        nEndIndex = sNote.find(']', nStartIndex + 1);
        if (nEndIndex == std::string::npos)
            return sNote;
    }
    else
    {
        nStartIndex = sNote.find('(');
        if (nStartIndex == std::string::npos)
            return sNote;

        nEndIndex = sNote.find(')', nStartIndex + 1);
        if (nEndIndex == std::string::npos)
            return sNote;
    }

    bool bPointer = false;
    std::wstring sWord;
    ra::data::models::CodeNoteModel::Parser::TokenType nTokenType;
    const ra::data::models::CodeNoteModel::Parser parser(sNote, nStartIndex + 1, nEndIndex);
    do
    {
        nTokenType = parser.NextToken(sWord);
        if (nTokenType == ra::data::models::CodeNoteModel::Parser::TokenType::Other)
        {
            if (sWord == L"pointer")
                bPointer = true;
            else
                return sNote;
        }
    } while (nTokenType != ra::data::models::CodeNoteModel::Parser::TokenType::None);

    while (nStartIndex > 0)
    {
        const wchar_t c = sNote.at(nStartIndex - 1);
        if (c >= 256 || !isspace(c))
            break;
        --nStartIndex;
    }

    while (nEndIndex < sNote.length() - 1)
    {
        const wchar_t c = sNote.at(nEndIndex + 1);
        if (c >= 256 || !isspace(c))
            break;
        ++nEndIndex;
    }

    std::wstring sNoteCopy = sNote;
    sNoteCopy.erase(nStartIndex, nEndIndex - nStartIndex + 1);

    if (bPointer && bKeepPointer)
        sNoteCopy.insert(0, L"[pointer] ");

    return sNoteCopy;
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
        if (!fCallback(nPointerAddress + pNote.GetAddress(), pNote))
            break;
    }
}

} // namespace models
} // namespace data
} // namespace ra
