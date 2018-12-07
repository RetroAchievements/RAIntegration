#include "ThreadPool.hh"

#include "RA_Log.h"

namespace ra {
namespace services {
namespace impl {

ThreadPool::~ThreadPool() noexcept
{
    Shutdown(true);
}

void ThreadPool::Initialize(size_t nThreads) noexcept
{
    assert(m_vThreads.empty());

    // require at least two threads. that way if one thread is reserved for processing timed events, another is still available to execute them.
    if (nThreads < 2)
        nThreads = 2; 

    RA_LOG_INFO("Initializing %zu worker threads", nThreads);

    for (size_t i = 0; i < nThreads; ++i)
        m_vThreads.emplace_back(&ThreadPool::RunThread, this);
}

void ThreadPool::RunThread()
{
    do
    {
        // check for work
        do
        {
            std::function<void()> pNext;
            {
                std::unique_lock<std::mutex> lock(m_oMutex);
                if (m_vQueue.empty())
                    break;

                pNext = std::move(m_vQueue.front());
                m_vQueue.pop_front();
            }

            // do work
            try
            {
                pNext();
            }
            catch (const std::exception& ex)
            {
                RA_LOG_ERR("Exception on background thread: %s", ex.what());
            }
        } while (!m_bShutdownInitiated);

        // wait for work
        if (!m_bShutdownInitiated)
        {
            std::unique_lock<std::mutex> lock(m_oMutex);
            m_cvWork.wait(lock);
        }
    } while (!m_bShutdownInitiated);
}

void ThreadPool::ProcessDelayedTasks()
{
    auto& pClock = ServiceLocator::Get<IClock>();
    constexpr auto tZeroMilliseconds = std::chrono::milliseconds(0);

    // check for work
    while (!m_bShutdownInitiated)
    {
        // move any work that needs to occur now to the work queue
        size_t nReadyTasks = 0U;
        {
            std::unique_lock<std::mutex> lock(m_oMutex);

            const auto tNow = pClock.UpTime();
            while (!m_vDelayedTasks.empty() && m_vDelayedTasks.front().tWhen <= tNow)
            {
                m_vQueue.emplace_front(std::move(m_vDelayedTasks.front().fTask));
                m_vDelayedTasks.pop_front();

                ++nReadyTasks;
            }
        }

        if (m_bShutdownInitiated)
            break;

        // wake up other threads to do the work
        if (nReadyTasks >= m_nThreads - 1) // we're using one thread
        {
            m_cvWork.notify_all();
        }
        else
        {
            while (nReadyTasks--)
                m_cvWork.notify_one();
        }


        // sleep until it's time to do the next work. use a wait_for instead of a sleep so we can can be woken
        // early if new work gets added that needs to occur sooner that we were expecting.
        {
            std::unique_lock<std::mutex> lock(m_oMutex);

            // no more delayed tasks, free up the thread for other work
            if (m_vDelayedTasks.empty())
                break;

            const auto tNext = m_vDelayedTasks.front().tWhen - pClock.UpTime();
            if (tNext > tZeroMilliseconds)
                m_cvDelayedWork.wait_for(lock, tNext);
        }
    }
}

void ThreadPool::Shutdown(bool bWait) noexcept
{
    m_bShutdownInitiated = true;
    m_cvDelayedWork.notify_all();
    m_cvWork.notify_all();

    if (bWait && !m_vThreads.empty())
    {
        RA_LOG_INFO("Waiting for background threads");

        for (auto& pThread : m_vThreads)
            pThread.join();

        RA_LOG_INFO("Background threads finished");

        m_vThreads.clear();
    }

}

} // namespace impl
} // namespace services
} // namespace ra
