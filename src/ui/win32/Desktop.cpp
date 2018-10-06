#include "ui/win32/Desktop.hh"

// Win32 specific implementation of Desktop.hh

#include "ui/win32/MessageBoxDialog.hh"
#include "ui/win32/RichPresenceDialog.hh"

#include <assert.h>

namespace ra {
namespace ui {
namespace win32 {

Desktop::Desktop() noexcept
{
    m_vDialogPresenters.emplace_back(new MessageBoxDialog::Presenter());
    m_vDialogPresenters.emplace_back(new RichPresenceDialog::Presenter());
}

void Desktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    auto* pPresenter = GetDialogPresenter(vmViewModel);
    if (pPresenter != nullptr)
    {
        pPresenter->ShowWindow(vmViewModel);
    }
    else
    {
        assert("!No presenter for view model");
    }
}

ra::ui::DialogResult Desktop::ShowModal(WindowViewModelBase& vmViewModel) const
{
    auto* pPresenter = GetDialogPresenter(vmViewModel);
    if (pPresenter != nullptr)
    {
        pPresenter->ShowModal(vmViewModel);
    }
    else
    {
        assert("!No presenter for view model");
    }

    return vmViewModel.GetDialogResult();
}

ra::ui::win32::IDialogPresenter* Desktop::GetDialogPresenter(ra::ui::WindowViewModelBase& oViewModel) const
{
    for (auto& pPresenter : m_vDialogPresenters)
    {
        if (pPresenter->IsSupported(oViewModel))
            return pPresenter.get();
    }

    return nullptr;
}

void Desktop::Shutdown()
{
}

} // namespace win32
} // namespace ui
} // namespace ra
