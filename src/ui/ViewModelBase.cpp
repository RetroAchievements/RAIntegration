#include "ViewModelBase.hh"

namespace ra {
namespace ui {

void ViewModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnViewModelBoolValueChanged(args);

        m_vNotifyTargets.Unlock();
    }

    ModelBase::OnValueChanged(args);
}

void ViewModelBase::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnViewModelStringValueChanged(args);

        m_vNotifyTargets.Unlock();
    }

    ModelBase::OnValueChanged(args);
}

void ViewModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnViewModelIntValueChanged(args);

        m_vNotifyTargets.Unlock();
    }

    ModelBase::OnValueChanged(args);
}

} // namespace ui
} // namespace ra
