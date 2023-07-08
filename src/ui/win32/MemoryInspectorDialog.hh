#ifndef RA_UI_WIN32_DLG_MEMORYINSPECTOR_H
#define RA_UI_WIN32_DLG_MEMORYINSPECTOR_H
#pragma once

#include "ui/viewmodels/MemoryInspectorViewModel.hh"

#include "ui/win32/DialogBase.hh"
#include "ui/win32/IDialogPresenter.hh"

#include "ui/win32/bindings/ComboBoxBinding.hh"
#include "ui/win32/bindings/GridBinding.hh"
#include "ui/win32/bindings/MemoryViewerControlBinding.hh"
#include "ui/win32/bindings/RadioButtonBinding.hh"
#include "ui/win32/bindings/TextBoxBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

class MemoryInspectorDialog : public DialogBase,
    protected ra::ui::viewmodels::MemoryViewerViewModel::RepaintNotifyTarget
{
public:
    explicit MemoryInspectorDialog(ra::ui::viewmodels::MemoryInspectorViewModel& vmMemoryBookmarks);
    ~MemoryInspectorDialog() noexcept = default;
    MemoryInspectorDialog(const MemoryInspectorDialog&) noexcept = delete;
    MemoryInspectorDialog& operator=(const MemoryInspectorDialog&) noexcept = delete;
    MemoryInspectorDialog(MemoryInspectorDialog&&) noexcept = delete;
    MemoryInspectorDialog& operator=(MemoryInspectorDialog&&) noexcept = delete;

    class Presenter : public IClosableDialogPresenter
    {
    public:
        bool IsSupported(const ra::ui::WindowViewModelBase& viewModel) noexcept override;
        void ShowWindow(ra::ui::WindowViewModelBase& viewModel) override;
        void ShowModal(ra::ui::WindowViewModelBase& viewModel, HWND hParentWnd) override;
        void OnClosed() noexcept override;

    private:
        std::unique_ptr<MemoryInspectorDialog> m_pDialog;
    };

protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WORD nCommand) override;
    INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    bindings::ComboBoxBinding m_bindSearchRanges;
    bindings::TextBoxBinding m_bindSearchRange;

    bindings::ComboBoxBinding m_bindSearchType;

    bindings::ComboBoxBinding m_bindComparison;
    bindings::ComboBoxBinding m_bindValueType;
    bindings::TextBoxBinding m_bindFilterValue;

    class SearchResultsGridBinding : public ra::ui::win32::bindings::GridBinding
    {
    public:
        explicit SearchResultsGridBinding(ViewModelBase& vmViewModel) noexcept
            : ra::ui::win32::bindings::GridBinding(vmViewModel)
        {
        }

        void OnLvnItemChanged(const LPNMLISTVIEW pnmListView) override;

        void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

        void UpdateColumnWidths();
    };

    SearchResultsGridBinding m_bindSearchResults;

    bindings::TextBoxBinding m_bindAddress;
    bindings::TextBoxBinding m_bindNoteText;

    bindings::RadioButtonBinding m_bindViewer8Bit;
    bindings::RadioButtonBinding m_bindViewer16Bit;
    bindings::RadioButtonBinding m_bindViewer32Bit;
    bindings::RadioButtonBinding m_bindViewer32BitBE;
    bindings::MemoryViewerControlBinding m_bindViewer;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DLG_MEMORYINSPECTOR_H
