#ifndef RA_DATA_ASYNCOBJECT_HH
#define RA_DATA_ASYNCOBJECT_HH
#pragma once

#include "ra_fwd.h"

namespace ra {
namespace data {

/// <summary>
/// Handle owned by an <see cref="AsyncObject"/> for thread safety and destruction tracking.
/// </summary>
class AsyncHandle
{
public:
    /// <summary>
    /// Determines whether the <see cref="AsyncObject" /> that owns this handle is waiting to be destructed.
    /// </summary>
    /// <remarks>Allows for early exit in long running code keeping the object alive</remarks>
    bool IsDestroying() const noexcept { return m_bDestroying; }

    /// <summary>
    /// Determines whether the <see cref="AsyncObject" /> that owns this handle has been destructed.
    /// </summary>
    bool IsDestroyed() const noexcept { return m_bDestroyed; }

private:
    bool m_bDestroyed = false;
    bool m_bDestroying = false;

    // allow AsyncKeepAlive access to m_pMutex
    friend class AsyncKeepAlive;

    // allow AsyncObject access to m_pMutex and SetDestroyed()
    friend class AsyncObject;
    std::mutex m_pMutex;

    GSL_SUPPRESS_F6 void SetDestroyed() noexcept
    {
        // immediately mark as destroying
        m_bDestroying = true;

        // then wait for anything holding the lock
        std::lock_guard<std::mutex> guard(m_pMutex);

        // then mark as destroyed
        m_bDestroyed = true;
    }
};

/// <summary>
/// Helper object to ensure an <see cref="AsyncObject"/> is not destroyed while it is being used.
/// </summary>
class AsyncKeepAlive
{
public:
    AsyncKeepAlive(AsyncHandle& pAsyncHandle) : m_pLock(pAsyncHandle.m_pMutex) {}

private:
    std::lock_guard<std::mutex> m_pLock;
};

/// <summary>
/// Helper class for validating existance of "this" pointer in an asynchronous callback
/// </summary>
/// <example>
///     run_async([this, asyncHandle = CreateAsyncHandle()]{
///         AsyncKeepAlive keepAlive(*asyncHandle); // prevents destroying the object while we're in the callback
///         if (asyncHandle->IsDestroyed())
///             return;
///
///         do_stuff(this);
///     });
/// </example>
class AsyncObject
{
public:
    AsyncObject() noexcept = default;

    virtual ~AsyncObject() noexcept
    {
        // subclass destructor should have called BeginDestruction, which ensures this is set to nullptr.
        assert(m_pAsyncHandle == nullptr);
    }

    AsyncObject(const AsyncObject&) noexcept = default;
    AsyncObject& operator=(const AsyncObject&) noexcept = default;
    AsyncObject(AsyncObject&&) noexcept = default;
    AsyncObject& operator=(AsyncObject&&) noexcept = default;

protected:    
    /// <summary>
    /// Updates the <see cref="AsyncHandle" /> to indicate that the destruction process has begun. 
    /// Should be called by the destructor of the class that inherits from <see cref="AsyncObject" />
    /// so any default destruction of member fields is delayed while other thread may be running.
    /// </summary>
    void BeginDestruction() noexcept
    {
        if (m_pAsyncHandle)
        {
            m_pAsyncHandle->SetDestroyed();
            m_pAsyncHandle.reset();
        }

        assert(m_pAsyncHandle == nullptr);
    }
        
    /// <summary>
    /// Creates a handle to be passed to an async function so the function can validate that
    /// the <see cref="AsyncObject"/> is still valid when it runs.
    /// </summary>
    /// <returns></returns>
    std::shared_ptr<AsyncHandle> CreateAsyncHandle()
    {
        if (!m_pAsyncHandle)
            m_pAsyncHandle = std::make_shared<AsyncHandle>();

        return m_pAsyncHandle;
    }

    /// <summary>
    /// Marks a block of code as requiring exclusive access to the object.
    /// (prevents other threads from accessing their guarded blocks while the lock is held).
    /// </summary>
    class LockGuard
    {
    public:
        LockGuard(AsyncObject& pAsyncObject) : m_pLock(pAsyncObject.CreateAsyncHandle()->m_pMutex) {}

    private:
        std::lock_guard<std::mutex> m_pLock;
    };

private:
    std::shared_ptr<AsyncHandle> m_pAsyncHandle;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_ASYNCOBJECT_HH
