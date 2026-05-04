#ifndef RA_CONTEXT_IGAMECONTEXT_HH
#define RA_CONTEXT_IGAMECONTEXT_HH
#pragma once

#include "data/Memory.hh"
#include "data/NotifyTargetSet.hh"

#include <string>

namespace ra {

namespace data {
namespace models {
    class MemoryNotesModel;
} // namespace models
} // namespace data

namespace context {

class IGameContext
{
public:
    IGameContext() noexcept = default;
    virtual ~IGameContext() noexcept = default;
    IGameContext(const IGameContext&) noexcept = delete;
    IGameContext& operator=(const IGameContext&) noexcept = delete;
    IGameContext(IGameContext&&) noexcept = delete;
    IGameContext& operator=(IGameContext&&) noexcept = delete;

    /// <summary>
    /// Gets the unique identifier of the currently loaded game.
    /// </summary>
    unsigned int GameId() const noexcept { return m_nGameId; }

    class NotifyTarget
    {
    public:
        NotifyTarget() noexcept = default;
        virtual ~NotifyTarget() noexcept = default;
        NotifyTarget(const NotifyTarget&) noexcept = delete;
        NotifyTarget& operator=(const NotifyTarget&) noexcept = delete;
        NotifyTarget(NotifyTarget&&) noexcept = default;
        NotifyTarget& operator=(NotifyTarget&&) noexcept = default;

        virtual void OnBeforeActiveGameChanged() noexcept(false) {}
        virtual void OnActiveGameChanged() noexcept(false) {}
        virtual void OnBeginGameLoad() noexcept(false) {}
        virtual void OnEndGameLoad() noexcept(false) {}
        virtual void OnMemoryNoteChanged(ra::data::ByteAddress, const std::wstring&) noexcept(false) {}
        virtual void OnMemoryNoteMoved(ra::data::ByteAddress, ra::data::ByteAddress, const std::wstring&) noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { m_vNotifyTargets.Add(pTarget); }
    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept { m_vNotifyTargets.Remove(pTarget); }

    virtual ra::data::models::MemoryNotesModel& MemoryNotes() noexcept = 0;
    virtual const ra::data::models::MemoryNotesModel& MemoryNotes() const noexcept = 0;

protected:
    unsigned int m_nGameId = 0;
    ra::data::NotifyTargetSet<NotifyTarget> m_vNotifyTargets;
};

} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_IGAMECONTEXT_HH
