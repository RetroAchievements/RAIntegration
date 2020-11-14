#include "ViewModelBase.hh"

namespace ra {
namespace ui {

void ViewModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnViewModelBoolValueChanged(args);
        }
    }

    ModelPropertyContainer::OnValueChanged(args);
}

void ViewModelBase::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnViewModelStringValueChanged(args);
        }
    }

    ModelPropertyContainer::OnValueChanged(args);
}

void ViewModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnViewModelIntValueChanged(args);
        }
    }

    ModelPropertyContainer::OnValueChanged(args);
}

} // namespace ui
} // namespace ra
