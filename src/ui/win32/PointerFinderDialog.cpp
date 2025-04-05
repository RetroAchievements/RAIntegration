#include "PointerFinderDialog.hh"

#include "RA_Defs.h"
#include "RA_Log.h"
#include "RA_Resource.h"

#include "ui/viewmodels/WindowManager.hh"

#include "ui/win32/bindings/GridTextColumnBinding.hh"

using ra::ui::viewmodels::PointerFinderViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

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

void PointerFinderDialog::PointerFinderStateBinding::OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) noexcept
{
    if (args.Property == PointerFinderViewModel::StateViewModel::CaptureButtonTextProperty)
        SetDlgItemTextW(m_hWnd, m_idcButton, args.tNewValue.c_str());
}

// ------------------------------------

class PointerAddressGridColumnBinding : public bindings::GridTextColumnBinding
{
public:
    PointerAddressGridColumnBinding(const StringModelProperty& pBoundProperty) noexcept
        : bindings::GridTextColumnBinding(pBoundProperty)
    {
    }

    bool HandleDoubleClick(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) override
    {
        if (!vmItems.GetItemValue(nIndex, PointerFinderViewModel::PotentialPointerViewModel::OffsetProperty).empty())
        {
            const auto* pItem = dynamic_cast<const PointerFinderViewModel::PotentialPointerViewModel*>(vmItems.GetViewModelAt(nIndex));
            Expects(pItem != nullptr);
            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector.SetCurrentAddress(pItem->GetRawAddress());

            return true;
        }

        return false;
    }
};

// ------------------------------------

PointerFinderDialog::PointerFinderDialog(PointerFinderViewModel& vmPointerFinder)
    : DialogBase(vmPointerFinder),
      m_bindSearchType(vmPointerFinder),
      m_bindResults(vmPointerFinder),
      m_bindViewer1(vmPointerFinder.States().at(0)),
      m_bindViewer2(vmPointerFinder.States().at(1)),
      m_bindViewer3(vmPointerFinder.States().at(2)),
      m_bindViewer4(vmPointerFinder.States().at(3))
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Pointer Finder");

    // Results
    m_bindWindow.BindLabel(IDC_RA_RESULT_COUNT, PointerFinderViewModel::ResultCountTextProperty);
    m_bindSearchType.BindItems(vmPointerFinder.SearchTypes());
    m_bindSearchType.BindSelectedItem(PointerFinderViewModel::SearchTypeProperty);

    auto pAddressColumn = std::make_unique<PointerAddressGridColumnBinding>(
        PointerFinderViewModel::PotentialPointerViewModel::PointerAddressProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->SetWidth(GridColumnBinding::WidthType::Fill, 72);
    m_bindResults.BindColumn(0, std::move(pAddressColumn));

    auto pOffsetColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerFinderViewModel::PotentialPointerViewModel::OffsetProperty);
    pOffsetColumn->SetHeader(L"Offset");
    pOffsetColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 80);
    m_bindResults.BindColumn(1, std::move(pOffsetColumn));

    auto pValue1Column = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerFinderViewModel::PotentialPointerViewModel::PointerValue1Property);
    pValue1Column->SetHeader(L"State 1");
    pValue1Column->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    m_bindResults.BindColumn(2, std::move(pValue1Column));

    auto pValue2Column = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerFinderViewModel::PotentialPointerViewModel::PointerValue2Property);
    pValue2Column->SetHeader(L"State 2");
    pValue2Column->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    m_bindResults.BindColumn(3, std::move(pValue2Column));

    auto pValue3Column = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerFinderViewModel::PotentialPointerViewModel::PointerValue3Property);
    pValue3Column->SetHeader(L"State 3");
    pValue3Column->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    m_bindResults.BindColumn(4, std::move(pValue3Column));

    auto pValue4Column = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        PointerFinderViewModel::PotentialPointerViewModel::PointerValue4Property);
    pValue4Column->SetHeader(L"State 4");
    pValue4Column->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    m_bindResults.BindColumn(5, std::move(pValue4Column));

    m_bindResults.BindItems(vmPointerFinder.PotentialPointers());
    m_bindResults.BindIsSelected(PointerFinderViewModel::PotentialPointerViewModel::IsSelectedProperty);

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
    SetAnchor(IDC_RA_RESULTS_BOOKMARK, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_RESULTS_EXPORT, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_RESULTS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_1, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_1, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_ADDRESS_1, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_CAPTURE_1, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_MEMVIEWER_1, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_2, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_2, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_ADDRESS_2, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_CAPTURE_2, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_MEMVIEWER_2, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_3, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_3, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_ADDRESS_3, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_CAPTURE_3, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_MEMVIEWER_3, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetAnchor(IDC_RA_GBX_STATE_4, Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_LBL_ADDRESS_4, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_ADDRESS_4, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_CAPTURE_4, Anchor::Left | Anchor::Bottom);
    SetAnchor(IDC_RA_MEMVIEWER_4, Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetMinimumSize(616, 583);
}

BOOL PointerFinderDialog::OnInitDialog()
{
    m_bindSearchType.SetControl(*this, IDC_RA_SEARCHTYPE);
    m_bindResults.SetControl(*this, IDC_RA_RESULTS);

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
        case IDC_RA_RESET_FILTER: {
            auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&m_vmWindow);
            if (vmPointerFinder)
                vmPointerFinder->Find();

            return TRUE;
        }

        case IDC_RA_RESULTS_BOOKMARK: {
            auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&m_vmWindow);
            if (vmPointerFinder)
                vmPointerFinder->BookmarkSelected();

            return TRUE;
        }

        case IDC_RA_RESULTS_EXPORT: {
            const auto* vmPointerFinder = dynamic_cast<PointerFinderViewModel*>(&m_vmWindow);
            if (vmPointerFinder)
                vmPointerFinder->ExportResults();

            return TRUE;
        }

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
