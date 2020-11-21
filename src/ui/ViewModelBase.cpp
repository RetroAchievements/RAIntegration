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

    ModelBase::OnValueChanged(args);
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

    ModelBase::OnValueChanged(args);
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

    ModelBase::OnValueChanged(args);
}

} // namespace ui
} // namespace ra
