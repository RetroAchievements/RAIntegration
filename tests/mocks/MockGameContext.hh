#ifndef RA_SERVICES_MOCK_GAMECONTEXT_HH
#define RA_SERVICES_MOCK_GAMECONTEXT_HH
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
    void SetGameId(unsigned int nGameId) { m_nGameId = nGameId; }

    /// <summary>
    /// Sets the title of the currently loaded game.
    /// </summary>
    void SetGameTitle(const std::wstring& sTitle) { m_sGameTitle = sTitle; }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::GameContext> m_Override;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_GAMECONTEXT_HH
