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

    void DoFrame();

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

        MemoryViewerViewModel& Viewer() noexcept { return m_pViewer; }
        const MemoryViewerViewModel& Viewer() const noexcept { return m_pViewer; }

        void DoFrame();

    protected:
        void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    private:
        void Capture();
        void ClearCapture();

        MemoryViewerViewModel m_pViewer;
        std::unique_ptr<ra::services::SearchResults> m_pCapture;
    };

    std::array<StateViewModel, 4>& States() noexcept { return m_vStates; }
    const std::array<StateViewModel, 4>& States() const noexcept { return m_vStates; }

private:
    std::array<StateViewModel, 4> m_vStates;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POINTERFINDERVIEWMODEL_H
