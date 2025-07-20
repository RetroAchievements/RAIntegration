#ifndef RA_DATA_MOCK_GAMEIDENTIFIER_HH
#define RA_DATA_MOCK_GAMEIDENTIFIER_HH
#pragma once

#include "services\GameIdentifier.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockGameIdentifier : public GameIdentifier
{
public:
    MockGameIdentifier() noexcept
        : m_Override(this)
    {
    }

    using GameIdentifier::AddHash;

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::GameIdentifier> m_Override;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_DATA_MOCK_GAMEIDENTIFIER_HH
