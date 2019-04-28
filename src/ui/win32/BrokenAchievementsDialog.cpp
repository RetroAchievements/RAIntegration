#include "BrokenAchievementsDialog.hh"

#include "RA_Resource.h"

#include "api\SubmitTicket.hh"

using ra::ui::viewmodels::BrokenAchievementsViewModel;

namespace ra {
namespace ui {
namespace win32 {

bool BrokenAchievementsDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const BrokenAchievementsViewModel*>(&vmViewModel) != nullptr);
}

void BrokenAchievementsDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto& vmBrokenAchievements = reinterpret_cast<BrokenAchievementsViewModel&>(vmViewModel);

    BrokenAchievementsDialog oDialog(vmBrokenAchievements);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_REPORTBROKENACHIEVEMENTS), this, hParentWnd);
}

void BrokenAchievementsDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    ShowModal(oViewModel, nullptr);
}

// ------------------------------------

BrokenAchievementsDialog::BrokenAchievementsDialog(BrokenAchievementsViewModel& vmBrokenAchievements)
    : DialogBase(vmBrokenAchievements),
      m_bindWrongTime(vmBrokenAchievements),
      m_bindDidNotTrigger(vmBrokenAchievements),
      m_bindComment(vmBrokenAchievements)
{
    m_bindWrongTime.BindCheck(BrokenAchievementsViewModel::SelectedProblemIdProperty,
        ra::etoi(ra::api::SubmitTicket::ProblemType::WrongTime));
    m_bindDidNotTrigger.BindCheck(BrokenAchievementsViewModel::SelectedProblemIdProperty,
        ra::etoi(ra::api::SubmitTicket::ProblemType::DidNotTrigger));

    m_bindComment.BindText(BrokenAchievementsViewModel::CommentProperty);
}

BOOL BrokenAchievementsDialog::OnInitDialog()
{
    m_bindWrongTime.SetControl(*this, IDC_RA_PROBLEMTYPE1);
    m_bindDidNotTrigger.SetControl(*this, IDC_RA_PROBLEMTYPE2);

    m_bindComment.SetControl(*this, IDC_RA_BROKENACHIEVEMENTREPORTCOMMENT);

    return DialogBase::OnInitDialog();
}

BOOL BrokenAchievementsDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDOK)
    {
        auto& vmBrokenAchievements = reinterpret_cast<BrokenAchievementsViewModel&>(m_vmWindow);
        if (vmBrokenAchievements.Submit())
            SetDialogResult(ra::ui::DialogResult::OK);
        return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
