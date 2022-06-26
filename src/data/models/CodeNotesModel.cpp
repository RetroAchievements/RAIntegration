#include "CodeNotesModel.hh"

#include "RA_Defs.h"

#include "api\DeleteCodeNote.hh"
#include "api\FetchCodeNotes.hh"
#include "api\UpdateCodeNote.hh"

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
                AddCodeNote(pNote.Address, pNote.Author, pNote.Note);
        }

        callback();
    });
}

void CodeNotesModel::ExtractSize(CodeNote& pNote)
{
    bool bIsBytes = false;
    bool bBytesFromBits = false;

    // provide defaults in case no matches are found
    pNote.Bytes = 1;
    pNote.MemSize = MemSize::Unknown;

    // "Nbit" smallest possible note - and that's just the size annotation
    if (pNote.Note.length() < 4)
        return;

    const size_t nStop = pNote.Note.length() - 3;
    for (size_t nIndex = 1; nIndex <= nStop; ++nIndex)
    {
        wchar_t c = pNote.Note.at(nIndex);
        if (c != L'b' && c != L'B')
            continue;

        c = pNote.Note.at(nIndex + 1);
        if (c == L'i' || c == L'I')
        {
            // already found one bit reference, give it precedence
            if (bBytesFromBits)
                continue;

            c = pNote.Note.at(nIndex + 2);
            if (c != L't' && c != L'T')
                continue;

            // match "bits", but not "bite" even if there is a numeric prefix
            if (nIndex < nStop)
            {
                c = pNote.Note.at(nIndex + 3);
                if (isalpha(c) && c != L's' && c != L'S')
                    continue;
            }

            // found "bit"
            bIsBytes = false;
        }
        else if (c == L'y' || c == L'Y')
        {
            if (nIndex == nStop)
                continue;

            c = pNote.Note.at(nIndex + 2);
            if (c != L't' && c != L'T')
                continue;

            c = pNote.Note.at(nIndex + 3);
            if (c != L'e' && c != L'E')
                continue;

            // found "byte"
            bIsBytes = true;
        }
        else if (c == L'F' || c == 'f')
        {
            if (nIndex == 0)
                continue;

            c = pNote.Note.at(nIndex - 1);
            if (c != 'm' && c != 'M')
                continue;

            // found "mbf", check for "mbf32" or "mbf40"
            std::wstring sBits = pNote.Note.substr(nIndex + 2, 2);
            if (nIndex + 4 < pNote.Note.length() && std::isdigit(pNote.Note.at(nIndex + 4)))
                continue;

            if (sBits == L"32")
            {
                pNote.Bytes = 4;
                pNote.MemSize = MemSize::MBF32;
                return;
            }
            else if (sBits == L"40")
            {
                // should be MBF40, but the runtime doesn't support 40-bit values. because of the way MBF40 is stored,
                // it can be read as an MBF32 value with only the loss of the smallest 8-bits of precision.
                pNote.MemSize = MemSize::MBF32;
                pNote.Bytes = 5;
                return;
            }
            continue;
        }
        else
        {
            continue;
        }

        // ignore single space or hyphen preceding "bit" or "byte"
        size_t nScan = nIndex - 1;
        c = pNote.Note.at(nScan);
        if (c == ' ' || c == '-')
        {
            if (nScan == 0)
                continue;

            c = pNote.Note.at(--nScan);
        }

        // extract the number
        unsigned int nBytes = 0;
        int nMultiplier = 1;
        while (c <= L'9' && c >= L'0')
        {
            nBytes += (c - L'0') * nMultiplier;

            if (nScan == 0)
                break;

            nMultiplier *= 10;
            c = pNote.Note.at(--nScan);
        }

        // if a number was found, process it
        if (nBytes > 0)
        {
            // convert bits to bytes, rounding up
            pNote.Bytes = bIsBytes ? nBytes : (nBytes + 7) / 8;
            switch (pNote.Bytes)
            {
                case 1: pNote.MemSize = MemSize::EightBit; break;
                case 2: pNote.MemSize = MemSize::SixteenBit; break;
                case 3: pNote.MemSize = MemSize::TwentyFourBit; break;
                case 4: pNote.MemSize = MemSize::ThirtyTwoBit; break;
                default: pNote.MemSize = MemSize::Array; break;
            }

            // find the next word after "bit(s) or byte(s)"
            nScan = nIndex + 3;
            while (nScan < pNote.Note.length() && isalpha(pNote.Note.at(nScan)))
                nScan++;
            while (nScan < pNote.Note.length())
            {
                c = pNote.Note.at(nScan);
                if (c != ' ' && c != '-' && c != '(' && c != '[' && c != '<')
                    break;
                nScan++;
            }

            size_t nLength = 0;
            while (nScan + nLength < pNote.Note.length() && isalpha(pNote.Note.at(nScan + nLength)))
                nLength++;

            if ((nLength >= 2 && nLength <= 5) || nLength == 9)
            {
                std::wstring sWord = pNote.Note.substr(nScan, nLength);
                ra::StringMakeLowercase(sWord);

                if (sWord == L"be" || sWord == L"bigendian")
                {
                    switch (pNote.Bytes)
                    {
                        case 2: pNote.MemSize = MemSize::SixteenBitBigEndian; break;
                        case 3: pNote.MemSize = MemSize::TwentyFourBitBigEndian; break;
                        case 4: pNote.MemSize = MemSize::ThirtyTwoBitBigEndian; break;
                    }
                }
                else if (sWord == L"float")
                {
                    if (pNote.Bytes == 4)
                        pNote.MemSize = MemSize::Float;
                }
                else if (sWord == L"mbf")
                {
                    if (pNote.Bytes == 4 || pNote.Bytes == 5)
                        pNote.MemSize = MemSize::MBF32;
                }
            }

            // if "bytes" were found, we're done. if bits were found, it might be indicating
            // the size of individual elements. capture the bit value and keep searching.
            if (bIsBytes)
                return;

            bBytesFromBits = true;
        }
    }

    // did not find a bytes annotation, look for float
    if (pNote.Note.length() >= 5)
    {
        const size_t nStopFloat = pNote.Note.length() - 5;
        for (size_t nIndex = 0; nIndex <= nStopFloat; ++nIndex)
        {
            const wchar_t c = pNote.Note.at(nIndex);
            if (c == L'f' || c == L'F')
            {
                if (nIndex == 0 || !isalpha(pNote.Note.at(nIndex - 1)))
                {
                    std::wstring sWord = pNote.Note.substr(nIndex, 5);
                    ra::StringMakeLowercase(sWord);
                    if (sWord == L"float")
                    {
                        if (nIndex == nStopFloat || !isalpha(pNote.Note.at(nIndex + 5)))
                        {
                            pNote.Bytes = 4;
                            pNote.MemSize = MemSize::Float;
                            return;
                        }
                    }
                }
            }
        }
    }
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
                const auto sNextNote = sNote.substr(nIndex, nNextIndex - nIndex);

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
                    pointerNote.PointerData = std::move(pointerData);

                    // extract pointer size from first line
                    pointerNote.Note = sFirstLine;
                    ExtractSize(pointerNote);

                    // assign entire note to pointer note
                    pointerNote.Note = sNote;
                    m_mCodeNotes.insert_or_assign(nAddress, std::move(pointerNote));
                    m_bHasPointers = true;

                    OnCodeNoteChanged(nAddress, sNote);
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
    m_mCodeNotes.insert_or_assign(nAddress, std::move(note));

    OnCodeNoteChanged(nAddress, sNote);
}

void CodeNotesModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote)
{
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
                    else if (pNote.Offset + pNote.Bytes - 1 >= nOffset && pNote.Offset <= nLastOffset)
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

bool CodeNotesModel::SetCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote)
{
    if (m_nGameId == 0)
        return false;

    ra::api::UpdateCodeNote::Request request;
    request.GameId = m_nGameId;
    request.Address = nAddress;
    request.Note = sNote;

    do
    {
        auto response = request.Call();
        if (response.Succeeded())
        {
            const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
            AddCodeNote(nAddress, pUserContext.GetDisplayName(), sNote);
            return true;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessage;
        vmMessage.SetHeader(ra::StringPrintf(L"Error saving note for address %s", ra::ByteAddressToString(nAddress)));
        vmMessage.SetMessage(ra::Widen(response.ErrorMessage));
        vmMessage.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel);
        if (vmMessage.ShowModal() == ra::ui::DialogResult::Cancel)
            break;

    } while (true);

    return false;
}

bool CodeNotesModel::DeleteCodeNote(ra::ByteAddress nAddress)
{
    if (m_mCodeNotes.find(nAddress) == m_mCodeNotes.end())
        return true;

    if (m_nGameId == 0)
        return false;

    ra::api::DeleteCodeNote::Request request;
    request.GameId = m_nGameId;
    request.Address = nAddress;

    do
    {
        auto response = request.Call();
        if (response.Succeeded())
        {
            m_mCodeNotes.erase(nAddress);
            OnCodeNoteChanged(nAddress, L"");
            return true;
        }

        ra::ui::viewmodels::MessageBoxViewModel vmMessage;
        vmMessage.SetHeader(ra::StringPrintf(L"Error deleting note for address %s", ra::ByteAddressToString(nAddress)));
        vmMessage.SetMessage(ra::Widen(response.ErrorMessage));
        vmMessage.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel);
        if (vmMessage.ShowModal() == ra::ui::DialogResult::Cancel)
            break;

    } while (true);

    return false;
}

const CodeNotesModel::CodeNote* CodeNotesModel::FindCodeNoteInternal(ra::ByteAddress nAddress) const
{
    const auto pIter = m_mCodeNotes.find(nAddress);
    if (pIter != m_mCodeNotes.end())
        return &pIter->second;

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

    return nullptr;
}

void CodeNotesModel::EnumerateCodeNotes(std::function<bool(ra::ByteAddress nAddress, const CodeNote& pCodeNote)> callback) const
{
    if (!m_bHasPointers)
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

} // namespace models
} // namespace data
} // namespace ra
