#include "TriggerSummaryDialog.hh"

#include "RA_Resource.h"

#include "data\context\EmulatorContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\PointerInspectorViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridMemoryWatchFormatColumnBinding.hh"
#include "ui\win32\bindings\GridMemoryWatchValueColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

#include "util\EnumOps.hh"
#include "util\Log.hh"

using ra::ui::viewmodels::TriggerSummaryViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool TriggerSummaryDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const TriggerSummaryViewModel*>(&vmViewModel) != nullptr);
}

void TriggerSummaryDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto* vmTriggerSummary = dynamic_cast<ra::ui::viewmodels::TriggerSummaryViewModel*>(&vmViewModel);
    Expects(vmTriggerSummary != nullptr);

    TriggerSummaryDialog oDialog(*vmTriggerSummary);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_TRIGGERSUMMARY), this, hParentWnd);
}

void TriggerSummaryDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    ShowModal(vmViewModel, nullptr);
}

// ------------------------------------

TriggerSummaryDialog::TriggerSummaryDialog(TriggerSummaryViewModel& vmTriggerSummary)
    : DialogBase(vmTriggerSummary),
      m_bindClauses(vmTriggerSummary)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center, "Trigger Summary");

    auto pIndicesColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        TriggerSummaryViewModel::TriggerClauseViewModel::IndicesProperty);
    pIndicesColumn->SetHeader(L"Indices");
    pIndicesColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    pIndicesColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindClauses.BindColumn(0, std::move(pIndicesColumn));

    auto pReferenceColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        TriggerSummaryViewModel::TriggerClauseViewModel::ReferenceProperty);
    pReferenceColumn->SetHeader(L"Reference");
    pReferenceColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    pReferenceColumn->BindTooltip(TriggerSummaryViewModel::TriggerClauseViewModel::ReferenceProperty);
    m_bindClauses.BindColumn(1, std::move(pReferenceColumn));

    auto pOperationColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        TriggerSummaryViewModel::TriggerClauseViewModel::OperationProperty);
    pOperationColumn->SetHeader(L"Operation");
    pOperationColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 140);
    m_bindClauses.BindColumn(2, std::move(pOperationColumn));

    auto pTargetColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        TriggerSummaryViewModel::TriggerClauseViewModel::TargetProperty);
    pTargetColumn->SetHeader(L"Target");
    pTargetColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    pTargetColumn->BindTooltip(TriggerSummaryViewModel::TriggerClauseViewModel::TargetProperty);
    m_bindClauses.BindColumn(3, std::move(pTargetColumn));

    auto pTallyColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        TriggerSummaryViewModel::TriggerClauseViewModel::TallyProperty);
    pTallyColumn->SetHeader(L"Tally");
    pTallyColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    m_bindClauses.BindColumn(4, std::move(pTallyColumn));

    m_bindClauses.BindRowColor(TriggerSummaryViewModel::TriggerClauseViewModel::ColorProperty);
    m_bindClauses.BindItems(vmTriggerSummary.Clauses());

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_LBX_CONDITIONS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDOK, Anchor::Bottom | Anchor::Right);

    SetMinimumSize(480, 192);
}

BOOL TriggerSummaryDialog::OnInitDialog()
{
    m_bindClauses.SetControl(*this, IDC_RA_LBX_CONDITIONS);
    m_bindClauses.InitializeTooltips(std::chrono::seconds(30));

    return DialogBase::OnInitDialog();
}

} // namespace win32
} // namespace ui
} // namespace ra
