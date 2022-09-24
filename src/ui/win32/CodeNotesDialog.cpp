#include "CodeNotesDialog.hh"

#include "RA_Log.h"
#include "RA_Resource.h"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
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

void CodeNotesDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&vmViewModel);
    Expects(vmCodeNotes != nullptr);

    CodeNotesDialog oDialog(*vmCodeNotes);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_CODENOTES), this, hParentWnd);
}

void CodeNotesDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&vmViewModel);
    Expects(vmCodeNotes != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new CodeNotesDialog(*vmCodeNotes));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_CODENOTES), this))
            RA_LOG_ERR("Could not create Code Notes dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void CodeNotesDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

CodeNotesDialog::CodeNotesGridBinding::CodeNotesGridBinding(ViewModelBase& vmViewModel)
    : ra::ui::win32::bindings::MultiLineGridBinding(vmViewModel)
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);
}

CodeNotesDialog::CodeNotesGridBinding::~CodeNotesGridBinding()
{
    if (ra::services::ServiceLocator::Exists<ra::data::context::EmulatorContext>())
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        pEmulatorContext.RemoveNotifyTarget(*this);
    }
}

void CodeNotesDialog::CodeNotesGridBinding::OnTotalMemorySizeChanged()
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vColumns.size()); ++nIndex)
    {
        auto* pAddressColumn = dynamic_cast<ra::ui::win32::bindings::GridTextColumnBinding*>(m_vColumns.at(nIndex).get());
        if (pAddressColumn != nullptr && pAddressColumn->GetHeader() == L"Address")
        {
            pAddressColumn->SetWidth(GridColumnBinding::WidthType::Pixels,
                ra::ui::win32::bindings::GridAddressColumnBinding::CalculateWidth() + 12);
            RefreshColumn(nIndex);
        }
    }

    UpdateLayout();
}

// ------------------------------------

CodeNotesDialog::CodeNotesDialog(CodeNotesViewModel& vmCodeNotes)
    : DialogBase(vmCodeNotes),
    m_bindNotes(vmCodeNotes),
    m_bindFilterValue(vmCodeNotes)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Code Notes");

    m_bindFilterValue.BindText(CodeNotesViewModel::FilterValueProperty);
    m_bindFilterValue.BindKey(VK_RETURN, [this]()
    {
        m_bindFilterValue.UpdateSource();

        auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&m_vmWindow);
        vmCodeNotes->ApplyFilter();
        return true;
    });
    m_bindFilterValue.BindKey(VK_ESCAPE, [this]()
    {
        auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&m_vmWindow);
        vmCodeNotes->SetFilterValue(L"");
        vmCodeNotes->ResetFilter();
        return true;
    });

    m_bindWindow.BindLabel(IDC_RA_RESULT_COUNT, CodeNotesViewModel::ResultCountProperty);

    auto pAddressColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        CodeNotesViewModel::CodeNoteViewModel::LabelProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->SetWidth(GridColumnBinding::WidthType::Pixels,
        ra::ui::win32::bindings::GridAddressColumnBinding::CalculateWidth() + 12);
    pAddressColumn->SetTextColorProperty(CodeNotesViewModel::CodeNoteViewModel::BookmarkColorProperty);
    m_bindNotes.BindColumn(0, std::move(pAddressColumn));

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        CodeNotesViewModel::CodeNoteViewModel::NoteProperty);
    pDescriptionColumn->SetHeader(L"Note");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    pDescriptionColumn->SetTextColorProperty(CodeNotesViewModel::CodeNoteViewModel::BookmarkColorProperty);
    m_bindNotes.BindColumn(1, std::move(pDescriptionColumn));

    m_bindNotes.BindItems(vmCodeNotes.Notes());
    m_bindNotes.BindIsSelected(CodeNotesViewModel::CodeNoteViewModel::IsSelectedProperty);
    m_bindNotes.SetDoubleClickHandler([this](gsl::index nIndex)
    {
        const auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&m_vmWindow);
        const auto* pItem = vmCodeNotes->Notes().GetItemAt(nIndex);
        if (pItem)
        {
            auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
            pWindowManager.MemoryInspector.SetCurrentAddress(pItem->nAddress);
        }
    });

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_FILTER_VALUE, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_APPLY_FILTER, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_RESULT_COUNT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_ADDBOOKMARK, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_RESET_FILTER, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_ADDRESSES, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetMinimumSize(327, 200);
}

BOOL CodeNotesDialog::OnInitDialog()
{
    m_bindNotes.SetControl(*this, IDC_RA_LBX_ADDRESSES);
    m_bindFilterValue.SetControl(*this, IDC_RA_FILTER_VALUE);

    auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&m_vmWindow);
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vmCodeNotes->Notes().Count()); ++nIndex)
    {
        auto* pItem = vmCodeNotes->Notes().GetItemAt(nIndex);
        if (pItem && pItem->IsSelected())
        {
            m_bindNotes.EnsureVisible(nIndex);
            break;
        }
    }

    return DialogBase::OnInitDialog();
}


BOOL CodeNotesDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_RESET_FILTER:
        {
            auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&m_vmWindow);
            if (vmCodeNotes)
                vmCodeNotes->ResetFilter();

            return TRUE;
        }

        case IDOK: // this dialog doesn't have an OK button. if the user pressed Enter, apply the current filter
        case IDC_RA_APPLY_FILTER:
        {
            auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&m_vmWindow);
            if (vmCodeNotes)
                vmCodeNotes->ApplyFilter();

            return TRUE;
        }

        case IDC_RA_ADDBOOKMARK:
        {
            const auto* vmCodeNotes = dynamic_cast<CodeNotesViewModel*>(&m_vmWindow);
            if (vmCodeNotes)
                vmCodeNotes->BookmarkSelected();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
