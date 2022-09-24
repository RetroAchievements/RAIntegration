#ifndef RA_UI_WIN32_COMBOBOXBINDING_H
#define RA_UI_WIN32_COMBOBOXBINDING_H
#pragma once

#include "ControlBinding.hh"

#include "RA_StringUtils.h"

#include "ui\ViewModelCollection.hh"
#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class ComboBoxBinding : public ControlBinding, protected ViewModelCollectionBase::NotifyTarget
{
public:
    explicit ComboBoxBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}

    GSL_SUPPRESS_F6 ~ComboBoxBinding() noexcept
    {
        if (m_pMutableViewModelCollection)
        {
            if (ra::services::ServiceLocator::IsInitialized())
                m_pMutableViewModelCollection->RemoveNotifyTarget(*this);
        }
    }

    ComboBoxBinding(const ComboBoxBinding&) noexcept = delete;
    ComboBoxBinding& operator=(const ComboBoxBinding&) noexcept = delete;
    ComboBoxBinding(ComboBoxBinding&&) noexcept = delete;
    ComboBoxBinding& operator=(ComboBoxBinding&&) noexcept = delete;

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);
        SetHWND(hControl);
    }

    void SetHWND(HWND hControl)
    {
        m_hWnd = hControl;

        if (m_pViewModelCollection)
            PopulateComboBox();

        if (m_pSelectedIdProperty)
            UpdateSelectedItem();
    }

    void BindItems(ra::ui::viewmodels::LookupItemViewModelCollection& pViewModels)
    {
        BindItems(pViewModels, ra::ui::viewmodels::LookupItemViewModel::IdProperty,
            ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    }

    void BindItems(const ra::ui::viewmodels::LookupItemViewModelCollection& pViewModels)
    {
        BindItems(pViewModels, ra::ui::viewmodels::LookupItemViewModel::IdProperty,
            ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    }

    void BindItems(ViewModelCollectionBase& pViewModels, const IntModelProperty& pIdProperty, const StringModelProperty& pTextProperty)
    {
        BindItems(const_cast<const ViewModelCollectionBase&>(pViewModels), pIdProperty, pTextProperty);

        m_pMutableViewModelCollection = &pViewModels;
        pViewModels.AddNotifyTarget(*this);
    }

    void BindItems(const ViewModelCollectionBase& pViewModels, const IntModelProperty& pIdProperty, const StringModelProperty& pTextProperty)
    {
        m_pViewModelCollection = &pViewModels;
        m_pItemIdProperty = &pIdProperty;
        m_pItemTextProperty = &pTextProperty;

        if (m_hWnd)
            PopulateComboBox();

        if (m_pSelectedIdProperty)
            UpdateSelectedItem();
    }

    void BindSelectedItem(const IntModelProperty& pSelectedIdProperty)
    {
        m_pSelectedIdProperty = &pSelectedIdProperty;
        if (m_pViewModelCollection)
            UpdateSelectedItem();
    }

    void OnValueChanged() override
    {
        const auto nIndex = ComboBox_GetCurSel(m_hWnd);
        if (m_pSelectedIdProperty)
        {
            const auto nSelectedId = m_pViewModelCollection->GetItemValue(nIndex, *m_pItemIdProperty);
            SetValue(*m_pSelectedIdProperty, nSelectedId);
        }
    }

protected:
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override
    {
        if (m_pSelectedIdProperty && *m_pSelectedIdProperty == args.Property)
            UpdateSelectedItem();
    }

    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override
    {
        if (args.Property == *m_pItemTextProperty)
        {
            const auto& pLabel = m_pViewModelCollection->GetItemValue(nIndex, *m_pItemTextProperty);
            ComboBox_DeleteString(m_hWnd, nIndex);
            ComboBox_InsertString(m_hWnd, nIndex, NativeStr(pLabel).c_str());

            if (m_nSelectedIndex == nIndex)
            {
                ComboBox_SetCurSel(m_hWnd, nIndex);
                ComboBox_SetText(m_hWnd, NativeStr(pLabel).c_str());
            }
        }
    }

    void OnViewModelAdded(gsl::index nIndex) override
    {
        const auto& pLabel = m_pViewModelCollection->GetItemValue(nIndex, *m_pItemTextProperty);
        ComboBox_InsertString(m_hWnd, nIndex, NativeStr(pLabel).c_str());
    }

    void OnViewModelRemoved(gsl::index nIndex) noexcept override
    {
        ComboBox_DeleteString(m_hWnd, nIndex);
    }

    virtual void PopulateComboBox()
    {
        const auto nCount = ra::to_signed(m_pViewModelCollection->Count());
        for (gsl::index nIndex = 0; nIndex < nCount; ++nIndex)
        {
            const auto& pLabel = m_pViewModelCollection->GetItemValue(nIndex, *m_pItemTextProperty);

            ComboBox_AddString(m_hWnd, NativeStr(pLabel).c_str());
        }
    }

    virtual void UpdateSelectedItem()
    {
        if (m_pViewModelCollection)
        {
            const auto nSelectedId = GetValue(*m_pSelectedIdProperty);
            m_nSelectedIndex = m_pViewModelCollection->FindItemIndex(*m_pItemIdProperty, nSelectedId);
        }
        else
        {
            m_nSelectedIndex = -1;
        }

        if (m_hWnd)
            ComboBox_SetCurSel(m_hWnd, m_nSelectedIndex);
    }

    const ViewModelCollectionBase* m_pViewModelCollection = nullptr;
    const IntModelProperty* m_pSelectedIdProperty = nullptr;

private:
    ViewModelCollectionBase* m_pMutableViewModelCollection = nullptr;
    const IntModelProperty* m_pItemIdProperty = nullptr;
    const StringModelProperty* m_pItemTextProperty = nullptr;
    gsl::index m_nSelectedIndex = -1;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_COMBOBOXBINDING_H
