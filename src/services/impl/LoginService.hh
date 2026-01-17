#ifndef RA_SERVICES_IMPL_LOGINSERVICE_HH
#define RA_SERVICES_IMPL_LOGINSERVICE_HH
#pragma once

#include "services/ILoginService.hh"

namespace ra {
namespace services {
namespace impl {

class LoginService : public ILoginService
{
public:
    LoginService() noexcept = default;
    ~LoginService() noexcept = default;
    LoginService(const LoginService&) noexcept = delete;
    LoginService& operator=(const LoginService&) noexcept = delete;
    LoginService(LoginService&&) noexcept = delete;
    LoginService& operator=(LoginService&&) noexcept = delete;

    bool IsLoggedIn() const override;
    bool Login(const std::string& sUsername, const std::string& sPassword) override;
    void Logout() override;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IMPL_LOGINSERVICE_HH
