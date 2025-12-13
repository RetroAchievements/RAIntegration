#include "DataModelCollection.hh"

namespace ra {
namespace data {

void DataModelCollectionBase::OnFrozen() noexcept
{
    m_vNotifyTargets.Clear();
}

void DataModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
                                                  const BoolModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnDataModelBoolValueChanged(nIndex, args);

        m_vNotifyTargets.Unlock();
    }
}

void DataModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
                                                  const StringModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnDataModelStringValueChanged(nIndex, args);

        m_vNotifyTargets.Unlock();
    }
}

void DataModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
                                                  const IntModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnDataModelIntValueChanged(nIndex, args);

        m_vNotifyTargets.Unlock();
    }
}

void DataModelCollectionBase::OnBeginUpdate()
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnBeginDataModelCollectionUpdate();

        m_vNotifyTargets.Unlock();
    }
}

void DataModelCollectionBase::OnEndUpdate()
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnEndDataModelCollectionUpdate();

        m_vNotifyTargets.Unlock();
    }
}

void DataModelCollectionBase::OnItemsRemoved(const std::vector<gsl::index>& vDeletedIndices)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
        {
            for (auto nDeletedIndex : vDeletedIndices)
                target.OnDataModelRemoved(nDeletedIndex);
        }

        m_vNotifyTargets.Unlock();
    }
}

void DataModelCollectionBase::OnItemsAdded(const std::vector<gsl::index>& vNewIndices)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
        {
            for (auto vNewIndex : vNewIndices)
                target.OnDataModelAdded(vNewIndex);
        }

        m_vNotifyTargets.Unlock();
    }
}

void DataModelCollectionBase::OnItemsChanged(const std::vector<gsl::index>& vChangedIndices)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
        {
            for (auto vChangedIndex : vChangedIndices)
                target.OnDataModelChanged(vChangedIndex);
        }

        m_vNotifyTargets.Unlock();
    }
}

} // namespace data
} // namespace ra
