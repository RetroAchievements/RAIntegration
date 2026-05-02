#include "MemoryNotesDialog.hh"

#include "RA_Resource.h"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

#include "util\EnumOps.hh"
#include "util\Log.hh"

using ra::ui::viewmodels::MemoryNotesViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool MemoryNotesDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const MemoryNotesViewModel*>(&vmViewModel) != nullptr);
}

void MemoryNotesDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&vmViewModel);
    Expects(vmMemoryNotes != nullptr);

    MemoryNotesDialog oDialog(*vmMemoryNotes);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_CODENOTES), this, hParentWnd);
}

void MemoryNotesDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&vmViewModel);
    Expects(vmMemoryNotes != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new MemoryNotesDialog(*vmMemoryNotes));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_CODENOTES), this))
            RA_LOG_ERR("Could not create Memory Notes dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void MemoryNotesDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

MemoryNotesDialog::MemoryNotesGridBinding::MemoryNotesGridBinding(ViewModelBase& vmViewModel)
    : ra::ui::win32::bindings::MultiLineGridBinding(vmViewModel)
{
    auto& pMemoryContext = ra::services::ServiceLocator::GetMutable<ra::context::IEmulatorMemoryContext>();
    pMemoryContext.AddNotifyTarget(*this);
}

MemoryNotesDialog::MemoryNotesGridBinding::~MemoryNotesGridBinding()
{
    if (ra::services::ServiceLocator::Exists<ra::context::IEmulatorMemoryContext>())
    {
        auto& pMemoryContext = ra::services::ServiceLocator::GetMutable<ra::context::IEmulatorMemoryContext>();
        pMemoryContext.RemoveNotifyTarget(*this);
    }
}

void MemoryNotesDialog::MemoryNotesGridBinding::OnTotalMemorySizeChanged()
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

MemoryNotesDialog::MemoryNotesDialog(MemoryNotesViewModel& vmMemoryNotes)
    : DialogBase(vmMemoryNotes),
    m_bindNotes(vmMemoryNotes),
    m_bindFilterValue(vmMemoryNotes),
    m_bindUnpublished(vmMemoryNotes)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Code Notes");

    m_bindFilterValue.BindText(MemoryNotesViewModel::FilterValueProperty);
    m_bindFilterValue.BindKey(VK_RETURN, [this]()
    {
        m_bindFilterValue.UpdateSource();

        auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
        vmMemoryNotes->ApplyFilter();
        return true;
    });
    m_bindFilterValue.BindKey(VK_ESCAPE, [this]()
    {
        auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
        vmMemoryNotes->SetFilterValue(L"");
        vmMemoryNotes->ResetFilter();
        return true;
    });

    m_bindUnpublished.BindCheck(MemoryNotesViewModel::OnlyUnpublishedFilterProperty);

    m_bindWindow.BindLabel(IDC_RA_RESULT_COUNT, MemoryNotesViewModel::ResultCountProperty);

    auto pAddressColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemoryNotesViewModel::MemoryNoteViewModel::LabelProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->SetWidth(GridColumnBinding::WidthType::Pixels,
        ra::ui::win32::bindings::GridAddressColumnBinding::CalculateWidth() + 12);
    pAddressColumn->SetTextColorProperty(MemoryNotesViewModel::MemoryNoteViewModel::BookmarkColorProperty);
    m_bindNotes.BindColumn(0, std::move(pAddressColumn));

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemoryNotesViewModel::MemoryNoteViewModel::NoteProperty);
    pDescriptionColumn->SetHeader(L"Note");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    pDescriptionColumn->SetTextColorProperty(MemoryNotesViewModel::MemoryNoteViewModel::BookmarkColorProperty);
    m_bindNotes.BindColumn(1, std::move(pDescriptionColumn));

    m_bindNotes.BindItems(vmMemoryNotes.Notes());
    m_bindNotes.BindIsSelected(MemoryNotesViewModel::MemoryNoteViewModel::IsSelectedProperty);
    m_bindNotes.SetDoubleClickHandler([this](gsl::index nIndex)
    {
        const auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
        const auto* pItem = vmMemoryNotes->Notes().GetItemAt(nIndex);
        if (pItem)
        {
            auto& pMemoryInspector = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector;
            pMemoryInspector.SetCurrentAddress(pItem->nAddress);

            if (!pMemoryInspector.IsVisible())
                pMemoryInspector.Show();
        }
    });

    m_bindWindow.BindEnabled(IDC_RA_PUBLISH_NOTE, MemoryNotesViewModel::CanPublishCurrentAddressNoteProperty);
    m_bindWindow.BindEnabled(IDC_RA_REVERT_NOTE, MemoryNotesViewModel::CanRevertCurrentAddressNoteProperty);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_FILTER_VALUE, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_APPLY_FILTER, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_RESULT_COUNT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_CHK_UNPUBLISHED, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_RESET_FILTER, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_ADDRESSES, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADDBOOKMARK, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_PUBLISH_NOTE, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_REVERT_NOTE, Anchor::Bottom | Anchor::Right);

    SetMinimumSize(327, 200);
}

BOOL MemoryNotesDialog::OnInitDialog()
{
    m_bindNotes.SetControl(*this, IDC_RA_LBX_ADDRESSES);
    m_bindFilterValue.SetControl(*this, IDC_RA_FILTER_VALUE);
    m_bindUnpublished.SetControl(*this, IDC_RA_CHK_UNPUBLISHED);

    auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vmMemoryNotes->Notes().Count()); ++nIndex)
    {
        auto* pItem = vmMemoryNotes->Notes().GetItemAt(nIndex);
        if (pItem && pItem->IsSelected())
        {
            m_bindNotes.EnsureVisible(nIndex);
            break;
        }
    }

    return DialogBase::OnInitDialog();
}


BOOL MemoryNotesDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_RESET_FILTER:
        {
            auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
            if (vmMemoryNotes)
                vmMemoryNotes->ResetFilter();

            return TRUE;
        }

        case IDOK: // this dialog doesn't have an OK button. if the user pressed Enter, apply the current filter
        case IDC_RA_APPLY_FILTER:
        {
            auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
            if (vmMemoryNotes)
                vmMemoryNotes->ApplyFilter();

            return TRUE;
        }

        case IDC_RA_ADDBOOKMARK:
        {
            const auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
            if (vmMemoryNotes)
                vmMemoryNotes->BookmarkSelected();

            return TRUE;
        }

        case IDC_RA_PUBLISH_NOTE:
        {
            auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
            if (vmMemoryNotes)
                vmMemoryNotes->PublishSelected();

            return TRUE;
        }

        case IDC_RA_REVERT_NOTE:
        {
            auto* vmMemoryNotes = dynamic_cast<MemoryNotesViewModel*>(&m_vmWindow);
            if (vmMemoryNotes)
                vmMemoryNotes->RevertSelected();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
