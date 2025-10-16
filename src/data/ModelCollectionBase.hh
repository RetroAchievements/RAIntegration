#ifndef RA_MODEL_COLLECTION_BASE_H
#define RA_MODEL_COLLECTION_BASE_H
#pragma once

#include "ModelBase.hh"

#include "ra_utility.h"

namespace ra {
namespace data {

class ModelCollectionBase
{
protected:
    GSL_SUPPRESS_F6 ModelCollectionBase() = default;

public:
    virtual ~ModelCollectionBase() noexcept
    {
#ifdef RA_UTEST
        m_bDisposed = true;
#endif

        if (!m_bFrozen)
            StopWatching();
    }

    ModelCollectionBase(const ModelCollectionBase&) noexcept = delete;
    ModelCollectionBase& operator=(const ModelCollectionBase&) noexcept = delete;
    GSL_SUPPRESS_F6 ModelCollectionBase(ModelCollectionBase&&) = default;
    GSL_SUPPRESS_F6 ModelCollectionBase& operator=(ModelCollectionBase&&) = default;

    /// <summary>
    /// Gets whether or not the collection has been frozen.
    /// </summary>
    bool IsFrozen() const noexcept { return m_bFrozen; }

    /// <summary>
    /// Indicates that the collection will not change in the future, so change events don't have to be propagated.
    /// </summary>
    void Freeze()
    {
        if (!m_bFrozen)
        {
            m_bFrozen = true;
            StopWatching();

            OnFrozen();
        }
    }

    /// <summary>
    /// Gets the number of items in the collection.
    /// </summary>
    size_t Count() const noexcept { return m_nSize; }

    /// <summary>
    /// Removes the item at the specified index.
    /// </summary>
    void RemoveAt(gsl::index nIndex);

    /// <summary>
    /// Removes all items from the collection.
    /// </summary>
    void Clear();

    /// <summary>
    /// Moves an item from one index to another.
    /// </summary>
    void MoveItem(gsl::index nIndex, gsl::index nNewIndex);

    /// <summary>
    /// Moves any items matching the provided filter immediately before the first item matching the filter.
    /// </summary>
    /// <param name="pProperty">The property that determines if the item should be moved.</param>
    /// <returns>The number of items that matched the filter.</returns>
    size_t ShiftItemsUp(const BoolModelProperty& pProperty);

    /// <summary>
    /// Moves any items matching the provided filter immediately after the last item matching the filter.
    /// </summary>
    /// <param name="pProperty">The property that determines if the item should be moved.</param>
    /// <returns>The number of items that matched the filter.</returns>
    size_t ShiftItemsDown(const BoolModelProperty& pProperty);

    /// <summary>
    /// Reverses the items in the list.
    /// </summary>
    void Reverse();

    /// <summary>
    /// Gets the value associated to the requested boolean property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to query.</param>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    bool GetItemValue(gsl::index nIndex, const BoolModelProperty& pProperty) const
    {
        const auto* pModel = GetModelAt(nIndex);
        return (pModel != nullptr) ? pModel->GetValue(pProperty) : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Gets the value associated to the requested string property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to query.</param>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    const std::wstring& GetItemValue(gsl::index nIndex, const StringModelProperty& pProperty) const
    {
        const auto* pModel = GetModelAt(nIndex);
        return (pModel != nullptr) ? pModel->GetValue(pProperty) : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Gets the value associated to the requested integer property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to query.</param>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    int GetItemValue(gsl::index nIndex, const IntModelProperty& pProperty) const
    {
        const auto* pModel = GetModelAt(nIndex);
        return (pModel != nullptr) ? pModel->GetValue(pProperty) : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Sets the value associated to the requested boolean property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to update.</param>
    /// <param name="pProperty">The property to update.</param>
    /// <param name="bValue">The new value for the property.</param>
    void SetItemValue(gsl::index nIndex, const BoolModelProperty& pProperty, bool bValue)
    {
        auto* pModel = GetModelAt(nIndex);
        if (pModel != nullptr)
            pModel->SetValue(pProperty, bValue);
    }

    /// <summary>
    /// Sets the value associated to the requested boolean property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to update.</param>
    /// <param name="pProperty">The property to update.</param>
    /// <param name="bValue">The new value for the property.</param>
    void SetItemValue(gsl::index nIndex, const StringModelProperty& pProperty, const std::wstring& sValue)
    {
        auto* pModel = GetModelAt(nIndex);
        if (pModel != nullptr)
            pModel->SetValue(pProperty, sValue);
    }

    /// <summary>
    /// Sets the value associated to the requested boolean property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to update.</param>
    /// <param name="pProperty">The property to update.</param>
    /// <param name="bValue">The new value for the property.</param>
    void SetItemValue(gsl::index nIndex, const IntModelProperty& pProperty, int nValue)
    {
        auto* pModel = GetModelAt(nIndex);
        if (pModel != nullptr)
            pModel->SetValue(pProperty, nValue);
    }

    /// <summary>
    /// Finds the index of the first item where the specified property has the specified value.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <param name="nValue">The value to find.</param>
    /// <returns>Index of the first matching item, <c>-1</c> if not found.</returns>
    gsl::index FindItemIndex(const IntModelProperty& pProperty, int nValue) const
    {
        for (gsl::index nIndex = 0; nIndex < gsl::narrow<gsl::index>(m_nSize); ++nIndex)
        {
            if (m_vItems.at(nIndex)->GetValue(pProperty) == nValue)
                return nIndex;
        }

        return -1;
    }

    /// <summary>
    /// Finds the index of the first item where the specified property has the specified value.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <param name="sValue">The value to find.</param>
    /// <returns>Index of the first matching item, <c>-1</c> if not found.</returns>
    gsl::index FindItemIndex(const StringModelProperty& pProperty, const std::wstring& sValue) const
    {
        for (gsl::index nIndex = 0; nIndex < gsl::narrow<gsl::index>(m_nSize); ++nIndex)
        {
            if (m_vItems.at(nIndex)->GetValue(pProperty) == sValue)
                return nIndex;
        }

        return -1;
    }

    /// <summary>
    /// Finds the index of the first item where the specified property has the specified value.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <param name="bValue">The value to find.</param>
    /// <returns>Index of the first matching item, <c>-1</c> if not found.</returns>
    gsl::index FindItemIndex(const BoolModelProperty& pProperty, bool bValue) const
    {
        for (gsl::index nIndex = 0; nIndex < gsl::narrow<gsl::index>(m_nSize); ++nIndex)
        {
            if (m_vItems.at(nIndex)->GetValue(pProperty) == bValue)
                return nIndex;
        }

        return -1;
    }

    /// <summary>
    /// Calls the OnBeginModelCollectionUpdate method of any attached NotifyTargets.
    /// </summary>
    void BeginUpdate();

    /// <summary>
    /// Calls the OnEndModelCollectionUpdate method of any attached NotifyTargets.
    /// </summary>
    void EndUpdate();

    /// <summary>
    /// Determines if the collection is being updated.
    /// </summary>
    bool IsUpdating() const noexcept { return (m_nUpdateCount > 0); }

protected:
    virtual void OnFrozen() noexcept(false) {}
    virtual void OnBeginUpdate() noexcept(false) {}
    virtual void OnEndUpdate() noexcept(false) {}
    virtual void OnBeforeItemRemoved(_UNUSED ModelBase& vmModel) noexcept(false) {}
    virtual void OnItemsRemoved(_UNUSED const std::vector<gsl::index>& vDeletedIndices) noexcept(false) {}
    virtual void OnItemsAdded(_UNUSED const std::vector<gsl::index>& vNewIndices) noexcept(false) {}
    virtual void OnItemsChanged(_UNUSED const std::vector<gsl::index>& vChangedIndices) noexcept(false) {}
    virtual void OnModelValueChanged(_UNUSED gsl::index nIndex, _UNUSED const BoolModelProperty::ChangeArgs& args) noexcept(false) {}
    virtual void OnModelValueChanged(_UNUSED gsl::index nIndex, _UNUSED const StringModelProperty::ChangeArgs& args) noexcept(false) {}
    virtual void OnModelValueChanged(_UNUSED gsl::index nIndex, _UNUSED const IntModelProperty::ChangeArgs& args) noexcept(false) {}

    ModelBase& AddItem(std::unique_ptr<ModelBase> vmModel);
    void MoveItemInternal(gsl::index nIndex, gsl::index nNewIndex);

    virtual bool IsWatching() const noexcept { return !IsFrozen(); }
    void StartWatching() noexcept;
    void StopWatching() noexcept;

    ModelBase* GetModelAt(gsl::index nIndex)
    {
        if (nIndex >= 0 && ra::to_unsigned(nIndex) < m_nSize)
            return m_vItems.at(nIndex).get();

        return nullptr;
    }

    const ModelBase* GetModelAt(gsl::index nIndex) const
    {
        if (nIndex >= 0 && ra::to_unsigned(nIndex) < m_nSize)
            return m_vItems.at(nIndex).get();

        return nullptr;
    }

#ifdef RA_UTEST
    bool m_bDisposed = false;
#endif

    template<class TItem>
    class iterator : public std::iterator_traits<typename  std::vector<std::unique_ptr<ModelBase>>::iterator>
    {
        static_assert(std::is_base_of<ModelBase, TItem>{}, "T must be a subclass of ModelBase");

    public:
        iterator(typename std::vector<std::unique_ptr<ModelBase>>::const_iterator pCurrent, const std::vector<std::unique_ptr<ModelBase>>& pList) noexcept
            : m_pCurrent(std::move(pCurrent)), m_pList(pList)
        {
            SkipMistyped();
        }

        TItem& operator*() noexcept
        {
            return *(dynamic_cast<TItem*>(m_pCurrent->get()));
        }

        iterator& operator++() noexcept
        {
            ++m_pCurrent;
            SkipMistyped();
            return *this;
        }

        iterator operator++(int) noexcept
        {
            auto pBeforeIncrement = *this;
            m_pCurrent++;
            SkipMistyped();
            return pBeforeIncrement;
        }

        bool operator==(const iterator& that) const noexcept
        {
            return m_pCurrent == that.m_pCurrent;
        }

        bool operator!=(const iterator& that) const noexcept
        {
            return !(*this == that);
        }

    private:
        void SkipMistyped() noexcept
        {
            while (m_pCurrent != m_pList.end() && dynamic_cast<const TItem*>(m_pCurrent->get()) == nullptr)
                ++m_pCurrent;
        }

        typename std::vector<std::unique_ptr<ModelBase>>::const_iterator m_pCurrent;
        const std::vector<std::unique_ptr<ModelBase>>& m_pList;
    };

    template<class TItem>
    iterator<TItem> CreateBeginIterator() noexcept
    {
        return iterator<TItem>(m_vItems.cbegin(), m_vItems);
    }

    template<class TItem>
    iterator<TItem> CreateEndIterator() noexcept
    {
        return iterator<TItem>(m_vItems.cbegin() + m_nSize, m_vItems);
    }

    template<class TItem>
    class const_iterator : public std::iterator_traits<typename  std::vector<std::unique_ptr<ModelBase>>::const_iterator>
    {
        static_assert(std::is_base_of<ModelBase, TItem>{}, "T must be a subclass of ModelBase");

    public:
        const_iterator(typename std::vector<std::unique_ptr<ModelBase>>::const_iterator pCurrent, const std::vector<std::unique_ptr<ModelBase>>& pList) noexcept
            : m_pCurrent(std::move(pCurrent)), m_pList(pList)
        {
            SkipMistyped();
        }

        const TItem& operator*() noexcept
        {
            return *(dynamic_cast<const TItem*>(m_pCurrent->get()));
        }

        const_iterator& operator++() noexcept
        {
            ++m_pCurrent;
            SkipMistyped();
            return *this;
        }

        const_iterator operator++(int) noexcept
        {
            auto pBeforeIncrement = *this;
            m_pCurrent++;
            SkipMistyped();
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
        void SkipMistyped() noexcept
        {
            while (m_pCurrent != m_pList.end() && dynamic_cast<const TItem*>(m_pCurrent->get()) == nullptr)
                ++m_pCurrent;
        }

        typename std::vector<std::unique_ptr<ModelBase>>::const_iterator m_pCurrent;
        const std::vector<std::unique_ptr<ModelBase>>& m_pList;
    };

    template<class TItem>
    const_iterator<TItem> CreateConstBeginIterator() const noexcept
    {
        return const_iterator<TItem>(m_vItems.cbegin(), m_vItems);
    }

    template<class TItem>
    const_iterator<TItem> CreateConstEndIterator() const noexcept
    {
        return const_iterator<TItem>(m_vItems.cbegin() + m_nSize, m_vItems);
    }

private:
    void UpdateIndices();
    void StartWatching(ModelBase& pModel, gsl::index nIndex) noexcept;
    void StopWatching(ModelBase& pModel) noexcept;

    // allow ModelBase to call NotifyModelValueChanged
    friend class ModelBase;
    void NotifyModelValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args);
    void NotifyModelValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args);
    void NotifyModelValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args);

    bool m_bFrozen = false;
    unsigned int m_nUpdateCount = 0;
    size_t m_nSize = 0;

    std::vector<std::unique_ptr<ModelBase>> m_vItems;
};

} // namespace data
} // namespace ra

#endif RA_MODEL_COLLECTION_BASE_H
