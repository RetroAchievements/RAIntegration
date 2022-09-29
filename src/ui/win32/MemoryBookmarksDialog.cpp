#include "MemoryBookmarksDialog.hh"

#include "RA_Resource.h"

#include "data\context\EmulatorContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
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
    return (dynamic_cast<const MemoryBookmarksViewModel*>(&vmViewModel) != nullptr);
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

class GridBookmarkValueColumnBinding : public ra::ui::win32::bindings::GridTextColumnBinding
{
public:
    GridBookmarkValueColumnBinding(const StringModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridTextColumnBinding(pBoundProperty)
    {
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        const auto& vmItems = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding)->GetItems();
        if (vmItems.GetItemValue(pInfo.nItemIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::ReadOnlyProperty))
            return nullptr;

        return ra::ui::win32::bindings::GridTextColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }

    bool SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue) override
    {
        auto* vmItem = dynamic_cast<MemoryBookmarksViewModel::MemoryBookmarkViewModel*>(vmItems.GetViewModelAt(nIndex));
        if (vmItem != nullptr)
        {
            std::wstring sError;
            if (!vmItem->SetCurrentValue(sValue, sError))
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                return false;
            }
        }

        return true;
    }
};

// ------------------------------------

class GridBookmarkFormatColumnBinding : public ra::ui::win32::bindings::GridLookupColumnBinding
{
public:
    GridBookmarkFormatColumnBinding(const IntModelProperty& pBoundProperty, const ra::ui::viewmodels::LookupItemViewModelCollection& vmItems) noexcept
        : ra::ui::win32::bindings::GridLookupColumnBinding(pBoundProperty, vmItems)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        if (IsHidden(vmItems, nIndex))
            return L"";

        return ra::ui::win32::bindings::GridLookupColumnBinding::GetText(vmItems, nIndex);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        if (pProperty == MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty)
            return true;

        return ra::ui::win32::bindings::GridLookupColumnBinding::DependsOn(pProperty);
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        auto* pGridBinding = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding);
        Expects(pGridBinding != nullptr);

        if (IsHidden(pGridBinding->GetItems(), pInfo.nItemIndex))
            return nullptr;

        return GridLookupColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }

private:
    static bool IsHidden(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex)
    {
        const auto nSize = ra::itoe<MemSize>(vmItems.GetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty));
        switch (nSize)
        {
            case MemSize::Float:
            case MemSize::MBF32:
            case MemSize::MBF32LE:
            case MemSize::Text:
                return true;

            default:
                return false;
        }
    }
};

// ------------------------------------

MemoryBookmarksDialog::MemoryBookmarksDialog(MemoryBookmarksViewModel& vmMemoryBookmarks)
    : DialogBase(vmMemoryBookmarks),
      m_bindBookmarks(vmMemoryBookmarks)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Memory Bookmarks");
    m_bindWindow.BindLabel(IDC_RA_FREEZE, MemoryBookmarksViewModel::FreezeButtonTextProperty);

    m_bindBookmarks.SetShowGridLines(true);
    m_bindBookmarks.BindIsSelected(MemoryBookmarksViewModel::MemoryBookmarkViewModel::IsSelectedProperty);
    m_bindBookmarks.BindRowColor(MemoryBookmarksViewModel::MemoryBookmarkViewModel::RowColorProperty);

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    pDescriptionColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(0, std::move(pDescriptionColumn));

    auto pAddressColumn = std::make_unique<ra::ui::win32::bindings::GridAddressColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->UpdateWidth();
    pAddressColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindBookmarks.BindColumn(1, std::move(pAddressColumn));

    auto pSizeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty, vmMemoryBookmarks.Sizes());
    pSizeColumn->SetHeader(L"Size");
    pSizeColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 62);
    pSizeColumn->SetAlignment(ra::ui::RelativePosition::Far);
    pSizeColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(2, std::move(pSizeColumn));

    auto pFormatColumn = std::make_unique<GridBookmarkFormatColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty, vmMemoryBookmarks.Formats());
    pFormatColumn->SetHeader(L"Format");
    pFormatColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 32);
    pFormatColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(3, std::move(pFormatColumn));

    auto pValueColumn = std::make_unique<GridBookmarkValueColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty);
    pValueColumn->SetHeader(L"Value");
    pValueColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 72);
    pValueColumn->SetAlignment(ra::ui::RelativePosition::Center);
    pValueColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(4, std::move(pValueColumn));

    auto pPriorColumn = std::make_unique<GridBookmarkValueColumnBinding>(
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
