#ifndef RA_SERVICES_ILOGINSERVICE_HH
#define RA_SERVICES_ILOGINSERVICE_HH
#pragma once

namespace ra {
namespace services {

class ILoginService
{
public:
    virtual ~ILoginService() noexcept = default;
    ILoginService(const ILoginService&) noexcept = delete;
    ILoginService& operator=(const ILoginService&) noexcept = delete;
    ILoginService(ILoginService&&) noexcept = delete;
    ILoginService& operator=(ILoginService&&) noexcept = delete;

    /// <summary>
    /// Determines whether the current user is logged in.
    /// </summary>
    virtual bool IsLoggedIn() const = 0;
    
    /// <summary>
    /// Gets whether login has been disabled.
    /// </summary>
    bool IsLoginDisabled() const noexcept { return m_bDisableLogin; }

    /// <summary>
    /// Disables the ability to login.
    /// </summary>
    void DisableLogin() noexcept { m_bDisableLogin = true; }

    /// <summary>
    /// Logs a user in.
    /// </summary>
    virtual bool Login(const std::string& sUsername, const std::string& sPassword) = 0;

    /// <summary>
    /// Logs the user out.
    /// </summary>
    virtual void Logout() = 0;

protected:
    ILoginService() noexcept = default;

    bool m_bDisableLogin{ false };
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ILOGINSERVICE_HH
