#ifndef RA_DATA_NOTIFYTARGET_SET_H
#define RA_DATA_NOTIFYTARGET_SET_H
#pragma once

#include "ra_fwd.h"

namespace ra {
namespace data {

/// <summary>
/// A collection of pointers to other objects.
/// </summary>
/// <remarks>
/// These are not allocated object and do not need to be free'd. It's
/// impossible to create a set of references.
/// </summary>
template<class TNotifyTarget>
class NotifyTargetSet
{
public:
    /// <summary>
    /// Adds an object reference to the collection.
    /// </summary>
    /// <remarks>
    /// If the collection is <see cref="Lock">ed, the add will be queued.
    /// </remarks>
    void Add(TNotifyTarget& pTarget) noexcept
    {
        for (const auto pIter : m_vNotifyTargets)
        {
            if (pIter == &pTarget)
                return;
        }

        // emplace_back may throw exceptions if the move constructor throws exceptions.
        // Since we're only dealing with raw pointers, that will never happen.
        GSL_SUPPRESS_F6
        if (!m_bLocked)
        {
            m_vNotifyTargets.emplace_back(&pTarget);
        }
        else
        {
            for (auto& pPending : m_vPendingChanges)
            {
                if (pPending.first == &pTarget)
                {
                    pPending.second = true;
                    return;
                }
            }

            m_vPendingChanges.emplace_back(&pTarget, true);
        }
    }

    /// <summary>
    /// Removes an object reference from the collection.
    /// </summary>
    /// <remarks>
    /// If the collection is <see cref="Lock">ed, the remove will be queued.
    /// </remarks>
    void Remove(TNotifyTarget& pTarget) noexcept
    {
        for (const auto pIter : m_vNotifyTargets)
        {
            if (pIter == &pTarget)
            {
                // std::vector erase and emplace_back may throw exceptions if the move
                // constructor or assignment operator throw exceptions. Since we're
                // only dealing with raw pointers, that will never happen
                GSL_SUPPRESS_F6
                if (!m_bLocked)
                {
                    m_vNotifyTargets.erase(std::remove(m_vNotifyTargets.begin(), m_vNotifyTargets.end(), pIter), m_vNotifyTargets.end());
                }
                else
                {
                    for (auto& pPending : m_vPendingChanges)
                    {
                        if (pPending.first == &pTarget)
                        {
                            pPending.second = false;
                            return;
                        }
                    }

                    m_vPendingChanges.emplace_back(&pTarget, false);
                }

                return;
            }
        }

        for (auto& pPending : m_vPendingChanges)
        {
            if (pPending.first == &pTarget)
            {
                pPending.second = false;
                break;
            }
        }
    }

    /// <summary>
    /// Gets the objects in the collection.
    /// </summary>
    /// <remarks>
    /// Should be called within a <see cref="Lock"> block to ensure the collection
    /// isn't modified while it's being processed.
    /// </remarks>
    const std::vector<gsl::not_null<TNotifyTarget*>>& Targets() const noexcept
    {
        return m_vNotifyTargets;
    }

    /// <summary>
    /// Locks the collection while it's being processed.
    /// </summary>
    /// <returns>
    /// <c>true</c> if the collection was locked,
    /// <c>false</c> if the collection is empty and was not locked.
    /// </returns>
    /// <remarks>
    /// Changes made to the collection while locked won't appear until the collection is unlocked.
    /// </remarks>
    bool LockIfNotEmpty() noexcept
    {
        if (m_vNotifyTargets.empty())
            return false;

        m_bLocked = true;
        return true;
    }

    /// <summary>
    /// Locks the collection while it's being processed.
    /// </summary>
    /// <remarks>
    /// Changes made to the collection while locked won't appear until the collection is unlocked.
    /// </remarks>
    void Lock() noexcept
    {
        m_bLocked = true;
    }

    /// <summary>
    /// Unlocks the collection and applies any pending changes.
    /// </summary>
    void Unlock()
    {
        m_bLocked = false;

        if (!m_vPendingChanges.empty())
        {
            for (const auto pIter : m_vPendingChanges)
            {
                if (pIter.second)
                    m_vNotifyTargets.emplace_back(pIter.first);
                else
                    m_vNotifyTargets.erase(std::remove(m_vNotifyTargets.begin(), m_vNotifyTargets.end(), pIter.first), m_vNotifyTargets.end());
            }

            m_vPendingChanges.clear();
        }
    }

private:
    std::vector<gsl::not_null<TNotifyTarget*>> m_vNotifyTargets;
    std::vector<std::pair<gsl::not_null<TNotifyTarget*>, bool>> m_vPendingChanges;
    bool m_bLocked = false;
};

} // namespace data
} // namespace ra

#endif RA_DATA_NOTIFYTARGET_SET_H
