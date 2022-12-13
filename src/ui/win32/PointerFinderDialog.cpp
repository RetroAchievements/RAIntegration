#include "PointerFinderDialog.hh"

#include "RA_Log.h"
#include "RA_Resource.h"

using ra::ui::viewmodels::MemoryViewerViewModel;
using ra::ui::viewmodels::PointerFinderViewModel;

namespace ra {
namespace ui {
namespace win32 {

bool PointerFinderDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const PointerFinderViewModel*>(&vmViewModel) != nullptr);
}

void PointerFinderDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void PointerFinderDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&vmViewModel);
    Expects(vmPointerFinder != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new PointerFinderDialog(*vmPointerFinder));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_POINTERFINDER), this))
            RA_LOG_ERR("Could not create Pointer Finder dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void PointerFinderDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

void PointerFinderDialog::PointerFinderStateBinding::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == PointerFinderViewModel::StateViewModel::CanCaptureProperty)
    {
        auto hControl = GetDlgItem(m_hWnd, m_idcAddress);
        if (hControl)
        {
            EnableWindow(hControl, args.tNewValue ? TRUE : FALSE);
            ControlBinding::ForceRepaint(hControl);
        }
    }
}

void PointerFinderDialog::PointerFinderStateBinding::OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == PointerFinderViewModel::StateViewModel::CaptureButtonTextProperty)
        SetDlgItemTextW(m_hWnd, m_idcButton, args.tNewValue.c_str());
}

// ------------------------------------

PointerFinderDialog::PointerFinderDialog(ra::ui::viewmodels::PointerFinderViewModel& vmPointerFinder)
    : DialogBase(vmPointerFinder),
      m_bindViewer1(vmPointerFinder.States().at(0)),
      m_bindViewer2(vmPointerFinder.States().at(1)),
      m_bindViewer3(vmPointerFinder.States().at(2)),
      m_bindViewer4(vmPointerFinder.States().at(3))
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Pointer Finder");

    // Results

    // States
    m_bindViewer1.BindAddress(IDC_RA_ADDRESS_1);
    m_bindViewer1.BindButton(IDC_RA_CAPTURE_1);
    m_bindViewer2.BindAddress(IDC_RA_ADDRESS_2);
    m_bindViewer2.BindButton(IDC_RA_CAPTURE_2);
    m_bindViewer3.BindAddress(IDC_RA_ADDRESS_3);
    m_bindViewer3.BindButton(IDC_RA_CAPTURE_3);
    m_bindViewer4.BindAddress(IDC_RA_ADDRESS_4);
    m_bindViewer4.BindButton(IDC_RA_CAPTURE_4);

    // Resize behavior
    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_GBX_RESULTS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_RESULT_COUNT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESET_FILTER, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_APPLY_FILTER, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULTS_BOOKMARK, Anchor::Top | Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_RESULTS_EXPORT, Anchor::Top | Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_RESULTS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_1, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_1, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADDRESS_1, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_CAPTURE_1, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MEMVIEWER_1, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_2, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_2, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADDRESS_2, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_CAPTURE_2, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MEMVIEWER_2, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_3, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_3, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADDRESS_3, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_CAPTURE_3, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MEMVIEWER_3, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_4, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_4, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADDRESS_4, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_CAPTURE_4, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MEMVIEWER_4, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetMinimumSize(603, 580);
}

BOOL PointerFinderDialog::OnInitDialog()
{
    m_bindViewer1.OnInitDialog(*this, IDC_RA_MEMVIEWER_1);
    m_bindViewer2.OnInitDialog(*this, IDC_RA_MEMVIEWER_2);
    m_bindViewer3.OnInitDialog(*this, IDC_RA_MEMVIEWER_3);
    m_bindViewer4.OnInitDialog(*this, IDC_RA_MEMVIEWER_4);

    return DialogBase::OnInitDialog();
}

BOOL PointerFinderDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_CAPTURE_1: {
            auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&m_vmWindow);
            if (vmPointerFinder)
                vmPointerFinder->States().at(0).ToggleCapture();

            return TRUE;
        }

        case IDC_RA_CAPTURE_2: {
            auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&m_vmWindow);
            if (vmPointerFinder)
                vmPointerFinder->States().at(1).ToggleCapture();

            return TRUE;
        }

        case IDC_RA_CAPTURE_3: {
            auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&m_vmWindow);
            if (vmPointerFinder)
                vmPointerFinder->States().at(2).ToggleCapture();

            return TRUE;
        }

        case IDC_RA_CAPTURE_4: {
            auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&m_vmWindow);
            if (vmPointerFinder)
                vmPointerFinder->States().at(3).ToggleCapture();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
