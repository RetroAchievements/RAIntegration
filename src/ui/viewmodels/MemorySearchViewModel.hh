#ifndef RA_UI_MEMORYSEARCHVIEWMODEL_H
#define RA_UI_MEMORYSEARCHVIEWMODEL_H
#pragma once

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\Types.hh"

#include "services\SearchResults.h"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemorySearchViewModel : public ViewModelBase,
    protected ViewModelCollectionBase::NotifyTarget,
    protected data::EmulatorContext::NotifyTarget,
    protected data::GameContext::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemorySearchViewModel();
    ~MemorySearchViewModel() noexcept = default;

    MemorySearchViewModel(const MemorySearchViewModel&) noexcept = delete;
    MemorySearchViewModel& operator=(const MemorySearchViewModel&) noexcept = delete;
    MemorySearchViewModel(MemorySearchViewModel&&) noexcept = delete;
    MemorySearchViewModel& operator=(MemorySearchViewModel&&) noexcept = delete;

    void InitializeNotifyTargets();


    void DoFrame();

    bool NeedsRedraw() noexcept;

    class PredefinedFilterRangeViewModel : public LookupItemViewModel
    {
    public:
        PredefinedFilterRangeViewModel(int nId, const std::wstring& sLabel) noexcept
            : LookupItemViewModel(nId, sLabel)
        {
        }

        PredefinedFilterRangeViewModel(int nId, const std::wstring&& sLabel) noexcept
            : LookupItemViewModel(nId, sLabel)
        {
        }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the start address of the predefined filter range.
        /// </summary>
        static const IntModelProperty StartAddressProperty;

        /// <summary>
        /// Gets the start address of the predefined filter range.
        /// </summary>
        const ra::ByteAddress GetStartAddress() const { return gsl::narrow_cast<ra::ByteAddress>(GetValue(StartAddressProperty)); }

        /// <summary>
        /// Sets the start address of the predefined filter range.
        /// </summary>
        void SetStartAddress(ra::ByteAddress nValue) { SetValue(StartAddressProperty, gsl::narrow_cast<int>(nValue)); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the end address of the predefined filter range.
        /// </summary>
        static const IntModelProperty EndAddressProperty;

        /// <summary>
        /// Gets the end address of the predefined filter range.
        /// </summary>
        const ra::ByteAddress GetEndAddress() const { return gsl::narrow_cast<ra::ByteAddress>(GetValue(EndAddressProperty)); }

        /// <summary>
        /// Sets the end address of the predefined filter range.
        /// </summary>
        void SetEndAddress(ra::ByteAddress nValue) { SetValue(EndAddressProperty, gsl::narrow_cast<int>(nValue)); }
    };

    /// <summary>
    /// The <see cref="ModelProperty" /> for the predefined filter ranges.
    /// </summary>
    static const IntModelProperty PredefinedFilterRangeProperty;

    /// <summary>
    /// Gets the selected predefined filter range.
    /// </summary>
    const int GetPredefinedFilterRange() const { return GetValue(PredefinedFilterRangeProperty); }

    /// <summary>
    /// Sets the selected predefined filter range.
    /// </summary>
    void SetPredefinedFilterRange(int nPredefinedFilterRangeIndex) { SetValue(PredefinedFilterRangeProperty, nPredefinedFilterRangeIndex); }

    /// <summary>
    /// Gets the list of selectable search types.
    /// </summary>
    ViewModelCollection<PredefinedFilterRangeViewModel>& PredefinedFilterRanges() noexcept
    {
        return m_vPredefinedFilterRanges;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the filter range.
    /// </summary>
    static const StringModelProperty FilterRangeProperty;

    /// <summary>
    /// Gets the filter range.
    /// </summary>
    const std::wstring& GetFilterRange() const { return GetValue(FilterRangeProperty); }

    /// <summary>
    /// Sets the filter range.
    /// </summary>
    void SetFilterRange(const std::wstring& sValue) { SetValue(FilterRangeProperty, sValue); }

    /// <summary>
    /// Gets the list of selectable search types.
    /// </summary>
    const LookupItemViewModelCollection& SearchTypes() const noexcept
    {
        return m_vSearchTypes;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected search type.
    /// </summary>
    static const IntModelProperty SearchTypeProperty;

    /// <summary>
    /// Gets the selected search type.
    /// </summary>
    ra::services::SearchType GetSearchType() const { return ra::itoe<ra::services::SearchType>(GetValue(SearchTypeProperty)); }

    /// <summary>
    /// Sets the selected search type.
    /// </summary>
    void SetSearchType(ra::services::SearchType value) { SetValue(SearchTypeProperty, ra::etoi(value)); }

    /// <summary>
    /// Gets the list of selectable comparison types.
    /// </summary>
    const LookupItemViewModelCollection& ComparisonTypes() const noexcept
    {
        return m_vComparisonTypes;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected comparison type.
    /// </summary>
    static const IntModelProperty ComparisonTypeProperty;

    /// <summary>
    /// Gets the selected comparison type.
    /// </summary>
    ComparisonType GetComparisonType() const { return ra::itoe<ComparisonType>(GetValue(ComparisonTypeProperty)); }

    /// <summary>
    /// Sets the selected comparison type.
    /// </summary>
    void SetComparisonType(ComparisonType value) { SetValue(ComparisonTypeProperty, ra::etoi(value)); }

    /// <summary>
    /// Gets the list of selectable value types.
    /// </summary>
    const LookupItemViewModelCollection& ValueTypes() const noexcept
    {
        return m_vValueTypes;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected value type.
    /// </summary>
    static const IntModelProperty ValueTypeProperty;

    /// <summary>
    /// Gets the selected value type.
    /// </summary>
    ra::services::SearchFilterType GetValueType() const { return ra::itoe<ra::services::SearchFilterType>(GetValue(ValueTypeProperty)); }

    /// <summary>
    /// Sets the selected value type.
    /// </summary>
    void SetValueType(ra::services::SearchFilterType value) { SetValue(ValueTypeProperty, ra::etoi(value)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the filter value.
    /// </summary>
    static const StringModelProperty FilterValueProperty;

    /// <summary>
    /// Gets the filter value.
    /// </summary>
    const std::wstring& GetFilterValue() const { return GetValue(FilterValueProperty); }

    /// <summary>
    /// Sets the filter value.
    /// </summary>
    void SetFilterValue(const std::wstring& sValue) { SetValue(FilterValueProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the filter summary.
    /// </summary>
    static const StringModelProperty FilterSummaryProperty;

    /// <summary>
    /// Gets the filter summary.
    /// </summary>
    const std::wstring& GetFilterSummary() const { return GetValue(FilterSummaryProperty); }

    class SearchResultViewModel : public ViewModelBase
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the description.
        /// </summary>
        static const StringModelProperty DescriptionProperty;

        /// <summary>
        /// Gets the description.
        /// </summary>
        const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

        /// <summary>
        /// Sets the description.
        /// </summary>
        void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the address.
        /// </summary>
        static const StringModelProperty AddressProperty;

        /// <summary>
        /// Gets the address.
        /// </summary>
        const std::wstring& GetAddress() const { return GetValue(AddressProperty); }

        /// <summary>
        /// Sets the address.
        /// </summary>
        void SetAddress(const std::wstring& value) { SetValue(AddressProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the current value of the address.
        /// </summary>
        static const StringModelProperty CurrentValueProperty;

        /// <summary>
        /// Gets the current value of the address. 
        /// </summary>
        const std::wstring& GetCurrentValue() const { return GetValue(CurrentValueProperty); }

        /// <summary>
        /// Sets the current value of the address.
        /// </summary>
        void SetCurrentValue(const std::wstring& value) { SetValue(CurrentValueProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the previous value of the address.
        /// </summary>
        static const StringModelProperty PreviousValueProperty;

        /// <summary>
        /// Gets the previous value of the address.
        /// </summary>
        const std::wstring& GetPreviousValue() const { return GetValue(PreviousValueProperty); }

        /// <summary>
        /// Sets the previous value of the address.
        /// </summary>
        void SetPreviousValue(const std::wstring& value) { SetValue(PreviousValueProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the whether the result is selected.
        /// </summary>
        static const BoolModelProperty IsSelectedProperty;

        /// <summary>
        /// Gets whether the result is selected.
        /// </summary>
        bool IsSelected() const { return GetValue(IsSelectedProperty); }

        /// <summary>
        /// Sets whether the result is selected.
        /// </summary>
        void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the row color.
        /// </summary>
        static const IntModelProperty RowColorProperty;

        /// <summary>
        /// Gets the row color.
        /// </summary>
        Color GetRowColor() const { return Color(ra::to_unsigned(GetValue(RowColorProperty))); }

        /// <summary>
        /// Sets the row color.
        /// </summary>
        void SetRowColor(Color value) { SetValue(RowColorProperty, ra::to_signed(value.ARGB)); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the row color.
        /// </summary>
        static const IntModelProperty DescriptionColorProperty;

        /// <summary>
        /// Gets the description color.
        /// </summary>
        Color GetDescriptionColor() const { return Color(ra::to_unsigned(GetValue(DescriptionColorProperty))); }

        /// <summary>
        /// Sets the description color.
        /// </summary>
        void SetDescriptionColor(Color value) { SetValue(DescriptionColorProperty, ra::to_signed(value.ARGB)); }

        ra::ByteAddress nAddress = 0;
        bool bMatchesFilter = false;
        bool bHasBookmark = false;
        bool bHasCodeNote = false;
        bool bHasBeenModified = false;

        void UpdateRowColor();
    };

    /// <summary>
    /// Gets the list of results.
    /// </summary>
    ViewModelCollection<SearchResultViewModel>& Results() noexcept
    {
        return m_vResults;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the memory size of result items.
    /// </summary>
    static const IntModelProperty ResultMemSizeProperty;

    /// <summary>
    /// Gets the memory size of the items in <see cref="Results"/>.
    /// </summary>
    MemSize ResultMemSize() const { return ra::itoe<MemSize>(GetValue(ResultMemSizeProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the number of results found.
    /// </summary>
    static const IntModelProperty ResultCountProperty;

    /// <summary>
    /// Gets the number of results found.
    /// </summary>
    unsigned int GetResultCount() const { return gsl::narrow_cast<unsigned int>(GetValue(ResultCountProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the string representation of <see cref="ResultCountProperty" />.
    /// </summary>
    static const StringModelProperty ResultCountTextProperty;

    /// <summary>
    /// Gets the string representation of the number of results found.
    /// </summary>
    const std::wstring& GetResultCountText() const { return GetValue(ResultCountTextProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the scroll offset.
    /// </summary>
    static const IntModelProperty ScrollOffsetProperty;

    /// <summary>
    /// Gets the scroll offset.
    /// </summary>
    unsigned int GetScrollOffset() const { return gsl::narrow_cast<unsigned int>(GetValue(ScrollOffsetProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the scroll maximum.
    /// </summary>
    static const IntModelProperty ScrollMaximumProperty;

    /// <summary>
    /// Gets the scroll maximum.
    /// </summary>
    unsigned int GetScrollMaximum() const { return gsl::narrow_cast<unsigned int>(GetValue(ScrollMaximumProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected page description.
    /// </summary>
    static const StringModelProperty SelectedPageProperty;

    /// <summary>
    /// Gets the selected page description (i.e. "5/6").
    /// </summary>
    const std::wstring& GetSelectedPage() const { return GetValue(SelectedPageProperty); }

    /// <summary>
    /// DO NOT CALL! This is a helper function used by the virtualizing GridBinding to notify the view model when
    /// multiple items (which may or may not be visible) are selected or deselected at once. This does not update
    /// the IsSelected property of each visible item in the range - that's handled by the GridBinding.
    /// <summary>
    void SelectRange(gsl::index nFrom, gsl::index nTo, bool bValue);

    /// <summary>
    /// Advances to the next page of results (if available).
    /// </summary>
    void NextPage();
    static const BoolModelProperty CanGoToNextPageProperty;

    /// <summary>
    /// Returns to the previous page of results (if available).
    /// </summary>
    void PreviousPage();
    static const BoolModelProperty CanGoToPreviousPageProperty;

    /// <summary>
    /// Starts a new search.
    /// </summary>
    void BeginNewSearch();
    static const BoolModelProperty CanBeginNewSearchProperty;

    /// <summary>
    /// Applies the current filter.
    /// </summary>
    void ApplyFilter();
    static const BoolModelProperty CanFilterProperty;
    static const BoolModelProperty CanEditFilterValueProperty;

    void ToggleContinuousFilter();
    static const BoolModelProperty CanContinuousFilterProperty;
    static const StringModelProperty ContinuousFilterLabelProperty;

    /// <summary>
    /// Excludes the currently selected items from the search results.
    /// </summary>
    void ExcludeSelected();
    static const BoolModelProperty HasSelectionProperty;

    /// <summary>
    /// Bookmarks the currently selected items from the search results.
    /// </summary>
    void BookmarkSelected();

protected:
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) noexcept override;
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) noexcept override;

    // EmulatorContext::NotifyTarget
    void OnTotalMemorySizeChanged() override;

    // GameContext::NotifyTarget
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNote) override;

private:
    bool ParseFilterRange(_Out_ ra::ByteAddress& nStart, _Out_ ra::ByteAddress& nEnd);
    void ApplyContinuousFilter();
    void UpdateResults();
    void AddNewPage();
    void ChangePage(size_t nNewPage);

    void OnPredefinedFilterRangeChanged();
    void OnFilterRangeChanged();

    ViewModelCollection<PredefinedFilterRangeViewModel> m_vPredefinedFilterRanges;
    LookupItemViewModelCollection m_vSearchTypes;
    LookupItemViewModelCollection m_vComparisonTypes;
    LookupItemViewModelCollection m_vValueTypes;

    ViewModelCollection<SearchResultViewModel> m_vResults;
    bool m_bIsContinuousFiltering = false;
    std::chrono::steady_clock::time_point m_tLastContinuousFilter;
    bool m_bNeedsRedraw = false;
    bool m_bScrolling = false;
    bool m_bSelectingFilter = false;

    struct SearchResult
    {
        ra::services::SearchResults pResults;
        std::set<unsigned int> vModifiedAddresses;

        std::wstring sSummary;
    };

    size_t m_nSelectedSearchResult = 0U;
    std::vector<SearchResult> m_vSearchResults;
    std::set<unsigned int> m_vSelectedAddresses;

    static bool TestFilter(const ra::services::SearchResults::Result& pResult, const SearchResult& pCurrentResults, unsigned int nPreviousValue) noexcept;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYSEARCHVIEWMODEL_H
