#include "UserContext.hh"

#include "services\AchievementRuntimeExports.hh"

namespace ra {
namespace data {
namespace context {

void UserContext::Initialize(const std::string& sUsername, const std::string& sDisplayName, const std::string& sApiToken)
{
    m_sUsername = sUsername;
    m_sDisplayName = sDisplayName;
    m_sApiToken = sApiToken;
    m_nScore = 0U;

    RaiseClientExternalMenuChanged();
}

} // namespace context
} // namespace data
} // namespace ra
