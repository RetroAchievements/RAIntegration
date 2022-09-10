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

    void RunAsync(std::function<void()>&& f) override
    {
        if (m_bSynchronous)
        {
            f();
        }
        else
        {
            m_vTasks.emplace(f);
        }
    }

    void ScheduleAsync(std::chrono::milliseconds nDelay, std::function<void()>&& f) override
    {
        m_vDelayedTasks.emplace_back(nDelay, f);
    }

    void AdvanceTime(std::chrono::milliseconds nDuration)
    {
        std::vector<std::function<void()>> vTasks;

        auto pIter = m_vDelayedTasks.begin();
        while (pIter != m_vDelayedTasks.end())
        {
            if (pIter->nDelay <= nDuration)
            {
                vTasks.push_back(std::move(pIter->fTask));
                pIter = m_vDelayedTasks.erase(pIter);
            }
            else
            {
                pIter->nDelay -= nDuration;
                ++pIter;
            }
        }

        for (auto fTask : vTasks)
            fTask();
    }

    /// <summary>
    /// Gets the delay until the next task would be executed.
    /// </summary>
    std::chrono::milliseconds NextTaskDelay() const noexcept
    {
        if (!m_vTasks.empty())
            return std::chrono::milliseconds(0);

        if (!m_vDelayedTasks.empty())
            return m_vDelayedTasks.front().nDelay;

        return std::chrono::hours(10000);
    }

    /// <summary>
    /// Gets the number of outstanding asynchronous tasks are queued
    /// </summary>
    size_t PendingTasks() const noexcept { return m_vTasks.size() + m_vDelayedTasks.size(); }

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

    void Shutdown([[maybe_unused]] bool /*bWait*/) noexcept override {}

    bool IsShutdownRequested() const noexcept override { return false; }

    /// <summary>
    /// Specifies whether non-scheduled tasks should be immediately executed when queued.
    /// </summary>
    void SetSynchronous(bool bValue) noexcept
    {
        m_bSynchronous = bValue;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IThreadPool> m_Override;

    std::queue<std::function<void()>> m_vTasks;
    bool m_bSynchronous = false;

    struct DelayedTask
    {
        DelayedTask(std::chrono::milliseconds nDelay, std::function<void()> fTask) : nDelay(nDelay), fTask(fTask) {};

        std::chrono::milliseconds nDelay;
        std::function<void()> fTask;
    };
    std::vector<DelayedTask> m_vDelayedTasks;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_THREADPOOL_HH
