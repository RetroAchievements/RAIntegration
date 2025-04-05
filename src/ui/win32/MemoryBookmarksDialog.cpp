#include "MemoryBookmarksDialog.hh"

#include "RA_Resource.h"

#include "data\context\EmulatorContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\PointerInspectorViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridBookmarkFormatColumnBinding.hh"
#include "ui\win32\bindings\GridBookmarkValueColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::MemoryBookmarksViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool MemoryBookmarksDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const MemoryBookmarksViewModel*>(&vmViewModel) != nullptr &&
            dynamic_cast<const ra::ui::viewmodels::PointerInspectorViewModel*>(&vmViewModel) == nullptr);
}

void MemoryBookmarksDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void MemoryBookmarksDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&vmViewModel);
    Expects(vmMemoryBookmarks != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new MemoryBookmarksDialog(*vmMemoryBookmarks));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_MEMBOOKMARK), this))
            RA_LOG_ERR("Could not create Memory Bookmarks dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void MemoryBookmarksDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

MemoryBookmarksDialog::BookmarksGridBinding::BookmarksGridBinding(ViewModelBase& vmViewModel)
    : ra::ui::win32::bindings::GridBinding(vmViewModel)
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);
}

MemoryBookmarksDialog::BookmarksGridBinding::~BookmarksGridBinding()
{
    if (ra::services::ServiceLocator::Exists<ra::data::context::EmulatorContext>())
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        pEmulatorContext.RemoveNotifyTarget(*this);
    }
}

void MemoryBookmarksDialog::BookmarksGridBinding::OnTotalMemorySizeChanged()
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vColumns.size()); ++nIndex)
    {
        auto* pAddressColumn = dynamic_cast<ra::ui::win32::bindings::GridAddressColumnBinding*>(m_vColumns.at(nIndex).get());
        if (pAddressColumn != nullptr)
        {
            pAddressColumn->UpdateWidth();
            RefreshColumn(nIndex);
        }
    }

    UpdateLayout();
}

// ------------------------------------

class GridIndirectAddressColumnBinding : public ra::ui::win32::bindings::GridAddressColumnBinding
{
public:
    GridIndirectAddressColumnBinding(const IntModelProperty& pBoundProperty) :
        ra::ui::win32::bindings::GridAddressColumnBinding(pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto* pItem = dynamic_cast<const MemoryBookmarksViewModel::MemoryBookmarkViewModel*>(vmItems.GetViewModelAt(nIndex));
        if (!pItem)
            return ra::ui::win32::bindings::GridAddressColumnBinding::GetText(vmItems, nIndex);

        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        if (pItem->HasIndirectAddress())
            return ra::StringPrintf(L"(%s)", ra::ByteAddressToString(nValue).substr(2));

        return ra::Widen(ra::ByteAddressToString(nValue));
    }
};

MemoryBookmarksDialog::MemoryBookmarksDialog(MemoryBookmarksViewModel& vmMemoryBookmarks)
    : DialogBase(vmMemoryBookmarks),
      m_bindBookmarks(vmMemoryBookmarks)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Memory Bookmarks");
    m_bindWindow.BindLabel(IDC_RA_PAUSE, MemoryBookmarksViewModel::PauseButtonTextProperty);
    m_bindWindow.BindLabel(IDC_RA_FREEZE, MemoryBookmarksViewModel::FreezeButtonTextProperty);
    m_bindWindow.BindEnabled(IDC_RA_DEL_BOOKMARK, MemoryBookmarksViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_PAUSE, MemoryBookmarksViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_FREEZE, MemoryBookmarksViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_MOVE_BOOKMARK_UP, MemoryBookmarksViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_MOVE_BOOKMARK_DOWN, MemoryBookmarksViewModel::HasSelectionProperty);

    m_bindBookmarks.SetShowGridLines(true);
    m_bindBookmarks.BindIsSelected(MemoryBookmarksViewModel::MemoryBookmarkViewModel::IsSelectedProperty);
    m_bindBookmarks.BindRowColor(MemoryBookmarksViewModel::MemoryBookmarkViewModel::RowColorProperty);

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    pDescriptionColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(0, std::move(pDescriptionColumn));

    auto pAddressColumn = std::make_unique<GridIndirectAddressColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->UpdateWidth();
    pAddressColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindBookmarks.BindColumn(1, std::move(pAddressColumn));

    auto pSizeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty, vmMemoryBookmarks.Sizes());
    pSizeColumn->SetHeader(L"Size");
    pSizeColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 76);
    pSizeColumn->SetAlignment(ra::ui::RelativePosition::Far);
    pSizeColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(2, std::move(pSizeColumn));

    auto pFormatColumn = std::make_unique<ra::ui::win32::bindings::GridBookmarkFormatColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty, vmMemoryBookmarks.Formats());
    pFormatColumn->SetHeader(L"Format");
    pFormatColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 32);
    pFormatColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(3, std::move(pFormatColumn));

    auto pValueColumn = std::make_unique<ra::ui::win32::bindings::GridBookmarkValueColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty);
    pValueColumn->SetHeader(L"Value");
    pValueColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    pValueColumn->SetAlignment(ra::ui::RelativePosition::Center);
    pValueColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(4, std::move(pValueColumn));

    auto pPriorColumn = std::make_unique<ra::ui::win32::bindings::GridBookmarkValueColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::PreviousValueProperty);
    pPriorColumn->SetHeader(L"Prior");
    pPriorColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    pPriorColumn->SetAlignment(ra::ui::RelativePosition::Center);
    m_bindBookmarks.BindColumn(5, std::move(pPriorColumn));

    auto pChangesColumn = std::make_unique<ra::ui::win32::bindings::GridNumberColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty);
    pChangesColumn->SetHeader(L"Changes");
    pChangesColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 56);
    pChangesColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindBookmarks.BindColumn(6, std::move(pChangesColumn));

    auto pBehaviorColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty, vmMemoryBookmarks.Behaviors());
    pBehaviorColumn->SetHeader(L"Behavior");
    pBehaviorColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    pBehaviorColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(7, std::move(pBehaviorColumn));

    m_bindBookmarks.BindItems(vmMemoryBookmarks.Bookmarks());

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_SAVEBOOKMARK, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_LOADBOOKMARK, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_CLEAR_CHANGE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LBX_ADDRESSES, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_DEL_BOOKMARK, Anchor::Bottom | Anchor::Left);
    SetAnchor(IDC_RA_PAUSE, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_FREEZE, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MOVE_BOOKMARK_UP, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MOVE_BOOKMARK_DOWN, Anchor::Bottom | Anchor::Right);

    SetMinimumSize(480, 192);
}

BOOL MemoryBookmarksDialog::OnInitDialog()
{
    m_bindBookmarks.SetControl(*this, IDC_RA_LBX_ADDRESSES);

    return DialogBase::OnInitDialog();
}

BOOL MemoryBookmarksDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_DEL_BOOKMARK:
        {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->RemoveSelectedBookmarks();

            return TRUE;
        }

        case IDC_RA_CLEAR_CHANGE:
        {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->ClearAllChanges();

            return TRUE;
        }

        case IDC_RA_LOADBOOKMARK:
        {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->LoadBookmarkFile();

            return TRUE;
        }

        case IDC_RA_SAVEBOOKMARK:
        {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->SaveBookmarkFile();

            return TRUE;
        }

        case IDC_RA_FREEZE:
        {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->ToggleFreezeSelected();

            return TRUE;
        }

        case IDC_RA_PAUSE: {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->TogglePauseSelected();

            return TRUE;
        }

        case IDC_RA_MOVE_BOOKMARK_UP:
        {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->MoveSelectedBookmarksUp();

            return TRUE;
        }

        case IDC_RA_MOVE_BOOKMARK_DOWN:
        {
            auto* vmMemoryBookmarks = dynamic_cast<MemoryBookmarksViewModel*>(&m_vmWindow);
            if (vmMemoryBookmarks)
                vmMemoryBookmarks->MoveSelectedBookmarksDown();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
