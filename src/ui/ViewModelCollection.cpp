#include "ViewModelCollection.hh"

namespace ra {
namespace ui {

void ViewModelCollectionBase::OnFrozen() noexcept
{
    m_vNotifyTargets.Clear();
}

void ViewModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
    const BoolModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnViewModelBoolValueChanged(nIndex, args);

        m_vNotifyTargets.Unlock();
    }
}

void ViewModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
    const StringModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnViewModelStringValueChanged(nIndex, args);

        m_vNotifyTargets.Unlock();
    }
}

void ViewModelCollectionBase::OnModelValueChanged(gsl::index nIndex,
    const IntModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnViewModelIntValueChanged(nIndex, args);

        m_vNotifyTargets.Unlock();
    }
}

void ViewModelCollectionBase::OnBeginUpdate()
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnBeginViewModelCollectionUpdate();

        m_vNotifyTargets.Unlock();
    }
}

void ViewModelCollectionBase::OnEndUpdate()
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnEndViewModelCollectionUpdate();

        m_vNotifyTargets.Unlock();
    }
}

void ViewModelCollectionBase::OnItemsRemoved(const std::vector<gsl::index>& vDeletedIndices)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
        {
            for (auto nDeletedIndex : vDeletedIndices)
                target.OnViewModelRemoved(nDeletedIndex);
        }

        m_vNotifyTargets.Unlock();
    }
}

void ViewModelCollectionBase::OnItemsAdded(const std::vector<gsl::index>& vNewIndices)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
        {
            for (auto vNewIndex : vNewIndices)
                target.OnViewModelAdded(vNewIndex);
        }

        m_vNotifyTargets.Unlock();
    }
}

void ViewModelCollectionBase::OnItemsChanged(const std::vector<gsl::index>& vChangedIndices)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
        {
            for (auto vChangedIndex : vChangedIndices)
                target.OnViewModelChanged(vChangedIndex);
        }

        m_vNotifyTargets.Unlock();
    }
}

} // namespace ui
} // namespace ra
