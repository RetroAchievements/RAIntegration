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

std::wstring CodeNotesModel::FindCodeNote(ra::ByteAddress nAddress, MemSize nSize) const
{
    const unsigned int nCheckSize = ra::data::MemSizeBytes(nSize);

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    auto pIter = m_mCodeNotes.lower_bound(nAddress);
    if (pIter != m_mCodeNotes.end())
    {
        const ra::ByteAddress nNoteAddress = pIter->first;
        const unsigned int nNoteSize = pIter->second.Bytes;
        std::wstring sNote = pIter->second.Note;

        const auto iNewLine = sNote.find('\n');
        if (iNewLine != std::string::npos)
        {
            sNote.resize(iNewLine);

            if (sNote.back() == '\r')
                sNote.pop_back();
        }

        // exact match
        if (nAddress == nNoteAddress && nNoteSize == nCheckSize)
            return sNote;

        // check for overlap
        if (nAddress + nCheckSize - 1 >= nNoteAddress)
        {
            if (nCheckSize == 1)
                sNote.append(ra::StringPrintf(L" [%d/%d]", nNoteAddress - nAddress + 1, nNoteSize));
            else
                sNote.append(L" [partial]");

            return sNote;
        }
    }

    // check previous note for overlap
    if (pIter != m_mCodeNotes.begin())
    {
        --pIter;

        const ra::ByteAddress nNoteAddress = pIter->first;
        const unsigned int nNoteSize = pIter->second.Bytes;
        if (nNoteAddress + nNoteSize - 1 >= nAddress)
        {
            std::wstring sNote = pIter->second.Note;

            const auto iNewLine = sNote.find('\n');
            if (iNewLine != std::string::npos)
            {
                sNote.resize(iNewLine);

                if (sNote.back() == '\r')
                    sNote.pop_back();
            }

            if (nCheckSize == 1)
                sNote.append(ra::StringPrintf(L" [%d/%d]", nAddress - nNoteAddress + 1, nNoteSize));
            else
                sNote.append(L" [partial]");

            return sNote;
        }
    }

    return std::wstring();
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

} // namespace models
} // namespace data
} // namespace ra
