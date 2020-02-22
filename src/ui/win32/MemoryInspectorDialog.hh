#ifndef RA_UI_WIN32_DLG_MEMORYINSPECTOR_H
#define RA_UI_WIN32_DLG_MEMORYINSPECTOR_H
#pragma once

#include "ui/viewmodels/MemoryInspectorViewModel.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"
#include "ui/win32/bindings/MemoryViewerControlBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

class MemoryInspectorDialog : public DialogBase
{
public:
    explicit MemoryInspectorDialog(ra::ui::viewmodels::MemoryInspectorViewModel& vmMemoryBookmarks);
    virtual ~MemoryInspectorDialog() noexcept = default;
    MemoryInspectorDialog(const MemoryInspectorDialog&) noexcept = delete;
    MemoryInspectorDialog& operator=(const MemoryInspectorDialog&) noexcept = delete;
    MemoryInspectorDialog(MemoryInspectorDialog&&) noexcept = delete;
    MemoryInspectorDialog& operator=(MemoryInspectorDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<MemoryInspectorDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    bindings::MemoryViewerControlBinding m_bindViewer;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_MEMORYINSPECTOR_H
