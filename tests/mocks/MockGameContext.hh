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

    bool HasActiveAchievements() const noexcept override { return m_bHasActiveAchievements; }
    
    /// <summary>
    /// Sets the value for <see cref="HasActiveAchievements" />
    /// </summary>
    void SetHasActiveAchievements(bool bValue) noexcept { m_bHasActiveAchievements = bValue; }

    AchievementSet::Type ActiveAchievementType() const noexcept override { return m_nActiveAchievementType; }

    /// <summary>
    /// Sets the value for <see cref="HasActiveAchievements" />
    /// </summary>
    void SetActiveAchievementType(AchievementSet::Type bValue) noexcept { m_nActiveAchievementType = bValue; }

    Achievement* FindAchievement(unsigned int nAchievementId) const noexcept override
    {
        for (auto& pAchievement : m_vAchievements)
        {
            if (pAchievement.ID() == nAchievementId)
                return const_cast<Achievement*>(&pAchievement);
        }

        return nullptr;
    }
    
    /// <summary>
    /// Adds an achivement.
    /// </summary>
    /// <param name="nId">The unique identifier of the achievement.</param>
    /// <param name="sTrigger">The trigger string.</param>
    void AddAchivement(unsigned int nId, const char* sTrigger)
    {
        Achievement achievement;

        std::string sLine = ra::StringPrintf("%zu:%s:Title:Desc::::Author:5:1234567890:1234567890:::", nId, sTrigger);
        achievement.ParseLine(sLine.c_str());
        m_vAchievements.emplace_back(std::move(achievement));
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::GameContext> m_Override;

    std::wstring m_sRichPresenceDisplayString;
    bool m_bHasActiveAchievements{ false };
    AchievementSet::Type m_nActiveAchievementType{ AchievementSet::Type::Core };

    std::vector<Achievement> m_vAchievements;
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_GAMECONTEXT_HH
