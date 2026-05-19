#ifndef RA_UI_WIN32_DLG_TRIGGERSUMMARY_H
#define RA_UI_WIN32_DLG_TRIGGERSUMMARY_H
#pragma once

#include "ui/viewmodels/TriggerSummaryViewModel.hh"
#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class TriggerSummaryDialog : public DialogBase
{
public:
    explicit TriggerSummaryDialog(ra::ui::viewmodels::TriggerSummaryViewModel& vmTriggerSummary);
    virtual ~TriggerSummaryDialog() noexcept = default;
    TriggerSummaryDialog(const TriggerSummaryDialog&) noexcept = delete;
    TriggerSummaryDialog& operator=(const TriggerSummaryDialog&) noexcept = delete;
    TriggerSummaryDialog(TriggerSummaryDialog&&) noexcept = delete;
    TriggerSummaryDialog& operator=(TriggerSummaryDialog&&) noexcept = delete;

    class Presenter : public IDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
    };

protected:
    BOOL OnInitDialog() override;

private:
    ra::ui::win32::bindings::GridBinding m_bindClauses;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_TRIGGERSUMMARY_H
