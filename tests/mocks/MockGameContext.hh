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

    bool HasRichPresence() const noexcept override { return !m_sRichPresenceDisplayString.empty(); }

    std::wstring GetRichPresenceDisplayString() const override { return m_sRichPresenceDisplayString; }

    /// <summary>
    /// Sets the rich presence display string.
    /// </summary>
    void SetRichPresenceDisplayString(std::wstring sValue) { m_sRichPresenceDisplayString = sValue; }

    AchievementSet::Type ActiveAchievementType() const noexcept override { return m_nActiveAchievementType; }

    /// <summary>
    /// Sets the value for <see cref="HasActiveAchievements" />
    /// </summary>
    void SetActiveAchievementType(AchievementSet::Type bValue) noexcept { m_nActiveAchievementType = bValue; }

    RA_Leaderboard& NewLeaderboard(ra::LeaderboardID nLeaderboardId)
    {
        return *m_vLeaderboards.emplace_back(std::make_unique<RA_Leaderboard>(nLeaderboardId));
    }

    void MockCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote)
    {
        AddCodeNote(nAddress, "Author", sNote);
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::GameContext> m_Override;

    std::wstring m_sRichPresenceDisplayString;
    bool m_bHasActiveAchievements{ false };
    bool m_bWasLoaded{ false };
    AchievementSet::Type m_nActiveAchievementType{ AchievementSet::Type::Core };
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_GAMECONTEXT_HH
