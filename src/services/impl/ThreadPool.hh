#ifndef RA_SERVICES_THREADPOOL_HH
#define RA_SERVICES_THREADPOOL_HH
#pragma once

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
    //       We could just delete the compiler defaulted constructor and handle the exception in an invariant (param constructor)
    // Info: https://en.cppreference.com/w/cpp/thread/condition_variable/condition_variable
    ThreadPool() 
        noexcept(std::is_nothrow_default_constructible_v<std::condition_variable>) = default;

    ~ThreadPool() noexcept;
    ThreadPool(const ThreadPool&) noexcept = delete;
    ThreadPool& operator=(const ThreadPool&) noexcept = delete;
    ThreadPool(ThreadPool&&) noexcept = delete;
    ThreadPool& operator=(ThreadPool&&) noexcept = delete;

    void Initialize(size_t nThreads) noexcept;

    void RunAsync(std::function<void()>&& f) noexcept override
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
    
    void ScheduleAsync(std::chrono::milliseconds nDelay, std::function<void()>&& f) noexcept override
    {
        if (m_bShutdownInitiated)
            return;

        assert(!m_vThreads.empty());

        const auto tWhen = ServiceLocator::Get<IClock>().UpTime() + nDelay;

        bool bStartScheduler = false;
        bool bNewPriority = false;
        {
            std::unique_lock<std::mutex> lock(m_oMutex);

            auto iter = m_vDelayedTasks.begin();

            if (m_vDelayedTasks.empty())
            {
                m_vQueue.push_front([this]() { ProcessDelayedTasks(); });
                bStartScheduler = true;
            }
            else if (tWhen < iter->tWhen)
            {
                bNewPriority = true;
            }
            else
            {
                do {
                    ++iter;
                } while (iter != m_vDelayedTasks.end() && iter->tWhen < tWhen);
            }

            m_vDelayedTasks.emplace(iter, tWhen, std::move(f));
        }

        if (bStartScheduler)
            m_cvWork.notify_one();

        if (bNewPriority)
            m_cvDelayedWork.notify_one();
    }

    void Shutdown(bool bWait) noexcept override;

    bool IsShutdownRequested() const noexcept override
    {
        return m_bShutdownInitiated;
    }

private:
    void RunThread() noexcept;
    void ProcessDelayedTasks() noexcept;

    std::vector<std::thread> m_vThreads;
    size_t m_nThreads{ 0U };
    bool m_bShutdownInitiated{ false };

    struct DelayedTask
    {
        DelayedTask(std::chrono::steady_clock::time_point tWhen, std::function<void()> fTask) : tWhen(tWhen), fTask(fTask) {};

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
