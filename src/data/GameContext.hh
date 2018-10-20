#ifndef RA_DATA_GAMECONTEXT_HH
#define RA_DATA_GAMECONTEXT_HH
#pragma once

#include "RA_RichPresence.h"

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
    unsigned int GameId() const { return m_nGameId; }       

    /// <summary>
    /// Gets the title of the currently loaded game.
    /// </summary>
    const std::wstring& GameTitle() const { return m_sGameTitle; }

    /// <summary>
    /// Gets the hash of the currently loaded game.
    /// </summary>
    const std::string& GameHash() const { return m_sGameHash; }

    /// <summary>
    /// Sets the game hash.
    /// </summary>
    void SetGameHash(const std::string& sGameHash) { m_sGameHash = sGameHash; }
    
    /// <summary>
    /// Gets whether or not the loaded game has a rich presence script.
    /// </summary>
    bool HasRichPresence() const;
    
    /// <summary>
    /// Gets the current rich presence display string.
    /// </summary>
    std::wstring GetRichPresenceDisplayString() const;
    
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
