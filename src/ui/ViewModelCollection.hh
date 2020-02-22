#ifndef RA_UI_VIEW_MODEL_COLLECTION_H
#define RA_UI_VIEW_MODEL_COLLECTION_H
#pragma once

#include "ViewModelBase.hh"

#include "ra_utility.h"

namespace ra {
namespace ui {

class ViewModelCollectionBase
{
protected:
    GSL_SUPPRESS_F6 ViewModelCollectionBase() = default;

public:
    virtual ~ViewModelCollectionBase() noexcept
    {
        if (!m_bFrozen)
            StopWatching();
    }

    ViewModelCollectionBase(const ViewModelCollectionBase&) noexcept = delete;
    ViewModelCollectionBase& operator=(const ViewModelCollectionBase&) noexcept = delete;
    GSL_SUPPRESS_F6 ViewModelCollectionBase(ViewModelCollectionBase&&) = default;
    GSL_SUPPRESS_F6 ViewModelCollectionBase& operator=(ViewModelCollectionBase&&) = default;

    class NotifyTarget
    {
    public:
        NotifyTarget() noexcept = default;
        virtual ~NotifyTarget() noexcept = default;
        NotifyTarget(const NotifyTarget&) noexcept = delete;
        NotifyTarget& operator=(const NotifyTarget&) noexcept = delete;
        NotifyTarget(NotifyTarget&&) noexcept = default;
        NotifyTarget& operator=(NotifyTarget&&) noexcept = default;

        virtual void 
            OnViewModelBoolValueChanged([[maybe_unused]] gsl::index nIndex,
                                        [[maybe_unused]] const BoolModelProperty::ChangeArgs& args) noexcept(false)
        {}

        virtual void
            OnViewModelStringValueChanged([[maybe_unused]] gsl::index nIndex,
                                          [[maybe_unused]] const StringModelProperty::ChangeArgs& args) noexcept(false)
        {}

        virtual void 
            OnViewModelIntValueChanged([[maybe_unused]] gsl::index nIndex,
                                       [[maybe_unused]] const IntModelProperty::ChangeArgs& args) noexcept(false)
        {}

        virtual void OnViewModelAdded([[maybe_unused]] gsl::index nIndex) noexcept(false) {}
        virtual void OnViewModelRemoved([[maybe_unused]] gsl::index nIndex) noexcept(false) {}
        virtual void OnViewModelChanged([[maybe_unused]] gsl::index nIndex) noexcept(false) {}

        virtual void OnBeginViewModelCollectionUpdate() noexcept(false) {}
        virtual void OnEndViewModelCollectionUpdate() noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget)
    {
        if (!IsFrozen())
        {
            if (m_vNotifyTargets.empty())
                StartWatching();

            m_vNotifyTargets.insert(&pTarget);
        }
    }

    void RemoveNotifyTarget(NotifyTarget& pTarget)
    {
        if (!m_vNotifyTargets.empty())
        {
            m_vNotifyTargets.erase(&pTarget);

            if (m_vNotifyTargets.empty())
                StopWatching();
        }
    }

    /// <summary>
    /// Gets whether or not the collection has been frozen.
    /// </summary>
    bool IsFrozen() const noexcept { return m_bFrozen; }

    /// <summary>
    /// Indicates that the collection will not change in the future, so change events don't have to be propogated.
    /// </summary>
    void Freeze() noexcept
    {
        if (!m_bFrozen)
        {
            m_bFrozen = true;
            StopWatching();

            m_vNotifyTargets.clear();
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
        const auto* pViewModel = GetViewModelAt(nIndex);
        return (pViewModel != nullptr) ? pViewModel->GetValue(pProperty) : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Gets the value associated to the requested string property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to query.</param>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    const std::wstring& GetItemValue(gsl::index nIndex, const StringModelProperty& pProperty) const
    {
        const auto* pViewModel = GetViewModelAt(nIndex);
        return (pViewModel != nullptr) ? pViewModel->GetValue(pProperty) : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Gets the value associated to the requested integer property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to query.</param>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    int GetItemValue(gsl::index nIndex, const IntModelProperty& pProperty) const
    {
        const auto* pViewModel = GetViewModelAt(nIndex);
        return (pViewModel != nullptr) ? pViewModel->GetValue(pProperty) : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Sets the value associated to the requested boolean property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to update.</param>
    /// <param name="pProperty">The property to update.</param>
    /// <param name="bValue">The new value for the property.</param>
    void SetItemValue(gsl::index nIndex, const BoolModelProperty& pProperty, bool bValue)
    {
        auto* pViewModel = GetViewModelAt(nIndex);
        if (pViewModel != nullptr)
            pViewModel->SetValue(pProperty, bValue);
    }

    /// <summary>
    /// Sets the value associated to the requested boolean property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to update.</param>
    /// <param name="pProperty">The property to update.</param>
    /// <param name="bValue">The new value for the property.</param>
    void SetItemValue(gsl::index nIndex, const StringModelProperty & pProperty, const std::wstring& sValue)
    {
        auto* pViewModel = GetViewModelAt(nIndex);
        if (pViewModel != nullptr)
            pViewModel->SetValue(pProperty, sValue);
    }

    /// <summary>
    /// Sets the value associated to the requested boolean property for the item at the specified index.
    /// </summary>
    /// <param name="nIndex">The index of the item to update.</param>
    /// <param name="pProperty">The property to update.</param>
    /// <param name="bValue">The new value for the property.</param>
    void SetItemValue(gsl::index nIndex, const IntModelProperty & pProperty, int nValue)
    {
        auto* pViewModel = GetViewModelAt(nIndex);
        if (pViewModel != nullptr)
            pViewModel->SetValue(pProperty, nValue);
    }

    /// <summary>
    /// Finds the index of the first item where the specified property has the specified value.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <param name="nValue">The value to find.</param>
    /// <returns>Index of the first matching item, <c>-1</c> if not found.</returns>
    gsl::index FindItemIndex(const IntModelProperty& pProperty, int nValue) const
    {
        for (const auto& pItem : m_vItems)
        {
            if (pItem.ViewModel().GetValue(pProperty) == nValue)
                return pItem.Index();
        }

        return -1;
    }

    /// <summary>
    /// Calls the OnBeginViewModelCollectionUpdate method of any attached NotifyTargets.
    /// </summary>
    void BeginUpdate();

    /// <summary>
    /// Calls the OnEndViewModelCollectionUpdate method of any attached NotifyTargets.
    /// </summary>
    void EndUpdate();

    /// <summary>
    /// Determines if the collection is being updated.
    /// </summary>
    bool IsUpdating() const noexcept { return (m_nUpdateCount > 0); }

protected:
    ViewModelBase& AddItem(std::unique_ptr<ViewModelBase> vmViewModel);
    void MoveItemInternal(gsl::index nIndex, gsl::index nNewIndex);
    void UpdateIndices();

    bool IsWatching() const noexcept { return !IsFrozen() && !m_vNotifyTargets.empty(); }

    void StartWatching() noexcept;

    void StopWatching() noexcept;

    ViewModelBase* GetViewModelAt(gsl::index nIndex)
    {
        if (nIndex >= 0 && ra::to_unsigned(nIndex) < m_vItems.size())
            return &m_vItems.at(nIndex).ViewModel();

        return nullptr;
    }

    const ViewModelBase* GetViewModelAt(gsl::index nIndex) const
    {
        if (nIndex >= 0 && ra::to_unsigned(nIndex) < m_vItems.size())
            return &m_vItems.at(nIndex).ViewModel();

        return nullptr;
    }

private:
    class Item : private ViewModelBase::NotifyTarget
    {
    public:
        Item(ViewModelCollectionBase& pOwner, gsl::index nIndex, std::unique_ptr<ViewModelBase> vmViewModel) noexcept :
            m_vmViewModel(std::move(vmViewModel)),
            m_nIndex(nIndex),
            m_pOwner(&pOwner)
        {}

        ~Item() noexcept
        {
            StopWatching();
        }

        Item(const Item&) noexcept = delete;
        Item& operator=(const Item&) noexcept = delete;

        Item(Item&& pSource) noexcept :
            m_vmViewModel(std::move(pSource.m_vmViewModel)),
            m_nIndex(pSource.m_nIndex),
            m_pOwner(pSource.m_pOwner)
        {
            if (m_vmViewModel && m_pOwner->IsWatching())
            {
                m_vmViewModel->RemoveNotifyTarget(pSource);
                m_vmViewModel->AddNotifyTarget(*this);
            }
        }

        GSL_SUPPRESS_C128 Item& operator=(Item&& pSource) noexcept
        {
            if (&pSource != this)
            {
                m_vmViewModel = std::move(pSource.m_vmViewModel);
                m_nIndex = pSource.m_nIndex;
                m_pOwner = pSource.m_pOwner;

                if (m_vmViewModel && m_pOwner->IsWatching())
                {
                    m_vmViewModel->RemoveNotifyTarget(pSource);
                    m_vmViewModel->AddNotifyTarget(*this);
                }
            }

            ViewModelBase::NotifyTarget::operator=(std::move(pSource));
            return *this;
        }

        void StartWatching() noexcept
        {
            if (m_vmViewModel)
                m_vmViewModel->AddNotifyTarget(*this);
        }

        void StopWatching() noexcept
        {
            if (m_vmViewModel)
                m_vmViewModel->RemoveNotifyTarget(*this);
        }

        bool HasViewModel() const noexcept { return m_vmViewModel != nullptr; }
        ViewModelBase& ViewModel() noexcept { return *m_vmViewModel; }
        const ViewModelBase& ViewModel() const noexcept { return *m_vmViewModel; }

        ViewModelBase* DetachViewModel() noexcept
        {
            if (m_vmViewModel && m_pOwner->IsWatching())
                m_vmViewModel->RemoveNotifyTarget(*this);

            return m_vmViewModel.release();
        }

        void AttachViewModel(ViewModelBase* vmViewModel) noexcept
        {
            m_vmViewModel.reset(vmViewModel);

            if (vmViewModel != nullptr && m_pOwner->IsWatching())
                m_vmViewModel->AddNotifyTarget(*this);
        }

        gsl::index Index() const noexcept { return m_nIndex; }
        void SetIndex(gsl::index nIndex) noexcept { m_nIndex = nIndex; }

    private:
        void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override
        {
            m_pOwner->OnViewModelBoolValueChanged(m_nIndex, args);
        }

        void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override
        {
            m_pOwner->OnViewModelStringValueChanged(m_nIndex, args);
        }

        void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override
        {
            m_pOwner->OnViewModelIntValueChanged(m_nIndex, args);
        }

        std::unique_ptr<ViewModelBase> m_vmViewModel;
        gsl::index m_nIndex{};
        gsl::not_null<ViewModelCollectionBase*> m_pOwner;
    };

    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args);
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args);
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args);

    bool m_bFrozen = false;
    unsigned int m_nUpdateCount = 0;
    size_t m_nSize = 0;

    std::vector<Item> m_vItems;

    using NotifyTargetSet = std::set<NotifyTarget*>;

    /// <summary>
    /// A collection of pointers to other objects. These are not allocated object and do not need to be free'd. It's
    /// impossible to create a set of <c>NotifyTarget</c> references.
    /// </summary>
    NotifyTargetSet m_vNotifyTargets;
};

template<class T>
class ViewModelCollection : public ViewModelCollectionBase
{
    static_assert(std::is_base_of<ViewModelBase, T>{}, "T must be a subclass of ViewModelBase");

public:
    /// <summary>
    /// Adds an item to the end of the collection.
    /// </summary>
    template<class... Args>
    T& Add(Args&&... args)
    {
        auto pItem = std::make_unique<T>(std::forward<Args>(args)...);
        return dynamic_cast<T&>(ViewModelCollectionBase::AddItem(std::move(pItem)));
    }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    T* GetItemAt(gsl::index nIndex) { return dynamic_cast<T*>(GetViewModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    const T* GetItemAt(gsl::index nIndex) const { return dynamic_cast<const T*>(GetViewModelAt(nIndex)); }
};

} // namespace ui
} // namespace ra

#endif RA_UI_VIEW_MODEL_COLLECTION_H
