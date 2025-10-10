#ifndef RA_UI_VIEW_MODEL_BASE_H
#define RA_UI_VIEW_MODEL_BASE_H
#pragma once

#include "ra_fwd.h"

#include "data\ModelBase.hh"
#include "data\NotifyTargetSet.hh"

namespace ra {
namespace ui {

using BoolModelProperty = ra::data::BoolModelProperty;
using StringModelProperty = ra::data::StringModelProperty;
using IntModelProperty = ra::data::IntModelProperty;

class ViewModelBase : public ra::data::ModelBase
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

        virtual void OnViewModelBoolValueChanged([[maybe_unused]] const BoolModelProperty::ChangeArgs& args) noexcept(false) {}
        virtual void OnViewModelStringValueChanged([[maybe_unused]] const StringModelProperty::ChangeArgs& args) noexcept(false) {}
        virtual void OnViewModelIntValueChanged([[maybe_unused]] const IntModelProperty::ChangeArgs& args) noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { m_vNotifyTargets.Add(pTarget); }

    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept
    {
#ifdef RA_UTEST
        GSL_SUPPRESS_F6 Expects(!m_bDestructed);
#endif
        m_vNotifyTargets.Remove(pTarget);
    }

protected:
    GSL_SUPPRESS_F6 ViewModelBase() = default;

    /// <summary>
    /// Called when a boolean value changes.
    /// </summary>
    /// <param name="args">Information about the change.</param>
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    /// <summary>
    /// Called when a string value changes.
    /// </summary>
    /// <param name="args">Information about the change.</param>
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    /// <summary>
    /// Called when a integer value changes.
    /// </summary>
    /// <param name="args">Information about the change.</param>
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // allow BindingBase to call GetValue(Property) and SetValue(Property) directly.
    friend class BindingBase;

    // allow ViewModelCollectionBase to call GetValue(Property) directly.
    friend class ViewModelCollectionBase;

private:
    /// <summary>
    /// A collection of pointers to other objects. These are not allocated object and do not need to be free'd. It's
    /// impossible to create a set of <c>NotifyTarget</c> references.
    /// </summary>
    ra::data::NotifyTargetSet<NotifyTarget> m_vNotifyTargets;
};

} // namespace ui
} // namespace ra

#endif RA_UI_VIEW_MODEL_BASE_H
