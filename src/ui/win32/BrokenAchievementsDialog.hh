#ifndef RA_UI_WIN32_DLG_BROKENACHIEVEMENTS_H
#define RA_UI_WIN32_DLG_BROKENACHIEVEMENTS_H
#pragma once

#include "ui/viewmodels/BrokenAchievementsViewModel.hh"
#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/bindings/RadioButtonBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class BrokenAchievementsDialog : public DialogBase
{
public:
    explicit BrokenAchievementsDialog(ra::ui::viewmodels::BrokenAchievementsViewModel& vmBrokenAchievements);
    virtual ~BrokenAchievementsDialog() noexcept = default;
    BrokenAchievementsDialog(const BrokenAchievementsDialog&) noexcept = delete;
    BrokenAchievementsDialog& operator=(const BrokenAchievementsDialog&) noexcept = delete;
    BrokenAchievementsDialog(BrokenAchievementsDialog&&) noexcept = delete;
    BrokenAchievementsDialog& operator=(BrokenAchievementsDialog&&) noexcept = delete;

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
    ra::ui::win32::bindings::RadioButtonBinding m_bindWrongTime;
    ra::ui::win32::bindings::RadioButtonBinding m_bindDidNotTrigger;
    ra::ui::win32::bindings::TextBoxBinding m_bindComment;
    ra::ui::win32::bindings::GridBinding m_bindAchievements;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_BROKENACHIEVEMENTS_H
