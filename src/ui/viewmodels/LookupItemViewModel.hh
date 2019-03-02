#ifndef RA_UI_LOOKUPITEMVIEWMODEL_H
#define RA_UI_LOOKUPITEMVIEWMODEL_H
#pragma once

#include "ui/ViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class LookupItemViewModel : public ViewModelBase
{
public:
    LookupItemViewModel() noexcept = default;
    LookupItemViewModel(int nId, const std::wstring& sLabel) noexcept
    {
        GSL_SUPPRESS_F6 SetId(nId);
        GSL_SUPPRESS_F6 SetLabel(sLabel);
    }

    LookupItemViewModel(int nId, const std::wstring&& sLabel) noexcept
    {
        GSL_SUPPRESS_F6 SetId(nId);
        GSL_SUPPRESS_F6 SetValue(LabelProperty, sLabel); // TODO: support std::move for setting strings in ViewModel
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the label.
    /// </summary>
    static const IntModelProperty IdProperty;

    /// <summary>
    /// Gets the label to display.
    /// </summary>
    int GetId() const { return GetValue(IdProperty); }

    /// <summary>
    /// Sets the label to display.
    /// </summary>
    void SetId(int nValue) { SetValue(IdProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the label.
    /// </summary>
    static const StringModelProperty LabelProperty;

    /// <summary>
    /// Gets the label to display.
    /// </summary>
    const std::wstring& GetLabel() const { return GetValue(LabelProperty); }

    /// <summary>
    /// Sets the label to display.
    /// </summary>
    void SetLabel(const std::wstring& sValue) { SetValue(LabelProperty, sValue); }
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_LOOKUPITEMVIEWMODEL_H
