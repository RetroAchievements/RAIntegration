#include "ui/win32/Desktop.hh"

// Win32 specific implementation of Desktop.hh

#include "ui/win32/Dlg_MessageBox.hh"

#include <assert.h>

namespace ra {
namespace ui {
namespace win32 {

Desktop::Desktop() noexcept
{
    m_vDialogControllers.push_back(new Dlg_MessageBox::Controller());
}

void Desktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    auto* pController = GetDialogController(vmViewModel);
    if (pController != nullptr)
    {
        pController->ShowWindow(vmViewModel);
    }
    else
    {
        assert("!No controller for view model");
    }
}

ra::ui::DialogResult Desktop::ShowModal(WindowViewModelBase& vmViewModel) const
{
    auto* pController = GetDialogController(vmViewModel);
    if (pController != nullptr)
    {
        pController->ShowModal(vmViewModel);
    }
    else
    {
        assert("!No controller for view model");
    }

    return vmViewModel.GetDialogResult();
}

ra::ui::win32::IDialogController* Desktop::GetDialogController(ra::ui::WindowViewModelBase& oViewModel) const
{
    for (ra::ui::win32::IDialogController* pController : m_vDialogControllers)
    {
        if (pController->IsSupported(oViewModel))
            return pController;
    }

    return nullptr;
}

void Desktop::Shutdown()
{
}

} // namespace win32
} // namespace ui
} // namespace ra
