#ifndef RA_SERVICES_IDEBUGGERDETECTOR_HH
#define RA_SERVICES_IDEBUGGERDETECTOR_HH
#pragma once

namespace ra {
namespace services {

class IDebuggerDetector
{
public:
    virtual ~IDebuggerDetector() noexcept = default;
    IDebuggerDetector(const IDebuggerDetector&) noexcept = delete;
    IDebuggerDetector& operator=(const IDebuggerDetector&) noexcept = delete;
    IDebuggerDetector(IDebuggerDetector&&) noexcept = delete;
    IDebuggerDetector& operator=(IDebuggerDetector&&) noexcept = delete;

    /// <summary>
    /// Determines if the process is running in a debugger.
    /// </summary>
    virtual bool IsDebuggerPresent() const = 0;

protected:
    IDebuggerDetector() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IDEBUGGERDETECTOR_HH
