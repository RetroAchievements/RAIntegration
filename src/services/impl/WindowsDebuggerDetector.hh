#ifndef RA_SERVICES_IMPL_DEBUGGER_DETECTOR
#define RA_SERVICES_IMPL_DEBUGGER_DETECTOR

#include "services/IDebuggerDetector.hh"

namespace ra {
namespace services {
namespace impl {

class WindowsDebuggerDetector : public IDebuggerDetector
{
public:
    bool IsDebuggerPresent() const override;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IMPL_DEBUGGER_DETECTOR
