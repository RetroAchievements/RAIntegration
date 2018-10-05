#include "ThreadPool.hh"

#include "RA_Log.h"

namespace ra {
namespace services {
namespace impl {

ThreadPool::~ThreadPool()
{
    Shutdown(true);
}

void ThreadPool::Initialize(size_t nThreads) noexcept
{
    assert(m_vThreads.empty());

    RA_LOG_INFO("Initializing %zu worker threads", nThreads);

    for (size_t i = 0; i < nThreads; ++i)
        m_vThreads.emplace_back(&ThreadPool::RunThread, this);
}

void ThreadPool::RunThread() noexcept
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
                m_vQueue.pop();
            }

            // do work
            try
            {
                pNext();
            }
            catch (std::exception ex)
            {
                RA_LOG_ERR("Exception on background thread: %s", ex.what());
            }
        } while (true);

        // wait for work
        if (!m_bShutdownInitiated)
        {
            std::unique_lock<std::mutex> lock(m_oMutex);
            m_cvMutex.wait(lock);
        }
    } while (!m_bShutdownInitiated);
}

void ThreadPool::Shutdown(bool bWait) noexcept
{
    m_bShutdownInitiated = true;
    m_cvMutex.notify_all();

    if (bWait && !m_vThreads.empty())
    {
        RA_LOG_INFO("Waiting for background threads");

        for (size_t i = 0; i < m_vThreads.size(); ++i)
            m_vThreads[i].join();

        RA_LOG_INFO("Background threads finished");

        m_vThreads.clear();
    }

}

} // namespace impl
} // namespace services
} // namespace ra
