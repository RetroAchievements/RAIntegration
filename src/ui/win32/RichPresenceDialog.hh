#ifndef RA_UI_WIN32_DLG_RICHPRESENCE_H
#define RA_UI_WIN32_DLG_RICHPRESENCE_H
#pragma once

#include "ui/viewmodels/RichPresenceMonitorViewModel.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"
namespace ra {
namespace ui {
namespace win32 {

class RichPresenceDialog : public DialogBase
{
public:
    explicit RichPresenceDialog(_Inout_ ra::ui::viewmodels::RichPresenceMonitorViewModel& vmRichPresenceDisplay);
    virtual ~RichPresenceDialog() noexcept;
    RichPresenceDialog(const RichPresenceDialog&) noexcept = delete;
    RichPresenceDialog& operator=(const RichPresenceDialog&) noexcept = delete;
    RichPresenceDialog(RichPresenceDialog&&) noexcept = delete;
    RichPresenceDialog& operator=(RichPresenceDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<RichPresenceDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    void OnShown() override;
    void OnDestroy() override;

private:
    HFONT m_hFont = nullptr;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_RICHPRESENCE_H
