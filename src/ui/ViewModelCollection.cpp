#include "ViewModelCollection.hh"

namespace ra {
namespace ui {

void ViewModelCollectionBase::OnFrozen() noexcept
{
    m_vNotifyTargets.clear();
}

void ViewModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
    const BoolModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnViewModelBoolValueChanged(nIndex, args);
    }
}

void ViewModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
    const StringModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnViewModelStringValueChanged(nIndex, args);
    }
}

void ViewModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
    const IntModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnViewModelIntValueChanged(nIndex, args);
    }
}

void ViewModelCollectionBase::OnBeginUpdate()
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnBeginViewModelCollectionUpdate();
    }
}

void ViewModelCollectionBase::OnEndUpdate()
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnEndViewModelCollectionUpdate();
    }
}

void ViewModelCollectionBase::OnItemsRemoved(const std::vector<gsl::index>& vDeletedIndices)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (auto nDeletedIndex : vDeletedIndices)
    {
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnViewModelRemoved(nDeletedIndex);
        }
    }
}

void ViewModelCollectionBase::OnItemsAdded(const std::vector<gsl::index>& vNewIndices)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (auto vNewIndex : vNewIndices)
    {
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnViewModelAdded(vNewIndex);
        }
    }
}

void ViewModelCollectionBase::OnItemsChanged(const std::vector<gsl::index>& vChangedIndices)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (auto vChangedIndex : vChangedIndices)
    {
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnViewModelChanged(vChangedIndex);
        }
    }
}

} // namespace ui
} // namespace ra
