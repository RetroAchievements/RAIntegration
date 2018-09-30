#ifndef RA_SERVICES_ITHREADPOOL_HH
#define RA_SERVICES_ITHREADPOOL_HH
#pragma once

#include <functional>

namespace ra {
namespace services {

class IThreadPool
{
public:
    virtual ~IThreadPool() noexcept = default;

    /// <summary>
    /// Queues work for a background thread
    /// </summary>
    template<typename F>
    void RunAsync(F&& f) noexcept
    {
        RunAsyncImpl(std::move(f));
    }

    virtual void RunAsyncImpl(std::function<void()> f) noexcept = 0;
    
    /// <summary>
    /// Sets the <see ref="IsShutdownRequested" /> flag so threads can start winding down.
    /// </summary>
    virtual void Shutdown() noexcept = 0;

    /// <summary>
    /// Determines whether the process has started shutting down.
    /// </summary>
    /// <returns><c>true</c> if shutdown has started.</returns>
    /// <remarks>Long running background threads should occassionally check this and terminate early.</remarks>
    virtual bool IsShutdownRequested() const noexcept = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ITHREADPOOL_HH
