#ifndef RA_SERVICES_ITHREADPOOL_HH
#define RA_SERVICES_ITHREADPOOL_HH
#pragma once

namespace ra {
namespace services {

class IThreadPool
{
public:
    virtual ~IThreadPool() noexcept = default;
    IThreadPool(const IThreadPool&) noexcept = delete;
    IThreadPool& operator=(const IThreadPool&) noexcept = delete;
    IThreadPool(IThreadPool&&) noexcept = delete;
    IThreadPool& operator=(IThreadPool&&) noexcept = delete;

    /// <summary>
    /// Queues work for a background thread
    /// </summary>
    virtual void RunAsync(std::function<void()>&& f) = 0;

    /// <summary>
    /// Queues work for a background thread to be run after a period of time
    /// </summary>
    virtual void ScheduleAsync(std::chrono::milliseconds nDelay, std::function<void()>&& f) = 0;

    /// <summary>
    /// Sets the <see ref="IsShutdownRequested" /> flag so threads can start winding down.
    /// </summary>
    /// <param name="bWait">
    /// if <c>false</c>, start shutting down background threads and return. if <c>true</c> waits for the threads to finish shutting down before returning.
    /// </param>
    virtual void Shutdown(bool bWait) noexcept = 0;

    /// <summary>
    /// Determines whether the process has started shutting down.
    /// </summary>
    /// <returns><c>true</c> if shutdown has started.</returns>
    /// <remarks>Long running background threads should occasionally check this and terminate early.</remarks>
    virtual bool IsShutdownRequested() const noexcept = 0;

protected:
    IThreadPool() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ITHREADPOOL_HH
