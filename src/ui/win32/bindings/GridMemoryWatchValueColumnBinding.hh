#ifndef RA_UI_WIN32_GRIDMEMORYWATCHVALUECOLUMNBINDING_H
#define RA_UI_WIN32_GRIDMEMORYWATCHVALUECOLUMNBINDING_H
#pragma once

#include "GridBinding.hh"
#include "GridTextColumnBinding.hh"

#include "ui\viewmodels\MemoryWatchViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridMemoryWatchValueColumnBinding : public ra::ui::win32::bindings::GridTextColumnBinding
{
public:
    GridMemoryWatchValueColumnBinding(const StringModelProperty& pBoundProperty) noexcept :
        ra::ui::win32::bindings::GridTextColumnBinding(pBoundProperty)
    {}

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        const auto& vmItems = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding)->GetItems();
        if (vmItems.GetItemValue(pInfo.nItemIndex, ra::ui::viewmodels::MemoryWatchViewModel::ReadOnlyProperty))
            return nullptr;

        return ra::ui::win32::bindings::GridTextColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }

    bool SetText(ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex, const std::wstring& sValue) override
    {
        auto* vmItem = dynamic_cast<ra::ui::viewmodels::MemoryWatchViewModel*>(
            vmItems.GetViewModelAt(nIndex));
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

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDMEMORYWATCHVALUECOLUMNBINDING_H
