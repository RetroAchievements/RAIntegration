#include "MemoryBookmarksDialog.hh"

#include "RA_Resource.h"

#include "RA_Dlg_Memory.h"

#include "data\EmulatorContext.hh"

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

class GridBookmarkValueColumnBinding : public ra::ui::win32::bindings::GridNumberColumnBinding
{
public:
    GridBookmarkValueColumnBinding(const IntModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridNumberColumnBinding(pBoundProperty)
    {
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        return (pProperty == *m_pBoundProperty ||
            pProperty == MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty ||
            pProperty == MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty);
    }

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nValue = gsl::narrow_cast<unsigned int>(vmItems.GetItemValue(nIndex, *m_pBoundProperty));

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

    bool SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue) override
    {
        unsigned int nValue = 0U;
        std::wstring sError;

        const auto nSize = ra::itoe<MemSize>(vmItems.GetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty));
        switch (nSize)
        {
            case MemSize::ThirtyTwoBit:
                m_nMaximum = 0xFFFFFFFF;
                break;

            case MemSize::SixteenBit:
                m_nMaximum = 0xFFFF;
                break;

            case MemSize::EightBit:
                m_nMaximum = 0xFF;
                break;
        }

        const auto nFormat = vmItems.GetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::FormatProperty);
        switch (ra::itoe<MemFormat>(nFormat))
        {
            case MemFormat::Dec:
                if (!ParseUnsignedInt(sValue, nValue, sError))
                {
                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                    return false;
                }
                break;

            default:
                if (!ParseHex(sValue, nValue, sError))
                {
                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                    return false;
                }
                break;
        }

        const auto nCurrentValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        if (ra::to_unsigned(nCurrentValue) != nValue)
        {
            const auto nAddress = vmItems.GetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::AddressProperty);
            ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().WriteMemory(nAddress, nSize, nValue);

            vmItems.SetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::PreviousValueProperty, nCurrentValue);
            vmItems.SetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::CurrentValueProperty, nValue);
            vmItems.SetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty,
                vmItems.GetItemValue(nIndex, MemoryBookmarksViewModel::MemoryBookmarkViewModel::ChangesProperty) + 1);
        }

        return true;
    }
};

MemoryBookmarksDialog::MemoryBookmarksDialog(MemoryBookmarksViewModel& vmMemoryBookmarks)
    : DialogBase(vmMemoryBookmarks),
      m_bindBookmarks(vmMemoryBookmarks)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Memory Bookmarks");
    m_bindBookmarks.SetShowGridLines(true);
    m_bindBookmarks.BindIsSelected(MemoryBookmarksViewModel::MemoryBookmarkViewModel::IsSelectedProperty);
    m_bindBookmarks.BindRowColor(MemoryBookmarksViewModel::MemoryBookmarkViewModel::RowColorProperty);

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemoryBookmarksViewModel::MemoryBookmarkViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(GridColumnBinding::WidthType::Fill, 40);
    pDescriptionColumn->SetReadOnly(false);
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
    pSizeColumn->SetReadOnly(false);
    m_bindBookmarks.BindColumn(2, std::move(pSizeColumn));

    auto pFormatColumn = std::make_unique<ra::ui::win32::bindings::GridLookupColumnBinding>(
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
    SetAnchor(IDC_RA_MOVE_BOOKMARK_UP, Anchor::Bottom | Anchor::Right);
    SetAnchor(IDC_RA_MOVE_BOOKMARK_DOWN, Anchor::Bottom | Anchor::Right);
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
            auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(m_vmWindow);
            vmMemoryBookmarks.RemoveSelectedBookmarks();

            return TRUE;
        }

        case IDC_RA_CLEAR_CHANGE:
        {
            auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(m_vmWindow);
            vmMemoryBookmarks.ClearAllChanges();

            return TRUE;
        }

        case IDC_RA_LOADBOOKMARK:
        {
            auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(m_vmWindow);
            vmMemoryBookmarks.LoadBookmarkFile();

            return TRUE;
        }

        case IDC_RA_SAVEBOOKMARK:
        {
            const auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(m_vmWindow);
            vmMemoryBookmarks.SaveBookmarkFile();

            return TRUE;
        }

        case IDC_RA_MOVE_BOOKMARK_UP:
        {
            auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(m_vmWindow);
            vmMemoryBookmarks.MoveSelectedBookmarksUp();

            return TRUE;
        }

        case IDC_RA_MOVE_BOOKMARK_DOWN:
        {
            auto& vmMemoryBookmarks = reinterpret_cast<MemoryBookmarksViewModel&>(m_vmWindow);
            vmMemoryBookmarks.MoveSelectedBookmarksDown();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
