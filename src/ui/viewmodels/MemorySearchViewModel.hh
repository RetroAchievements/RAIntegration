#ifndef RA_UI_MEMORYSEARCHVIEWMODEL_H
#define RA_UI_MEMORYSEARCHVIEWMODEL_H
#pragma once

#include "data\Types.hh"

#include "services\SearchResults.h"

#include "ui\WindowViewModelBase.hh"

#include "ui\viewmodels\LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MemorySearchViewModel : public ViewModelBase,
                              protected ViewModelBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 MemorySearchViewModel();
    ~MemorySearchViewModel() noexcept = default;

    MemorySearchViewModel(const MemorySearchViewModel&) noexcept = delete;
    MemorySearchViewModel& operator=(const MemorySearchViewModel&) noexcept = delete;
    MemorySearchViewModel(MemorySearchViewModel&&) noexcept = delete;
    MemorySearchViewModel& operator=(MemorySearchViewModel&&) noexcept = delete;

    void DoFrame();

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

    enum SearchType
    {
        FourBit,
        EightBit,
        SixteenBit,
    };

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
    SearchType GetSearchType() const { return ra::itoe<SearchType>(GetValue(SearchTypeProperty)); }

    /// <summary>
    /// Sets the selected search type.
    /// </summary>
    void SetSearchType(SearchType value) { SetValue(SearchTypeProperty, ra::etoi(value)); }

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

    enum ValueType
    {
        Constant,
        LastKnownValue,
    };

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
    ValueType GetValueType() const { return ra::itoe<ValueType>(GetValue(SearchTypeProperty)); }

    /// <summary>
    /// Sets the selected value type.
    /// </summary>
    void SetValueType(ValueType value) { SetValue(SearchTypeProperty, ra::etoi(value)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the value.
    /// </summary>
    static const StringModelProperty FilterValueProperty;

    /// <summary>
    /// Gets the value.
    /// </summary>
    const std::wstring& GetFilterValue() const { return GetValue(FilterValueProperty); }

    /// <summary>
    /// Sets the value.
    /// </summary>
    void SetFilterValue(const std::wstring& sValue) { SetValue(FilterValueProperty, sValue); }

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
        void SetPreviousValue(const std::wstring&  value) { SetValue(PreviousValueProperty, value); }

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
    };

    /// <summary>
    /// Gets the list of results.
    /// </summary>
    ViewModelCollection<SearchResultViewModel>& Results() noexcept
    {
        return m_vResults;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the number of results found.
    /// </summary>
    static const IntModelProperty ResultCountProperty;

    /// <summary>
    /// Gets the number of results found.
    /// </summary>
    unsigned int GetResultCount() const { return gsl::narrow_cast<unsigned int>(GetValue(ResultCountProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the scroll offset.
    /// </summary>
    static const IntModelProperty ScrollOffsetProperty;

    /// <summary>
    /// Gets the scroll offset.
    /// </summary>
    unsigned int GetScrollOffset() const { return gsl::narrow_cast<unsigned int>(GetValue(ScrollOffsetProperty)); }

    void SetScrollOffset(unsigned int nOffset) { SetValue(ScrollOffsetProperty, ra::to_signed(nOffset)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the maximum scroll offset.
    /// </summary>
    static const IntModelProperty ScrollMaximumProperty;

    /// <summary>
    /// Gets the maximum scroll offset.
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
    /// Advances to the next page of results (if available).
    /// </summary>
    void NextPage();

    /// <summary>
    /// Returns to the previous page of results (if available).
    /// </summary>
    void PreviousPage();

    /// <summary>
    /// Starts a new search.
    /// </summary>
    void BeginNewSearch();

    /// <summary>
    /// Applies the current filter.
    /// </summary>
    void ApplyFilter();

    /// <summary>
    /// Excludes the currently selected items from the search results.
    /// </summary>
    void ExcludeSelected();

    /// <summary>
    /// Bookmarks the currently selected items from the search results.
    /// </summary>
    void BookmarkSelected();

protected:

    // ViewModelBase::NotifyTarget
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

private:
    bool ParseFilterRange(_Out_ ra::ByteAddress& nStart, _Out_ ra::ByteAddress& nEnd);
    void UpdateResults();
    void AddNewPage();
    void ChangePage(size_t nNewPage);

    LookupItemViewModelCollection m_vSearchTypes;
    LookupItemViewModelCollection m_vComparisonTypes;
    LookupItemViewModelCollection m_vValueTypes;

    ViewModelCollection<SearchResultViewModel> m_vResults;

    struct SearchResult
    {
        ra::services::SearchResults pResults;

        ValueType nValueType = ValueType::Constant;
        ComparisonType nCompareType = ComparisonType::Equals;
        unsigned int nValue;
    };

    size_t m_nSelectedSearchResult;
    std::vector<SearchResult> m_vSearchResults;
    std::set<unsigned int> m_vModifiedAddresses;
    std::set<unsigned int> m_vSelectedAddresses;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MEMORYSEARCHVIEWMODEL_H