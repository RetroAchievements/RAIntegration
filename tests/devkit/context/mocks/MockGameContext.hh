#ifndef RA_CONTEXT_MOCK_GAMECONTEXT_HH
#define RA_CONTEXT_MOCK_GAMECONTEXT_HH
#pragma once

#include "context/IGameContext.hh"

#include "data/models/LocalBadgesModel.hh"
#include "data/models/MemoryNotesModel.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace context {
namespace mocks {

class MockGameContext : public IGameContext
{
public:
    GSL_SUPPRESS_F6 MockGameContext() noexcept
        : m_Override(this)
    {
    }

    void SetGameId(uint32_t nId) noexcept { m_nGameId = m_nActiveGameId = nId; }
    void SetActiveGameId(uint32_t nId) noexcept { m_nActiveGameId = nId; }
    void SetGameTitle(const std::wstring& sTitle) noexcept { m_sGameTitle = sTitle; }

    ra::data::models::MemoryNotesModel& MemoryNotes() noexcept override { return m_oMemoryNotes; }
    const ra::data::models::MemoryNotesModel& MemoryNotes() const noexcept override { return m_oMemoryNotes; }

    ra::data::models::LocalBadgesModel& LocalBadges() noexcept override { return m_oLocalBadges; }
    const ra::data::models::LocalBadgesModel& LocalBadges() const noexcept override { return m_oLocalBadges; }

    bool SetNote(ra::data::ByteAddress nAddress, const std::wstring& sNote)
    {
        if (m_oMemoryNotes.NoteCount() == 0)
        {
            m_oMemoryNotes.Initialize(
                [this](ra::data::ByteAddress nAddress, const std::wstring& sNote) {
                    // a note with pointer notation is expected to keep track of where each
                    // pointed-at note exists. this normally occurs in DoFrame, but for
                    // the unit tests, force the update immediately after the note is updated
                    GSL_SUPPRESS_TYPE3
                        auto* pNote = const_cast<ra::data::models::MemoryNoteModel*>(m_oMemoryNotes.FindMemoryNoteModel(nAddress, false));
                    if (pNote && pNote->IsPointer())
                    {
                        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
                        pNote->UpdateRawPointerValue(nAddress, pMemoryContext, nullptr);
                    }

                    OnMemoryNoteChanged(nAddress, sNote);
                },
                [this](ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote) {
                    OnMemoryNoteMoved(nOldAddress, nNewAddress, sNote);
                });
        }

        m_oMemoryNotes.SetServerNote(nAddress, sNote);
        return true;
    }

private:
    void OnMemoryNoteChanged(ra::data::ByteAddress nAddress, const std::wstring& sNewNote)
    {
        if (m_vNotifyTargets.LockIfNotEmpty())
        {
            for (auto& target : m_vNotifyTargets.Targets())
                target.OnMemoryNoteChanged(nAddress, sNewNote);

            m_vNotifyTargets.Unlock();
        }
    }

    void OnMemoryNoteMoved(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote)
    {
        if (m_vNotifyTargets.LockIfNotEmpty())
        {
            for (auto& target : m_vNotifyTargets.Targets())
                target.OnMemoryNoteMoved(nOldAddress, nNewAddress, sNote);

            m_vNotifyTargets.Unlock();
        }
    }

    class MockMemoryNotesModel : public ra::data::models::MemoryNotesModel
    {
    public:
        void Initialize(MemoryNoteChangedFunction fMemoryNoteChanged, MemoryNoteMovedFunction fMemoryNoteMoved)
        {
            m_fMemoryNoteChanged = fMemoryNoteChanged;
            m_fMemoryNoteMoved = fMemoryNoteMoved;
        }
    };

    MockMemoryNotesModel m_oMemoryNotes;
    ra::data::models::LocalBadgesModel m_oLocalBadges;

    ra::services::ServiceLocator::ServiceOverride<ra::context::IGameContext> m_Override;
};

} // namespace mocks
} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_MOCK_CONSOLECONTEXT_HH
