#ifndef RA_UI_WIN32_MEMORYVIEWERCONTROLBINDING_H
#define RA_UI_WIN32_MEMORYVIEWERCONTROLBINDING_H
#pragma once

#include "ControlBinding.hh"

#include "ui/viewmodels/MemoryViewerViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class MemoryViewerControlBinding : public ControlBinding
{
public:
    explicit MemoryViewerControlBinding(ra::ui::viewmodels::MemoryViewerViewModel& vmViewModel) noexcept
        : ControlBinding(vmViewModel), m_pViewModel(vmViewModel)
    {
    }

    ~MemoryViewerControlBinding() noexcept = default;

    MemoryViewerControlBinding(const MemoryViewerControlBinding&) noexcept = delete;
    MemoryViewerControlBinding& operator=(const MemoryViewerControlBinding&) noexcept = delete;
    MemoryViewerControlBinding(MemoryViewerControlBinding&&) noexcept = delete;
    MemoryViewerControlBinding& operator=(MemoryViewerControlBinding&&) noexcept = delete;


    static void RegisterControlClass() noexcept;

    void RenderMemViewer();

    void ScrollUp();
    void ScrollDown();

    void OnClick(POINT point);

    bool OnKeyDown(UINT nChar);
    bool OnEditInput(UINT c);

    void OnGotFocus() override;
    void OnLostFocus() override;

    void Invalidate();

    MemSize GetDataSize();

    void SetHWND(DialogBase& pDialog, HWND hControl) override;

private:
    bool m_bSuppressMemoryViewerInvalidate = false;

    ra::ui::viewmodels::MemoryViewerViewModel& m_pViewModel;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_MEMORYVIEWERCONTROLBINDING_H
