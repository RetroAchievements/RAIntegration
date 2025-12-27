#ifndef RA_UI_WIN32_GRIDMEMORYWATCHFORMATCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDMEMORYWATCHFORMATCOLUMNBINDING_H
#pragma once

#include "GridBinding.hh"
#include "GridLookupColumnBinding.hh"

#include "ui\viewmodels\MemoryWatchViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridMemoryWatchFormatColumnBinding : public ra::ui::win32::bindings::GridLookupColumnBinding
{
public:
    GridMemoryWatchFormatColumnBinding(const IntModelProperty& pBoundProperty,
                                       const ra::ui::viewmodels::LookupItemViewModelCollection& vmItems) noexcept :
        ra::ui::win32::bindings::GridLookupColumnBinding(pBoundProperty, vmItems)
    {}

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        if (IsHidden(vmItems, nIndex))
            return L"";

        return ra::ui::win32::bindings::GridLookupColumnBinding::GetText(vmItems, nIndex);
    }

    bool DependsOn(const ra::ui::IntModelProperty& pProperty) const noexcept override
    {
        if (pProperty == ra::ui::viewmodels::MemoryWatchViewModel::SizeProperty)
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
        const auto nSize = ra::itoe<ra::data::Memory::Size>(vmItems.GetItemValue(nIndex, ra::ui::viewmodels::MemoryWatchViewModel::SizeProperty));
        switch (nSize)
        {
            case ra::data::Memory::Size::Float:
            case ra::data::Memory::Size::FloatBigEndian:
            case ra::data::Memory::Size::Double32:
            case ra::data::Memory::Size::Double32BigEndian:
            case ra::data::Memory::Size::MBF32:
            case ra::data::Memory::Size::MBF32LE:
            case ra::data::Memory::Size::Text:
                return true;

            default:
                return false;
        }
    }
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDMEMORYWATCHFORMATCOLUMNBINDING_H
