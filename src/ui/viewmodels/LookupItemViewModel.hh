#ifndef RA_UI_LOOKUPITEMVIEWMODEL_H
#define RA_UI_LOOKUPITEMVIEWMODEL_H
#pragma once

#include "ui/ViewModelBase.hh"
#include "ui/ViewModelCollection.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class LookupItemViewModel : public ViewModelBase
{
public:
    GSL_SUPPRESS_F6 LookupItemViewModel() = default;
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

class LookupItemViewModelCollection : public ViewModelCollectionBase
{
public:
    /// <summary>
    /// Adds an item to the end of the collection.
    /// </summary>
    GSL_SUPPRESS_C128 LookupItemViewModel& Add(int nId, const std::wstring& sLabel)
    {
        auto pItem = std::make_unique<LookupItemViewModel>(nId, sLabel);
        return dynamic_cast<LookupItemViewModel&>(ViewModelCollectionBase::AddItem(std::move(pItem)));
    }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    LookupItemViewModel* GetItemAt(gsl::index nIndex) { return dynamic_cast<LookupItemViewModel*>(GetViewModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    const LookupItemViewModel* GetItemAt(gsl::index nIndex) const { return dynamic_cast<const LookupItemViewModel*>(GetViewModelAt(nIndex)); }

    /// <summary>
    /// Gets the label for item specified by the provided ID.
    /// </summary>
    /// <returns>Requested label, empty string if no match was found.</returns>
    const std::wstring& GetLabelForId(int nId) const
    {
        const auto nIndex = FindItemIndex(LookupItemViewModel::IdProperty, nId);
        return GetItemValue(nIndex, LookupItemViewModel::LabelProperty);
    }
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_LOOKUPITEMVIEWMODEL_H
