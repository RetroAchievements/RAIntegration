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
    m_vCodeNotes.clear();

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

static int CompareNoteAddresses(const ra::data::models::CodeNoteModel& left, const ra::data::models::CodeNoteModel& right) noexcept
{
    return left.GetAddress() < right.GetAddress();
}

void CodeNotesModel::AddCodeNote(ra::ByteAddress nAddress, const std::string& sAuthor, const std::wstring& sNote)
{
    CodeNoteModel note;
    note.SetAuthor(sAuthor);
    note.SetNote(sNote);
    note.SetAddress(nAddress);

    const bool bIsPointer = note.IsPointer();
    if (bIsPointer)
    {
        m_bHasPointers = true;

        // capture the initial value of the pointer
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
        note.UpdateRawPointerValue(nAddress, pEmulatorContext, nullptr);
    }

    {
        std::unique_lock<std::mutex> lock(m_oMutex);
        auto iter = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), note, CompareNoteAddresses);
        if (iter != m_vCodeNotes.end() && iter->GetAddress() == note.GetAddress())
            *iter = std::move(note);
        else
            m_vCodeNotes.insert(iter, std::move(note));
    }

    OnCodeNoteChanged(nAddress, sNote);

    // also raise CodeNoteChanged events for each indirect child note
    if (bIsPointer && m_fCodeNoteChanged)
    {
        const auto* pCodeNote = FindCodeNoteModel(nAddress);
        if (pCodeNote != nullptr)
        {
            pCodeNote->EnumeratePointerNotes([this](ra::ByteAddress nAddress, const CodeNoteModel& pOffsetNote) {
                m_fCodeNoteChanged(nAddress, pOffsetNote.GetNote());
                return true;
            });
        }
    }
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
    CodeNoteModel searchNote;
    searchNote.SetAddress(nAddress);
    auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);

    // exact match, return it
    if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == nAddress)
        return nAddress;

    // lower_bound returns the first item _after_ the search value. scan all items before
    // the found item to see if any of them contain the target address. have to scan
    // all items because a singular note may exist within a range.
    if (pCodeNote != m_vCodeNotes.begin())
    {
        do
        {
            --pCodeNote;

            if (pCodeNote->GetBytes() > 1 && pCodeNote->GetBytes() + pCodeNote->GetAddress() > nAddress)
                return pCodeNote->GetAddress();

        } while (pCodeNote != m_vCodeNotes.begin());
    }

    // also check for derived code notes
    if (m_bHasPointers)
    {
        for (const auto& pNote : m_vCodeNotes)
        {
            const auto pair = pNote.GetPointerNoteAtAddress(nAddress);
            if (pair.second != nullptr)
                return pair.first;
        }
    }

    return 0xFFFFFFFF;
}

std::wstring CodeNotesModel::BuildCodeNoteSized(ra::ByteAddress nAddress,
    unsigned nCheckBytes, ra::ByteAddress nNoteAddress, const CodeNoteModel& pNote)
{
    // extract the first line
    std::wstring sNote = pNote.GetNote();
    const auto iNewLine = sNote.find('\n');
    if (iNewLine != std::string::npos)
    {
        sNote.resize(iNewLine);

        if (sNote.back() == '\r')
            sNote.pop_back();
    }

    const unsigned int nNoteSize = pNote.GetBytes();
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
    CodeNoteModel searchNote;
    searchNote.SetAddress(nAddress);
    auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
    if (pCodeNote != m_vCodeNotes.end())
    {
        if (nAddress == pCodeNote->GetAddress())
        {
            // exact match
            return BuildCodeNoteSized(nAddress, nCheckBytes, pCodeNote->GetAddress(), *pCodeNote);
        }
        else if (nAddress + nCheckBytes - 1 >= pCodeNote->GetAddress())
        {
            // requested number of bytes will overlap with the next item
            return BuildCodeNoteSized(nAddress, nCheckBytes, pCodeNote->GetAddress(), *pCodeNote);
        }
    }

    // did not match/overlap with the found item, check the item before the found item
    if (pCodeNote != m_vCodeNotes.begin())
    {
        --pCodeNote;
        if (pCodeNote->GetAddress() + pCodeNote->GetBytes() - 1 >= nAddress)
        {
            // previous item overlaps with requested address
            return BuildCodeNoteSized(nAddress, nCheckBytes, pCodeNote->GetAddress(), *pCodeNote);
        }
    }

    // no code note on the address, check for pointers
    if (m_bHasPointers)
    {
        for (const auto& pCodeNote2 : m_vCodeNotes)
        {
            const auto pair = pCodeNote2.GetPointerNoteAtAddress(nAddress);
            if (pair.second != nullptr)
                return BuildCodeNoteSized(nAddress, nCheckBytes, pair.first, *pair.second) + L" [indirect]";
        }

        const auto nLastAddress = nAddress + nCheckBytes - 1;
        for (const auto& pCodeNote2 : m_vCodeNotes)
        {
            const auto pair = pCodeNote2.GetPointerNoteAtAddress(nLastAddress);
            if (pair.second != nullptr)
                return BuildCodeNoteSized(nAddress, nCheckBytes, pair.first, *pair.second) + L" [indirect]";
        }
    }

    return std::wstring();
}

const std::wstring* CodeNotesModel::FindCodeNote(ra::ByteAddress nAddress, _Inout_ std::string& sAuthor) const
{
    CodeNoteModel searchNote;
    searchNote.SetAddress(nAddress);
    const auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
    if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == nAddress)
    {
        sAuthor = pCodeNote->GetAuthor();
        return &pCodeNote->GetNote();
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

        CodeNoteModel searchNote;
        searchNote.SetAddress(nAddress);
        const auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
        if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == nAddress)
        {
            if (pCodeNote->GetNote() == sNote)
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
            if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == nAddress)
            {
                // capture the original value
                m_mOriginalCodeNotes.insert_or_assign(nAddress,
                    std::make_pair(pCodeNote->GetAuthor(), pCodeNote->GetNote()));
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
                m_vCodeNotes.erase(pCodeNote);
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

const CodeNoteModel* CodeNotesModel::FindCodeNoteModel(ra::ByteAddress nAddress) const
{
    CodeNoteModel searchNote;
    searchNote.SetAddress(nAddress);

    const auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
    if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == nAddress)
        return &(*pCodeNote);

    if (m_bHasPointers)
        return FindIndirectCodeNoteInternal(nAddress).second;

    return nullptr;
}

std::pair<ra::ByteAddress, const CodeNoteModel*>
    CodeNotesModel::FindIndirectCodeNoteInternal(ra::ByteAddress nAddress) const
{
    for (const auto& pCodeNote : m_vCodeNotes)
    {
        auto pair= pCodeNote.GetPointerNoteAtAddress(nAddress);
        if (pair.second != nullptr && pair.first == nAddress) // only match start of note
            return {pCodeNote.GetAddress(), pair.second};
    }

    return {0, nullptr};
}

ra::ByteAddress CodeNotesModel::GetIndirectSource(ra::ByteAddress nAddress) const
{
    if (m_bHasPointers)
    {
        const auto pCodeNote = FindIndirectCodeNoteInternal(nAddress);
        if (pCodeNote.second != nullptr)
            return pCodeNote.first;
    }

    return 0xFFFFFFFF;
}

ra::ByteAddress CodeNotesModel::GetNextNoteAddress(ra::ByteAddress nAfterAddress, bool bIncludeDerived) const
{
    ra::ByteAddress nBestAddress = 0xFFFFFFFF;

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    CodeNoteModel searchNote;
    searchNote.SetAddress(nAfterAddress + 1);
    const auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
    if (pCodeNote != m_vCodeNotes.end())
        nBestAddress = pCodeNote->GetAddress();

    if (m_bHasPointers && bIncludeDerived)
    {
        ra::ByteAddress nNextAddress = 0U;
        for (const auto& pNote : m_vCodeNotes)
        {
            if (pNote.GetNextAddress(nAfterAddress, nNextAddress))
                nBestAddress = std::min(nBestAddress, nNextAddress);
        }
    }

    return nBestAddress;
}

ra::ByteAddress CodeNotesModel::GetPreviousNoteAddress(ra::ByteAddress nBeforeAddress, bool bIncludeDerived) const
{
    unsigned nBestAddress = 0xFFFFFFFF;

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    CodeNoteModel searchNote;
    searchNote.SetAddress(nBeforeAddress - 1);
    auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
    if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == nBeforeAddress - 1)
    {
        // exact match for 1 byte lower, return it.
        return pCodeNote->GetAddress();
    }

    if (pCodeNote != m_vCodeNotes.begin())
    {
        // found next lower item, claim it
        --pCodeNote;
        nBestAddress = pCodeNote->GetAddress();
    }

    if (m_bHasPointers && bIncludeDerived)
    {
        ra::ByteAddress nPreviousAddress = 0U;

        // scan pointed-at addresses to see if there's anything between the next lower item and nBeforeAddress
        for (const auto& pNote : m_vCodeNotes)
        {
            if (pNote.GetPreviousAddress(nBeforeAddress, nPreviousAddress))
                nBestAddress = std::max(nBestAddress, nPreviousAddress);
        }
    }

    return nBestAddress;
}

void CodeNotesModel::EnumerateCodeNotes(std::function<bool(ra::ByteAddress nAddress, const CodeNoteModel& pCodeNote)> callback, bool bIncludeDerived) const
{
    if (!bIncludeDerived || !m_bHasPointers)
    {
        // no pointers, just iterate over the normal code notes
        for (const auto& pCodeNote : m_vCodeNotes)
        {
            if (!callback(pCodeNote.GetAddress(), pCodeNote))
                break;
        }

        return;
    }

    // create a hash from the current pointer addresses
    std::map<ra::ByteAddress, const CodeNoteModel*> mNotes;
    for (const auto& pCodeNote : m_vCodeNotes)
    {
        if (!pCodeNote.IsPointer())
            continue;

        pCodeNote.EnumeratePointerNotes(
            [&mNotes](ra::ByteAddress nAddress, const CodeNoteModel& pNote) {
                mNotes[nAddress] = &pNote;
                return true;
            });
    }

    // merge in the non-pointer notes
    for (const auto& pCodeNote : m_vCodeNotes)
        mNotes[pCodeNote.GetAddress()] = &pCodeNote;

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

    for (auto& pCodeNote : m_vCodeNotes)
    {
        if (pCodeNote.IsPointer())
        {
            pCodeNote.UpdateRawPointerValue(pCodeNote.GetAddress(), pEmulatorContext,
                [this](ra::ByteAddress nOldAddress, ra::ByteAddress nNewAddress, const CodeNoteModel& pOffsetNote) {
                    m_fCodeNoteChanged(nOldAddress, L"");
                    m_fCodeNoteChanged(nNewAddress, pOffsetNote.GetNote());
                });
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
    CodeNoteModel searchNote;
    searchNote.SetAddress(nAddress);
    const auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
    if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == nAddress && pCodeNote->GetNote() == sNote)
    {
        if (sNote.empty())
            m_vCodeNotes.erase(pCodeNote);

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

        CodeNoteModel searchNote;
        searchNote.SetAddress(pIter.first);
        const auto pCodeNote = std::lower_bound(m_vCodeNotes.begin(), m_vCodeNotes.end(), searchNote, CompareNoteAddresses);
        if (pCodeNote != m_vCodeNotes.end() && pCodeNote->GetAddress() == pIter.first)
            WriteQuoted(pWriter, pCodeNote->GetNote());
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
