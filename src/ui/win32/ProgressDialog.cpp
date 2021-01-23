#include "ProgressDialog.hh"

#include "RA_Log.h"
#include "RA_Resource.h"

using ra::ui::viewmodels::ProgressViewModel;

namespace ra {
namespace ui {
namespace win32 {

bool ProgressDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& oViewModel) noexcept
{
    return (dynamic_cast<const ProgressViewModel*>(&oViewModel) != nullptr);
}

void ProgressDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto* vmUnknownGame = dynamic_cast<ra::ui::viewmodels::ProgressViewModel*>(&vmViewModel);
    Expects(vmUnknownGame != nullptr);

    ProgressDialog oDialog(*vmUnknownGame);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_PROGRESS), this, hParentWnd);
}

void ProgressDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmProgress = dynamic_cast<ProgressViewModel*>(&vmViewModel);
    Expects(vmProgress != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new ProgressDialog(*vmProgress));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_PROGRESS), this))
            RA_LOG_ERR("Could not create Progress dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void ProgressDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

ProgressDialog::ProgressDialog(ProgressViewModel& vmProgress)
    : DialogBase(vmProgress),
      m_bindProgress(vmProgress)
{
    m_bindWindow.BindLabel(IDC_RA_MESSAGE, ProgressViewModel::MessageProperty);

    m_bindProgress.BindProgress(ProgressViewModel::ProgressProperty);
}

BOOL ProgressDialog::OnInitDialog()
{
    m_bindProgress.SetControl(*this, IDC_RA_PROGRESSBAR);

    return DialogBase::OnInitDialog();
}

BOOL ProgressDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDCLOSE:
        {
            auto* vmProgress = dynamic_cast<ProgressViewModel*>(&m_vmWindow);
            if (vmProgress)
                vmProgress->SetDialogResult(ra::ui::DialogResult::Cancel);

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
