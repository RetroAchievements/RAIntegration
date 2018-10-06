#ifndef RA_UI_WIN32_DLG_RICHPRESENCE_H
#define RA_UI_WIN32_DLG_RICHPRESENCE_H
#pragma once

#include "ui/viewmodels/RichPresenceMonitorViewModel.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include <memory>

namespace ra {
namespace ui {
namespace win32 {

class RichPresenceDialog : public DialogBase
{
public:
    RichPresenceDialog(ra::ui::viewmodels::RichPresenceMonitorViewModel& vmRichPresenceDisplay);
    ~RichPresenceDialog();

    INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM) override;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel) override;
        void OnClosed() override;

    private:
        std::unique_ptr<RichPresenceDialog> m_pDialog;
    };

protected:
    void OnInitDialog() override;
    void OnDestroy() override;

private:
    void StartTimer();
    void StopTimer();

    HFONT m_hFont = nullptr;
    bool m_bTimerActive = false;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_RICHPRESENCE_H
