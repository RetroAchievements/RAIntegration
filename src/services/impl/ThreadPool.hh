#ifndef RA_SERVICES_THREADPOOL_HH
#define RA_SERVICES_THREADPOOL_HH
#pragma once

#include "ra_fwd.h"

#include "services\IClock.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace impl {

class ThreadPool : public IThreadPool
{
public:
    // TODO: std::condition_variable can throw std::system_error (a.k.a fatal error).
    //       We could just delete the compiler defaulted constructor and handle the exception in an invariant (param
    //       constructor)
    // Info: https://en.cppreference.com/w/cpp/thread/condition_variable/condition_variable
    ThreadPool() noexcept(std::is_nothrow_default_constructible_v<std::condition_variable>) = default;

    ~ThreadPool() noexcept;
    ThreadPool(const ThreadPool&) noexcept = delete;
    ThreadPool& operator=(const ThreadPool&) noexcept = delete;
    ThreadPool(ThreadPool&&) noexcept = delete;
    ThreadPool& operator=(ThreadPool&&) noexcept = delete;

    GSL_SUPPRESS_F6 void Initialize(size_t nThreads) noexcept;

    void RunAsync(std::function<void()>&& f) override
    {
        if (m_bShutdownInitiated)
            return;

        assert(!m_vThreads.empty());

        {
            std::unique_lock<std::mutex> lock(m_oMutex);
            m_vQueue.push_back(std::move(f));
        }

        m_cvWork.notify_one();
    }

    void ScheduleAsync(std::chrono::milliseconds nDelay, std::function<void()>&& f) override
    {
        if (m_bShutdownInitiated)
            return;

        assert(!m_vThreads.empty());

        const auto tWhen = ServiceLocator::Get<IClock>().UpTime() + nDelay;

        bool bStartScheduler = false;
        bool bNewPriority    = false;
        {
            std::unique_lock<std::mutex> lock(m_oMutex);

            auto iter = m_vDelayedTasks.begin();

            if (m_vDelayedTasks.empty())
            {
                // first scheduled task - dedicate one of the background threads to timed events
                m_vQueue.push_front([this]() { ProcessDelayedTasks(); });
                bStartScheduler = true;
            }
            else if (tWhen < iter->tWhen)
            {
                // sooner than the next scheduled task, we'll need to wake the timed events thread to reset the wait
                // time
                bNewPriority = true;
            }
            else
            {
                // after the next scheduled task, find where to insert it
                do
                {
                    ++iter;
                } while (iter != m_vDelayedTasks.end() && iter->tWhen < tWhen);
            }

            m_vDelayedTasks.emplace(iter, tWhen, std::move(f));
        }

        if (bStartScheduler)
        {
            // wake one of the background threads to process the timed events
            m_cvWork.notify_one();
        }

        if (bNewPriority)
        {
            // wake the timed events thread to recalculate the time until the next event
            m_cvDelayedWork.notify_one();
        }
    }

    GSL_SUPPRESS_F6 void Shutdown(bool bWait) noexcept override;

    bool IsShutdownRequested() const noexcept override { return m_bShutdownInitiated; }

private:
    void RunThread();
    void ProcessDelayedTasks();

    std::vector<std::thread> m_vThreads;
    size_t m_nThreads{0U};
    bool m_bShutdownInitiated{false};

    struct DelayedTask
    {
        DelayedTask(std::chrono::steady_clock::time_point tWhen, std::function<void()> fTask) :
            tWhen(tWhen),
            fTask(fTask){};

        std::chrono::steady_clock::time_point tWhen;
        std::function<void()> fTask;
    };
    std::deque<DelayedTask> m_vDelayedTasks;

    std::deque<std::function<void()>> m_vQueue;
    std::mutex m_oMutex;
    std::condition_variable m_cvWork;
    std::condition_variable m_cvDelayedWork;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_THREADPOOL_HH
