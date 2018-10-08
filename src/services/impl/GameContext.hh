#ifndef RA_SERVICES_GAMECONTEXT_HH
#define RA_SERVICES_GAMECONTEXT_HH
#pragma once

#include "services\IGameContext.hh"

namespace ra {
namespace services {
namespace impl {

class GameContext : public IGameContext
{
public:
    void LoadGame(unsigned int nGameId, const std::wstring& sGameTitle) override;
    
    unsigned int GameId() const override { return m_nGameId; }       
    const std::wstring& GameTitle() const override { return m_sGameTitle; }

    const std::string& GameHash() const override { return m_sGameHash; }
    void SetGameHash(const std::string& sGameHash) override { m_sGameHash = sGameHash; }

private:
    unsigned int m_nGameId = 0;
    std::wstring m_sGameTitle;
    std::string m_sGameHash;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_GAMECONTEXT_HH
