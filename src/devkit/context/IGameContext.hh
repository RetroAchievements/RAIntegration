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
    class LocalBadgesModel;
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
    uint32_t GameId() const noexcept { return m_nGameId; }

    /// <summary>
    /// Gets the unique identifier of the base game for the currently loaded subset.
    /// </summary>
    /// <remarks>
    /// Will match <see cref="GameId"/> unless an exclusive or specialty subset is loaded.
    /// </remarks>
    uint32_t ActiveGameId() const noexcept { return m_nActiveGameId; }

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

    /// <summary>
    /// Gets the <see cref="MemoryNote"/>s for the currently loaded game.
    /// </summary>
    virtual ra::data::models::MemoryNotesModel& MemoryNotes() noexcept(false) = 0;

    /// <summary>
    /// Gets the <see cref="MemoryNote"/>s for the currently loaded game.
    /// </summary>
    virtual const ra::data::models::MemoryNotesModel& MemoryNotes() const noexcept(false) = 0;

    /// <summary>
    /// Gets the <see cref="MemoryNote"/>s for the currently loaded game.
    /// </summary>
    virtual ra::data::models::LocalBadgesModel& LocalBadges() noexcept(false) = 0;

    /// <summary>
    /// Gets the <see cref="MemoryNote"/>s for the currently loaded game.
    /// </summary>
    virtual const ra::data::models::LocalBadgesModel& LocalBadges() const noexcept(false) = 0;

protected:
    uint32_t m_nGameId = 0;
    uint32_t m_nActiveGameId = 0;

    ra::data::NotifyTargetSet<NotifyTarget> m_vNotifyTargets;
};

} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_IGAMECONTEXT_HH
