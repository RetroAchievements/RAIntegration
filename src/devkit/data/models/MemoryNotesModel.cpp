#include "MemoryNotesModel.hh"

#include "context/IEmulatorMemoryContext.hh"
#include "context/IRcClient.hh"
#include "context/UserContext.hh"

#include "data/models/ArrayMemoryNoteModel.hh"
#include "data/models/AuthoredMemoryNoteModel.hh"
#include "data/models/PointerMemoryNoteModel.hh"
#include "data/models/StructuredMemoryNoteModel.hh"

#include "services/ServiceLocator.hh"
#include "services/IMessageDispatcher.hh"

#include "util/Strings.hh"

#include <rcheevos/include/rc_api_editor.h>

namespace ra {
namespace data {
namespace models {

MemoryNotesModel::MemoryNotesModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::MemoryNotes));
    GSL_SUPPRESS_F6 SetName(L"Memory Notes");
}

void MemoryNotesModel::Refresh(unsigned int nGameId,
    MemoryNoteChangedFunction fMemoryNoteChanged, MemoryNoteMovedFunction fMemoryNoteMoved,
    std::function<void()> callback)
{
    m_vMemoryNotes.clear();
    m_bHasStructuredData = false;

    if (nGameId == 0)
    {
        m_fMemoryNoteChanged = nullptr;
        m_fMemoryNoteMoved = nullptr;
        callback();
        return;
    }

    m_fMemoryNoteChanged = fMemoryNoteChanged;
    m_fMemoryNoteMoved = fMemoryNoteMoved;

    if (callback == nullptr) // unit test workaround to avoid server call
        return;

    {
        std::unique_lock<std::mutex> lock(m_oMutex);
        m_bRefreshing = true;
    }

    const auto& pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>();
    rc_api_fetch_code_notes_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));
    api_params.game_id = nGameId;

    rc_api_request_t request;
    const auto nResult = rc_api_init_fetch_code_notes_request_hosted(&request, &api_params, pClient.GetHost());
    Expects(nResult == RC_OK);

    pClient.DispatchRequest(request, [this, callback](const rc_api_server_response_t& api_response, void*) {
        rc_api_fetch_code_notes_response_t response;
        const auto nResult = rc_api_process_fetch_code_notes_server_response(&response, &api_response);
        if (nResult != RC_OK)
        {
            ra::services::ServiceLocator::Get<ra::services::IMessageDispatcher>()
                .ReportErrorMessage(L"Failed to download memory notes",
                    ra::context::IRcClient::GetErrorMessage(nResult, response.response));
        }
        else
        {
            const auto* pNote = response.notes;
            const auto* pStop = pNote + response.num_notes;
            for (; pNote < pStop; ++pNote)
            {
                auto sNote = ra::util::String::Widen(pNote->note);
                ra::util::String::NormalizeLineEndings(sNote);

                {
                    std::unique_lock<std::mutex> lock(m_oMutex);
                    const auto pIter = m_mOriginalNotes.find(pNote->address);
                    if (pIter != m_mOriginalNotes.end())
                    {
                        pIter->second.first = pNote->author;
                        pIter->second.second = sNote;
                        continue;
                    }
                }

                AddMemoryNote(pNote->address, pNote->author, sNote);
            }
        }

        rc_api_destroy_fetch_code_notes_response(&response);

        std::map<ra::data::ByteAddress, std::wstring> mPendingNotes;
        {
            std::unique_lock<std::mutex> lock(m_oMutex);
            mPendingNotes.swap(m_mPendingNotes);
            m_bRefreshing = false;
        }

        for (const auto& pNote : mPendingNotes)
            SetNote(pNote.first, pNote.second);

        for (const auto& pNote : m_vMemoryNotes)
        {
            const auto* pStructuredData = dynamic_cast<const StructuredMemoryNoteModel*>(pNote.get());
            if (pStructuredData)
            {
                m_bHasStructuredData = true;
                break;
            }
        }

        callback();
    }, nullptr);
}

GSL_SUPPRESS_R30 // left has to be a const ref to the unique_ptr because the function is used in lower_bound
GSL_SUPPRESS_R32 // left has to be a const ref to the unique_ptr because the function is used in lower_bound
static int CompareNoteAddresses(const std::unique_ptr<MemoryNoteModel>& left,
                                ra::data::ByteAddress nAddress) noexcept
{
    return left->GetAddress() < nAddress;
}

void MemoryNotesModel::AddMemoryNote(ra::data::ByteAddress nAddress, const std::string& sAuthor, const std::wstring& sNote)
{
    std::unique_ptr<MemoryNoteModel> note = MemoryNoteModel::Parse(sNote);
    note->SetAddress(nAddress);

    auto* pAuthoredNote = dynamic_cast<AuthoredMemoryNoteModel*>(note.get());
    if (pAuthoredNote != nullptr)
        pAuthoredNote->SetAuthor(sAuthor);

    auto* pStructuredNote = dynamic_cast<StructuredMemoryNoteModel*>(note.get());
    if (pStructuredNote)
    {
        const auto* pArrayNote = dynamic_cast<ArrayMemoryNoteModel*>(note.get());
        if (pArrayNote)
        {
            const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
            pStructuredNote->UpdateBaseAddress(pStructuredNote->GetAddress(), pMemoryContext, nullptr);
        }

        if (!m_bRefreshing)
            m_bHasStructuredData = true;
    }

    {
        std::unique_lock<std::mutex> lock(m_oMutex);
        auto iter = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nAddress, CompareNoteAddresses);
        if (iter != m_vMemoryNotes.end() && (*iter)->GetAddress() == note->GetAddress())
            iter->swap(note);
        else
            m_vMemoryNotes.insert(iter, std::move(note));
    }

    OnMemoryNoteChanged(nAddress, sNote);

    // MemoryNoteChanged events for indirect child notes will be raised by first call to DoFrame
}

void MemoryNotesModel::OnMemoryNoteChanged(ra::data::ByteAddress nAddress, const std::wstring& sNewNote)
{
    SetValue(ra::data::models::AssetModelBase::ChangesProperty,
             m_mOriginalNotes.empty() ?
                 ra::etoi(ra::data::models::AssetChanges::None) :
                 ra::etoi(ra::data::models::AssetChanges::Unpublished));

    if (m_fMemoryNoteChanged != nullptr)
        m_fMemoryNoteChanged(nAddress, sNewNote);
}

MemoryNoteModel::Reference MemoryNotesModel::FindNoteContaining(ra::data::ByteAddress nAddress) const
{
    std::vector<MemoryNoteModel::Reference> vChain;
    if (GetChainTo(vChain, nAddress))
        return vChain.back();

    return { nullptr, 0, 0 };
}

std::wstring MemoryNotesModel::BuildNoteForAddress(ra::data::ByteAddress nAddress,
    unsigned nCheckBytes, ra::data::ByteAddress nNoteAddress, const MemoryNoteModel& pNote)
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
        sNote.append(ra::util::String::Printf(L" [%d/%d]", nAddress - nNoteAddress + 1, nNoteSize));
    }
    else
    {
        // searching on something other than bytes, just indicate that it's overlapping with the note
        sNote.append(L" [partial]");
    }

    return sNote;
}

std::wstring MemoryNotesModel::FindNote(ra::data::ByteAddress nAddress, Memory::Size nSize) const
{
    const unsigned int nCheckBytes = ra::data::Memory::SizeBytes(nSize);

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    auto pIter = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nAddress, CompareNoteAddresses);
    if (pIter != m_vMemoryNotes.end())
    {
        const auto* pMemoryNote = pIter->get();
        Expects(pMemoryNote != nullptr);
        if (nAddress == pMemoryNote->GetAddress())
        {
            // exact match
            return BuildNoteForAddress(nAddress, nCheckBytes, pMemoryNote->GetAddress(), *pMemoryNote);
        }
        else if (nAddress + nCheckBytes - 1 >= pMemoryNote->GetAddress())
        {
            // requested number of bytes will overlap with the next item
            return BuildNoteForAddress(nAddress, nCheckBytes, pMemoryNote->GetAddress(), *pMemoryNote);
        }
    }

    // did not match/overlap with the found item, check the item before the found item
    if (pIter != m_vMemoryNotes.begin())
    {
        --pIter;

        const auto* pMemoryNote = pIter->get();
        if (pMemoryNote && pMemoryNote->GetAddress() + pMemoryNote->GetBytes() - 1 >= nAddress)
        {
            // previous item overlaps with requested address
            return BuildNoteForAddress(nAddress, nCheckBytes, pMemoryNote->GetAddress(), *pMemoryNote);
        }
    }

    // no memory note on the address, check for pointers
    if (m_bHasStructuredData)
    {
        for (const auto& pMemoryNote2 : m_vMemoryNotes)
        {
            const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pMemoryNote2.get());
            if (pStructuredNote)
            {
                std::vector<MemoryNoteModel::Reference> vMatches;
                if (pStructuredNote->GetChainTo(vMatches, nAddress))
                    return BuildNoteForAddress(nAddress, nCheckBytes, vMatches.back().nAddress, *vMatches.back().pMemoryNote) + L" [indirect]";
            }
        }

        if (nCheckBytes > 1)
        {
            const auto nLastAddress = nAddress + nCheckBytes - 1;
            for (const auto& pMemoryNote2 : m_vMemoryNotes)
            {
                const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pMemoryNote2.get());
                if (pStructuredNote)
                {
                    std::vector<MemoryNoteModel::Reference> vMatches;
                    if (pStructuredNote->GetChainTo(vMatches, nLastAddress))
                        return BuildNoteForAddress(nAddress, nCheckBytes, vMatches.back().nAddress, *vMatches.back().pMemoryNote) + L" [indirect]";
                }
            }
        }
    }

    return std::wstring();
}

void MemoryNotesModel::SetNote(ra::data::ByteAddress nAddress, const std::wstring& sNote)
{
    std::string sOriginalAuthor;

    {
        std::unique_lock<std::mutex> lock(m_oMutex);

        if (m_bRefreshing)
        {
            m_mPendingNotes[nAddress] = sNote;
            return;
        }

        const auto pIter = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nAddress, CompareNoteAddresses);
        if (pIter != m_vMemoryNotes.end() && (*pIter)->GetAddress() == nAddress)
        {
            if ((*pIter)->GetNote() == sNote)
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

        const auto pIter2 = m_mOriginalNotes.find(nAddress);
        if (pIter2 == m_mOriginalNotes.end())
        {
            // note wasn't previously modified
            if (pIter != m_vMemoryNotes.end() && (*pIter)->GetAddress() == nAddress)
            {
                // capture the original value
                const auto* pMemoryNote = pIter->get();
                Expects(pMemoryNote != nullptr);
                const auto* pAuthorNote = dynamic_cast<const AuthoredMemoryNoteModel*>(pMemoryNote);
                Expects(pAuthorNote != nullptr);
                m_mOriginalNotes.insert_or_assign(nAddress,
                    std::make_pair(pAuthorNote->GetAuthor(), pMemoryNote->GetNote()));
            }
            else
            {
                // add a dummy original value so it appears modified
                m_mOriginalNotes.insert_or_assign(nAddress, std::make_pair("", L""));
            }
        }
        else if (pIter2->second.second == sNote)
        {
            // note restored to original value. assign the note back to the original author
            // and discard the modification tracker
            sOriginalAuthor = pIter2->second.first;

            m_mOriginalNotes.erase(pIter2);

            if (sNote.empty())
            {
                // note didn't originally exist, don't keep a modification record if the
                // changes were discarded.
                m_vMemoryNotes.erase(pIter);
                OnMemoryNoteChanged(nAddress, sNote);
                return;
            }
        }
    }

    if (!sOriginalAuthor.empty())
    {
        AddMemoryNote(nAddress, sOriginalAuthor, sNote);
    }
    else
    {
        const auto& pUserContext = ra::services::ServiceLocator::Get<ra::context::UserContext>();
        AddMemoryNote(nAddress, pUserContext.GetDisplayName(), sNote);
    }
}

const MemoryNoteModel* MemoryNotesModel::FindNote(ra::data::ByteAddress nAddress, bool bIncludeDerived) const
{
    const auto pIter = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nAddress, CompareNoteAddresses);
    if (pIter != m_vMemoryNotes.end() && (*pIter)->GetAddress() == nAddress)
        return pIter->get();

    if (m_bHasStructuredData && bIncludeDerived)
        return FindIndirectMemoryNoteInternal(nAddress).pMemoryNote;

    return nullptr;
}

MemoryNoteModel::Reference MemoryNotesModel::FindIndirectMemoryNoteInternal(ra::data::ByteAddress nAddress) const
{
    for (const auto& pMemoryNote : m_vMemoryNotes)
    {
        const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pMemoryNote.get());
        if (pStructuredNote)
        {
            std::vector<MemoryNoteModel::Reference> vMatches;
            if (pStructuredNote->GetChainTo(vMatches, nAddress) && vMatches.back().nAddress == nAddress)
                return vMatches.back();
        }
    }

    return {};
}

bool MemoryNotesModel::GetChainTo(std::vector<MemoryNoteModel::Reference>& vChain, ra::data::ByteAddress nAddress) const
{
    auto pIter = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nAddress, CompareNoteAddresses);

    // exact match, return it
    if (pIter != m_vMemoryNotes.end() && (*pIter)->GetAddress() == nAddress)
    {
        vChain.emplace_back(pIter->get(), nAddress, 0);
        return true;
    }

    // lower_bound returns the first item _after_ the search value. scan all items before
    // the found item to see if any of them contain the target address. have to scan
    // all items because a singular note may exist within a range.
    if (pIter != m_vMemoryNotes.begin())
    {
        do
        {
            --pIter;
            const auto* pMemoryNote = pIter->get();

            if (pMemoryNote && pMemoryNote->GetType() != MemoryNoteType::Pointer)
            {
                if (pMemoryNote->GetChainTo(vChain, nAddress))
                    return true;
            }
        } while (pIter != m_vMemoryNotes.begin());
    }

    // also check for derived memory notes
    if (m_bHasStructuredData)
    {
        for (const auto& pMemoryNote : m_vMemoryNotes)
        {
            const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pMemoryNote.get());
            if (pStructuredNote && pStructuredNote->GetChainTo(vChain, nAddress))
                return true;
        }
    }

    return false;
}

ra::data::ByteAddress MemoryNotesModel::GetNextNoteAddress(ra::data::ByteAddress nAfterAddress, bool bIncludeDerived) const
{
    ra::data::ByteAddress nBestAddress = 0xFFFFFFFF;

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    const auto pIter = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nAfterAddress + 1, CompareNoteAddresses);
    if (pIter != m_vMemoryNotes.end())
        nBestAddress = (*pIter)->GetAddress();

    if (m_bHasStructuredData && bIncludeDerived)
    {
        ra::data::ByteAddress nNextAddress = 0U;
        for (const auto& pNote : m_vMemoryNotes)
        {
            if (pNote->GetNextAddress(nAfterAddress, nNextAddress))
                nBestAddress = std::min(nBestAddress, nNextAddress);
        }
    }

    return nBestAddress;
}

ra::data::ByteAddress MemoryNotesModel::GetPreviousNoteAddress(ra::data::ByteAddress nBeforeAddress, bool bIncludeDerived) const
{
    unsigned nBestAddress = 0xFFFFFFFF;

    // lower_bound will return the item if it's an exact match, or the *next* item otherwise
    auto pIter = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nBeforeAddress - 1, CompareNoteAddresses);
    if (pIter != m_vMemoryNotes.end() && (*pIter)->GetAddress() == nBeforeAddress - 1)
    {
        // exact match for 1 byte lower, return it.
        return (*pIter)->GetAddress();
    }

    if (pIter != m_vMemoryNotes.begin())
    {
        // found next lower item, claim it
        --pIter;
        nBestAddress = (*pIter)->GetAddress();
    }

    if (m_bHasStructuredData && bIncludeDerived)
    {
        ra::data::ByteAddress nPreviousAddress = 0U;

        // scan pointed-at addresses to see if there's anything between the next lower item and nBeforeAddress
        for (const auto& pNote : m_vMemoryNotes)
        {
            if (pNote->GetPreviousAddress(nBeforeAddress, nPreviousAddress))
                nBestAddress = std::max(nBestAddress, nPreviousAddress);
        }
    }

    return nBestAddress;
}

void MemoryNotesModel::EnumerateNotes(std::function<bool(const MemoryNoteModel::Reference& pMemoryNote)> callback, bool bIncludeDerived) const
{
    if (!bIncludeDerived || !m_bHasStructuredData)
    {
        // No pointers. Just iterate over the root memory notes.
        for (const auto& pMemoryNote : m_vMemoryNotes)
        {
            const MemoryNoteModel::Reference pReference(pMemoryNote.get(), pMemoryNote->GetAddress(), 0);
            if (!callback(pReference))
                break;
        }

        return;
    }

    // Create a hash from the current pointer addresses.
    std::map<ra::data::ByteAddress, MemoryNoteModel::Reference> mNotes;
    std::function<bool(const MemoryNoteModel::Reference&)> fCallback =
        [&mNotes, &fCallback](const MemoryNoteModel::Reference& pNote)
        {
            mNotes[pNote.nAddress] = pNote;

            // Recurse if appropriate.
            const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pNote.pMemoryNote);
            if (pStructuredNote)
                pStructuredNote->EnumerateOffsetNotes(fCallback, pNote.nAddress);

            return true;
        };

    for (const auto& pMemoryNote : m_vMemoryNotes)
    {
        const auto* pStructuredNote = dynamic_cast<const StructuredMemoryNoteModel*>(pMemoryNote.get());
        if (pStructuredNote)
            pStructuredNote->EnumerateOffsetNotes(fCallback);
    }

    // Merge in the non-pointer notes.
    for (const auto& pMemoryNote : m_vMemoryNotes)
        mNotes[pMemoryNote->GetAddress()] = { pMemoryNote.get(), pMemoryNote->GetAddress(), 0 };

    // And iterate the result.
    for (const auto& pIter : mNotes)
    {
        if (!callback(pIter.second))
            break;
    }
}

void MemoryNotesModel::DoFrame()
{
    if (!m_bHasStructuredData)
        return;

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();

    for (auto& pMemoryNote : m_vMemoryNotes)
    {
        auto* pPointerNote = dynamic_cast<PointerMemoryNoteModel*>(pMemoryNote.get());
        if (pPointerNote)
        {
            if (!m_fMemoryNoteMoved)
            {
                pPointerNote->UpdateBaseAddress(pMemoryNote->GetAddress(), pMemoryContext, nullptr);
            }
            else if (pPointerNote->HasRawPointerValue())
            {
                pPointerNote->UpdateBaseAddress(pMemoryNote->GetAddress(), pMemoryContext,
                    [this](ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const MemoryNoteModel& pOffsetNote) {
                        m_fMemoryNoteMoved(nOldAddress, nNewAddress, pOffsetNote.GetNote());
                    });
            }
            else
            {
                // pointer hasn't been read before, provide dummy previous address
                pPointerNote->UpdateBaseAddress(pMemoryNote->GetAddress(), pMemoryContext,
                    [this](ra::data::ByteAddress, ra::data::ByteAddress nNewAddress, const MemoryNoteModel& pOffsetNote) {
                        m_fMemoryNoteMoved(0xFFFFFFFF, nNewAddress, pOffsetNote.GetNote());
                    });
            }
        }
    }
}

void MemoryNotesModel::SetServerNote(ra::data::ByteAddress nAddress, const std::wstring& sNote)
{
    const auto pIter = m_mOriginalNotes.find(nAddress);
    if (pIter != m_mOriginalNotes.end())
    {
        // remove the original entry - we're setting a new original entry
        m_mOriginalNotes.erase(pIter);
    }

    // if we're just committing the current value, we're done
    const auto pIter2 = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), nAddress, CompareNoteAddresses);
    if (pIter2 != m_vMemoryNotes.end() && (*pIter2)->GetAddress() == nAddress && (*pIter2)->GetNote() == sNote)
    {
        if (sNote.empty())
            m_vMemoryNotes.erase(pIter2);

        if (m_mOriginalNotes.empty())
            SetValue(ra::data::models::AssetModelBase::ChangesProperty, ra::etoi(ra::data::models::AssetChanges::None));
        OnMemoryNoteChanged(nAddress, sNote);
        return;
    }

    // update the current value
    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::context::UserContext>();
    AddMemoryNote(nAddress, pUserContext.GetDisplayName(), sNote);
}

const std::wstring* MemoryNotesModel::GetServerNote(ra::data::ByteAddress nAddress) const
{
    const auto pIter = m_mOriginalNotes.find(nAddress);
    if (pIter != m_mOriginalNotes.end())
        return &pIter->second.second;

    return nullptr;
}

const std::string* MemoryNotesModel::GetServerNoteAuthor(ra::data::ByteAddress nAddress) const
{
    const auto pIter = m_mOriginalNotes.find(nAddress);
    if (pIter != m_mOriginalNotes.end())
        return &pIter->second.first;

    return nullptr;
}

void MemoryNotesModel::Serialize(ra::services::TextWriter& pWriter) const
{
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    bool first = true;

    for (const auto& pIter : m_mOriginalNotes)
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

        pWriter.Write(pMemoryContext.FormatAddress(pIter.first));

        const auto pIter2 = std::lower_bound(m_vMemoryNotes.begin(), m_vMemoryNotes.end(), pIter.first, CompareNoteAddresses);
        if (pIter2 != m_vMemoryNotes.end() && (*pIter2)->GetAddress() == pIter.first)
            WriteQuoted(pWriter, (*pIter2)->GetNote());
        else
            WriteQuoted(pWriter, "");
    }
}

bool MemoryNotesModel::Deserialize(ra::util::Tokenizer& pTokenizer)
{
    const auto sAddress = pTokenizer.ReadTo(':');
    pTokenizer.Consume(':');
    const auto nAddress = ra::data::Memory::ParseAddress(sAddress);

    std::wstring sNote;
    if (!ReadQuoted(pTokenizer, sNote))
        return false;

    ra::util::String::NormalizeLineEndings(sNote);
    SetNote(nAddress, sNote);
    return true;
}

} // namespace models
} // namespace data
} // namespace ra
