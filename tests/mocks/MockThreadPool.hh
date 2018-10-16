#ifndef RA_SERVICES_MOCK_THREADPOOL_HH
#define RA_SERVICES_MOCK_THREADPOOL_HH
#pragma once

#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include <queue>

namespace ra {
namespace services {
namespace mocks {

class MockThreadPool : public IThreadPool
{
public:
    explicit MockThreadPool() noexcept
        : m_Override(this)
    {
    }

    void RunAsync(std::function<void()>&& f) noexcept override
    {
        m_vTasks.emplace(f);
    }

    void ScheduleAsync(std::chrono::milliseconds nDelay, std::function<void()>&& f) noexcept override
    {

    }

    /// <summary>
    /// Gets the number of outstanding asynchronous tasks are queued
    /// </summary>
    size_t PendingTasks() const { return m_vTasks.size(); }

    /// <summary>
    /// Executes the next outstanding task.
    /// </summary>
    void ExecuteNextTask()
    {
        if (m_vTasks.empty())
            return;

        auto fTask = m_vTasks.front();
        m_vTasks.pop();
        fTask();
    }

    void Shutdown(bool bWait) noexcept override {}

    bool IsShutdownRequested() const noexcept override { return false; }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IThreadPool> m_Override;

    std::queue<std::function<void()>> m_vTasks;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_THREADPOOL_HH
