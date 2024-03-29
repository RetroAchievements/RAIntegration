#ifndef RA_UI_BINDINGBASE_H
#define RA_UI_BINDINGBASE_H
#pragma once

#include "services/ServiceLocator.hh"
#include "ui/ViewModelBase.hh"

namespace ra {
namespace ui {

class BindingBase : protected ViewModelBase::NotifyTarget
{
public:
    ~BindingBase() noexcept
    {
        if (ra::services::ServiceLocator::IsInitialized())
            m_vmViewModel.RemoveNotifyTarget(*this);
    }
    BindingBase(const BindingBase&) noexcept = delete;
    BindingBase& operator=(const BindingBase&) noexcept = delete;
    BindingBase(BindingBase&&) noexcept = delete;
    BindingBase& operator=(BindingBase&&) noexcept = delete;

protected:
    explicit BindingBase(_Inout_ ViewModelBase& vmViewModel) noexcept :
        m_vmViewModel{ vmViewModel }
    {
        vmViewModel.AddNotifyTarget(*this);
    }

    /// <summary>
    /// Gets the value associated to the requested boolean property from the view-model.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for the bound view model.</returns>
    bool GetValue(const BoolModelProperty& pProperty) const
    {
        return m_vmViewModel.GetValue(pProperty);
    }

    /// <summary>
    /// Sets the specified boolean property of the view-model to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="bValue">The value to set.</param>
    void SetValue(const BoolModelProperty& pProperty, bool bValue)
    {
        if (m_nSetDepth++ == 0)
            m_vmViewModel.RemoveNotifyTarget(*this);

        m_vmViewModel.SetValue(pProperty, bValue);

        if (--m_nSetDepth == 0)
            m_vmViewModel.AddNotifyTarget(*this);
    }

    /// <summary>
    /// Gets the value associated to the requested string property from the view-model.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for the bound view model.</returns>
    const std::wstring& GetValue(const StringModelProperty& pProperty) const
    {
        return m_vmViewModel.GetValue(pProperty);
    }

    /// <summary>
    /// Sets the specified string property of the view-model to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="sValue">The value to set.</param>
    void SetValue(const StringModelProperty& pProperty, const std::wstring& sValue)
    {
        if (m_nSetDepth++ == 0)
            m_vmViewModel.RemoveNotifyTarget(*this);

        m_vmViewModel.SetValue(pProperty, sValue);

        if (--m_nSetDepth == 0)
            m_vmViewModel.AddNotifyTarget(*this);
    }

    /// <summary>
    /// Gets the value associated to the requested integer property from the view-model.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for the bound view model.</returns>
    int GetValue(const IntModelProperty& pProperty) const
    {
        return m_vmViewModel.GetValue(pProperty);
    }

    /// <summary>
    /// Sets the specified integer property of the view-model to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="nValue">The value to set.</param>
    void SetValue(const IntModelProperty& pProperty, int nValue)
    {
        if (m_nSetDepth++ == 0)
            m_vmViewModel.RemoveNotifyTarget(*this);

        m_vmViewModel.SetValue(pProperty, nValue);

        if (--m_nSetDepth == 0)
            m_vmViewModel.AddNotifyTarget(*this);
    }

protected:
    template <class T>
    T& GetViewModel() const noexcept { return dynamic_cast<T&>(m_vmViewModel); }

private:
    ra::ui::ViewModelBase& m_vmViewModel;
    int m_nSetDepth = 0;
};

} // namespace ui
} // namespace ra

#endif // !RA_UI_BINDINGBASE_H
