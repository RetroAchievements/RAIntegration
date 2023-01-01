#ifndef RA_UI_POINTERFINDERVIEWMODEL_H
#define RA_UI_POINTERFINDERVIEWMODEL_H
#pragma once

#include "services/SearchResults.h"

#include "ui/WindowViewModelBase.hh"

#include "ui/viewmodels/MemoryViewerViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class PointerFinderViewModel : public WindowViewModelBase
{
public:
    GSL_SUPPRESS_F6 PointerFinderViewModel();
    ~PointerFinderViewModel() = default;

    PointerFinderViewModel(const PointerFinderViewModel&) noexcept = delete;
    PointerFinderViewModel& operator=(const PointerFinderViewModel&) noexcept = delete;
    PointerFinderViewModel(PointerFinderViewModel&&) noexcept = delete;
    PointerFinderViewModel& operator=(PointerFinderViewModel&&) noexcept = delete;

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
    /// Gets the list of selectable search types.
    /// </summary>
    const LookupItemViewModelCollection& SearchTypes() const noexcept
    {
        return m_vSearchTypes;
    }

    void DoFrame();

    void Find();

    void ExportResults() const;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the string representation of the number of results found. />.
    /// </summary>
    static const StringModelProperty ResultCountTextProperty;

    /// <summary>
    /// Gets the string representation of the number of results found.
    /// </summary>
    const std::wstring& GetResultCountText() const { return GetValue(ResultCountTextProperty); }

    /// <summary>
    /// Bookmarks the currently item from the search results.
    /// </summary>
    void BookmarkSelected();

    class StateViewModel : public ViewModelBase
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the address.
        /// </summary>
        static const StringModelProperty AddressProperty;

        /// <summary>
        /// Gets the current address.
        /// </summary>
        const std::wstring& GetAddress() const { return GetValue(AddressProperty); }

        /// <summary>
        /// Sets the current address.
        /// </summary>
        void SetAddress(const std::wstring& sValue) { SetValue(AddressProperty, sValue); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the capture button text.
        /// </summary>
        static const StringModelProperty CaptureButtonTextProperty;

        /// <summary>
        /// Gets the capture button text.
        /// </summary>
        const std::wstring& GetCaptureButtonText() const { return GetValue(CaptureButtonTextProperty); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for whether or not the capture button is enabled.
        /// </summary>
        static const BoolModelProperty CanCaptureProperty;

        /// <summary>
        /// Gets whether or not the capture button is enabled.
        /// </summary>
        bool CanCapture() const { return GetValue(CanCaptureProperty); }

        void ToggleCapture();

        const ra::services::SearchResults* CapturedMemory() const noexcept { return m_pCapture.get(); }

        MemoryViewerViewModel& Viewer() noexcept { return m_pViewer; }
        const MemoryViewerViewModel& Viewer() const noexcept { return m_pViewer; }

        void DoFrame();

        void SetOwner(PointerFinderViewModel* pOwner) noexcept { m_pOwner = pOwner; }

    protected:
        void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    private:
        void Capture();
        void ClearCapture();

        PointerFinderViewModel* m_pOwner = nullptr;
        MemoryViewerViewModel m_pViewer;
        std::unique_ptr<ra::services::SearchResults> m_pCapture;
    };

    std::array<StateViewModel, 4>& States() noexcept { return m_vStates; }
    const std::array<StateViewModel, 4>& States() const noexcept { return m_vStates; }

    class PotentialPointerViewModel : public ViewModelBase
    {
    public:
        /// <summary>
        /// The <see cref="ModelProperty" /> for the pointer address.
        /// </summary>
        static const StringModelProperty PointerAddressProperty;

        /// <summary>
        /// Gets the pointer address.
        /// </summary>
        const std::wstring& GetPointerAddress() const { return GetValue(PointerAddressProperty); }

        /// <summary>
        /// Sets the pointer address.
        /// </summary>
        void SetPointerAddress(const std::wstring& value) { SetValue(PointerAddressProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the offset from the pointer address to the data.
        /// </summary>
        static const StringModelProperty OffsetProperty;

        /// <summary>
        /// Gets the offset from the pointer address to the data. 
        /// </summary>
        const std::wstring& GetOffset() const { return GetValue(OffsetProperty); }

        /// <summary>
        /// Sets the offset from the pointer address to the data.
        /// </summary>
        void SetOffset(const std::wstring& value) { SetValue(OffsetProperty, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the state 1 pointer value.
        /// </summary>
        static const StringModelProperty PointerValue1Property;

        /// <summary>
        /// Gets the state 1 pointer value.
        /// </summary>
        const std::wstring& GetPointerValue1() const { return GetValue(PointerValue1Property); }

        /// <summary>
        /// Sets the state 1 pointer value.
        /// </summary>
        void SetPointerValue1(const std::wstring& value) { SetValue(PointerValue1Property, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the state 2 pointer value.
        /// </summary>
        static const StringModelProperty PointerValue2Property;

        /// <summary>
        /// Gets the state 2 pointer value.
        /// </summary>
        const std::wstring& GetPointerValue2() const { return GetValue(PointerValue2Property); }

        /// <summary>
        /// Sets the state 2 pointer value.
        /// </summary>
        void SetPointerValue2(const std::wstring& value) { SetValue(PointerValue2Property, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the state 3 pointer value.
        /// </summary>
        static const StringModelProperty PointerValue3Property;

        /// <summary>
        /// Gets the state 3 pointer value.
        /// </summary>
        const std::wstring& GetPointerValue3() const { return GetValue(PointerValue3Property); }

        /// <summary>
        /// Sets the state 3 pointer value.
        /// </summary>
        void SetPointerValue3(const std::wstring& value) { SetValue(PointerValue3Property, value); }

        /// <summary>
        /// The <see cref="ModelProperty" /> for the state 4 pointer value.
        /// </summary>
        static const StringModelProperty PointerValue4Property;

        /// <summary>
        /// Gets the state 4 pointer value.
        /// </summary>
        const std::wstring& GetPointerValue4() const { return GetValue(PointerValue4Property); }

        /// <summary>
        /// Sets the state 4 pointer value.
        /// </summary>
        void SetPointerValue4(const std::wstring& value) { SetValue(PointerValue4Property, value); }

        const std::wstring& GetPointerValue(gsl::index nIndex) const
        {
            switch (nIndex)
            {
                default: return GetPointerValue1();
                case 1: return GetPointerValue2();
                case 2: return GetPointerValue3();
                case 3: return GetPointerValue4();
            }
        }

        void SetPointerValue(gsl::index nIndex, const std::wstring& sValue)
        {
            switch (nIndex)
            {
                case 0: SetPointerValue1(sValue); break;
                case 1: SetPointerValue2(sValue); break;
                case 2: SetPointerValue3(sValue); break;
                case 3: SetPointerValue4(sValue); break;
            }
        }
        
        ra::ByteAddress GetRawAddress() const noexcept { return m_nAddress; }
        
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

    private:
        friend class PointerFinderViewModel;
        ra::ByteAddress m_nAddress = 0;
        bool m_bMatched = false;
    };

    ra::ui::ViewModelCollection<PotentialPointerViewModel>& PotentialPointers() noexcept
    {
        return m_vResults;
    }

    const ra::ui::ViewModelCollection<PotentialPointerViewModel>& PotentialPointers() const noexcept
    {
        return m_vResults;
    }

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

private:
    std::array<StateViewModel, 4> m_vStates;

    ra::ui::ViewModelCollection<PotentialPointerViewModel> m_vResults;
    LookupItemViewModelCollection m_vSearchTypes;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POINTERFINDERVIEWMODEL_H
