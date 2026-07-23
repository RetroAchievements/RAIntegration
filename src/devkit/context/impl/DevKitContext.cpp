#include "DevKitContext.hh"

#include "RA_BuildVer.h"

namespace ra {
namespace context {
namespace impl {

DevKitContext::DevKitContext() noexcept
    : IDevKitContext(), m_sVersion(RA_INTEGRATION_VERSION)
{
}

} // namespace impl
} // namespace context
} // namespace ra
