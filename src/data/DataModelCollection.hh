#ifndef RA_DATA_MODEL_COLLECTION_H
#define RA_DATA_MODEL_COLLECTION_H
#pragma once

#include "ModelCollectionBase.hh"
#include "DataModelBase.hh"

namespace ra {
namespace data {

class DataModelCollectionBase : public ModelCollectionBase
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
            OnDataModelBoolValueChanged([[maybe_unused]] gsl::index nIndex,
                [[maybe_unused]] const BoolModelProperty::ChangeArgs& args) noexcept(false)
        {
        }

        virtual void
            OnDataModelStringValueChanged([[maybe_unused]] gsl::index nIndex,
                [[maybe_unused]] const StringModelProperty::ChangeArgs& args) noexcept(false)
        {
        }

        virtual void
            OnDataModelIntValueChanged([[maybe_unused]] gsl::index nIndex,
                [[maybe_unused]] const IntModelProperty::ChangeArgs& args) noexcept(false)
        {
        }

        virtual void OnDataModelAdded([[maybe_unused]] gsl::index nIndex) noexcept(false) {}
        virtual void OnDataModelRemoved([[maybe_unused]] gsl::index nIndex) noexcept(false) {}
        virtual void OnDataModelChanged([[maybe_unused]] gsl::index nIndex) noexcept(false) {}

        virtual void OnBeginDataModelCollectionUpdate() noexcept(false) {}
        virtual void OnEndDataModelCollectionUpdate() noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept
    {
        if (!IsFrozen())
        {
            if (m_vNotifyTargets.IsEmpty())
                StartWatching();

            m_vNotifyTargets.Add(pTarget);
        }
    }

    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept(false)
    {
#ifdef RA_UTEST
        Expects(!m_bDisposed);
#endif

        if (!m_vNotifyTargets.IsEmpty())
        {
            m_vNotifyTargets.Remove(pTarget);

            if (m_vNotifyTargets.IsEmpty())
                StopWatching();
        }
    }

protected:
    void OnModelValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnModelValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnModelValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;

    bool IsWatching() const noexcept override 
    { 
        return !IsFrozen() && !m_vNotifyTargets.IsEmpty();
    }

    void OnFrozen() noexcept override;
    void OnBeginUpdate() override;
    void OnEndUpdate() override;

    void OnItemsRemoved(const std::vector<gsl::index>& vDeletedIndices) override;
    void OnItemsAdded(const std::vector<gsl::index>& vNewIndices) override;
    void OnItemsChanged(const std::vector<gsl::index>& vChangedIndices) override;

private:
    NotifyTargetSet<NotifyTarget> m_vNotifyTargets;
};

template<class T>
class DataModelCollection : public DataModelCollectionBase
{
    static_assert(std::is_base_of<DataModelBase, T>{}, "T must be a subclass of DataModelBase");

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

    auto begin() noexcept { return CreateBeginIterator<T>(); }
    auto end() noexcept { return CreateEndIterator<T>(); }
    auto begin() const noexcept { return CreateConstBeginIterator<T>(); }
    auto end() const noexcept { return CreateConstEndIterator<T>(); }
};

} // namespace data
} // namespace ra

#endif RA_DATA_MODEL_COLLECTION_H
