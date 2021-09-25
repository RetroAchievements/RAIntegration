#ifndef RA_UI_WIN32_DLG_ASSET_EDITOR_H
#define RA_UI_WIN32_DLG_ASSET_EDITOR_H
#pragma once

#include "ui/viewmodels/AssetEditorViewModel.hh"

#include "ui/win32/bindings/CheckBoxBinding.hh"
#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/bindings/ImageBinding.hh"
#include "ui/win32/bindings/NumericTextBoxBinding.hh"
#include "ui/win32/bindings/NumericUpDownBinding.hh"
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
        Presenter() noexcept;

        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<AssetEditorDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM) override;
    BOOL OnCommand(WORD nCommand) override;
    void OnDestroy() override;

private:
    class BadgeNameBinding : public ra::ui::win32::bindings::NumericUpDownBinding
    {
    public:
        BadgeNameBinding(ViewModelBase& vmViewModel) noexcept
            : ra::ui::win32::bindings::NumericUpDownBinding(vmViewModel) {}

    protected:
        void UpdateSourceFromText(const std::wstring& sText) override;
        void UpdateTextFromSource(const std::wstring& sText) override;
    };

    class DecimalPreferredBinding : public ra::ui::win32::bindings::CheckBoxBinding
    {
    public:
        DecimalPreferredBinding(AssetEditorDialog* pOwner, ViewModelBase& vmViewModel) noexcept
            : ra::ui::win32::bindings::CheckBoxBinding(vmViewModel), m_pOwner(pOwner) {}

    protected:
        void OnValueChanged() override;

    private:
        AssetEditorDialog* m_pOwner;
    };

    class ActiveCheckBoxBinding : public ra::ui::win32::bindings::ControlBinding
    {
    public:
        explicit ActiveCheckBoxBinding(ViewModelBase& vmViewModel) noexcept :
            ra::ui::win32::bindings::ControlBinding(vmViewModel) {}

        void SetHWND(DialogBase& pDialog, HWND hControl) override;
        void OnCommand() override;

    protected:
        void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;
        bool IsActive() const;
    };

    class ConditionsGridBinding : public ra::ui::win32::bindings::GridBinding
    {
    public:
        explicit ConditionsGridBinding(ViewModelBase& vmViewModel) noexcept : GridBinding(vmViewModel) {}

    protected:
        void OnViewModelAdded(gsl::index nIndex) override;
        void OnViewModelRemoved(gsl::index nIndex) override;
        void OnEndViewModelCollectionUpdate() override;

    private:
        void CheckIdWidth();
        bool m_bWideId = false;
    };

    class ErrorIconBinding : public ra::ui::win32::bindings::ControlBinding
    {
    public:
        ErrorIconBinding(ViewModelBase& vmViewModel) noexcept
            : ra::ui::win32::bindings::ControlBinding(vmViewModel) {}
        ~ErrorIconBinding();
        ErrorIconBinding(const ErrorIconBinding&) noexcept = delete;
        ErrorIconBinding& operator=(const ErrorIconBinding&) noexcept = delete;
        ErrorIconBinding(ErrorIconBinding&&) noexcept = delete;
        ErrorIconBinding& operator=(ErrorIconBinding&&) noexcept = delete;

    protected:
        void SetHWND(DialogBase& pDialog, HWND hControl) override;

        void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    private:
        void UpdateImage();
        void SetErrorIcon() noexcept;
        void SetWarningIcon() noexcept;

        HICON m_hErrorIcon = nullptr;
        HICON m_hWarningIcon = nullptr;
    };

    ra::ui::win32::bindings::NumericTextBoxBinding m_bindID;
    ra::ui::win32::bindings::TextBoxBinding m_bindName;
    ra::ui::win32::bindings::TextBoxBinding m_bindDescription;
    BadgeNameBinding m_bindBadge;
    ra::ui::win32::bindings::ImageBinding m_bindBadgeImage;
    ra::ui::win32::bindings::NumericTextBoxBinding m_bindPoints;

    ErrorIconBinding m_bindErrorIcon;
    ra::ui::win32::bindings::CheckBoxBinding m_bindMeasuredAsPercent;
    ra::ui::win32::bindings::CheckBoxBinding m_bindDebugHighlights;
    ra::ui::win32::bindings::CheckBoxBinding m_bindPauseOnReset;
    ra::ui::win32::bindings::CheckBoxBinding m_bindPauseOnTrigger;
    ActiveCheckBoxBinding m_bindActive;
    DecimalPreferredBinding m_bindDecimalPreferred;

    ra::ui::win32::bindings::GridBinding m_bindGroups;
    ConditionsGridBinding m_bindConditions;

    HWND m_hTooltip = nullptr;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_ASSET_EDITOR_H
