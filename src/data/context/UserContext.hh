#ifndef RA_DATA_USERCONTEXT_HH
#define RA_DATA_USERCONTEXT_HH
#pragma once

#include <string>

namespace ra {
namespace data {
namespace context {

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
    void Initialize(const std::string& sUsername, const std::string& sDisplayName, const std::string& sApiToken);

    /// <summary>
    /// Gets the username of the current user.
    /// </summary>
    const std::string& GetUsername() const noexcept { return m_sUsername; }

    /// <summary>
    /// Gets the API token for the current user.
    /// </summary>
    const std::string& GetApiToken() const noexcept { return m_sApiToken; }

    /// <summary>
    /// Gets the display name of the current user.
    /// </summary>
    const std::string& GetDisplayName() const noexcept { return m_sDisplayName; }
    
    /// <summary>
    /// Gets the number of points earned by the user.
    /// </summary>
    unsigned int GetScore() const noexcept { return m_nScore; }

    /// <summary>
    /// Updates the number of points earned by the user.
    /// </summary>
    void SetScore(unsigned int nValue) noexcept { m_nScore = nValue; }

protected:
    std::string m_sUsername;
    std::string m_sDisplayName;
    std::string m_sApiToken;
    unsigned int m_nScore{ 0U };
};

} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_USERCONTEXT_HH
