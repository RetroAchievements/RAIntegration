#include "MemoryInspectorDialog.hh"

#include "RA_Resource.h"

#include "RA_Dlg_Memory.h"


#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::MemoryInspectorViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool MemoryInspectorDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const MemoryInspectorViewModel*>(&vmViewModel) != nullptr);
}

void MemoryInspectorDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void MemoryInspectorDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryBookmarks = dynamic_cast<MemoryInspectorViewModel*>(&vmViewModel);
    Expects(vmMemoryBookmarks != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new MemoryInspectorDialog(*vmMemoryBookmarks));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_MEMORY), this))
            RA_LOG_ERR("Could not create Memory Inspector dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void MemoryInspectorDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

MemoryInspectorDialog::MemoryInspectorDialog(MemoryInspectorViewModel& vmMemoryInspector)
    : DialogBase(vmMemoryInspector),
      m_bindViewer(vmMemoryInspector.Viewer())
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Memory Inspector");



    using namespace ra::bitwise_ops;
    //SetAnchor(IDC_RA_SAVEBOOKMARK, Anchor::Top | Anchor::Left);
    //SetAnchor(IDC_RA_LOADBOOKMARK, Anchor::Top | Anchor::Left);
    //SetAnchor(IDC_RA_CLEAR_CHANGE, Anchor::Top | Anchor::Right);
    //SetAnchor(IDC_RA_LBX_ADDRESSES, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    //SetAnchor(IDC_RA_DEL_BOOKMARK, Anchor::Bottom | Anchor::Left);
    //SetAnchor(IDC_RA_MOVE_BOOKMARK_UP, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MEMTEXTVIEWER, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
}

BOOL MemoryInspectorDialog::OnInitDialog()
{
    m_bindViewer.SetControl(*this, IDC_RA_MEMTEXTVIEWER);

    return DialogBase::OnInitDialog();
}

BOOL MemoryInspectorDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_VIEW_CODENOTES:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->OpenNotesList();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
