#ifndef RA_DATA_MOCK_GAMECONTEXT_HH
#define RA_DATA_MOCK_GAMECONTEXT_HH
#pragma once

#include "data\GameContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace mocks {

class MockGameContext : public GameContext
{
public:
    MockGameContext() noexcept
        : m_Override(this)
    {
    }

    void LoadGame(unsigned int nGameId, ra::data::GameContext::Mode nMode) noexcept override
    {
        m_nGameId = nGameId;
        m_nMode = nMode;
        m_bWasLoaded = true;
    }

    bool WasLoaded() const noexcept { return m_bWasLoaded; }
    void ResetWasLoaded() noexcept { m_bWasLoaded = false; }

    /// <summary>
    /// Sets the unique identifier of the currently loaded game.
    /// </summary>
    void SetGameId(unsigned int nGameId) noexcept { m_nGameId = nGameId; }

    void NotifyActiveGameChanged() { OnActiveGameChanged(); }

    void NotifyGameLoad() { BeginLoad(); EndLoad(); }

    bool HasRichPresence() const noexcept override { return !m_sRichPresenceDisplayString.empty(); }

    std::wstring GetRichPresenceDisplayString() const override { return m_sRichPresenceDisplayString; }

    /// <summary>
    /// Sets the rich presence display string.
    /// </summary>
    void SetRichPresenceDisplayString(std::wstring sValue) { m_sRichPresenceDisplayString = sValue; }

    void SetRichPresenceFromFile(bool bValue) noexcept { m_bRichPresenceFromFile = bValue; }

    RA_Leaderboard& NewLeaderboard(ra::LeaderboardID nLeaderboardId)
    {
        return *m_vLeaderboards.emplace_back(std::make_unique<RA_Leaderboard>(nLeaderboardId));
    }

    bool SetCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote) override
    {
        // non-API part of SetCodeNote
        AddCodeNote(nAddress, "Author", sNote);
        return true;
    }

    bool DeleteCodeNote(ra::ByteAddress nAddress) override
    {
        // non-API part of DeleteCodeNote
        m_mCodeNotes.erase(nAddress);
        OnCodeNoteChanged(nAddress, L"");
        return true;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::GameContext> m_Override;

    std::wstring m_sRichPresenceDisplayString;
    bool m_bHasActiveAchievements{ false };
    bool m_bWasLoaded{ false };
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_GAMECONTEXT_HH
