#include "DevKitContext.hh"

#include "RA_BuildVer.h"

#include "util/GSL.hh"

namespace ra {
namespace context {
namespace impl {

GSL_SUPPRESS_F6
DevKitContext::DevKitContext() noexcept
    : IDevKitContext(), m_sVersion(RA_INTEGRATION_VERSION)
{
}

} // namespace impl
} // namespace context
} // namespace ra
