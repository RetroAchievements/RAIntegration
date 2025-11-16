#include "BrokenAchievementsDialog.hh"

#include "RA_Resource.h"

#include "api\SubmitTicket.hh"

#include "ui\win32\bindings\GridBooleanColumnBinding.hh"
#include "ui\win32\bindings\GridCheckBoxColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

#include "util\EnumOps.hh"

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
    auto* vmBrokenAchievements = dynamic_cast<BrokenAchievementsViewModel*>(&vmViewModel);
    Expects(vmBrokenAchievements != nullptr);

    BrokenAchievementsDialog oDialog(*vmBrokenAchievements);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_REPORTBROKENACHIEVEMENTS), this, hParentWnd);
}

void BrokenAchievementsDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    ShowModal(oViewModel, nullptr);
}

// ------------------------------------

BrokenAchievementsDialog::BrokenAchievementsDialog(BrokenAchievementsViewModel& vmBrokenAchievements)
    : DialogBase(vmBrokenAchievements),
      m_bindAchievements(vmBrokenAchievements)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center, "Report Broken Achievements");

    auto pSelectedColumn = std::make_unique<ra::ui::win32::bindings::GridCheckBoxColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::IsSelectedProperty);
    pSelectedColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 20);
    m_bindAchievements.BindColumn(0, std::move(pSelectedColumn));

    auto pTitleColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::LabelProperty);
    pTitleColumn->SetHeader(L"Title");
    pTitleColumn->SetWidth(GridColumnBinding::WidthType::Fill, 20);
    m_bindAchievements.BindColumn(1, std::move(pTitleColumn));

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    m_bindAchievements.BindColumn(2, std::move(pDescriptionColumn));

    auto pAchievedColumn = std::make_unique<ra::ui::win32::bindings::GridBooleanColumnBinding>(
        BrokenAchievementsViewModel::BrokenAchievementViewModel::IsAchievedProperty, L"Yes", L"No");
    pAchievedColumn->SetHeader(L"Achieved");
    pAchievedColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    m_bindAchievements.BindColumn(3, std::move(pAchievedColumn));

    m_bindAchievements.BindItems(vmBrokenAchievements.Achievements());

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_SELECT_ACHIEVEMENTS, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_REPORTBROKENACHIEVEMENTSLIST, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDCANCEL, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDOK, Anchor::Bottom | Anchor::Right);

    SetMinimumSize(416, 322);
}

BOOL BrokenAchievementsDialog::OnInitDialog()
{
    m_bindAchievements.SetControl(*this, IDC_RA_REPORTBROKENACHIEVEMENTSLIST);

    return DialogBase::OnInitDialog();
}

BOOL BrokenAchievementsDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDOK)
    {
        auto* vmBrokenAchievements = dynamic_cast<BrokenAchievementsViewModel*>(&m_vmWindow);
        if (vmBrokenAchievements && vmBrokenAchievements->Submit())
            SetDialogResult(ra::ui::DialogResult::OK);
        return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
