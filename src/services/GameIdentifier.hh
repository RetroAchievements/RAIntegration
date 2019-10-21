#ifndef RA_SERVICES_GAMEIDENTIFIER_HH
#define RA_SERVICES_GAMEIDENTIFIER_HH
#pragma once

#include "data\GameContext.hh"

namespace ra {
namespace services {

class GameIdentifier
{
public:    
    /// <summary>
    /// Identifies and activates a game.
    /// </summary>
    /// <param name="pMemory">Pointer to memory that unique identifies the game.</param>
    /// <param name="nSize">Size of the unique memory block.</param>
    void IdentifyAndActivateGame(const BYTE* pMemory, size_t nSize);

    /// <summary>
    /// Identifies a game.
    /// </summary>
    /// <param name="pMemory">Pointer to memory that unique identifies the game.</param>
    /// <param name="nSize">Size of the unique memory block.</param>
    unsigned int IdentifyGame(const BYTE* pMemory, size_t nSize);

    /// <summary>
    /// Activates a game.
    /// </summary>
    /// <param name="nGameId">Unique identifier of the game to activate.</param>
    void ActivateGame(unsigned int nGameId);

private:
    std::string m_sPendingMD5;
    unsigned int m_nPendingGameId{};
    ra::data::GameContext::Mode m_nPendingMode{};
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_GAMEIDENTIFIER_HH
