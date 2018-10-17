#ifndef RA_UI_BINDINGBASE_H
#define RA_UI_BINDINGBASE_H
#pragma once

#include "ui/ViewModelBase.hh"

namespace ra {
namespace ui {

class BindingBase : protected ViewModelBase::NotifyTarget
{
protected:
    explicit BindingBase(ViewModelBase& vmViewModel) noexcept
        : m_vmViewModel(vmViewModel)
    {
        vmViewModel.AddNotifyTarget(*this);
    }

    virtual ~BindingBase() noexcept
    {
        m_vmViewModel.RemoveNotifyTarget(*this);
    }

    /// <summary>
    /// Gets the value associated to the requested boolean property from the viewmodel.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for the bound view model.</returns>
    bool GetValue(const BoolModelProperty& pProperty) const
    {
        return m_vmViewModel.GetValue(pProperty);
    }

    /// <summary>
    /// Sets the specified boolean property of the viewmodel to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="bValue">The value to set.</param>
    void SetValue(const BoolModelProperty& pProperty, bool bValue)
    {
        m_vmViewModel.SetValue(pProperty, bValue);
    }

    /// <summary>
    /// Gets the value associated to the requested string property from the viewmodel.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for the bound view model.</returns>
    const std::wstring& GetValue(const StringModelProperty& pProperty) const
    {
        return m_vmViewModel.GetValue(pProperty);
    }

    /// <summary>
    /// Sets the specified string property of the viewmodel to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="sValue">The value to set.</param>
    void SetValue(const StringModelProperty& pProperty, const std::wstring& sValue)
    {
        m_vmViewModel.SetValue(pProperty, sValue);
    }

    /// <summary>
    /// Gets the value associated to the requested integer property from the viewmodel.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for the bound view model.</returns>
    int GetValue(const IntModelProperty& pProperty) const
    {
        return m_vmViewModel.GetValue(pProperty);
    }

    /// <summary>
    /// Sets the specified integer property of the viewmodel to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="nValue">The value to set.</param>
    void SetValue(const IntModelProperty& pProperty, int nValue)
    {
        m_vmViewModel.SetValue(pProperty, nValue);
    }

private:
    ra::ui::ViewModelBase& m_vmViewModel;
};

} // namespace ui
} // namespace ra

#endif // !RA_UI_BINDINGBASE_H
