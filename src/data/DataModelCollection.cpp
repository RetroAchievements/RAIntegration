#include "DataModelCollection.hh"

namespace ra {
namespace data {

void DataModelCollectionBase::OnFrozen() noexcept
{
    m_vNotifyTargets.clear();
}

void DataModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
                                                  const BoolModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnDataModelBoolValueChanged(nIndex, args);
    }
}

void DataModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
                                                  const StringModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnDataModelStringValueChanged(nIndex, args);
    }
}

void DataModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
                                                  const IntModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnDataModelIntValueChanged(nIndex, args);
    }
}

void DataModelCollectionBase::OnBeginUpdate()
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnBeginDataModelCollectionUpdate();
    }
}

void DataModelCollectionBase::OnEndUpdate()
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnEndDataModelCollectionUpdate();
    }
}

void DataModelCollectionBase::OnItemsRemoved(const std::vector<gsl::index>& vDeletedIndices)
{
    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (auto nDeletedIndex : vDeletedIndices)
        {
            for (NotifyTarget* target : vNotifyTargets)
            {
                Expects(target != nullptr);
                target->OnDataModelRemoved(nDeletedIndex);
            }
        }
    }
}

void DataModelCollectionBase::OnItemsAdded(const std::vector<gsl::index>& vNewIndices)
{
    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (auto vNewIndex : vNewIndices)
        {
            for (NotifyTarget* target : vNotifyTargets)
            {
                Expects(target != nullptr);
                target->OnDataModelAdded(vNewIndex);
            }
        }
    }
}

void DataModelCollectionBase::OnItemsChanged(const std::vector<gsl::index>& vChangedIndices)
{
    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (auto vChangedIndex : vChangedIndices)
        {
            for (NotifyTarget* target : vNotifyTargets)
            {
                Expects(target != nullptr);
                target->OnDataModelChanged(vChangedIndex);
            }
        }
    }
}

} // namespace data
} // namespace ra
