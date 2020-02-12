#include "CodeNotesDialog.hh"

#include "RA_Log.h"
#include "RA_Resource.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::CodeNotesViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool CodeNotesDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const CodeNotesViewModel*>(&vmViewModel) != nullptr);
}

void CodeNotesDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void CodeNotesDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto& vmCodeNotes = reinterpret_cast<CodeNotesViewModel&>(vmViewModel);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new CodeNotesDialog(vmCodeNotes));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_CODENOTES), this))
            RA_LOG_ERR("Could not create Code Notes dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void CodeNotesDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

CodeNotesDialog::CodeNotesDialog(CodeNotesViewModel& vmCodeNotes)
    : DialogBase(vmCodeNotes),
      m_bindNotes(vmCodeNotes)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Code Notes");
    m_bindNotes.BindIsSelected(CodeNotesViewModel::CodeNoteViewModel::IsSelectedProperty);

    auto pAddressColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        CodeNotesViewModel::CodeNoteViewModel::LabelProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    pAddressColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindNotes.BindColumn(0, std::move(pAddressColumn));

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        CodeNotesViewModel::CodeNoteViewModel::NoteProperty);
    pDescriptionColumn->SetHeader(L"Note");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    m_bindNotes.BindColumn(1, std::move(pDescriptionColumn));

    m_bindNotes.BindItems(vmCodeNotes.Notes());

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_FILTER_VALUE, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_APPLY_FILTER, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_ADDBOOKMARK, Anchor::Top);
    SetAnchor(IDC_RA_RESET_FILTER_RESET, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_ADDRESSES, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
}

BOOL CodeNotesDialog::OnInitDialog()
{
    m_bindNotes.SetControl(*this, IDC_RA_LBX_ADDRESSES);

    return DialogBase::OnInitDialog();
}

BOOL CodeNotesDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_APPLY_FILTER:
        {
            auto& vmCodeNotes = reinterpret_cast<CodeNotesViewModel&>(m_vmWindow);
            vmCodeNotes.ApplyFilter();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
