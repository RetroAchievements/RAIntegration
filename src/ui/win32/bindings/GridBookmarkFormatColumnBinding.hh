#ifndef RA_UI_WIN32_GRIDBOOKMARKFORMATCOLUMNBINDING_H
#define RA_UI_WIN32_GRIDBOOKMARKFORMATCOLUMNBINDING_H
#pragma once

#include "GridBinding.hh"
#include "GridLookupColumnBinding.hh"

#include "ui\viewmodels\MemoryBookmarksViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridBookmarkFormatColumnBinding : public ra::ui::win32::bindings::GridLookupColumnBinding
{
public:
    GridBookmarkFormatColumnBinding(const IntModelProperty& pBoundProperty,
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
        if (pProperty == ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty)
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
        const auto nSize = ra::itoe<MemSize>(
            vmItems.GetItemValue(nIndex, ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel::SizeProperty));
        switch (nSize)
        {
            case MemSize::Float:
            case MemSize::FloatBigEndian:
            case MemSize::Double32:
            case MemSize::Double32BigEndian:
            case MemSize::MBF32:
            case MemSize::MBF32LE:
            case MemSize::Text:
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

#endif // !RA_UI_WIN32_GRIDBOOKMARKFORMATCOLUMNBINDING_H
