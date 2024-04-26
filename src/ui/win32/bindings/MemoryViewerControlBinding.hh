#ifndef RA_UI_WIN32_MEMORYVIEWERCONTROLBINDING_H
#define RA_UI_WIN32_MEMORYVIEWERCONTROLBINDING_H
#pragma once

#include "ControlBinding.hh"

#include "ui/viewmodels/MemoryViewerViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class MemoryViewerControlBinding : public ControlBinding,
    protected ra::ui::viewmodels::MemoryViewerViewModel::RepaintNotifyTarget
{
public:
    explicit MemoryViewerControlBinding(ra::ui::viewmodels::MemoryViewerViewModel& vmViewModel) noexcept
        : ControlBinding(vmViewModel), m_pViewModel(vmViewModel)
    {
        vmViewModel.AddRepaintNotifyTarget(*this);
    }

    ~MemoryViewerControlBinding() noexcept
    {
        m_pViewModel.RemoveRepaintNotifyTarget(*this);
    }

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

    void SetHWND(DialogBase& pDialog, HWND hControl) override;

protected:
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) noexcept override;

    // MemoryViewerViewModel::RepaintNotifyTarget
    void OnRepaintMemoryViewer() override { Invalidate(); }

    void OnSizeChanged(const ra::ui::Size& pNewSize) override;

    INT_PTR CALLBACK WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    bool HandleNavigation(UINT nChar);
    bool m_bSuppressMemoryViewerInvalidate = false;

    ra::ui::viewmodels::MemoryViewerViewModel& m_pViewModel;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_MEMORYVIEWERCONTROLBINDING_H
