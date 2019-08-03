#include "MemoryBookmarksDialog.hh"

#include "RA_Resource.h"

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
    auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(vmViewModel);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new MemoryBookmarksDialog(vmMemoryBookmarks));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_MEMBOOKMARK), this))
            RA_LOG_ERR("Could not create Memory Bookmarks dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void MemoryBookmarksDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

class GridBookmarkValueColumnBinding : public GridColumnBinding
{
public:
    GridBookmarkValueColumnBinding(const IntModelProperty& pBoundProperty) noexcept
        : m_pBoundProperty(&pBoundProperty)
    {
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nValue = static_cast<unsigned int>(vmItems.GetItemValue(nIndex, *m_pBoundProperty));

        const auto nFormat = vmItems.GetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty);
        switch (ra::itoe<MemFormat>(nFormat))
        {
            case MemFormat::Dec:
                return std::to_wstring(nValue);

            default:
            {
                const auto nSize = vmItems.GetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty);
                switch (ra::itoe<MemSize>(nSize))
                {
                    case MemSize::ThirtyTwoBit:
                        return ra::StringPrintf(L"%08X", nValue);

                    case MemSize::SixteenBit:
                        return ra::StringPrintf(L"%04X", nValue);

                    default:
                    case MemSize::EightBit:
                        return ra::StringPrintf(L"%02X", nValue);
                }
            }
        }
    }

protected:
    const IntModelProperty* m_pBoundProperty = nullptr;
};

MemoryBookmarksDialog::MemoryBookmarksDialog(MemoryBookmarksViewModel& vmMemoryBookmarks)
    : DialogBase(vmMemoryBookmarks),
      m_bindBookmarks(vmMemoryBookmarks)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Memory Bookmarks");

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    m_bindBookmarks.BindColumn(0, std::move(pDescriptionColumn));

    auto pAchievedColumn = std::make_unique<ra::ui::win32::bindings::GridAddressColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty);
    pAchievedColumn->SetHeader(L"Address");
    pAchievedColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    pAchievedColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindBookmarks.BindColumn(1, std::move(pAchievedColumn));

    auto pSizeColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty, vmMemoryBookmarks.Sizes());
    pSizeColumn->SetHeader(L"Size");
    pSizeColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 40);
    pSizeColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindBookmarks.BindColumn(2, std::move(pSizeColumn));

    auto pFormatColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty, vmMemoryBookmarks.Formats());
    pFormatColumn->SetHeader(L"Format");
    pFormatColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 40);
    m_bindBookmarks.BindColumn(3, std::move(pFormatColumn));

    auto pValueColumn = std::make_unique<GridBookmarkValueColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty);
    pValueColumn->SetHeader(L"Value");
    pValueColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    pValueColumn->SetAlignment(ra::ui::RelativePosition::Center);
    m_bindBookmarks.BindColumn(4, std::move(pValueColumn));

    auto pPriorColumn = std::make_unique<GridBookmarkValueColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::PreviousValueProperty);
    pPriorColumn->SetHeader(L"Prior");
    pPriorColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    pPriorColumn->SetAlignment(ra::ui::RelativePosition::Center);
    m_bindBookmarks.BindColumn(5, std::move(pPriorColumn));

    auto pChangesColumn = std::make_unique<ra::ui::win32::bindings::GridNumberColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty);
    pChangesColumn->SetHeader(L"Changes");
    pChangesColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    pChangesColumn->SetAlignment(ra::ui::RelativePosition::Far);
    m_bindBookmarks.BindColumn(6, std::move(pChangesColumn));

    auto pBehaviorColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::BehaviorProperty, vmMemoryBookmarks.Behaviors());
    pBehaviorColumn->SetHeader(L"Behavior");
    pBehaviorColumn->SetWidth(GridColumnBinding::WidthType::Pixels, 60);
    m_bindBookmarks.BindColumn(7, std::move(pBehaviorColumn));

    m_bindBookmarks.BindItems(vmMemoryBookmarks.Bookmarks());

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_LBX_ADDRESSES, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_ADD_BOOKMARK, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_DEL_BOOKMARK, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_CLEAR_CHANGE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_FREEZE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_BREAKPOINT, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_DECIMALBOOKMARK, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_SAVEBOOKMARK, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_LOADBOOKMARK, Anchor::Top | Anchor::Right);
}

BOOL MemoryBookmarksDialog::OnInitDialog()
{
    m_bindBookmarks.SetControl(*this, IDC_RA_LBX_ADDRESSES);

    // === remove the LVS_OWNERDRAWFIXED style - not used in new mode
    const HWND hListView = GetDlgItem(GetHWND(), IDC_RA_LBX_ADDRESSES);
    auto wsListView = GetWindowStyle(hListView);
    wsListView &= ~LVS_OWNERDRAWFIXED;
    wsListView &= ~WS_CLIPCHILDREN;
    SetWindowLong(hListView, GWL_STYLE, wsListView);

    wsListView = GetWindowExStyle(hListView);
    wsListView |= LVS_EX_FULLROWSELECT;
    SetWindowLong(hListView, GWL_EXSTYLE, wsListView);
    // ===

    return DialogBase::OnInitDialog();
}

BOOL MemoryBookmarksDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDC_RA_ADD_BOOKMARK)
    {
        //auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(m_vmWindow);
        //vmMemoryBookmarks.AddBookmark();
        return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
