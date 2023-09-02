#include "CodeNotesModel.hh"

#include "RA_Defs.h"

#include "api\DeleteCodeNote.hh"
#include "api\FetchCodeNotes.hh"
#include "api\UpdateCodeNote.hh"

#include "data\context\ConsoleContext.hh"
#include "data\context\EmulatorContext.hh"
#include "data\context\UserContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace data {
namespace models {

CodeNotesModel::CodeNotesModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::CodeNotes));
    GSL_SUPPRESS_F6 SetName(L"Code Notes");
}

void CodeNotesModel::Refresh(unsigned int nGameId, CodeNoteChangedFunction fCodeNoteChanged, std::function<void()> callback)
{
    m_nGameId = nGameId;
    m_mCodeNotes.clear();

    if (nGameId == 0)
    {
        m_fCodeNoteChanged = nullptr;
        callback();
        return;
    }

    m_fCodeNoteChanged = fCodeNoteChanged;

    if (callback == nullptr) // unit test workaround to avoid server call
        return;

    {
        std::unique_lock<std::mutex> lock(m_oMutex);
        m_bRefreshing = true;
    }

    ra::api::FetchCodeNotes::Request request;
    request.GameId = nGameId;
    request.CallAsync([this, callback](const ra::api::FetchCodeNotes::Response& response)
    {
        if (response.Failed())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Failed to download code notes",
                ra::Widen(response.ErrorMessage));
        }
        else
        {
            for (const auto& pNote : response.Notes)
            {
                {
                    std::unique_lock<std::mutex> lock(m_oMutex);
                    const auto pIter = m_mOriginalCodeNotes.find(pNote.Address);
                    if (pIter != m_mOriginalCodeNotes.end())
                    {
                        pIter->second.first = pNote.Author;
                        pIter->second.second = pNote.Note;
                        continue;
                    }
                }

                AddCodeNote(pNote.Address, pNote.Author, pNote.Note);
            }
        }

        std::map<ra::ByteAddress, std::wstring> mPendingCodeNotes;
        {
            std::unique_lock<std::mutex> lock(m_oMutex);
            mPendingCodeNotes.swap(m_mPendingCodeNotes);
            m_bRefreshing = false;
        }

        for (const auto pNote : mPendingCodeNotes)
            SetCodeNote(pNote.first, pNote.second);

        callback();
    });
}

void CodeNotesModel::ExtractSize(CodeNote& pNote)
{
    // provide defaults in case no matches are found
    pNote.Bytes = 1;
    pNote.MemSize = MemSize::Unknown;

    // "Nbit" smallest possible note - and that's just the size annotation
    if (pNote.Note.length() < 4)
        return;

    bool bBytesFromBits = false;
    bool bFoundSize = false;
    bool bLastWordIsSize = false;
    bool bLastWordIsNumber = false;
    bool bWordIsNumber = false;

    std::wstring sPreviousWord, sWord;
    const size_t nLength = pNote.Note.length();
    for (size_t nIndex = 0; nIndex <= nLength; ++nIndex)
    {
        // support reading null terminator so we process the last word in the string
        const wchar_t c = (nIndex == nLength) ? 0 : pNote.Note.at(nIndex);

        // find the next word
        if (isalpha(c))
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
                    pNote.Bytes = 4;
                    pNote.MemSize = MemSize::MBF32;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
                else if (nBits == 40)
                {
                    pNote.Bytes = 5;
                    pNote.MemSize = MemSize::MBF32;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
        }
        else if (bLastWordIsSize)
        {
            if (sWord == L"float")
            {
                if (pNote.MemSize == MemSize::ThirtyTwoBit)
                    pNote.MemSize = MemSize::Float;
            }
            else if (sWord == L"be" || sWord == L"bigendian")
            {
                switch (pNote.MemSize)
                {
                    case MemSize::SixteenBit: pNote.MemSize = MemSize::SixteenBitBigEndian; break;
                    case MemSize::TwentyFourBit: pNote.MemSize = MemSize::TwentyFourBitBigEndian; break;
                    case MemSize::ThirtyTwoBit: pNote.MemSize = MemSize::ThirtyTwoBitBigEndian; break;
                    case MemSize::Float: pNote.MemSize = MemSize::FloatBigEndian; break;
                    default: break;
                }
            }
            else if (sWord == L"le")
            {
                if (pNote.MemSize == MemSize::MBF32)
                    pNote.MemSize = MemSize::MBF32LE;
            }
            else if (sWord == L"mbf")
            {
                if (pNote.Bytes == 4 || pNote.Bytes == 5)
                    pNote.MemSize = MemSize::MBF32;
            }
        }
        else if (bLastWordIsNumber)
        {
            if (sWord == L"bit" || sWord == L"bits")
            {
                if (!bFoundSize)
                {
                    const auto nBits = _wtoi(sPreviousWord.c_str());
                    pNote.Bytes = (nBits + 7) / 8;
                    pNote.MemSize = MemSize::Unknown;
                    bBytesFromBits = true;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }
            else if (sWord == L"byte" || sWord == L"bytes")
            {
                if (!bFoundSize || bBytesFromBits)
                {
                    pNote.Bytes = _wtoi(sPreviousWord.c_str());
                    pNote.MemSize = MemSize::Unknown;
                    bBytesFromBits = false;
                    bWordIsSize = true;
                    bFoundSize = true;
                }
            }

            if (bWordIsSize)
            {
                switch (pNote.Bytes)
                {
                    case 0: pNote.Bytes = 1; break; // Unexpected size, reset to defaults (1 byte, Unknown)
                    case 1: pNote.MemSize = MemSize::EightBit; break;
                    case 2: pNote.MemSize = MemSize::SixteenBit; break;
                    case 3: pNote.MemSize = MemSize::TwentyFourBit; break;
                    case 4: pNote.MemSize = MemSize::ThirtyTwoBit; break;
                    default: pNote.MemSize = MemSize::Array; break;
                }
            }
        }
        else if (sWord == L"float")
        {
            if (!bFoundSize)
            {
                pNote.Bytes = 4;
                pNote.MemSize = MemSize::Float;
                bWordIsSize = true;

                if (sPreviousWord == L"be" || sPreviousWord == L"bigendian")
                    pNote.MemSize = MemSize::FloatBigEndian;
            }
        }

        // store information about the word for later
        bLastWordIsSize = bWordIsSize;
        bLastWordIsNumber = bWordIsNumber;

        if (isalnum(c))
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

static unsigned ReadPointer(const ra::data::context::EmulatorContext& pEmulatorContext,
    ra::ByteAddress nPointerAddress, MemSize nSize)
{
    auto nAddress = pEmulatorContext.ReadMemory(nPointerAddress, nSize);

    // assume anything annotated as a 32-bit pointer is providing a real (non-translated) address and
    // attempt to do the translation ourself.
    if (nSize == MemSize::ThirtyTwoBit)
    {
        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
        const auto nConvertedAddress = pConsoleContext.ByteAddressFromRealAddress(nAddress);
        if (nConvertedAddress != 0xFFFFFFFF)
            nAddress = nConvertedAddress;
    }

    return nAddress;
}

void CodeNotesModel::AddCodeNote(ra::ByteAddress nAddress, const std::string& sAuthor, const std::wstring& sNote)
{
    auto nIndex = sNote.find(L'\n');
    auto sFirstLine = (nIndex == std::string::npos) ? sNote : sNote.substr(0, nIndex);
    StringMakeLowercase(sFirstLine);

    if (sFirstLine.find(L"pointer") != std::string::npos)
    {
        nIndex = sNote.find(L"\n+"); // look for line starting with plus sign
        if (nIndex != std::string::npos)
        {
            nIndex += 2;

            auto pointerData = std::make_unique<PointerData>();
            do
            {
                OffsetCodeNote offsetNote;
                const auto nNextIndex = sNote.find(L"\n+", nIndex);
                auto sNextNote = sNote.substr(nIndex, nNextIndex - nIndex);
                ra::Trim(sNextNote);

                wchar_t* pEnd = nullptr;

                try
                {
                    if (sNextNote.length() > 2 && sNextNote.at(1) == 'x')
                        offsetNote.Offset = gsl::narrow_cast<int>(std::wcstol(sNextNote.c_str() + 2, &pEnd, 16));
                    else
                        offsetNote.Offset = gsl::narrow_cast<int>(std::wcstol(sNextNote.c_str(), &pEnd, 10));
                }
                catch (const std::exception&)
                {
                    break;
                }

                if (!pEnd || isalnum(*pEnd))
                    break;

                const wchar_t* pStop = sNextNote.c_str() + sNextNote.length();
                while (pEnd < pStop && isspace(*pEnd))
                    pEnd++;
                if (pEnd < pStop && !isalnum(*pEnd))
                {
                    pEnd++;
                    while (pEnd < pStop && isspace(*pEnd))
                        pEnd++;
                }

                offsetNote.Author = sAuthor;
                offsetNote.Note = sNextNote.substr(pEnd - sNextNote.c_str());
                ExtractSize(offsetNote);

                const auto nRangeOffset = offsetNote.Offset + offsetNote.Bytes;
                pointerData->OffsetRange = std::max(pointerData->OffsetRange, nRangeOffset);

                pointerData->OffsetNotes.push_back(std::move(offsetNote));

                if (nNextIndex == std::string::npos)
                {
                    CodeNote pointerNote;
                    pointerNote.Author = sAuthor;

                    // extract pointer size from first line (assume 32-bit if not specified)
                    pointerNote.Note = sFirstLine;
                    ExtractSize(pointerNote);
                    if (pointerNote.MemSize == MemSize::Unknown)
                    {
                        pointerNote.MemSize = MemSize::ThirtyTwoBit;
                        pointerNote.Bytes = 4;
                    }

                    // capture the initial value of the pointer
                    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
                    const auto nPointerValue = ReadPointer(pEmulatorContext, nAddress, pointerNote.MemSize);
                    pointerData->PointerValue = nPointerValue;
                    pointerNote.PointerData = std::move(pointerData);

                    // assign entire note to pointer note
                    pointerNote.Note = sNote;
                    {
                        std::unique_lock<std::mutex> lock(m_oMutex);
                        m_mCodeNotes.insert_or_assign(nAddress, std::move(pointerNote));
                    }
                    m_bHasPointers = true;

                    OnCodeNoteChanged(nAddress, sNote);

                    if (m_fCodeNoteChanged)
                    {
                        for (const auto& pNote : m_mCodeNotes[nAddress].PointerData->OffsetNotes)
                            m_fCodeNoteChanged(nPointerValue + pNote.Offset, pNote.Note);
                    }

                    return;
                }

                nIndex = nNextIndex + 2;
            } while (true);
        }
    }

    CodeNote note;
    note.Author = sAuthor;
    note.Note = sNote;
    ExtractSize(note);
    {
        std::unique_lock<std::mutex> lock(m_oMutex);
        m_mCodeNotes.insert_or_assign(nAddress, std::move(note));
    }

    OnCodeNoteChanged(nAddress, sNote);
}

void CodeNotesModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
    SetValue(ra::data::models::AssetModelBase::ChangesProperty,
             m_mOriginalCodeNotes.empty() ?
                 ra::etoi(ra::data::models::AssetChanges::None) :
                 ra::etoi(ra::data::models::AssetChanges::Unpublished));

    if (m_fCodeNoteChanged != nullptr)
        m_fCodeNoteChanged(nAddress, sNewNote);
}

ra::ByteAddress CodeNotesModel::FindCodeNoteStart(ra::ByteAddress nAddress) const
{
    auto pIter = m_mCodeNotes.lower_bound(nAddress);

    // exact match, return it
    if (pIter != m_mCodeNotes.end() && pIter->first == nAddress)
        return nAddress;

    // lower_bound returns the first item _after_ the search value. scan all items before
    // the found item to see if any of them contain the target address. have to scan
    // all items because a singular note may exist within a range.
    if (pIter != m_mCodeNotes.begin())
    {
        do
        {
            --pIter;

            if (pIter->second.Bytes > 1 && pIter->second.Bytes + pIter->first > nAddress)
                return pIter->first;

        } while (pIter != m_mCodeNotes.begin());
    }

    // also check for derived code notes
    if (m_bHasPointers)
    {
        for (const auto& pCodeNote : m_mCodeNotes)
        {
            if (pCodeNote.second.PointerData == nullptr)
                continue;

            if (nAddress >= pCodeNote.second.PointerData->PointerValue &&
                nAddress < pCodeNote.second.PointerData->PointerValue + pCodeNote.second.PointerData->OffsetRange)
            {
                const auto nOffset = ra::to_signed(nAddress - pCodeNote.second.PointerData->PointerValue);
                for (const auto& pOffsetNote : pCodeNote.second.PointerData->OffsetNotes)
                {
                    if (pOffsetNote.Offset <= nOffset && pOffsetNote.Offset + ra::to_signed(pOffsetNote.Bytes) > nOffset)
                        return pCodeNote.second.PointerData->PointerValue + pOffsetNote.Offset;
                }
            }
        }
    }

    return 0xFFFFFFFF;
}

std::wstring CodeNotesModel::BuildCodeNoteSized(ra::ByteAddress nAddress,
    unsigned nCheckBytes, ra::ByteAddress nNoteAddress, const CodeNote& pNote)
{
    // extract the first line
    std::wstring sNote = pNote.Note;
    const auto iNewLine = sNote.find('\n');
    if (iNewLine != std::string::npos)
    {
        sNote.resize(iNewLine);

        if (sNote.back() == '\r')
            sNote.pop_back();
    }

    const unsigned int nNoteSize = pNote.Bytes;
    if (nNoteAddress == nAddress && nNoteSize == nCheckBytes)
    {
        // exact size match - don't add a suffix
    }
    else if (nCheckBytes == 1)
    {
        // searching on bytes, indicate the offset into the note
        sNote.append(ra::StringPrintf(L" [%d/%d]", nAddress - nNoteAddress + 1, nNoteSize));
    }
    else
    {
        // searching on something other than bytes, just indicate that it's overlapping with the note
        sNote.append(L" [partial]");
    }

    return sNote;
}

std::wstring CodeNotesModel::FindCodeNote(ra::ByteAddress nAddress, MemSize nSize) const
{
    const unsigned int nCheckBytes = ra::data::MemSizeBytes(nSize);

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    auto pIter = m_mCodeNotes.lower_bound(nAddress);
    if (pIter != m_mCodeNotes.end())
    {
        if (nAddress == pIter->first)
        {
            // exact match
            return BuildCodeNoteSized(nAddress, nCheckBytes, pIter->first, pIter->second);
        }
        else if (nAddress + nCheckBytes - 1 >= pIter->first)
        {
            // requested number of bytes will overlap with the next item
            return BuildCodeNoteSized(nAddress, nCheckBytes, pIter->first, pIter->second);
        }
    }

    // did not match/overlap with the found item, check the item before the found item
    if (pIter != m_mCodeNotes.begin())
    {
        --pIter;
        if (pIter->first + pIter->second.Bytes - 1 >= nAddress)
        {
            // previous item overlaps with requested address
            return BuildCodeNoteSized(nAddress, nCheckBytes, pIter->first, pIter->second);
        }
    }

    // no code note on the address, check for pointers
    if (m_bHasPointers)
    {
        const auto nLastAddress = nAddress + nCheckBytes - 1;
        for (const auto& pIter2 : m_mCodeNotes)
        {
            if (!pIter2.second.PointerData)
                continue;

            if (nLastAddress >= pIter2.second.PointerData->PointerValue)
            {
                const auto nOffset = ra::to_signed(nAddress - pIter2.second.PointerData->PointerValue);
                const auto nLastOffset = nOffset + ra::to_signed(nCheckBytes) - 1;
                for (const auto& pNote : pIter2.second.PointerData->OffsetNotes)
                {
                    if (pNote.Offset == nOffset)
                    {
                        // exact match
                        return BuildCodeNoteSized(nAddress, nCheckBytes, nAddress, pNote) + L" [indirect]";
                    }
                    else if (pNote.Offset + ra::to_signed(pNote.Bytes) - 1 >= nOffset && pNote.Offset <= nLastOffset)
                    {
                        // overlap
                        return BuildCodeNoteSized(nAddress, nCheckBytes, pIter2.second.PointerData->PointerValue + pNote.Offset, pNote) + L" [indirect]";
                    }
                }
            }
        }
    }

    return std::wstring();
}

const std::wstring* CodeNotesModel::FindCodeNote(ra::ByteAddress nAddress, _Inout_ std::string& sAuthor) const
{
    const auto pIter = m_mCodeNotes.find(nAddress);
    if (pIter != m_mCodeNotes.end())
    {
        sAuthor = pIter->second.Author;
        return &pIter->second.Note;
    }

    return nullptr;
}

void CodeNotesModel::SetCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote)
{
    std::string sOriginalAuthor;

    {
        std::unique_lock<std::mutex> lock(m_oMutex);

        if (m_bRefreshing)
        {
            m_mPendingCodeNotes[nAddress] = sNote;
            return;
        }

        const auto pIter = m_mCodeNotes.find(nAddress);
        if (pIter != m_mCodeNotes.end())
        {
            if (pIter->second.Note == sNote)
            {
                // the note at this address is unchanged
                return;
            }
        }
        else if (sNote.empty())
        {
            // the note at this address is unchanged
            return;
        }

        const auto pIter2 = m_mOriginalCodeNotes.find(nAddress);
        if (pIter2 == m_mOriginalCodeNotes.end())
        {
            // note wasn't previously modified
            if (pIter != m_mCodeNotes.end())
            {
                // capture the original value
                m_mOriginalCodeNotes.insert_or_assign(nAddress,
                    std::make_pair(pIter->second.Author, pIter->second.Note));
            }
            else
            {
                // add a dummy original value so it appears modified
                m_mOriginalCodeNotes.insert_or_assign(nAddress, std::make_pair("", L""));
            }
        }
        else if (pIter2->second.second == sNote)
        {
            // note restored to original value. assign the note back to the original author
            // and discard the modification tracker
            sOriginalAuthor = pIter2->second.first;

            m_mOriginalCodeNotes.erase(pIter2);

            if (sNote.empty())
            {
                // note didn't originally exist, don't keep a modification record if the
                // changes were discarded.
                m_mCodeNotes.erase(nAddress);
                OnCodeNoteChanged(nAddress, sNote);
                return;
            }
        }
    }

    if (!sOriginalAuthor.empty())
    {
        AddCodeNote(nAddress, sOriginalAuthor, sNote);
    }
    else
    {
        const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
        AddCodeNote(nAddress, pUserContext.GetDisplayName(), sNote);
    }
}

const CodeNotesModel::CodeNote* CodeNotesModel::FindCodeNoteInternal(ra::ByteAddress nAddress) const
{
    const auto pIter = m_mCodeNotes.find(nAddress);
    if (pIter != m_mCodeNotes.end())
        return &pIter->second;

    if (m_bHasPointers)
    {
        for (const auto& pCodeNote : m_mCodeNotes)
        {
            if (pCodeNote.second.PointerData == nullptr)
                continue;

            if (nAddress >= pCodeNote.second.PointerData->PointerValue &&
                nAddress < pCodeNote.second.PointerData->PointerValue + pCodeNote.second.PointerData->OffsetRange)
            {
                const auto nOffset = ra::to_signed(nAddress - pCodeNote.second.PointerData->PointerValue);
                for (const auto& pOffsetNote : pCodeNote.second.PointerData->OffsetNotes)
                {
                    if (pOffsetNote.Offset == nOffset)
                        return &pOffsetNote;
                }
            }
        }
    }

    return nullptr;
}

const std::wstring* CodeNotesModel::FindIndirectCodeNote(ra::ByteAddress nAddress, unsigned nOffset) const noexcept
{
    if (!m_bHasPointers)
        return nullptr;

    for (const auto& pCodeNote : m_mCodeNotes)
    {
        if (pCodeNote.second.PointerData == nullptr)
            continue;

        if (nAddress == pCodeNote.first)
        {
            for (const auto& pOffsetNote : pCodeNote.second.PointerData->OffsetNotes)
            {
                if (pOffsetNote.Offset == ra::to_signed(nOffset))
                    return &pOffsetNote.Note;
            }

            break;
        }
    }

    return nullptr;
}

ra::ByteAddress CodeNotesModel::GetIndirectSource(ra::ByteAddress nAddress) const noexcept
{
    if (m_bHasPointers)
    {
        for (const auto& pCodeNote : m_mCodeNotes)
        {
            if (pCodeNote.second.PointerData == nullptr)
                continue;

            if (nAddress >= pCodeNote.second.PointerData->PointerValue &&
                nAddress < pCodeNote.second.PointerData->PointerValue + pCodeNote.second.PointerData->OffsetRange)
            {
                const auto nOffset = ra::to_signed(nAddress - pCodeNote.second.PointerData->PointerValue);
                for (const auto& pOffsetNote : pCodeNote.second.PointerData->OffsetNotes)
                {
                    if (pOffsetNote.Offset == nOffset)
                        return pCodeNote.first;
                }
            }
        }
    }

    return 0xFFFFFFFF;
}

ra::ByteAddress CodeNotesModel::GetNextNoteAddress(ra::ByteAddress nAfterAddress, bool bIncludeDerived) const
{
    ra::ByteAddress nBestAddress = 0xFFFFFFFF;

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    const auto pIter = m_mCodeNotes.lower_bound(nAfterAddress + 1);
    if (pIter != m_mCodeNotes.end())
        nBestAddress = pIter->first;

    if (m_bHasPointers && bIncludeDerived)
    {
        for (const auto& pNote : m_mCodeNotes)
        {
            if (!pNote.second.PointerData)
                continue;

            if (pNote.second.PointerData->PointerValue > nBestAddress)
                continue;

            if (pNote.second.PointerData->PointerValue + pNote.second.PointerData->OffsetRange < nAfterAddress)
                continue;

            for (const auto& pOffset : pNote.second.PointerData->OffsetNotes)
            {
                const auto pOffsetAddress = pNote.second.PointerData->PointerValue + pOffset.Offset;
                if (pOffsetAddress > nAfterAddress)
                {
                    nBestAddress = std::min(nBestAddress, pOffsetAddress);
                    break;
                }
            }
        }
    }

    return nBestAddress;
}

ra::ByteAddress CodeNotesModel::GetPreviousNoteAddress(ra::ByteAddress nBeforeAddress, bool bIncludeDerived) const
{
    unsigned nBestAddress = 0xFFFFFFFF;

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    auto pIter = m_mCodeNotes.lower_bound(nBeforeAddress - 1);
    if (pIter != m_mCodeNotes.end() && pIter->first == nBeforeAddress - 1)
    {
        // exact match for 1 byte lower, return it.
        return pIter->first;
    }

    if (pIter != m_mCodeNotes.begin())
    {
        // found next lower item, claim it
        --pIter;
        nBestAddress = pIter->first;
    }

    if (m_bHasPointers && bIncludeDerived)
    {
        // scan pointed-at addresses to see if there's anything between the next lower item and nBeforeAddress
        for (const auto& pNote : m_mCodeNotes)
        {
            if (!pNote.second.PointerData)
                continue;

            if (pNote.second.PointerData->PointerValue > nBeforeAddress)
                continue;

            if (pNote.second.PointerData->PointerValue + pNote.second.PointerData->OffsetRange < nBestAddress)
                continue;

            for (const auto& pOffset : pNote.second.PointerData->OffsetNotes)
            {
                const auto pOffsetAddress = pNote.second.PointerData->PointerValue + pOffset.Offset;
                if (pOffsetAddress >= nBeforeAddress)
                    break;

                if (pOffsetAddress > nBestAddress || nBestAddress == 0xFFFFFFFF)
                    nBestAddress = pOffsetAddress;
            }
        }
    }

    return nBestAddress;
}

void CodeNotesModel::EnumerateCodeNotes(std::function<bool(ra::ByteAddress nAddress, const CodeNote& pCodeNote)> callback, bool bIncludeDerived) const
{
    if (!bIncludeDerived || !m_bHasPointers)
    {
        // no pointers, just iterate over the normal code notes
        for (const auto& pIter : m_mCodeNotes)
        {
            if (!callback(pIter.first, pIter.second))
                break;
        }

        return;
    }

    // create a hash from the current pointer addresses
    std::map<ra::ByteAddress, const CodeNote*> mNotes;
    for (const auto& pIter : m_mCodeNotes)
    {
        if (!pIter.second.PointerData)
            continue;

        for (const auto& pNote : pIter.second.PointerData->OffsetNotes)
            mNotes[pIter.second.PointerData->PointerValue + pNote.Offset] = &pNote;
    }

    // merge in the non-pointer notes
    for (const auto& pIter : m_mCodeNotes)
        mNotes[pIter.first] = &pIter.second;

    // and iterate the result
    for (const auto& pIter : mNotes)
    {
        if (!callback(pIter.first, *pIter.second))
            break;
    }
}

void CodeNotesModel::DoFrame()
{
    if (!m_bHasPointers)
        return;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();

    for (auto& pNote : m_mCodeNotes)
    {
        if (!pNote.second.PointerData)
            continue;

        const auto nNewAddress = ReadPointer(pEmulatorContext, pNote.first, pNote.second.MemSize);

        const auto nOldAddress = pNote.second.PointerData->PointerValue;
        if (nNewAddress == nOldAddress)
            continue;

        pNote.second.PointerData->PointerValue = nNewAddress;

        if (m_fCodeNoteChanged)
        {
            for (const auto& pOffset : pNote.second.PointerData->OffsetNotes)
                m_fCodeNoteChanged(nOldAddress + pOffset.Offset, L"");

            for (const auto& pOffset : pNote.second.PointerData->OffsetNotes)
                m_fCodeNoteChanged(nNewAddress + pOffset.Offset, pOffset.Note);
        }
    }
}

void CodeNotesModel::SetServerCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote)
{
    const auto pIter = m_mOriginalCodeNotes.find(nAddress);
    if (pIter != m_mOriginalCodeNotes.end())
    {
        // remove the original entry - we're setting a new original entry
        m_mOriginalCodeNotes.erase(pIter);
    }

    // if we're just committing the current value, we're done
    const auto pIter2 = m_mCodeNotes.find(nAddress);
    if (pIter2 != m_mCodeNotes.end() && pIter2->second.Note == sNote)
    {
        if (sNote.empty())
            m_mCodeNotes.erase(pIter2);

        if (m_mOriginalCodeNotes.empty())
            SetValue(ra::data::models::AssetModelBase::ChangesProperty, ra::etoi(ra::data::models::AssetChanges::None));
        OnCodeNoteChanged(nAddress, sNote);
        return;
    }

    // update the current value
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    AddCodeNote(nAddress, pUserContext.GetDisplayName(), sNote);
}

const std::wstring* CodeNotesModel::GetServerCodeNote(ra::ByteAddress nAddress) const
{
    const auto pIter = m_mOriginalCodeNotes.find(nAddress);
    if (pIter != m_mOriginalCodeNotes.end())
        return &pIter->second.second;

    return nullptr;
}

const std::string* CodeNotesModel::GetServerCodeNoteAuthor(ra::ByteAddress nAddress) const
{
    const auto pIter = m_mOriginalCodeNotes.find(nAddress);
    if (pIter != m_mOriginalCodeNotes.end())
        return &pIter->second.first;

    return nullptr;
}

void CodeNotesModel::Serialize(ra::services::TextWriter& pWriter) const
{
    bool first = true;

    for (const auto& pIter : m_mOriginalCodeNotes)
    {
        // "N0" of first note will be written by GameAssets
        if (first)
        {
            first = false;
            pWriter.Write(":");
        }
        else
        {
            pWriter.WriteLine();
            pWriter.Write("N0:");
        }

        pWriter.Write(ra::ByteAddressToString(pIter.first));

        const auto pNote = m_mCodeNotes.find(pIter.first);
        if (pNote != m_mCodeNotes.end())
            WriteQuoted(pWriter, pNote->second.Note);
        else
            WriteQuoted(pWriter, "");
    }
}

bool CodeNotesModel::Deserialize(ra::Tokenizer& pTokenizer)
{
    const auto sAddress = pTokenizer.ReadTo(':');
    pTokenizer.Consume(':');
    const auto nAddress = ra::ByteAddressFromString(sAddress);

    std::wstring sNote;
    if (!ReadQuoted(pTokenizer, sNote))
        return false;

    ra::NormalizeLineEndings(sNote);
    SetCodeNote(nAddress, sNote);
    return true;
}

} // namespace models
} // namespace data
} // namespace ra
