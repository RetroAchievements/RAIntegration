#include "PointerInspectorDialog.hh"

#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_Resource.h"

#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridBookmarkFormatColumnBinding.hh"
#include "ui\win32\bindings\GridBookmarkValueColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::PointerInspectorViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool PointerInspectorDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const PointerInspectorViewModel*>(&vmViewModel) != nullptr);
}

void PointerInspectorDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void PointerInspectorDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmPointerFinder = dynamic_cast<PointerInspectorViewModel*>(&vmViewModel);
    Expects(vmPointerFinder != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new PointerInspectorDialog(*vmPointerFinder));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_POINTERINSPECTOR), this))
            RA_LOG_ERR("Could not create Pointer Inspector dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void PointerInspectorDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

PointerInspectorDialog::PointerInspectorDialog(PointerInspectorViewModel& vmPointerFinder)
    : DialogBase(vmPointerFinder),
      m_bindAddress(vmPointerFinder),
      m_bindNodes(vmPointerFinder),
      m_bindDescription(vmPointerFinder),
      m_bindPointerChain(vmPointerFinder),
      m_bindFields(vmPointerFinder),
      m_bindFieldNote(vmPointerFinder)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Pointer Finder");
    m_bindWindow.BindLabel(IDC_RA_PAUSE, PointerInspectorViewModel::PauseButtonTextProperty);
    m_bindWindow.BindLabel(IDC_RA_FREEZE, PointerInspectorViewModel::FreezeButtonTextProperty);
    m_bindWindow.BindEnabled(IDC_RA_ADDBOOKMARK, PointerInspectorViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_COPY_ALL, PointerInspectorViewModel::HasSingleSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_PAUSE, PointerInspectorViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_FREEZE, PointerInspectorViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_NOTE_TEXT, PointerInspectorViewModel::HasSingleSelectionProperty);

    m_bindAddress.BindText(PointerInspectorViewModel::CurrentAddressTextProperty, ra::ui::win32::bindings::TextBoxBinding::UpdateMode::Typing);
    m_bindNodes.BindItems(vmPointerFinder.Nodes());
    m_bindNodes.BindSelectedItem(PointerInspectorViewModel::SelectedNodeProperty);
    m_bindDescription.BindText(PointerInspectorViewModel::CurrentAddressNoteProperty);

    auto pOffsetColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::OffsetProperty);
    pOffsetColumn->SetHeader(L"Offset");
    pOffsetColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    m_bindFields.BindColumn(0, std::move(pOffsetColumn));

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 80);
    m_bindFields.BindColumn(1, std::move(pDescriptionColumn));

    auto pSizeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::SizeProperty, vmPointerFinder.Sizes());
    pSizeColumn->SetHeader(L"Size");
    pSizeColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 76);
    pSizeColumn->SetAlignment(ra::ui::RelativePosition::Far);
    pSizeColumn->SetReadOnly(false);
    m_bindFields.BindColumn(2, std::move(pSizeColumn));

    auto pFormatColumn = std::make_unique<ra::ui::win32::bindings::GridBookmarkFormatColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::FormatProperty, vmPointerFinder.Formats());
    pFormatColumn->SetHeader(L"Format");
    pFormatColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 32);
    pFormatColumn->SetReadOnly(false);
    m_bindFields.BindColumn(3, std::move(pFormatColumn));

    auto pValueColumn = std::make_unique<ra::ui::win32::bindings::GridBookmarkValueColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::CurrentValueProperty);
    pValueColumn->SetHeader(L"Value");
    pValueColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    pValueColumn->SetAlignment(ra::ui::RelativePosition::Center);
    pValueColumn->SetReadOnly(false);
    m_bindFields.BindColumn(4, std::move(pValueColumn));

    auto pBehaviorColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::BehaviorProperty, vmPointerFinder.Behaviors());
    pBehaviorColumn->SetHeader(L"Behavior");
    pBehaviorColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    pBehaviorColumn->SetReadOnly(false);
    m_bindFields.BindColumn(5, std::move(pBehaviorColumn));

    m_bindFields.BindItems(vmPointerFinder.Bookmarks());
    m_bindFields.BindIsSelected(PointerInspectorViewModel::StructFieldViewModel::IsSelectedProperty);
    m_bindFields.BindRowColor(PointerInspectorViewModel::StructFieldViewModel::RowColorProperty);
    m_bindFields.SetShowGridLines(true);

    pOffsetColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::OffsetProperty);
    pOffsetColumn->SetHeader(L"Offset");
    pOffsetColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    m_bindPointerChain.BindColumn(0, std::move(pOffsetColumn));

    auto pAddressColumn = std::make_unique<ra::ui::win32::bindings::GridAddressColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::AddressProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    m_bindPointerChain.BindColumn(1, std::move(pAddressColumn));

    pValueColumn = std::make_unique<ra::ui::win32::bindings::GridBookmarkValueColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::CurrentValueProperty);
    pValueColumn->SetHeader(L"Value");
    pValueColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    pValueColumn->SetAlignment(ra::ui::RelativePosition::Center);
    m_bindPointerChain.BindColumn(2, std::move(pValueColumn));

    pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerInspectorViewModel::StructFieldViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 80);
    m_bindPointerChain.BindColumn(3, std::move(pDescriptionColumn));

    m_bindPointerChain.BindItems(vmPointerFinder.PointerChain());
    m_bindPointerChain.BindRowColor(PointerInspectorViewModel::StructFieldViewModel::RowColorProperty);

    m_bindFieldNote.BindText(PointerInspectorViewModel::CurrentFieldNoteProperty,
                             ra::ui::win32::bindings::TextBoxBinding::UpdateMode::Typing);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_ADDRESS, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_FILTER_VALUE, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_DESCRIPTION, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_LBX_GROUPS, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_LBX_ADDRESSES, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_NOTE_TEXT, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADDBOOKMARK, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_COPY_ALL, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_PAUSE, Anchor::Right | Anchor::Bottom);
    SetAnchor(IDC_RA_FREEZE, Anchor::Right | Anchor::Bottom);

    SetMinimumSize(480, 258);
}

BOOL PointerInspectorDialog::OnInitDialog()
{
    m_bindAddress.SetControl(*this, IDC_RA_ADDRESS);
    m_bindNodes.SetControl(*this, IDC_RA_FILTER_VALUE);
    m_bindDescription.SetControl(*this, IDC_RA_DESCRIPTION);
    m_bindPointerChain.SetControl(*this, IDC_RA_LBX_GROUPS);
    m_bindFields.SetControl(*this, IDC_RA_LBX_ADDRESSES);
    m_bindFieldNote.SetControl(*this, IDC_RA_NOTE_TEXT);

    return DialogBase::OnInitDialog();
}

BOOL PointerInspectorDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_ADDBOOKMARK: {
            const auto* vmPointerInspector = dynamic_cast<PointerInspectorViewModel*>(&m_vmWindow);
            if (vmPointerInspector)
                vmPointerInspector->BookmarkCurrentField();

            return TRUE;
        }

        case IDC_RA_COPY_ALL: {
            const auto* vmPointerInspector = dynamic_cast<PointerInspectorViewModel*>(&m_vmWindow);
            if (vmPointerInspector)
                vmPointerInspector->CopyDefinition();

            return TRUE;
        }

        case IDC_RA_PAUSE: {
            auto* vmPointerInspector = dynamic_cast<PointerInspectorViewModel*>(&m_vmWindow);
            if (vmPointerInspector)
                vmPointerInspector->TogglePauseSelected();

            return TRUE;
        }

        case IDC_RA_FREEZE: {
            auto* vmPointerInspector = dynamic_cast<PointerInspectorViewModel*>(&m_vmWindow);
            if (vmPointerInspector)
                vmPointerInspector->ToggleFreezeSelected();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
