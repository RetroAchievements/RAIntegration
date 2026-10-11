#include "MemoryNoteModel.hh"

#include "ArrayMemoryNoteModel.hh"
#include "AuthoredMemoryNoteModel.hh"
#include "PointerMemoryNoteModel.hh"
#include "ValueMemoryNoteModel.hh"

#include "context/IConsoleContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

namespace ra {
namespace data {
namespace models {

class MemoryNoteParser
{
public:
    MemoryNoteParser(std::wstring_view sNote) noexcept :
        m_sNote(sNote)
    {}

    enum TokenType
    {
        None = 0,
        Number,
        Bits,
        Bytes,
        Float,
        Double,
        MBF,
        BigEndian,
        LittleEndian,
        BCD,
        Hex,
        ASCII,
        HexNumber,
        Multiplier,
        Pointer,
        Struct,
        Other,
    };

    TokenType NextToken(std::wstring& sWord) const;
    wchar_t Peek() const { return (m_nIndex < m_sNote.size()) ? m_sNote.at(m_nIndex) : 0; }

private:
    std::wstring_view m_sNote;
    mutable size_t m_nIndex = 0;
};

class UnknownMemoryNoteModel : public StructuredMemoryNoteModel
{
public:
    UnknownMemoryNoteModel() noexcept
        : StructuredMemoryNoteModel(L"", 1, Memory::Format::Unknown, Memory::Size::Unknown, MemoryNoteType::None)
    {
    }

    void Parse(std::wstring_view svNote, std::wstring_view svParentIndent);

    bool IsPointer() const noexcept { return m_bIsPointer; }
    uint32_t GetBytesPerElement() const noexcept { return m_nBytesPerElement; }

private:
    void ExtractSize(std::wstring_view sNote);
    static Memory::Size GetImpliedPointerSize();

    uint32_t m_nBytesPerElement = 0;
    bool m_bIsPointer = false;
};

class RootPointerMemoryNoteModel : public PointerMemoryNoteModel, public AuthoredMemoryNoteModel
{
public:
    RootPointerMemoryNoteModel(const std::wstring& sNote, const UnknownMemoryNoteModel& pNote)
        : PointerMemoryNoteModel(sNote, pNote.GetBytes(), pNote.GetDefaultMemFormat(), pNote.GetMemSize()),
          AuthoredMemoryNoteModel(sNote)
    {
        m_svNote = m_sNote;
    }
};

class RootArrayMemoryNoteModel : public ArrayMemoryNoteModel, public AuthoredMemoryNoteModel
{
public:
    RootArrayMemoryNoteModel(const std::wstring& sNote, const UnknownMemoryNoteModel& pNote)
        : ArrayMemoryNoteModel(sNote, pNote.GetBytes(), pNote.GetBytesPerElement()),
          AuthoredMemoryNoteModel(sNote)
    {
        m_svNote = m_sNote;
    }
};

class RootValueMemoryNoteModel : public ValueMemoryNoteModel, public AuthoredMemoryNoteModel
{
public:
    RootValueMemoryNoteModel(const std::wstring& sNote, const UnknownMemoryNoteModel& pNote)
        : ValueMemoryNoteModel(sNote, pNote.GetBytes(), pNote.GetDefaultMemFormat(), pNote.GetMemSize()),
          AuthoredMemoryNoteModel(sNote)
    {
        m_svNote = m_sNote;
    }
};

std::unique_ptr<MemoryNoteModel> MemoryNoteModel::Parse(const std::wstring& sNote)
{
    UnknownMemoryNoteModel pNote;
    pNote.Parse(sNote, L"\n");

    if (pNote.IsPointer())
    {
        auto pPointerNote = std::make_unique<RootPointerMemoryNoteModel>(sNote, pNote);
        pPointerNote->ExtractIndirectNotes(L"\n");
        return std::move(pPointerNote);
    }

    if (pNote.GetBytesPerElement() != 0 || pNote.GetMemSize() == Memory::Size::Array)
    {
        auto pArrayNote = std::make_unique<RootArrayMemoryNoteModel>(sNote, pNote);
        pArrayNote->ExtractIndirectNotes(L"\n");
        return std::move(pArrayNote);
    }

    auto pValueNote = std::make_unique<RootValueMemoryNoteModel>(sNote, pNote);
    pValueNote->DeterminePreferredMemFormat();
    return std::move(pValueNote);
}

std::unique_ptr<MemoryNoteModel> MemoryNoteModel::ParseOffsetNote(std::wstring_view svNote, std::wstring_view svIndent)
{
    UnknownMemoryNoteModel pNote;
    pNote.Parse(svNote, svIndent);

    if (pNote.IsPointer())
    {
        auto pPointerNote = std::make_unique<PointerMemoryNoteModel>(svNote, pNote.GetBytes(), pNote.GetDefaultMemFormat(), pNote.GetMemSize());
        pPointerNote->ExtractIndirectNotes(svIndent);
        return std::move(pPointerNote);
    }

    if (pNote.GetBytesPerElement() != 0 || pNote.GetMemSize() == Memory::Size::Array)
    {
        auto pArrayNote = std::make_unique<ArrayMemoryNoteModel>(svNote, pNote.GetBytes(), pNote.GetBytesPerElement());
        pArrayNote->ExtractIndirectNotes(svIndent);
        return std::move(pArrayNote);
    }

    auto pValueNote = std::make_unique<ValueMemoryNoteModel>(svNote, pNote.GetBytes(), pNote.GetDefaultMemFormat(), pNote.GetMemSize());
    pValueNote->DeterminePreferredMemFormat();
    return std::move(pValueNote);
}

void UnknownMemoryNoteModel::Parse(std::wstring_view svNote, std::wstring_view svParentIndent)
{
    std::wstring_view svLine;
    size_t nIndex = 0;
    size_t nNextIndex;
    do
    {
        nNextIndex = svNote.find(L'\n', nIndex);
        if (nNextIndex == std::wstring::npos)
            svLine = svNote.substr(nIndex);
        else if (nNextIndex > 0 && svNote.at(nNextIndex - 1) == '\r') // expect data to be normalized for Windows so it will load into the controls correctly
            svLine = svNote.substr(nIndex, nNextIndex - nIndex - 1);
        else
            svLine = svNote.substr(nIndex, nNextIndex - nIndex);

        if (!svLine.empty())
        {
            ExtractSize(svLine);

            // If a size was found, stop processing.
            if (m_nMemSize != Memory::Size::Unknown)
                break;
        }

        if (nNextIndex == std::wstring::npos) // end of string
            break;

        nIndex = nNextIndex + 1;
    } while (true);

    if (!m_bIsPointer && !m_nBytesPerElement && svNote.find(L'+') != std::wstring::npos)
    {
        m_svNote = svNote;
        DetermineIndent(svParentIndent);
        if (!m_svIndent.empty())
        {
            m_bIsPointer = (m_nBytes <= 4);

            // Reset size and format and only parse the pre-indent part of the note.
            m_nMemSize = Memory::Size::Unknown;
            m_nBytes = 1;
            m_nMemFormat = Memory::Format::Unknown;

            const auto svPreIndent = svNote.substr(0, svNote.find(m_svIndent));
            Parse(svPreIndent, svParentIndent);

            return;
        }
    }

    if (m_bIsPointer)
    {
        m_nMemFormat = Memory::Format::Hex;

        if (m_nMemSize == Memory::Size::Unknown)
        {
            // pointer size not specified. assume 32-bit
            m_nMemSize = GetImpliedPointerSize();
            m_nBytes = Memory::SizeBytes(m_nMemSize);
        }
    }
}

Memory::Size UnknownMemoryNoteModel::GetImpliedPointerSize()
{
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();

    Memory::Size nSize;
    uint32_t nMask;
    uint32_t nOffset;
    if (pConsoleContext.GetRealAddressConversion(&nSize, &nMask, &nOffset))
        return nSize;

    return Memory::Size::ThirtyTwoBit;
}

MemoryNoteParser::TokenType MemoryNoteParser::NextToken(std::wstring& sWord) const
{
    wchar_t cFirstLetter = '\0';
    bool bWordIsNumber = false;
    bool bWordIsHexNumber = false;
    sWord.clear();

    for (; m_nIndex < m_sNote.size(); ++m_nIndex)
    {
        const wchar_t c = m_sNote.at(m_nIndex);

        // find the next word
        if (c > 255)
        {
            // ignore unicode characters
            // if we've found any alphanumeric characters, process them.
            if (!sWord.empty())
                break;
        }
        else if (ra::util::String::IsAlpha(c))
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
                if (ra::util::String::IsHexDigit(c))
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
        else if (ra::util::String::IsDigit(c))
        {
            if (sWord.empty())
            {
                // start of number
                sWord.push_back(c);

                if (c == '0' && m_nIndex < m_sNote.size() - 2 &&
                    m_sNote.at(m_nIndex + 1) == 'x' &&
                    ra::util::String::IsHexDigit(m_sNote.at(m_nIndex + 2)))
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

        case 'p':
            if (sWord == L"pointer")
                return TokenType::Pointer;
            break;

        case 's':
            if (sWord == L"struct" || sWord == L"structure")
                return TokenType::Struct;
            break;

        case 'x':
            if (sWord == L"x")
                return TokenType::Multiplier;
            break;

        default:
            break;
    }

    return TokenType::Other;
}

void UnknownMemoryNoteModel::ExtractSize(std::wstring_view sNote)
{
    // "Nbit" smallest possible note - and that's just the size annotation
    if (sNote.length() < 4)
        return;

    bool bBytesFromBits = false;
    bool bFoundSize = false;
    bool bFoundASCII = false;
    bool bFoundPointer = false;
    bool bLastWordIsSize = false;
    auto nLastTokenType = MemoryNoteParser::TokenType::None;
    uint32_t nElementCount = 0;

    std::wstring sPreviousWord, sWord;
    const MemoryNoteParser parser(sNote);
    do
    {
        const auto nTokenType = parser.NextToken(sWord);
        if (nTokenType == MemoryNoteParser::TokenType::None)
            break;

        // process the word
        bool bWordIsSize = false;
        if (nTokenType == MemoryNoteParser::TokenType::Number)
        {
            if (nLastTokenType == MemoryNoteParser::TokenType::MBF)
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
            else if (nLastTokenType == MemoryNoteParser::TokenType::Double && sWord == L"32")
            {
                m_nBytes = 4;
                m_nMemSize = Memory::Size::Double32;
                bWordIsSize = true;
                bFoundSize = true;
            }
        }
        else if (nTokenType == MemoryNoteParser::TokenType::BCD || nTokenType == MemoryNoteParser::TokenType::Hex)
        {
            m_nMemFormat = Memory::Format::Hex;
        }
        else if (nTokenType == MemoryNoteParser::TokenType::ASCII)
        {
            bFoundASCII = true;
        }
        else if (bLastWordIsSize)
        {
            switch (nTokenType)
            {
                case MemoryNoteParser::TokenType::Pointer:
                    m_bIsPointer = true;
                    break;

                case MemoryNoteParser::TokenType::Float:
                    if (m_nMemSize == Memory::Size::ThirtyTwoBit)
                    {
                        m_nMemSize = Memory::Size::Float;
                        bWordIsSize = true; // allow trailing be/bigendian
                    }
                    break;

                case MemoryNoteParser::TokenType::Double:
                    if (m_nMemSize == Memory::Size::ThirtyTwoBit || m_nBytes == 8)
                    {
                        m_nMemSize = Memory::Size::Double32;
                        bWordIsSize = true; // allow trailing be/bigendian
                    }
                    break;

                case MemoryNoteParser::TokenType::BigEndian:
                    switch (m_nMemSize)
                    {
                        case Memory::Size::SixteenBit: m_nMemSize = Memory::Size::SixteenBitBigEndian; break;
                        case Memory::Size::TwentyFourBit: m_nMemSize = Memory::Size::TwentyFourBitBigEndian; break;
                        case Memory::Size::ThirtyTwoBit: m_nMemSize = Memory::Size::ThirtyTwoBitBigEndian; break;
                        case Memory::Size::Float: m_nMemSize = Memory::Size::FloatBigEndian; break;
                        case Memory::Size::Double32: m_nMemSize = Memory::Size::Double32BigEndian; break;
                        default: break;
                    }
                    break;

                case MemoryNoteParser::TokenType::LittleEndian:
                    if (m_nMemSize == Memory::Size::MBF32)
                        m_nMemSize = Memory::Size::MBF32LE;
                    break;

                case MemoryNoteParser::TokenType::MBF:
                    if (m_nBytes == 4 || m_nBytes == 5)
                        m_nMemSize = Memory::Size::MBF32;
                    break;

                case MemoryNoteParser::TokenType::Struct:
                    if (m_nBytesPerElement == 0)
                    {
                        m_nMemSize = Memory::Size::Array;
                        m_nBytesPerElement = m_nBytes;
                    }
                    break;

                default:
                    break;
            }
        }
        else if (nLastTokenType == MemoryNoteParser::TokenType::Number)
        {
            if (nTokenType == MemoryNoteParser::TokenType::Bits)
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
            else if (nTokenType == MemoryNoteParser::TokenType::Bytes)
            {
                if (!bFoundSize || (bBytesFromBits && !bFoundPointer))
                {
                    m_nBytes = _wtoi(sPreviousWord.c_str());
                    bBytesFromBits = false;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
            else if (nTokenType == MemoryNoteParser::TokenType::Multiplier)
            {
                if (!bFoundSize)
                    nElementCount = _wtoi(sPreviousWord.c_str());
            }

            if (bWordIsSize)
            {
                if (nElementCount != 0)
                {
                    m_nMemSize = Memory::Size::Array;
                    m_nBytesPerElement = m_nBytes;
                    m_nBytes *= nElementCount;
                }
                else if (m_nMemSize == Memory::Size::Unknown ||     // size not yet determined
                    Memory::SizeBytes(m_nMemSize) != m_nBytes) // size mismatch
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
        }
        else if (nTokenType == MemoryNoteParser::TokenType::Float)
        {
            if (!bFoundSize)
            {
                m_nBytes = 4;
                m_nMemSize = Memory::Size::Float;
                bWordIsSize = true; // allow trailing be/bigendian

                if (nLastTokenType == MemoryNoteParser::TokenType::BigEndian)
                    m_nMemSize = Memory::Size::FloatBigEndian;
            }
        }
        else if (nTokenType == MemoryNoteParser::TokenType::Double)
        {
            if (!bFoundSize)
            {
                m_nBytes = 8;
                m_nMemSize = Memory::Size::Double32;
                bWordIsSize = true; // allow trailing be/bigendian

                if (nLastTokenType == MemoryNoteParser::TokenType::BigEndian)
                    m_nMemSize = Memory::Size::Double32BigEndian;
            }
        }
        else if (nTokenType == MemoryNoteParser::TokenType::Pointer)
        {
            bFoundPointer = true;
        }
        else if (nLastTokenType == MemoryNoteParser::TokenType::HexNumber)
        {
            if (nTokenType == MemoryNoteParser::TokenType::Bytes)
            {
                if (!bFoundSize || (bBytesFromBits && !bFoundPointer))
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
        else
        {
            nElementCount = 0;
        }

        // store information about the word for later
        bLastWordIsSize = bWordIsSize;
        nLastTokenType = nTokenType;

        const wchar_t c = parser.Peek();
        if (ra::util::String::IsAlNum(c))
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
            nLastTokenType = MemoryNoteParser::TokenType::None;
        }
    } while (true);

    if (m_nMemSize == Memory::Size::Array && bFoundASCII)
        m_nMemSize = Memory::Size::Text;

    if (bFoundPointer && !m_bIsPointer && m_nMemSize != Memory::Size::Unknown)
        m_bIsPointer = true;
}

std::wstring MemoryNoteModel::TrimSize(std::wstring_view svNote, bool bKeepPointer)
{
    size_t nEndIndex = 0;
    size_t nStartIndex = svNote.find('[');
    if (nStartIndex != std::string::npos)
    {
        nEndIndex = svNote.find(']', nStartIndex + 1);
        if (nEndIndex == std::string::npos)
            return std::wstring(svNote);
    }
    else
    {
        nStartIndex = svNote.find('(');
        if (nStartIndex == std::string::npos)
            return std::wstring(svNote);

        nEndIndex = svNote.find(')', nStartIndex + 1);
        if (nEndIndex == std::string::npos)
            return std::wstring(svNote);
    }

    bool bPointer = false;
    std::wstring sWord;
    MemoryNoteParser::TokenType nTokenType;
    const MemoryNoteParser parser(svNote.substr(nStartIndex + 1, nEndIndex - nStartIndex - 1));
    do
    {
        nTokenType = parser.NextToken(sWord);
        if (nTokenType == MemoryNoteParser::TokenType::Pointer)
            bPointer = true;
        else if (nTokenType == MemoryNoteParser::TokenType::Other)
            return std::wstring(svNote);
    } while (nTokenType != MemoryNoteParser::TokenType::None);

    std::wstring sSummary;
    if (bPointer && bKeepPointer)
        sSummary.append(L"[pointer] ");

    while (nStartIndex > 0)
    {
        const wchar_t c = svNote.at(nStartIndex - 1);
        if (!ra::util::String::IsSpace(c))
            break;
        --nStartIndex;
    }
    if (nStartIndex > 0)
        sSummary.append(svNote.substr(0, nStartIndex));

    ++nEndIndex;
    while (nEndIndex < svNote.length())
    {
        const wchar_t c = svNote.at(nEndIndex);
        if (!ra::util::String::IsSpace(c))
            break;
        ++nEndIndex;
    }

    if (nEndIndex < svNote.length())
    {
        if (sSummary.length() > 0 && !ra::util::String::IsSpace(sSummary.back()))
            sSummary.push_back(' ');
        sSummary.append(svNote.substr(nEndIndex));
    }

    return sSummary;
}

std::wstring_view MemoryNoteModel::GetFullSummaryStringView() const
{
    const auto nIndex = m_svNote.find('\n');
    auto svSummary = m_svNote.substr(0, nIndex);
    while (!svSummary.empty() && ra::util::String::IsSpace(svSummary.back()))
        svSummary.remove_suffix(1);

    return svSummary;
}

bool MemoryNoteModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, ra::data::ByteAddress nSearchAddress) const
{
    const auto nBaseAddress = vChain.empty() ? 0 : vChain.back().nAddress;
    const auto nOffset = nSearchAddress - nBaseAddress;
    if (nOffset >= m_nAddress && nOffset < m_nAddress + m_nBytes)
    {
        vChain.emplace_back(this, nBaseAddress + m_nAddress, 0);
        return true;
    }

    return false;
}

bool MemoryNoteModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, const MemoryNoteModel& pNote) const
{
    if (&pNote == this)
    {
        const auto nBaseAddress = vChain.empty() ? 0 : vChain.back().nAddress;
        vChain.emplace_back(this, nBaseAddress + m_nAddress, 0);
        return true;
    }

    return false;
}

} // namespace models
} // namespace data
} // namespace ra
