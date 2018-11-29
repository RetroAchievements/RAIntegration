#ifndef RA_DATA_USERCONTEXT_HH
#define RA_DATA_USERCONTEXT_HH
#pragma once

#include <string>

namespace ra {
namespace data {

class UserContext
{
public:
    UserContext() noexcept = default;
    virtual ~UserContext() noexcept = default;
    UserContext(const UserContext&) noexcept = delete;
    UserContext& operator=(const UserContext&) noexcept = delete;
    UserContext(UserContext&&) noexcept = delete;
    UserContext& operator=(UserContext&&) noexcept = delete;

    /// <summary>
    /// Sets the username and API token for the current user.
    /// </summary>
    void Initialize(const std::string& sUsername, const std::string& sApiToken)
    {
        m_sUsername = sUsername;
        m_sApiToken = sApiToken;
        m_nScore = 0U;
    }

    /// <summary>
    /// Gets the username of the current user.
    /// </summary>
    const std::string& GetUsername() const { return m_sUsername; }

    /// <summary>
    /// Gets the API token for the current user.
    /// </summary>
    const std::string& GetApiToken() const { return m_sApiToken; }

    /// <summary>
    /// Determines whether the current user is logged in.
    /// </summary>
    bool IsLoggedIn() const { return !m_sApiToken.empty(); }
    
    /// <summary>
    /// Gets the number of points earned by the user.
    /// </summary>
    unsigned int GetScore() const { return m_nScore; }

    /// <summary>
    /// Updates the number of points earned by the user.
    /// </summary>
    void SetScore(unsigned int nValue) { m_nScore = nValue; }

protected:
    std::string m_sUsername;
    std::string m_sApiToken;
    unsigned int m_nScore{0U};
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_USERCONTEXT_HH
