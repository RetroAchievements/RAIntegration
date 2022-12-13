#ifndef RA_UI_WIN32_DLG_POINTERFINDER_H
#define RA_UI_WIN32_DLG_POINTERFINDER_H
#pragma once

#include "ui/viewmodels/PointerFinderViewModel.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include "ui/win32/bindings/TextBoxBinding.hh"
#include "ui/win32/bindings/MemoryViewerControlBinding.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class PointerFinderDialog : public DialogBase
{
public:
    explicit PointerFinderDialog(viewmodels::PointerFinderViewModel& vmNewAsset);
    virtual ~PointerFinderDialog() noexcept = default;
    PointerFinderDialog(const PointerFinderDialog&) noexcept = delete;
    PointerFinderDialog& operator=(const PointerFinderDialog&) noexcept = delete;
    PointerFinderDialog(PointerFinderDialog&&) noexcept = delete;
    PointerFinderDialog& operator=(PointerFinderDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<PointerFinderDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    class PointerFinderStateBinding : public bindings::ControlBinding
    {
    public:
        explicit PointerFinderStateBinding(viewmodels::PointerFinderViewModel::StateViewModel& vmState)
            : bindings::ControlBinding(vmState), 
              m_bindViewer(vmState.Viewer()), 
              m_bindAddress(vmState)
        {
        }

        void BindAddress(int idcAddress)
        {
            m_bindAddress.BindText(viewmodels::PointerFinderViewModel::StateViewModel::AddressProperty, bindings::TextBoxBinding::UpdateMode::Typing);
            m_idcAddress = idcAddress;
        }

        void BindButton(int idcButton)
        {
            m_idcButton = idcButton;
        }

        void OnInitDialog(DialogBase& pDialog, int idcMemViewer)
        {
            m_hWnd = pDialog.GetHWND();
            m_bindAddress.SetControl(pDialog, m_idcAddress);
            m_bindViewer.SetControl(pDialog, idcMemViewer);
        }

    protected:
        void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override;
        void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override;

    private:
        bindings::TextBoxBinding m_bindAddress;
        bindings::MemoryViewerControlBinding m_bindViewer;
        const StringModelProperty* m_pAddressProperty = nullptr;
        int m_idcAddress = 0;
        int m_idcButton = 0;
    };

    PointerFinderStateBinding m_bindViewer1;
    PointerFinderStateBinding m_bindViewer2;
    PointerFinderStateBinding m_bindViewer3;
    PointerFinderStateBinding m_bindViewer4;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_POINTERFINDER_H
