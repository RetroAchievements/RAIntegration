#include "ViewModelCollection.hh"

namespace ra {
namespace ui {

ViewModelBase& ViewModelCollectionBase::AddItem(std::unique_ptr<ViewModelBase> vmViewModel)
{
    assert(!IsFrozen());

    auto& pItem = m_vItems.emplace_back(*this, m_vItems.size(), std::move(vmViewModel));

    if (IsWatching())
    {
        pItem.StartWatching();

        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnViewModelAdded(pItem.Index());
        }
    }

    return pItem.ViewModel();
}

void ViewModelCollectionBase::RemoveAt(gsl::index nIndex)
{
    if (nIndex >= 0 && ra::to_unsigned(nIndex) < m_vItems.size())
    {
        assert(!IsFrozen());

        auto pIter = m_vItems.begin() + nIndex;
        if (IsWatching())
            pIter->StopWatching();

        pIter = m_vItems.erase(pIter);

        for (; pIter != m_vItems.end(); ++pIter)
            pIter->DecrementIndex();

        if (IsWatching())
        {
            // create a copy of the list of pointers in case it's modified by one of the callbacks
            NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
            for (NotifyTarget* target : vNotifyTargets)
            {
                Expects(target != nullptr);
                target->OnViewModelRemoved(nIndex);
            }
        }
    }
}

void ViewModelCollectionBase::StartWatching() noexcept
{
    for (auto& pItem : m_vItems)
        pItem.StartWatching();
}

void ViewModelCollectionBase::StopWatching() noexcept
{
    for (auto& pItem : m_vItems)
        pItem.StopWatching();
}

void ViewModelCollectionBase::OnViewModelBoolValueChanged(gsl::index nIndex,
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

void ViewModelCollectionBase::OnViewModelStringValueChanged(gsl::index nIndex,
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

void ViewModelCollectionBase::OnViewModelIntValueChanged(gsl::index nIndex,
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

} // namespace ui
} // namespace ra
