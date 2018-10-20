#ifndef RA_SERVICES_THREADPOOL_HH
#define RA_SERVICES_THREADPOOL_HH
#pragma once

#include "services\IThreadPool.hh"

namespace ra {
namespace services {
namespace impl {

class ThreadPool : public IThreadPool
{
public:
	ThreadPool() noexcept(std::is_nothrow_default_constructible_v<std::condition_variable>) = default;
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
            m_vQueue.push(std::move(f));
        }

        m_cvMutex.notify_one();
    }

    void Shutdown(bool bWait) noexcept override;

    bool IsShutdownRequested() const noexcept override
    {
        return m_bShutdownInitiated;
    }

private:
    void RunThread() noexcept;

    std::vector<std::thread> m_vThreads;
    std::queue<std::function<void()>> m_vQueue;
    std::mutex m_oMutex;
    std::condition_variable m_cvMutex;
    size_t m_nThreads{ 0U };
    bool m_bShutdownInitiated{ false };
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_THREADPOOL_HH
