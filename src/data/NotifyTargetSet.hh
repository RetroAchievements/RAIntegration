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
    using TargetList = std::vector<TNotifyTarget*>;

public:
    /// <summary>
    /// Adds an object reference to the collection.
    /// </summary>
    void Add(TNotifyTarget& pTarget) noexcept
    {
        GSL_SUPPRESS_F6 // operator == for pointers won't throw an exception
        auto pIter = std::find(m_vNotifyTargets.begin(), m_vNotifyTargets.end(), &pTarget);
        if (pIter == m_vNotifyTargets.end())
        {
            GSL_SUPPRESS_F6 // copy/move constructors for pointers won't throw exceptions
            m_vNotifyTargets.push_back(&pTarget);
        }
    }

    /// <summary>
    /// Removes an object reference from the collection.
    /// </summary>
    void Remove(TNotifyTarget& pTarget) noexcept
    {
        GSL_SUPPRESS_F6 // operator == for pointers won't throw an exception
        auto pIter = std::find(m_vNotifyTargets.begin(), m_vNotifyTargets.end(), &pTarget);
        if (pIter != m_vNotifyTargets.end())
            m_vNotifyTargets.erase(pIter);
    }

    /// <summary>
    /// Gets whether the collection contains no items.
    /// </summary>
    bool IsEmpty() const noexcept
    {
        return m_vNotifyTargets.empty();
    }

    /// <summary>
    /// Removes all objects from the collection.
    /// </summary>
    void Clear() noexcept
    {
        m_vNotifyTargets.clear();
    }

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
        if (!m_pTargetListChain)
            return ValidTargets(m_vNotifyTargets);

        return ValidTargets(m_pTargetListChain->vTargetList);
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
    bool LockIfNotEmpty()
    {
        if (m_vNotifyTargets.empty())
            return false;

        Lock();
        return true;
    }

    /// <summary>
    /// Locks the collection while it's being processed.
    /// </summary>
    /// <remarks>
    /// Changes made to the collection while locked won't appear until the collection is unlocked.
    /// </remarks>
    void Lock()
    {
        auto pTargetListChain = std::make_unique<TargetListChain>(m_vNotifyTargets);
        pTargetListChain->pNext = std::move(m_pTargetListChain);
        m_pTargetListChain = std::move(pTargetListChain);
    }

    /// <summary>
    /// Unlocks the collection and applies any pending changes.
    /// </summary>
    void Unlock() noexcept
    {
        if (m_pTargetListChain != nullptr)
            m_pTargetListChain = std::move(m_pTargetListChain->pNext);
    }

private:
    TargetList m_vNotifyTargets;

    struct TargetListChain
    {
    public:
        TargetListChain(const TargetList& vTargetList)
            : vTargetList(vTargetList.begin(), vTargetList.end())
        {
        }
        ~TargetListChain() noexcept = default;

        TargetListChain(const TargetListChain&) noexcept = delete;
        TargetListChain& operator=(const TargetListChain&) noexcept = delete;
        TargetListChain(TargetListChain&&) noexcept = default;
        TargetListChain& operator=(TargetListChain&&) noexcept = default;

        const TargetList vTargetList;
        std::unique_ptr<TargetListChain> pNext;
    };

    std::unique_ptr<TargetListChain> m_pTargetListChain;
};

} // namespace data
} // namespace ra

#endif RA_DATA_NOTIFYTARGET_SET_H
