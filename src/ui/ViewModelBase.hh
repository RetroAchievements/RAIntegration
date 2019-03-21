#ifndef RA_UI_VIEW_MODEL_BASE_H
#define RA_UI_VIEW_MODEL_BASE_H
#pragma once

#include "ra_fwd.h"

#include "ModelProperty.hh"

namespace ra {
namespace ui {

class ViewModelBase
{
public:
    virtual ~ViewModelBase() noexcept = default;
    ViewModelBase(const ViewModelBase&) = delete;
    ViewModelBase& operator=(const ViewModelBase&) = delete;
    GSL_SUPPRESS_F6 ViewModelBase(ViewModelBase&&) = default;
    GSL_SUPPRESS_F6 ViewModelBase& operator=(ViewModelBase&&) = default;

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

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.insert(&pTarget); }
    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.erase(&pTarget); }

private:
    using NotifyTargetSet = std::set<NotifyTarget*>;

protected:
    GSL_SUPPRESS_F6 ViewModelBase() = default;

    /// <summary>
    /// Gets the value associated to the requested boolean property.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    bool GetValue(const BoolModelProperty& pProperty) const
    {
        IntModelProperty::ValueMap::const_iterator iter = m_mIntValues.find(pProperty.GetKey());
        return static_cast<bool>(iter != m_mIntValues.end() ? iter->second : pProperty.GetDefaultValue());
    }

    /// <summary>
    /// Sets the specified boolean property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="bValue">The value to set.</param>
    void SetValue(const BoolModelProperty& pProperty, bool bValue);

    /// <summary>
    /// Gets the value associated to the requested string property.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    const std::wstring& GetValue(const StringModelProperty& pProperty) const
    {
        StringModelProperty::ValueMap::const_iterator iter = m_mStringValues.find(pProperty.GetKey());
        return (iter != m_mStringValues.end() ? iter->second : pProperty.GetDefaultValue());
    }

    /// <summary>
    /// Sets the specified string property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="sValue">The value to set.</param>
    void SetValue(const StringModelProperty& pProperty, const std::wstring& sValue);

    /// <summary>
    /// Gets the value associated to the requested integer property.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    int GetValue(const IntModelProperty& pProperty) const
    {
        IntModelProperty::ValueMap::const_iterator iter = m_mIntValues.find(pProperty.GetKey());
        return (iter != m_mIntValues.end() ? iter->second : pProperty.GetDefaultValue());
    }

    /// <summary>
    /// Sets the specified integer property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="nValue">The value to set.</param>
    void SetValue(const IntModelProperty& pProperty, int nValue);

    // allow BindingBase to call GetValue(Property) and SetValue(Property) directly.
    friend class BindingBase;

    // allow ViewModelCollectionBase to call GetValue(Property) directly.
    friend class ViewModelCollectionBase;

private:
    StringModelProperty::ValueMap m_mStringValues;
    IntModelProperty::ValueMap m_mIntValues;

#ifdef _DEBUG
    /// <summary>
    /// Complete list of values as strings for viewing in the debugger
    /// </summary>
    std::map<std::string, std::wstring> m_mDebugValues;
#endif

    /// <summary>
    /// A collection of pointers to other objects. These are not allocated object and do not need to be free'd. It's
    /// impossible to create a set of <c>NotifyTarget</c> references.
    /// </summary>
    NotifyTargetSet m_vNotifyTargets;
};

} // namespace ui
} // namespace ra

#endif RA_UI_VIEW_MODEL_BASE_H
