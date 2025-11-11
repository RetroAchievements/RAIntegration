#ifndef RA_UI_MEMORYREGIONSVIEWMODEL_H
#define RA_UI_MEMORYREGIONSVIEWMODEL_H
#pragma once

#include "data\context\GameContext.hh"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemoryRegionsViewModel : public WindowViewModelBase,
    protected ra::ui::ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemoryRegionsViewModel();
    ~MemoryRegionsViewModel() = default;

    MemoryRegionsViewModel(const MemoryRegionsViewModel&) noexcept = delete;
    MemoryRegionsViewModel& operator=(const MemoryRegionsViewModel&) noexcept = delete;
    MemoryRegionsViewModel(MemoryRegionsViewModel&&) noexcept = delete;
    MemoryRegionsViewModel& operator=(MemoryRegionsViewModel&&) noexcept = delete;
    
    class MemoryRegionViewModel : public LookupItemViewModel
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the note.
        /// </summary>
        static const StringModelProperty RangeProperty;

        /// <summary>
        /// Gets the range of the region.
        /// </summary>
        const std::wstring& GetRange() const { return GetValue(RangeProperty); }

        /// <summary>
        /// Sets the range of the region.
        /// </summary>
        void SetRange(const std::wstring& sValue) { SetValue(RangeProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether the region is a custom region.
        /// </summary>
        static const BoolModelProperty IsCustomProperty;

        /// <summary>
        /// Gets whether the region is a custom region.
        /// </summary>
        bool IsCustom() const { return GetValue(IsCustomProperty); }

        /// <summary>
        /// Sets whether the region is a custom region.
        /// </summary>
        void SetCustom(bool bValue) { SetValue(IsCustomProperty, bValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether the region is invalid.
        /// </summary>
        static const BoolModelProperty IsInvalidProperty;

        /// <summary>
        /// Gets whether the region is invalid.
        /// </summary>
        bool IsInvalid() const { return GetValue(IsInvalidProperty); }

        /// <summary>
        /// Sets whether the region is invalid.
        /// </summary>
        void SetInvalid(bool bValue) { SetValue(IsInvalidProperty, bValue); }
    };

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether a region can be removed.
    /// </summary>
    static const BoolModelProperty CanRemoveRegionProperty;

    /// <summary>
    /// Gets whether a region can be removed.
    /// </summary>
    bool CanRemove() const { return GetValue(CanRemoveRegionProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether regions can be saved.
    /// </summary>
    static const BoolModelProperty CanSaveProperty;

    /// <summary>
    /// Gets whether regions can be saved.
    /// </summary>
    bool CanSave() const { return GetValue(CanSaveProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected region.
    /// </summary>
    static const IntModelProperty SelectedRegionIndexProperty;

    /// <summary>
    /// Gets the selected region index.
    /// </summary>
    int GetSelectedRegionIndex() const { return GetValue(SelectedRegionIndexProperty); }
    
    /// <summary>
    /// Sets the selected region index.
    /// </summary>
    void SetSelectedRegionIndex(int nIndex) { SetValue(SelectedRegionIndexProperty, nIndex); }

    /// <summary>
    /// Gets the list of regions.
    /// </summary>
    ViewModelCollection<MemoryRegionViewModel>& Regions() noexcept
    {
        return m_vRegions;
    }

    /// <summary>
    /// Gets the list of regions.
    /// </summary>
    const ViewModelCollection<MemoryRegionViewModel>& Regions() const noexcept
    {
        return m_vRegions;
    }

    void InitializeRegions();
    void AddNewRegion();
    void RemoveRegion();

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // ra::ui::ViewModelCollectionBase::NotifyTarget
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;

private:
    void UpdateCanSave();
    void SaveCustomRegions();

    ViewModelCollection<MemoryRegionViewModel> m_vRegions;
    int m_vSelectedItem = -1;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYREGIONSVIEWMODEL_H
