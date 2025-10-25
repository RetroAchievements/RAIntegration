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
/// These are not allocated objects and do not need to be free'd.
/// This behaves like a set of references, which isn't allowed.
/// </summary>
template<class TNotifyTarget>
class NotifyTargetSet
{
private:
    using TargetList = std::vector<TNotifyTarget*>;

public:
    class ValidTargets
    {
    public:
        ValidTargets(const TargetList& pTargets) noexcept
            : m_pBegin(pTargets.cbegin()), m_pEnd(pTargets.cend())
        {
        }
        ~ValidTargets() noexcept = default;

        ValidTargets(const ValidTargets&) noexcept = default;
        ValidTargets& operator=(const ValidTargets&) noexcept = default;
        ValidTargets(ValidTargets&&) noexcept = default;
        ValidTargets& operator=(ValidTargets&&) noexcept = default;

        class const_iterator : public std::iterator_traits<typename TargetList::const_iterator>
        {
        public:
            TNotifyTarget& operator*() noexcept { return *(*m_pCurrent); }

            const_iterator& operator++() noexcept
            {
                ++m_pCurrent;
                return *this;
            }

            const_iterator operator++(int)
            {
                auto pBeforeIncrement = *this;
                m_pCurrent++;
                return pBeforeIncrement;
            }

            bool operator==(const const_iterator& that) const noexcept
            {
                return m_pCurrent == that.m_pCurrent;
            }

            bool operator!=(const const_iterator& that) const noexcept
            {
                return !(*this == that);
            }

        private:
            friend class ValidTargets;
            const_iterator(typename TargetList::const_iterator pCurrent) noexcept
                : m_pCurrent(std::move(pCurrent))
            {
            }

            typename TargetList::const_iterator m_pCurrent;
        };

        auto begin() const noexcept { return m_pBegin; }
        auto end() const noexcept { return m_pEnd; }

        size_t size() const 
        {
            return m_pEnd.m_pCurrent - m_pBegin.m_pCurrent;
        }

    private:
        const_iterator m_pBegin;
        const_iterator m_pEnd;
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
        if (!m_pTargetList)
        {
            GSL_SUPPRESS_F6 m_pTargetList = std::make_unique<TargetListNode>();
        }

        return ValidTargets(m_pTargetList->vTargetList);
    }

    /// <summary>
    /// Adds an object reference to the collection.
    /// </summary>
    GSL_SUPPRESS_F6 // this should only throw an exception if we're out of memory
    void Add(TNotifyTarget& pTarget) noexcept
    {
        if (!m_pTargetList)
        {
            m_pTargetList = std::make_unique<TargetListNode>();
        }
        else
        {
            const auto& vNotifyTargets = m_pTargetList->vTargetList;
            auto pIter = std::find(vNotifyTargets.begin(), vNotifyTargets.end(), &pTarget);
            if (pIter != vNotifyTargets.end())
                return;

            EnsureMutable();
        }

        m_pTargetList->vTargetList.push_back(&pTarget);
    }

    /// <summary>
    /// Removes an object reference from the collection.
    /// </summary>
    GSL_SUPPRESS_F6 // processing collection of raw pointers should never throw an exception
    void Remove(TNotifyTarget& pTarget) noexcept
    {
        if (!m_pTargetList)
            return;

        gsl::not_null<TargetList*> vNotifyTargets = gsl::make_not_null(&m_pTargetList->vTargetList);
        auto pIter = std::find(vNotifyTargets->begin(), vNotifyTargets->end(), &pTarget);
        if (pIter == vNotifyTargets->end())
            return;

        EnsureMutable();

        if (vNotifyTargets != &m_pTargetList->vTargetList)
        {
            // m_pTargetList changed. find the element in the new list.
            vNotifyTargets = gsl::make_not_null(&m_pTargetList->vTargetList);
            pIter = std::find(vNotifyTargets->begin(), vNotifyTargets->end(), &pTarget);
        }

        if (pIter != vNotifyTargets->end())
            vNotifyTargets->erase(pIter);
    }

    /// <summary>
    /// Removes all objects from the collection.
    /// </summary>
    void Clear() noexcept
    {
        if (m_pTargetList)
        {
            EnsureMutable();
            m_pTargetList->vTargetList.clear();
        }
    }

    /// <summary>
    /// Gets whether the collection contains no items.
    /// </summary>
    bool IsEmpty() const noexcept
    {
        return !m_pTargetList || m_pTargetList->vTargetList.empty();
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
        if (IsEmpty())
            return false;

        ++m_pTargetList->nLockCount;
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
        if (!m_pTargetList)
        {
            GSL_SUPPRESS_F6 // this should only throw an exception if we're out of memory
            m_pTargetList = std::make_unique<TargetListNode>();
        }

        ++m_pTargetList->nLockCount;
    }

    /// <summary>
    /// Unlocks the collection and applies any pending changes.
    /// </summary>
    void Unlock() noexcept
    {
        if (!m_pTargetList)
            return;

        if (m_pTargetList->nLockCount > 0)
        {
            // list not modified. release lock
            --m_pTargetList->nLockCount;
        }
        else if (m_pTargetList->pNext)
        {
            // list modified. release lock on copy of list (in m_pTargetList->pNext).
            // if there are no more locks on the copy, discard it.
            if (--m_pTargetList->pNext->nLockCount == 0)
                m_pTargetList->pNext = std::move(m_pTargetList->pNext->pNext);
        }
    }

private:
    struct TargetListNode
    {
    public:
        TargetListNode() noexcept {}
        TargetListNode(const TargetList& vTargetList)
            : vTargetList(vTargetList)
        {
        }
        ~TargetListNode() noexcept = default;

        TargetListNode(const TargetListNode&) noexcept = delete;
        TargetListNode& operator=(const TargetListNode&) noexcept = delete;
        TargetListNode(TargetListNode&&) noexcept = default;
        TargetListNode& operator=(TargetListNode&&) noexcept = default;

        TargetList vTargetList;
        std::unique_ptr<TargetListNode> pNext;
        int nLockCount = 0;
    };

    mutable std::unique_ptr<TargetListNode> m_pTargetList;

    void EnsureMutable() noexcept
    {
        if (m_pTargetList->nLockCount)
        {
            // current list is locked, clone it so it can be mutated
            GSL_SUPPRESS_F6 auto pTargetList = std::make_unique<TargetListNode>(m_pTargetList->vTargetList);
            pTargetList->pNext = std::move(m_pTargetList);
            m_pTargetList = std::move(pTargetList);
        }
    }
};

} // namespace data
} // namespace ra

#endif RA_DATA_NOTIFYTARGET_SET_H
