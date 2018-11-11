#ifndef RA_UI_WINDOW_VIEW_MODEL_BASE_H
#define RA_UI_WINDOW_VIEW_MODEL_BASE_H
#pragma once

#include "Types.hh"
#include "ViewModelBase.hh"
#include "ra_utility.h"

namespace ra {
namespace ui {

enum class DialogResult
{
    None = 0,
    OK,
    Cancel,
    Yes,
    No,
    Retry,
};

class WindowViewModelBase : public ViewModelBase
{
public:
    ~WindowViewModelBase() noexcept = default;
    WindowViewModelBase(const WindowViewModelBase&) = delete;
    WindowViewModelBase& operator=(const WindowViewModelBase&) = delete;

    WindowViewModelBase(WindowViewModelBase&&) 
        noexcept(std::is_nothrow_move_constructible_v<ViewModelBase>) = default;

    WindowViewModelBase& operator=(WindowViewModelBase&&) noexcept = default;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the window title.
    /// </summary>
    static StringModelProperty WindowTitleProperty;

    /// <summary>
    /// Gets the window title.
    /// </summary>
    const std::wstring& GetWindowTitle() const { return GetValue(WindowTitleProperty); }

    /// <summary>
    /// Sets the window title.
    /// </summary>
    void SetWindowTitle(const std::wstring& sTitle) { SetValue(WindowTitleProperty, sTitle); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the dialog result.
    /// </summary>
    static const IntModelProperty DialogResultProperty;

    /// <summary>
    /// Gets the dialog result.
    /// </summary>
    const DialogResult GetDialogResult() const { return itoe<DialogResult>(GetValue(DialogResultProperty)); }

    /// <summary>
    /// Sets the dialog result.
    /// </summary>
    /// <remarks>Setting this to anything other than 'None' will cause the window to close.</remarks>
    void SetDialogResult(DialogResult nValue) { SetValue(DialogResultProperty, etoi(nValue)); }

    /// <summary>
    /// Shows a window for this view model.
    /// </summary>
    void Show();

    /// <summary>
    /// Shows a modal window for this view model. Method will not return until the window is closed.
    /// </summary>
    DialogResult ShowModal();

protected:
    WindowViewModelBase() noexcept(std::is_nothrow_default_constructible_v<ViewModelBase>) = default;
};

} // namespace ui
} // namespace ra

#endif RA_UI_WINDOW_VIEW_MODEL_BASE_H
