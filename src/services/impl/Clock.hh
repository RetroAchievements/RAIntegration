#ifndef RA_SERVICES_CLOCK_HH
#define RA_SERVICES_CLOCK_HH
#pragma once

#include "services\IClock.hh"

namespace ra {
namespace services {
namespace impl {

class Clock : public ra::services::IClock
{
public:
    std::chrono::system_clock::time_point Now() const noexcept override
    {
        return std::chrono::system_clock::now();
    }

    std::chrono::steady_clock::time_point UpTime() const noexcept override
    {
        return std::chrono::steady_clock::now();
    }
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_CLOCK_HH
