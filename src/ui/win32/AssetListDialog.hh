#ifndef RA_UI_WIN32_DLG_ASSET_LIST_H
#define RA_UI_WIN32_DLG_ASSET_LIST_H
#pragma once

#include "ui/viewmodels/AssetListViewModel.hh"

#include "ui/win32/bindings/CheckBoxBinding.hh"
#include "ui/win32/bindings/ComboBoxBinding.hh"
#include "ui/win32/bindings/GridBinding.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class AssetListDialog : public DialogBase
{
public:
    explicit AssetListDialog(ra::ui::viewmodels::AssetListViewModel& vmAssetList);
    virtual ~AssetListDialog() noexcept = default;
    AssetListDialog(const AssetListDialog&) noexcept = delete;
    AssetListDialog& operator=(const AssetListDialog&) noexcept = delete;
    AssetListDialog(AssetListDialog&&) noexcept = delete;
    AssetListDialog& operator=(AssetListDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<AssetListDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    ra::ui::win32::bindings::GridBinding m_bindAssets;
    ra::ui::win32::bindings::ComboBoxBinding m_bindCategories;
    ra::ui::win32::bindings::ComboBoxBinding m_bindSpecialFilters;
    ra::ui::win32::bindings::CheckBoxBinding m_bindProcessingActive;
    ra::ui::win32::bindings::CheckBoxBinding m_bindKeepActive;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_ASSET_LIST_H
