#ifndef RA_DATA_GAMECONTEXT_HH
#define RA_DATA_GAMECONTEXT_HH
#pragma once

#include "RA_RichPresence.h"
#include "RA_AchievementSet.h"

#include <string>

namespace ra {
namespace data {

class GameContext
{
public:
    GameContext() noexcept = default;
    virtual ~GameContext() noexcept = default;
    GameContext(const GameContext&) noexcept = delete;
    GameContext& operator=(const GameContext&) noexcept = delete;
    GameContext(GameContext&&) noexcept = delete;
    GameContext& operator=(GameContext&&) noexcept = delete;

    /// <summary>
    /// Loads the data for the specified game id.
    /// </summary>
    void LoadGame(unsigned int nGameId, const std::wstring& sGameTitle);

    /// <summary>
    /// Gets the unique identifier of the currently loaded game.
    /// </summary>
    unsigned int GameId() const noexcept { return m_nGameId; }       

    /// <summary>
    /// Gets the title of the currently loaded game.
    /// </summary>
    const std::wstring& GameTitle() const noexcept { return m_sGameTitle; }

    /// <summary>
    /// Gets the hash of the currently loaded game.
    /// </summary>
    const std::string& GameHash() const noexcept { return m_sGameHash; }

    /// <summary>
    /// Sets the game hash.
    /// </summary>
    void SetGameHash(const std::string& sGameHash) { m_sGameHash = sGameHash; }
        
    /// <summary>
    /// Gets which achievements are active.
    /// </summary>
    virtual AchievementSet::Type ActiveAchievementType() const noexcept
    {
#ifdef RA_UTEST
        return AchievementSet::Type::Core;
#else
        return g_nActiveAchievementSet; 
#endif
    }
    
    /// <summary>
    /// Determines if any achievements are currently active.
    /// </summary>
    virtual bool HasActiveAchievements() const noexcept
    {
#ifdef RA_UTEST
        return false;
#else
        return g_pActiveAchievements && g_pActiveAchievements->NumAchievements() > 0; 
#endif
    }

    virtual Achievement* FindAchievement(_UNUSED unsigned int nAchievementId) const noexcept
    {
#ifdef RA_UTEST
        return nullptr;
#else
        return g_pActiveAchievements->Find(nAchievementId);
#endif
    }

    /// <summary>
    /// Gets whether or not the loaded game has a rich presence script.
    /// </summary>
    virtual bool HasRichPresence() const noexcept;
    
    /// <summary>
    /// Gets the current rich presence display string.
    /// </summary>
    virtual std::wstring GetRichPresenceDisplayString() const;
    
    /// <summary>
    /// Reloads the rich presence script.
    /// </summary>
    void ReloadRichPresenceScript();

protected:
    unsigned int m_nGameId = 0;
    std::wstring m_sGameTitle;
    std::string m_sGameHash;

    std::unique_ptr<RA_RichPresenceInterpreter> m_pRichPresenceInterpreter;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_GAMECONTEXT_HH
