#include "BrokenAchievementsDialog.hh"

#include "RA_Resource.h"

#include "api\SubmitTicket.hh"

#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::BrokenAchievementsViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

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
      m_bindComment(vmBrokenAchievements),
      m_bindAchievements(vmBrokenAchievements)
{
    m_bindWrongTime.BindCheck(BrokenAchievementsViewModel::SelectedProblemIdProperty,
        ra::etoi(ra::api::SubmitTicket::ProblemType::WrongTime));
    m_bindDidNotTrigger.BindCheck(BrokenAchievementsViewModel::SelectedProblemIdProperty,
        ra::etoi(ra::api::SubmitTicket::ProblemType::DidNotTrigger));

    m_bindComment.BindText(BrokenAchievementsViewModel::CommentProperty);

    auto pSelectedColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::LabelProperty);
    pSelectedColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 20);
    m_bindAchievements.BindColumn(0, std::move(pSelectedColumn));

    auto pTitleColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::LabelProperty);
    pTitleColumn->SetHeader(L"Title");
    pTitleColumn->SetWidth(GridColumnBinding::WidthType::Fill, 20);
    m_bindAchievements.BindColumn(1, std::move(pTitleColumn));

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::LabelProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    m_bindAchievements.BindColumn(2, std::move(pDescriptionColumn));

    auto pAchievedColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::LabelProperty);
    pAchievedColumn->SetHeader(L"Achieved");
    pAchievedColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    m_bindAchievements.BindColumn(3, std::move(pAchievedColumn));

    m_bindAchievements.BindItems(vmBrokenAchievements.Achievements());
}

BOOL BrokenAchievementsDialog::OnInitDialog()
{
    m_bindWrongTime.SetControl(*this, IDC_RA_PROBLEMTYPE1);
    m_bindDidNotTrigger.SetControl(*this, IDC_RA_PROBLEMTYPE2);

    m_bindComment.SetControl(*this, IDC_RA_BROKENACHIEVEMENTREPORTCOMMENT);

    m_bindAchievements.SetControl(*this, IDC_RA_REPORTBROKENACHIEVEMENTSLIST);

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
