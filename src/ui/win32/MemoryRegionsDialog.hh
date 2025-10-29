#ifndef RA_UI_WIN32_DLG_MEMORYREGIONS_H
#define RA_UI_WIN32_DLG_MEMORYREGIONS_H
#pragma once

#include "ui/viewmodels/MemoryRegionsViewModel.hh"

#include "ui/win32/bindings/GridBinding.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class MemoryRegionsDialog : public DialogBase
{
public:
    explicit MemoryRegionsDialog(ra::ui::viewmodels::MemoryRegionsViewModel& vmMemoryRegions);
    virtual ~MemoryRegionsDialog() noexcept = default;
    MemoryRegionsDialog(const MemoryRegionsDialog&) noexcept = delete;
    MemoryRegionsDialog& operator=(const MemoryRegionsDialog&) noexcept = delete;
    MemoryRegionsDialog(MemoryRegionsDialog&&) noexcept = delete;
    MemoryRegionsDialog& operator=(MemoryRegionsDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<MemoryRegionsDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    ra::ui::win32::bindings::GridBinding m_bindRegions;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_MEMORYREGIONS_H
