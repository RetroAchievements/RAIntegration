#include "CodeNoteModel.hh"

#include "context\IConsoleContext.hh"

#include "services\ServiceLocator.hh"

#include "util\Strings.hh"

namespace ra {
namespace data {
namespace models {

struct CodeNoteModel::PointerData
{
    uint32_t RawPointerValue = 0xFFFFFFFF;       // last raw value of pointer captured
    ra::data::ByteAddress PointerAddress = 0xFFFFFFFF; // raw pointer value converted to RA address
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

ra::data::ByteAddress CodeNoteModel::GetPointerAddress() const noexcept
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

static ra::data::ByteAddress ConvertPointer(ra::data::ByteAddress nAddress)
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
    const auto nConvertedAddress = pConsoleContext.ByteAddressFromRealAddress(nAddress);
    if (nConvertedAddress != 0xFFFFFFFF)
        nAddress = nConvertedAddress;

    return nAddress;
}

void CodeNoteModel::UpdateRawPointerValue(ra::data::ByteAddress nAddress, const ra::context::IEmulatorMemoryContext& pMemoryContext,
                                          NoteMovedFunction fNoteMovedCallback)
{
    if (m_pPointerData == nullptr)
        return;

    m_pPointerData->PointerRead = true;

    const uint32_t nValue = pMemoryContext.ReadMemory(nAddress, GetMemSize());
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
                                            pMemoryContext, fNoteMovedCallback);
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

std::pair<ra::data::ByteAddress, const CodeNoteModel*> CodeNoteModel::GetPointerNoteAtAddress(ra::data::ByteAddress nAddress) const
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

bool CodeNoteModel::GetPreviousAddress(ra::data::ByteAddress nBeforeAddress, ra::data::ByteAddress& nPreviousAddress) const
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

bool CodeNoteModel::GetNextAddress(ra::data::ByteAddress nAfterAddress, ra::data::ByteAddress& nNextAddress) const
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
                m_nMemFormat = Memory::Format::Hex;
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
            m_nMemFormat = Memory::Format::Hex;
            break;
        }
    }

    if (bAllValuesPotentialHex && bFoundPotentialHexValueWithLeadingZero)
        m_nMemFormat = Memory::Format::Hex;
}

void CodeNoteModel::SetNote(const std::wstring& sNote, bool bImpliedPointer)
{
    if (m_sNote == sNote)
        return;

    m_sNote = sNote;
    m_nMemFormat = Memory::Format::Dec;

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
                m_nBytes = Memory::SizeBytes(m_nMemSize);

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

                m_nMemFormat = Memory::Format::Hex;

                if (m_nMemSize == Memory::Size::Unknown)
                {
                    // pointer size not specified. assume 32-bit
                    m_nMemSize = GetImpliedPointerSize();
                    m_nBytes = Memory::SizeBytes(m_nMemSize);
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

            if (m_nMemSize != Memory::Size::Unknown) // found a size. stop processing.
                break;
        }

        if (nNextIndex == std::string::npos) // end of string
            break;

        nIndex = nNextIndex + 1;
    } while (true);

    if (m_nMemFormat == Memory::Format::Dec) // implicitly ignored nested notes as pointers will be flagged as hex
        CheckForHexEnum(nNextIndex);
}

Memory::Size CodeNoteModel::GetImpliedPointerSize()
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();

    Memory::Size nSize;
    uint32_t nMask;
    uint32_t nOffset;
    if (pConsoleContext.GetRealAddressConversion(&nSize, &nMask, &nOffset))
        return nSize;

    return Memory::Size::ThirtyTwoBit;
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
    m_nMemSize = Memory::Size::Unknown;

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
                    m_nMemSize = Memory::Size::MBF32;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
                else if (nBits == 40)
                {
                    m_nBytes = 5;
                    m_nMemSize = Memory::Size::MBF32;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
            else if (nLastTokenType == Parser::TokenType::Double && sWord == L"32")
            {
                m_nBytes = 4;
                m_nMemSize = Memory::Size::Double32;
                bWordIsSize = true;
                bFoundSize = true;
            }
        }
        else if (nTokenType == Parser::TokenType::BCD || nTokenType == Parser::TokenType::Hex)
        {
            m_nMemFormat = Memory::Format::Hex;
        }
        else if (nTokenType == Parser::TokenType::ASCII)
        {
            bFoundASCII = true;
        }
        else if (bLastWordIsSize)
        {
            if (nTokenType == Parser::TokenType::Float)
            {
                if (m_nMemSize == Memory::Size::ThirtyTwoBit)
                {
                    m_nMemSize = Memory::Size::Float;
                    bWordIsSize = true; // allow trailing be/bigendian
                }
            }
            else if (nTokenType == Parser::TokenType::Double)
            {
                if (m_nMemSize == Memory::Size::ThirtyTwoBit || m_nBytes == 8)
                {
                    m_nMemSize = Memory::Size::Double32;
                    bWordIsSize = true; // allow trailing be/bigendian
                }
            }
            else if (nTokenType == Parser::TokenType::BigEndian)
            {
                switch (m_nMemSize)
                {
                    case Memory::Size::SixteenBit: m_nMemSize = Memory::Size::SixteenBitBigEndian; break;
                    case Memory::Size::TwentyFourBit: m_nMemSize = Memory::Size::TwentyFourBitBigEndian; break;
                    case Memory::Size::ThirtyTwoBit: m_nMemSize = Memory::Size::ThirtyTwoBitBigEndian; break;
                    case Memory::Size::Float: m_nMemSize = Memory::Size::FloatBigEndian; break;
                    case Memory::Size::Double32: m_nMemSize = Memory::Size::Double32BigEndian; break;
                    default: break;
                }
            }
            else if (nTokenType == Parser::TokenType::LittleEndian)
            {
                if (m_nMemSize == Memory::Size::MBF32)
                    m_nMemSize = Memory::Size::MBF32LE;
            }
            else if (nTokenType == Parser::TokenType::MBF)
            {
                if (m_nBytes == 4 || m_nBytes == 5)
                    m_nMemSize = Memory::Size::MBF32;
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
                (m_nMemSize == Memory::Size::Unknown ||      // size not yet determined
                 Memory::SizeBytes(m_nMemSize) != m_nBytes)) // size mismatch
            {
                switch (m_nBytes)
                {
                    case 0: m_nBytes = 1; break; // Unexpected size, reset to defaults (1 byte, Unknown)
                    case 1: m_nMemSize = Memory::Size::EightBit; break;
                    case 2: m_nMemSize = Memory::Size::SixteenBit; break;
                    case 3: m_nMemSize = Memory::Size::TwentyFourBit; break;
                    case 4: m_nMemSize = Memory::Size::ThirtyTwoBit; break;
                    default: m_nMemSize = Memory::Size::Array; break;
                }
            }
        }
        else if (nTokenType == Parser::TokenType::Float)
        {
            if (!bFoundSize)
            {
                m_nBytes = 4;
                m_nMemSize = Memory::Size::Float;
                bWordIsSize = true; // allow trailing be/bigendian

                if (nLastTokenType == Parser::TokenType::BigEndian)
                    m_nMemSize = Memory::Size::FloatBigEndian;
            }
        }
        else if (nTokenType == Parser::TokenType::Double)
        {
            if (!bFoundSize)
            {
                m_nBytes = 8;
                m_nMemSize = Memory::Size::Double32;
                bWordIsSize = true; // allow trailing be/bigendian

                if (nLastTokenType == Parser::TokenType::BigEndian)
                    m_nMemSize = Memory::Size::Double32BigEndian;
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
                    m_nMemSize = Memory::Size::Array;
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

    if (m_nMemSize == Memory::Size::Array && bFoundASCII)
        m_nMemSize = Memory::Size::Text;
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
        const auto nAddress = gsl::narrow_cast<ra::data::ByteAddress>(nOffset);

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
    if (m_nMemSize == Memory::Size::ThirtyTwoBit || m_nMemSize == Memory::Size::ThirtyTwoBitBigEndian)
    {
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        const auto nMaxAddress = pMemoryContext.TotalMemorySize();
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

    if (nStartIndex > 0 && nEndIndex < sNote.length() - 1)
    {
        if (isspace(sNote.at(nStartIndex)))
            ++nStartIndex;
        else if (isspace(sNote.at(nEndIndex - 1)))
            --nEndIndex;
    }

    std::wstring sNoteCopy = sNote;
    sNoteCopy.erase(nStartIndex, nEndIndex - nStartIndex + 1);

    if (bPointer && bKeepPointer)
        sNoteCopy.insert(0, L"[pointer] ");

    return sNoteCopy;
}

void CodeNoteModel::EnumeratePointerNotes(
    std::function<bool(ra::data::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const
{
    if (m_pPointerData == nullptr)
        return;

    if (m_pPointerData->OffsetType == PointerData::OffsetType::Overflow)
        EnumeratePointerNotes(m_pPointerData->RawPointerValue, fCallback);
    else
        EnumeratePointerNotes(m_pPointerData->PointerAddress, fCallback);
}

void CodeNoteModel::EnumeratePointerNotes(ra::data::ByteAddress nPointerAddress,
    std::function<bool(ra::data::ByteAddress nAddress, const CodeNoteModel&)> fCallback) const
{
    if (m_pPointerData == nullptr)
        return;

    for (const auto& pNote : m_pPointerData->OffsetNotes)
    {
        if (!fCallback(nPointerAddress + pNote.GetAddress(), pNote))
            break;
    }
}

static uint32_t Convert(std::wstring_view svValue, bool isHex) noexcept
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

    if (isdigit(svRange.at(1)))
        return 1;

    if (svRange.size() < 4)
        return std::wstring::npos;

    if (svRange.at(1) != 'i' && svRange.at(1) != 'I')
        return std::wstring::npos;

    if (svRange.at(2) != 't' && svRange.at(2) != 'T')
        return std::wstring::npos;

    if (isdigit(svRange.at(3)))
        return 3;

    if (svRange.size() < 5 || (svRange.at(3) != 's' && svRange.at(3) != 'S'))
        return std::wstring::npos;

    if (isdigit(svRange.at(4)))
        return 4;

    return std::wstring::npos;
}

static bool ParseBitRange(std::wstring_view svRange, uint32_t& nLow, uint32_t& nHigh)
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
    while (nIndex < svRange.size() && isdigit(svRange.at(nIndex)))
        ++nIndex;

    const auto svLow = svRange.substr(nStart, nIndex - nStart);
    std::wstring_view svHigh;

    while (nIndex < svRange.size() && isspace(svRange.at(nIndex)))
        ++nIndex;

    if (nIndex < svRange.size())
    {
        if (svRange.at(nIndex) != '-')
        {
            if (svRange.substr(nIndex) == L"set")
            {
                nLow = nHigh = Convert(svLow, false);
                return true;
            }

            return false;
        }

        ++nIndex;
        while (nIndex < svRange.size() && isspace(svRange.at(nIndex)))
            ++nIndex;

        if (nIndex < svRange.size() && !isdigit(svRange.at(nIndex)))
        {
            nStart = nIndex;
            nIndex = MatchBitPrefix(svRange.substr(nIndex));
            if (nIndex == std::wstring::npos)
                return false;
            nIndex += nStart;
        }

        nStart = nIndex;
        while (nIndex < svRange.size() && isdigit(svRange.at(nIndex)))
            ++nIndex;

        svHigh = svRange.substr(nStart, nIndex - nStart);
    }

    nLow = Convert(svLow, false);
    nHigh = svHigh.empty() ? nLow : Convert(svHigh, false);

    return true;
}

static bool ParseRange(std::wstring_view svRange, uint32_t& nLow, uint32_t& nHigh, bool isHex)
{
    if (svRange.size() > 2 && svRange.at(0) == L'0' && svRange.at(1) == L'x')
        svRange = svRange.substr(2);
    else if (svRange.size() > 1 && (svRange.at(0) == L'h' || svRange.at(0) == L'H'))
        svRange = svRange.substr(1);

    size_t nIndex = 0;
    while (nIndex < svRange.size() && IsHexDigit(svRange.at(nIndex)))
        ++nIndex;

    if (nIndex == 0)
        return false;

    const auto svLow = svRange.substr(0, nIndex);
    std::wstring_view svHigh;

    while (nIndex < svRange.size() && isspace(svRange.at(nIndex)))
        ++nIndex;

    if (nIndex < svRange.size())
    {
        if (svRange.at(nIndex++) != '-')
            return false;

        while (nIndex < svRange.size() && isspace(svRange.at(nIndex)))
            ++nIndex;

        const auto nStart = nIndex;
        while (nIndex < svRange.size() && IsHexDigit(svRange.at(nIndex)))
            ++nIndex;

        if (nIndex == nStart)
            return false;

        svHigh = svRange.substr(nStart, nIndex);
    }

    nLow = Convert(svLow, isHex);
    nHigh = svHigh.empty() ? nLow : Convert(svHigh, isHex);

    return true;
}

static constexpr size_t FindValueSplit(const std::wstring_view svLine) noexcept
{
    auto nSplit = svLine.find_first_of(L"=:");
    if (nSplit == std::wstring::npos)
        nSplit = svLine.find(L"->");

    return nSplit;
}

static std::wstring_view GetValues(const std::wstring_view svLine)
{
    const auto nSplit = FindValueSplit(svLine);
    if (nSplit == std::wstring::npos)
        return {};

    size_t nRightBracket = svLine.size() + 1;
    const auto nLeftBracket = svLine.find_last_of(L"([{", nSplit);
    if (nLeftBracket == std::wstring::npos)
    {
        uint32_t nLow, nHigh;
        if (ParseBitRange(svLine, nLow, nHigh) || ParseRange(svLine, nLow, nHigh, true))
            return svLine;

        auto nIndex = nSplit;
        while (nIndex > 0 && (isalnum(svLine.at(nIndex - 1)) || isspace(svLine.at(nIndex - 1))))
            --nIndex;

        if (nIndex == 0 || nIndex == nSplit)
            return svLine;

        while (isspace(svLine.at(nIndex)))
            ++nIndex;

        return svLine.substr(nIndex);
    }

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

    if (nRightBracket == std::wstring::npos)
        return svLine;

    return svLine.substr(nLeftBracket + 1, nRightBracket - nLeftBracket - 1);
}

CodeNoteModel::EnumState CodeNoteModel::DetermineEnumState(const std::wstring_view svNote)
{
    EnumState nState = EnumState::None;
    size_t nStart = 0;
    while (nStart < svNote.size())
    {
        auto nEnd = svNote.find_first_of(L"\n\r", nStart);

        if (nStart != nEnd)
        {
            const auto svLine = svNote.substr(nStart, nEnd - nStart);
            const auto svValues = GetValues(svLine);
            if (!svValues.empty())
            {
                size_t nFront = 0;
                do {
                    const auto nComma = svValues.find_first_of(L",;", nFront);
                    const auto svValue = (nComma == std::wstring::npos) ? svValues.substr(nFront) : svValues.substr(nFront, nComma - nFront);

                    const auto nSplit = FindValueSplit(svValue);
                    if (nSplit != std::wstring::npos)
                    {
                        uint32_t nLow, nHigh;
                        const auto svLeft = svValue.substr(0, nSplit);
                        if (ParseBitRange(svLeft, nLow, nHigh))
                        {
                            if (nState == EnumState::None)
                                nState = EnumState::Bits;
                        }
                        else if (ParseRange(svLeft, nLow, nHigh, true))
                        {
                            if (svLeft.find_first_of(L"ABCDEFabcdefHhx") != std::wstring::npos)
                                return EnumState::Hex;

                            nState = EnumState::Dec;
                        }
                    }

                    if (nComma == std::wstring::npos)
                        break;

                    nFront = nComma + 1;
                    while (nFront < svValues.size() && isspace(svValues.at(nFront)))
                        ++nFront;
                } while (nFront < svValues.size());
            }
        }

        while (nEnd < svNote.size() && (svNote.at(nEnd) == L'\n' || svNote.at(nEnd) == L'\r'))
            ++nEnd;

        nStart = nEnd;
    }

    return nState;
}

static std::wstring_view MatchSubNote(std::wstring_view svNote, std::function<bool(std::wstring_view)> fMatch)
{
    size_t nStart = 0;
    while (nStart < svNote.size())
    {
        auto nEnd = svNote.find_first_of(L"\n\r", nStart);

        if (nStart != nEnd)
        {
            const auto svLine = svNote.substr(nStart, nEnd - nStart);
            const auto svValues = GetValues(svLine);
            if (!svValues.empty())
            {
                size_t nFront = 0;
                do {
                    const auto nComma = svValues.find_first_of(L",;", nFront);
                    const auto svValue = (nComma == std::wstring::npos) ? svValues.substr(nFront) : svValues.substr(nFront, nComma - nFront);

                    if (fMatch(svValue))
                        return svValue;

                    if (nComma == std::wstring::npos)
                        break;

                    nFront = nComma + 1;
                    while (nFront < svValues.size() && isspace(svValues.at(nFront)))
                        ++nFront;
                } while (nFront < svValues.size());
            }
        }

        while (nEnd < svNote.size() && (svNote.at(nEnd) == L'\n' || svNote.at(nEnd) == L'\r'))
            ++nEnd;

        nStart = nEnd;
    }

    return {};
}

static bool MatchEnumText(const std::wstring_view svValue, uint32_t nValue, bool isHex)
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
        while (!svRight.empty() && isspace(svRight.at(0)))
            svRight.remove_prefix(1);

        if (ParseRange(svRight, nLow, nHigh, isHex))
        {
            if (nValue >= nLow && nValue <= nHigh)
                return true;
        }
    }

    return false;
}

std::wstring_view CodeNoteModel::GetEnumText(uint32_t nValue) const
{
    if (m_nEnumState == EnumState::None)
        return {};

    std::wstring_view svNote(m_sNote);

    if (m_pPointerData != nullptr)
        svNote = svNote.substr(0, m_pPointerData->HeaderLength);

    if (m_nEnumState == EnumState::Unknown)
    {
        m_nEnumState = DetermineEnumState(svNote);
        if (m_nEnumState == EnumState::None)
            return {};
    }

    const bool isHex = (m_nEnumState == EnumState::Hex || m_nEnumState == EnumState::Bits);
    return MatchSubNote(svNote, [nValue, isHex](std::wstring_view svValue) {
        return MatchEnumText(svValue, nValue, isHex);
    });
}

static bool MatchBitsText(const std::wstring_view svValue, ra::data::Memory::Size nBits)
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

std::wstring_view CodeNoteModel::GetSubNote(ra::data::Memory::Size nBits) const
{
    if (ra::data::Memory::SizeBits(nBits) >= 8 || nBits == ra::data::Memory::Size::BitCount)
        return {};

    if (m_nEnumState != EnumState::Bits && m_nEnumState != EnumState::Unknown)
        return {};

    std::wstring_view svNote(m_sNote);

    if (m_pPointerData != nullptr)
        svNote = svNote.substr(0, m_pPointerData->HeaderLength);

    if (m_nEnumState == EnumState::Unknown)
    {
        m_nEnumState = DetermineEnumState(svNote);
        if (m_nEnumState != EnumState::Bits)
            return {};
    }

    return MatchSubNote(svNote, [nBits](std::wstring_view svValue) {
        return MatchBitsText(svValue, nBits);
    });
}

std::wstring CodeNoteModel::GetSummary() const
{
    if (m_pPointerData != nullptr)
    {
        std::wstring_view svHeader(m_sNote.data(), m_pPointerData->HeaderLength);
        const auto nHeaderLineEnd = svHeader.find_first_of(L"\n\r");
        if (nHeaderLineEnd != std::string::npos)
            svHeader = svHeader.substr(0, nHeaderLineEnd);
        return TrimSize(std::wstring(svHeader), false);
    }

    std::wstring_view svNote(m_sNote);
    const auto nLineEnd = svNote.find_first_of(L"\n\r");
    if (nLineEnd != std::string::npos)
        svNote = svNote.substr(0, nLineEnd);

    if (m_nEnumState == EnumState::Unknown)
        m_nEnumState = DetermineEnumState(m_sNote);

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
                while (nIndex > 0 && isspace(svNote.at(nIndex - 1)))
                    --nIndex;

                if (nIndex > 0 && !isalnum(svNote.at(nIndex - 1)))
                {
                    --nIndex;
                    while (nIndex > 0 && isspace(svNote.at(nIndex - 1)))
                        --nIndex;
                }
            }

            svNote = svNote.substr(0, nIndex);
        }
    }

    return TrimSize(std::wstring(svNote), false);
}

} // namespace models
} // namespace data
} // namespace ra
