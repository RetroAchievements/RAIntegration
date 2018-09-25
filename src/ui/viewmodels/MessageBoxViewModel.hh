#ifndef RA_UI_MESSAGEBOX_VIEW_MODEL_H
#define RA_UI_MESSAGEBOX_VIEW_MODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

enum class MessageBoxButtons
{
    OK,
    OKCancel,
    YesNo,
    YesNoCancel,
    RetryCancel,
};

enum class MessageBoxIcon
{
    None,
    Info,
    Warning,
    Error,
};

class MessageBoxViewModel : public WindowViewModelBase
{
public:
    MessageBoxViewModel() noexcept {}

    MessageBoxViewModel(const std::wstring& sMessage) noexcept
    {
        SetMessage(sMessage);
    }

#pragma push_macro("GetMessage") // windows stole my method name
#undef GetMessage
    /// <summary>
    /// The <see cref="ModelProperty" /> for the message.
    /// </summary>
    static StringModelProperty MessageProperty;

    /// <summary>
    /// Gets the message to display.
    /// </summary>
    const std::wstring& GetMessage() const { return GetValue(MessageProperty); }

    /// <summary>
    /// Sets the message to display.
    /// </summary>
    void SetMessage(const std::wstring& sValue) { SetValue(MessageProperty, sValue); }
#pragma pop_macro("GetMessage")

    /// <summary>
    /// The <see cref="ModelProperty" /> for the header message.
    /// </summary>
    static StringModelProperty HeaderProperty;

    /// <summary>
    /// Gets the header message to display.
    /// </summary>
    const std::wstring& GetHeader() const { return GetValue(HeaderProperty); }

    /// <summary>
    /// Sets the header message to display, empty string for no header message.
    /// </summary>
    void SetHeader(const std::wstring& sValue) { SetValue(HeaderProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the icon.
    /// </summary>
    static IntModelProperty IconProperty;

    /// <summary>
    /// Gets the icon to display.
    /// </summary>
    const MessageBoxIcon GetIcon() const { return static_cast<MessageBoxIcon>(GetValue(IconProperty)); }

    /// <summary>
    /// Sets the icon to display (default: None).
    /// </summary>
    void SetIcon(MessageBoxIcon nValue) { SetValue(IconProperty, static_cast<int>(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the buttons to display.
    /// </summary>
    static IntModelProperty ButtonsProperty;

    /// <summary>
    /// Gets the buttons to display.
    /// </summary>
    const MessageBoxButtons GetButtons() const { return static_cast<MessageBoxButtons>(GetValue(ButtonsProperty)); }

    /// <summary>
    /// Sets the buttons to display (default: OK).
    /// </summary>
    void SetButtons(MessageBoxButtons nValue) { SetValue(ButtonsProperty, static_cast<int>(nValue)); }

    /// <summary>
    /// Shows a generic message.
    /// </summary>
    static void ShowMessage(const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows an informational message.
    /// </summary>
    static void ShowInfoMessage(const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetIcon(MessageBoxIcon::Info);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows a warning message.
    /// </summary>
    static void ShowWarningMessage(const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetIcon(MessageBoxIcon::Warning);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows a warning message.
    /// </summary>
    static void ShowWarningMessage(const std::wstring& sHeader, const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetHeader(sHeader);
        viewModel.SetIcon(MessageBoxIcon::Warning);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows an error message.
    /// </summary>
    static void ShowErrorMessage(const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetIcon(MessageBoxIcon::Error);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows an error message with a header message.
    /// </summary>
    static void ShowErrorMessage(const std::wstring& sHeader, const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetHeader(sHeader);
        viewModel.SetIcon(MessageBoxIcon::Error);
        viewModel.ShowModal();
    }
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MESSAGEBOX_VIEW_MODEL_H
