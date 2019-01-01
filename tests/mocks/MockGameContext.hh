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

    /// <summary>
    /// Sets the unique identifier of the currently loaded game.
    /// </summary>
    void SetGameId(unsigned int nGameId) noexcept { m_nGameId = nGameId; }

    /// <summary>
    /// Sets the title of the currently loaded game.
    /// </summary>
    void SetGameTitle(const std::wstring& sTitle) { m_sGameTitle = sTitle; }

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

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::GameContext> m_Override;

    std::wstring m_sRichPresenceDisplayString;
    bool m_bHasActiveAchievements{ false };
    AchievementSet::Type m_nActiveAchievementType{ AchievementSet::Type::Core };
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_GAMECONTEXT_HH
