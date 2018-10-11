#include "ui/win32/Desktop.hh"

// Win32 specific implementation of Desktop.hh

#include "ui/win32/Dlg_MessageBox.hh"

#ifndef PCH_H
#include <cassert>
#endif /* !PCH_H */

namespace ra {
namespace ui {
namespace win32 {

Desktop::Desktop() noexcept
{
    m_vDialogPresenters.emplace_back(new Dlg_MessageBox::Presenter());
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

ra::ui::win32::IDialogPresenter* Desktop::GetDialogController(ra::ui::WindowViewModelBase& oViewModel) const
{
    for (auto& pController : m_vDialogPresenters)
    {
        if (pController->IsSupported(oViewModel))
            return pController.get();
    }

    return nullptr;
}

void Desktop::Shutdown()
{
}

} // namespace win32
} // namespace ui
} // namespace ra
