#ifndef RA_SERVICES_ICLOCK_HH
#define RA_SERVICES_ICLOCK_HH
#pragma once

#include <chrono>

namespace ra {
namespace services {

class IClock
{
public:
    virtual ~IClock() noexcept = default;

    /// <summary>
    /// Gets the current system time.
    /// </summary>
    /// <remarks>May change if the user adjusts their clock or the system does (daylight savings).</remarks>
    virtual std::chrono::system_clock::time_point Now() const = 0;

    /// <summary>
    /// Gets a time_point that represents how long the system has been running.
    /// </summary>
    /// <remarks>Is not affected by the user adjusting their clock</remarks>
    virtual std::chrono::steady_clock::time_point UpTime() const = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ILOGGER_HH
