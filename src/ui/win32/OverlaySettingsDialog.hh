#ifndef RA_UI_WIN32_DLG_OVERLAYSETTINGS_H
#define RA_UI_WIN32_DLG_OVERLAYSETTINGS_H
#pragma once

#include "ui/viewmodels/OverlaySettingsViewModel.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"
#include "ui/win32/bindings/CheckBoxBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

class OverlaySettingsDialog : public DialogBase
{
public:
    explicit OverlaySettingsDialog(ra::ui::viewmodels::OverlaySettingsViewModel& vmSettings);
    virtual ~OverlaySettingsDialog() noexcept = default;
    OverlaySettingsDialog(const OverlaySettingsDialog&) noexcept = delete;
    OverlaySettingsDialog& operator=(const OverlaySettingsDialog&) noexcept = delete;
    OverlaySettingsDialog(OverlaySettingsDialog&&) noexcept = delete;
    OverlaySettingsDialog& operator=(OverlaySettingsDialog&&) noexcept = delete;

    class Presenter : public IDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;

private:
    ra::ui::win32::bindings::CheckBoxBinding m_bindDisplayAchievementTrigger;
    ra::ui::win32::bindings::CheckBoxBinding m_bindScreenshotAchievementTrigger;
    ra::ui::win32::bindings::CheckBoxBinding m_bindDisplayLeaderboardStarted;
    ra::ui::win32::bindings::CheckBoxBinding m_bindDisplayLeaderboardCanceled;
    ra::ui::win32::bindings::CheckBoxBinding m_bindDisplayLeaderboardValue;
    ra::ui::win32::bindings::CheckBoxBinding m_bindDisplayLeaderboardScoreboards;
    ra::ui::win32::bindings::TextBoxBinding m_bindScreenshotLocation;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_OVERLAYSETTINGS_H
