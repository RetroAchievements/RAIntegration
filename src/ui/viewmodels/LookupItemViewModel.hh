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

    /// <summary>
    /// The <see cref="ModelProperty" /> for the whether the bookmark is selected.
    /// </summary>
    static const BoolModelProperty IsSelectedProperty;

    /// <summary>
    /// Gets whether the bookmark is selected.
    /// </summary>
    bool IsSelected() const { return GetValue(IsSelectedProperty); }

    /// <summary>
    /// Sets whether the bookmark is selected.
    /// </summary>
    void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }
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
    /// Adds an item to the end of the collection.
    /// </summary>
    template<class T, class... Args>
    GSL_SUPPRESS_C128 T& Add(Args&&... args)
    {
        static_assert(std::is_base_of<LookupItemViewModel, T>::value, "T not derived from base class LookupItemViewModel");

        auto pItem = std::make_unique<T>(std::forward<Args>(args)...);
        return dynamic_cast<T&>(ViewModelCollectionBase::AddItem(std::move(pItem)));
    }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    LookupItemViewModel* GetItemAt(gsl::index nIndex) { return dynamic_cast<LookupItemViewModel*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    const LookupItemViewModel* GetItemAt(gsl::index nIndex) const { return dynamic_cast<const LookupItemViewModel*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    template<class T>
    T* GetItemAt(gsl::index nIndex) { return dynamic_cast<T*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the item at the specified index.
    /// </summary>
    template<class T>
    const T* GetItemAt(gsl::index nIndex) const { return dynamic_cast<const T*>(GetModelAt(nIndex)); }

    /// <summary>
    /// Gets the label for item specified by the provided ID.
    /// </summary>
    /// <returns>Requested label, empty string if no match was found.</returns>
    const std::wstring& GetLabelForId(int nId) const
    {
        const auto nIndex = FindItemIndex(LookupItemViewModel::IdProperty, nId);
        return GetItemValue(nIndex, LookupItemViewModel::LabelProperty);
    }

    auto begin() noexcept { return CreateBeginIterator<LookupItemViewModel>(); }
    auto end() noexcept { return CreateEndIterator<LookupItemViewModel>(); }
    auto begin() const noexcept { return CreateConstBeginIterator<LookupItemViewModel>(); }
    auto end() const noexcept { return CreateConstEndIterator<LookupItemViewModel>(); }
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_LOOKUPITEMVIEWMODEL_H
