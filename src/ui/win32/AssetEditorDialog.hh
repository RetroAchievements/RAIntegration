#ifndef RA_UI_WIN32_DLG_ASSET_EDITOR_H
#define RA_UI_WIN32_DLG_ASSET_EDITOR_H
#pragma once

#include "ui/viewmodels/AssetEditorViewModel.hh"

#include "ui/win32/bindings/CheckBoxBinding.hh"
#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/bindings/ImageBinding.hh"
#include "ui/win32/bindings/NumericTextBoxBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class AssetEditorDialog : public DialogBase
{
public:
    explicit AssetEditorDialog(ra::ui::viewmodels::AssetEditorViewModel& vmAssetList);
    virtual ~AssetEditorDialog() noexcept = default;
    AssetEditorDialog(const AssetEditorDialog&) noexcept = delete;
    AssetEditorDialog& operator=(const AssetEditorDialog&) noexcept = delete;
    AssetEditorDialog(AssetEditorDialog&&) noexcept = delete;
    AssetEditorDialog& operator=(AssetEditorDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<AssetEditorDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    class BadgeNameBinding : public ra::ui::win32::bindings::TextBoxBinding
    {
    public:
        BadgeNameBinding(ViewModelBase& vmViewModel) : ra::ui::win32::bindings::TextBoxBinding(vmViewModel) {}

    protected:
        void UpdateSourceFromText(const std::wstring& sText) override;
    };

    ra::ui::win32::bindings::NumericTextBoxBinding m_bindID;
    ra::ui::win32::bindings::TextBoxBinding m_bindName;
    ra::ui::win32::bindings::TextBoxBinding m_bindDescription;
    BadgeNameBinding m_bindBadge;
    ra::ui::win32::bindings::ImageBinding m_bindBadgeImage;
    ra::ui::win32::bindings::NumericTextBoxBinding m_bindPoints;

    ra::ui::win32::bindings::CheckBoxBinding m_bindPauseOnReset;
    ra::ui::win32::bindings::CheckBoxBinding m_bindPauseOnTrigger;

    ra::ui::win32::bindings::GridBinding m_bindGroups;
    ra::ui::win32::bindings::GridBinding m_bindTrigger;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_ASSET_EDITOR_H
