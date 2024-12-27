#ifndef RA_UI_VIEW_MODEL_COLLECTION_H
#define RA_UI_VIEW_MODEL_COLLECTION_H
#pragma once

#include "ViewModelBase.hh"

#include "data\ModelCollectionBase.hh"

namespace ra {
namespace ui {

class ViewModelCollectionBase : public ra::data::ModelCollectionBase
{
public:
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
        {
        }

        virtual void
            OnViewModelStringValueChanged([[maybe_unused]] gsl::index nIndex,
                [[maybe_unused]] const StringModelProperty::ChangeArgs& args) noexcept(false)
        {
        }

        virtual void
            OnViewModelIntValueChanged([[maybe_unused]] gsl::index nIndex,
                [[maybe_unused]] const IntModelProperty::ChangeArgs& args) noexcept(false)
        {
        }

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

    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept(false)
    {
#ifdef RA_UTEST
        Expects(!m_bDisposed);
#endif

        if (!m_vNotifyTargets.empty())
        {
            m_vNotifyTargets.erase(&pTarget);

            if (m_vNotifyTargets.empty())
                StopWatching();
        }
    }

    /// <summary>
    /// Gets the <see cref="ViewModelBase" /> at the specified index.
    /// </summary>
    ViewModelBase* GetViewModelAt(gsl::index nIndex) { return dynamic_cast<ViewModelBase*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the <see cref="ViewModelBase" /> at the specified index.
    /// </summary>
    const ViewModelBase* GetViewModelAt(gsl::index nIndex) const { return dynamic_cast<const ViewModelBase*>(GetModelAt(nIndex)); }

protected:
    void OnModelValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnModelValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnModelValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;

    bool IsWatching() const noexcept override
    {
        return !IsFrozen() && !m_vNotifyTargets.empty();
    }

    void OnFrozen() noexcept override;
    void OnBeginUpdate() override;
    void OnEndUpdate() override;

    void OnItemsRemoved(const std::vector<gsl::index>& vDeletedIndices) override;
    void OnItemsAdded(const std::vector<gsl::index>& vNewIndices) override;
    void OnItemsChanged(const std::vector<gsl::index>& vChangedIndices) override;

private:
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
        return dynamic_cast<T&>(ModelCollectionBase::AddItem(std::move(pItem)));
    }

    /// <summary>
    /// Adds an item to the end of the collection.
    /// </summary>
    template<class T2, class... Args>
    T2& Add(Args&&... args)
    {
        static_assert(std::is_base_of<T, T2>::value, "T2 not derived from base class T");

        auto pItem = std::make_unique<T2>(std::forward<Args>(args)...);
        return dynamic_cast<T2&>(ModelCollectionBase::AddItem(std::move(pItem)));
    }

    /// <summary>
    /// Adds an item to the end of the collection.
    /// </summary>
    /// <remarks>
    /// This function is called Append instead of Add to prevent confusion with the templated parameters.
    /// </summary>
    T& Append(std::unique_ptr<T> pItem)
    {
        return dynamic_cast<T&>(ModelCollectionBase::AddItem(std::move(pItem)));
    }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    T* GetItemAt(gsl::index nIndex) { return dynamic_cast<T*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    const T* GetItemAt(gsl::index nIndex) const { return dynamic_cast<const T*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    template<class T2>
    T2* GetItemAt(gsl::index nIndex) { return dynamic_cast<T2*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    template<class T2>
    const T2* GetItemAt(gsl::index nIndex) const { return dynamic_cast<const T2*>(GetModelAt(nIndex)); }
};

} // namespace ui
} // namespace ra

#endif RA_UI_VIEW_MODEL_COLLECTION_H
