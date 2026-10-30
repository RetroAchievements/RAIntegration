#ifndef RA_UI_WIN32_GRIDMEMORYWATCHSIZECOLUMNBINDING_H
#define RA_UI_WIN32_GRIDMEMORYWATCHSIZECOLUMNBINDING_H
#pragma once

#include "GridBinding.hh"
#include "GridLookupColumnBinding.hh"

#include "ui\viewmodels\MemoryWatchViewModel.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class GridMemoryWatchSizeColumnBinding : public ra::ui::win32::bindings::GridLookupColumnBinding
{
public:
    GridMemoryWatchSizeColumnBinding(const IntModelProperty& pBoundProperty,
                                     const ra::ui::viewmodels::LookupItemViewModelCollection& vmItems) noexcept :
        ra::ui::win32::bindings::GridLookupColumnBinding(pBoundProperty, vmItems)
    {}

    std::wstring GetText(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto nValue = vmItems.GetItemValue(nIndex, *m_pBoundProperty);
        const auto sLabel = m_vmItems.GetLabelForId(nValue);
        if (!sLabel.empty())
            return sLabel;

        return ra::data::Memory::SizeString(ra::itoe<ra::data::Memory::Size>(nValue));
    }

    HWND CreateInPlaceEditor(HWND hParent, InPlaceEditorInfo& pInfo) override
    {
        const auto& vmItems = static_cast<ra::ui::win32::bindings::GridBinding*>(pInfo.pGridBinding)->GetItems();
        if (vmItems.GetItemValue(pInfo.nItemIndex, ra::ui::viewmodels::MemoryWatchViewModel::ReadOnlyProperty))
            return nullptr;

        return ra::ui::win32::bindings::GridLookupColumnBinding::CreateInPlaceEditor(hParent, pInfo);
    }
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_GRIDMEMORYWATCHSIZECOLUMNBINDING_H
