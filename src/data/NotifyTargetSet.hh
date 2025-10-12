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
private:
    enum TargetState
    {
        Valid = 0,
        AddPending,
        RemovePending,
    };
    typedef struct Target
    {
        Target(TNotifyTarget& pTarget, TargetState nState)
            : pTarget(&pTarget), nState(nState)
        {
        }

        gsl::not_null<TNotifyTarget*> pTarget;
        TargetState nState;
    } Target;

    using TargetList = std::vector<Target>;

    class const_iterator : public std::iterator_traits<typename TargetList::const_iterator>
    {
    public:
        TNotifyTarget& operator*() { return *m_pCurrent->pTarget; }

        const_iterator& operator++()
        {
            ++m_pCurrent;
            Skip();
            return *this;
        }

        const_iterator operator++(int)
        {
            auto pBeforeIncrement = *this;
            m_pCurrent++;
            Skip();
            return pBeforeIncrement;
        }

        bool operator==(const const_iterator& that) const
        {
            return m_pCurrent == that.m_pCurrent;
        }

        bool operator!=(const const_iterator& that) const
        {
            return !(*this == that);
        }

    private:
        friend NotifyTargetSet;
        const_iterator(typename TargetList::const_iterator pCurrent, const TargetList& pOwner)
            : m_pCurrent(std::move(pCurrent)), m_pOwner(pOwner)
        {
            Skip();
        }

        void Skip()
        {
            while (m_pCurrent != m_pOwner.cend() && m_pCurrent->nState != TargetState::Valid)
                ++m_pCurrent;
        }

        typename TargetList::const_iterator m_pCurrent;
        const TargetList& m_pOwner;
    };

public:
    /// <summary>
    /// Adds an object reference to the collection.
    /// </summary>
    /// <remarks>
    /// If the collection is <see cref="Lock">ed, the add will be queued.
    /// </remarks>
    void Add(TNotifyTarget& pTarget) noexcept
    {
        for (auto& pIter : m_vNotifyTargets)
        {
            if (pIter.pTarget == &pTarget)
            {
                if (pIter.nState != TargetState::Valid)
                    pIter.nState = m_nLockCount ? TargetState::AddPending : TargetState::Valid;

                return;
            }
        }

        // emplace_back may throw exceptions if the move constructor throws exceptions.
        // Since we're only dealing with raw pointers, that will never happen.
        GSL_SUPPRESS_F6
        m_vNotifyTargets.emplace_back(pTarget, m_nLockCount ? TargetState::AddPending : TargetState::Valid);
    }

    /// <summary>
    /// Removes an object reference from the collection.
    /// </summary>
    /// <remarks>
    /// If the collection is <see cref="Lock">ed, the remove will be queued.
    /// </remarks>
    void Remove(TNotifyTarget& pTarget) noexcept
    {
        for (auto pIter = m_vNotifyTargets.begin(); pIter < m_vNotifyTargets.end(); ++pIter)
        {
            if (pIter->pTarget == &pTarget)
            {
                // std::vector erase and emplace_back may throw exceptions if the move
                // constructor or assignment operator throw exceptions. Since we're
                // only dealing with raw pointers, that will never happen.
                GSL_SUPPRESS_F6
                if (!m_nLockCount)
                    m_vNotifyTargets.erase(pIter);
                else
                    pIter->nState = TargetState::RemovePending;

                return;
            }
        }
    }

    /// <summary>
    /// Gets whether the collection contains no items.
    /// </summary>
    bool IsEmpty() const noexcept { return m_vNotifyTargets.empty(); }

    /// <summary>
    /// Removes all objects from the collection.
    /// </summary>
    void Clear() noexcept
    {
        if (!m_nLockCount)
        {
            m_vNotifyTargets.clear();
        }
        else
        {
            for (auto& pIter : m_vNotifyTargets)
                pIter.nState = TargetState::RemovePending;
        }
    }

    class ValidTargets
    {
    public:
        ValidTargets(const TargetList& pTargets)
            : pBegin(pTargets.cbegin(), pTargets), pEnd(pTargets.cend(), pTargets)
        {
        }

        auto begin() const { return pBegin; }
        auto end() const { return pEnd; }

        size_t size() const
        {
            size_t count = 0;
            for (auto pIter = begin(); pIter != end(); ++pIter)
                ++count;

            return count;
        }

    private:
        const_iterator pBegin;
        const_iterator pEnd;
    };

    /// <summary>
    /// Gets the objects in the collection.
    /// </summary>
    /// <remarks>
    /// Should be called within a <see cref="Lock"> block to ensure the collection
    /// isn't modified while it's being processed.
    /// </remarks>
    const ValidTargets Targets() const noexcept
    {
        return ValidTargets(m_vNotifyTargets);
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

        ++m_nLockCount;
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
        ++m_nLockCount;
    }

    /// <summary>
    /// Unlocks the collection and applies any pending changes.
    /// </summary>
    void Unlock()
    {
        if (!m_nLockCount || --m_nLockCount)
            return;

        auto pIter = m_vNotifyTargets.begin();
        while (pIter < m_vNotifyTargets.end())
        {
            switch (pIter->nState)
            {
                case TargetState::RemovePending:
                    pIter = m_vNotifyTargets.erase(pIter);
                    continue;

                case TargetState::AddPending:
                    pIter->nState = TargetState::Valid;
                    break;
            }

            ++pIter;
        }
    }

private:
    TargetList m_vNotifyTargets;
    int m_nLockCount = 0;
};

} // namespace data
} // namespace ra

#endif RA_DATA_NOTIFYTARGET_SET_H
