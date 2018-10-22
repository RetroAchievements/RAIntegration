#ifndef RA_DATA_GAMECONTEXT_HH
#define RA_DATA_GAMECONTEXT_HH
#pragma once

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

protected:
    unsigned int m_nGameId = 0;
    std::wstring m_sGameTitle;
    std::string m_sGameHash;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_GAMECONTEXT_HH
