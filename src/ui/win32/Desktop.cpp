#include "ui/win32/Desktop.hh"

// Win32 specific implementation of Desktop.hh

#include "ui/win32/GameChecksumDialog.hh"
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
    m_vDialogPresenters.emplace_back(new GameChecksumDialog::Presenter());
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

void Desktop::GetWorkArea(ra::ui::Position& oUpperLeftCorner, ra::ui::Size& oSize) const
{
    if (m_oWorkAreaSize.Width == 0)
    {
        RECT rcWorkArea;
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0))
        {
            m_oWorkAreaPosition = { rcWorkArea.left, rcWorkArea.top };
            m_oWorkAreaSize = { rcWorkArea.right - rcWorkArea.left, rcWorkArea.bottom - rcWorkArea.top };

            RA_LOG_INFO("Desktop work area: %dx%d", m_oWorkAreaSize.Width, m_oWorkAreaSize.Height);
        }
        else
        {
            RA_LOG_ERR("Unable to determine work area");

            m_oWorkAreaPosition = { 0, 0 };
            m_oWorkAreaSize = { 800, 600 };
        }
    }

    oUpperLeftCorner = m_oWorkAreaPosition;
    oSize = m_oWorkAreaSize;
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
