#ifndef RA_SERVICES_IGAMECONTEXT_HH
#define RA_SERVICES_IGAMECONTEXT_HH
#pragma once

#include <string>

namespace ra {
namespace services {

class IGameContext
{
public:
    /// <summary>
    /// Loads the data for the specified game id.
    /// </summary>
    virtual void LoadGame(unsigned int nGameId, const std::wstring& sGameTitle) = 0;

    /// <summary>
    /// Gets the unique identifier of the currently loaded game.
    /// </summary>
    virtual unsigned int GameId() const = 0;

    /// <summary>
    /// Gets the title of the currently loaded game.
    /// </summary>
    virtual const std::wstring& GameTitle() const = 0;

    /// <summary>
    /// Gets the hash of the currently loaded game.
    /// </summary>
    virtual const std::string& GameHash() const = 0;
    
    /// <summary>
    /// Sets the game hash.
    /// </summary>
    virtual void SetGameHash(const std::string& sGameHash) = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IGAMECONTEXT_HH
