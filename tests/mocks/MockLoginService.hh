#ifndef RA_SERVICES_MOCK_LOGINSERVICE_HH
#define RA_SERVICES_MOCK_LOGINSERVICE_HH
#pragma once

#include "services\ILoginService.hh"

#include "data\context\UserContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockLoginService : public ILoginService
{
public:
    MockLoginService() noexcept
        : m_Override(this)
    {
    }

    bool IsLoggedIn() const override { return m_bIsLoggedIn; }

    void SetLoggedIn(bool bIsLoggedIn) noexcept { m_bIsLoggedIn = bIsLoggedIn; }

    void MockLoginFailure(bool bIsFailure) noexcept { m_bFailLogin = bIsFailure; }

    bool Login(const std::string& sUsername, const std::string&) override
    {
        if (m_bFailLogin)
            return false;

        if (ra::services::ServiceLocator::Exists<ra::data::context::UserContext>())
            ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>().Initialize(sUsername, sUsername + "_", "APITOKEN");

        m_bIsLoggedIn = true;
        return true;
    }

    void Logout() override
    {
        if (ra::services::ServiceLocator::Exists<ra::data::context::UserContext>())
            ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>().Initialize("", "", "");

        m_bIsLoggedIn = false;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::ILoginService> m_Override;

    bool m_bIsLoggedIn = false;
    bool m_bFailLogin = false;
};

} // namespace mocks
} // namespace service
} // namespace ra

#endif // !RA_SERVICES_MOCK_LOGINSERVICE_HH
