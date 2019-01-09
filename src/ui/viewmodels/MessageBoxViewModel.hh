#ifndef RA_UI_MESSAGEBOX_VIEW_MODEL_H
#define RA_UI_MESSAGEBOX_VIEW_MODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class MessageBoxViewModel : public WindowViewModelBase
{
public:
    MessageBoxViewModel() noexcept(std::is_nothrow_default_constructible_v<WindowViewModelBase>) = default;

    explicit MessageBoxViewModel(const std::wstring& sMessage) 
        noexcept(std::is_nothrow_default_constructible_v<WindowViewModelBase>) : WindowViewModelBase{}
    {
        SetMessage(sMessage);
    }
    ~MessageBoxViewModel() noexcept = default;
    MessageBoxViewModel(const MessageBoxViewModel&) = delete;
    MessageBoxViewModel& operator=(const MessageBoxViewModel&) = delete;

    MessageBoxViewModel(MessageBoxViewModel&&) 
        noexcept(std::is_nothrow_move_constructible_v<WindowViewModelBase>) = default;

    MessageBoxViewModel& operator=(MessageBoxViewModel&&) noexcept = default;

    enum class Buttons
    {
        OK,
        OKCancel,
        YesNo,
        YesNoCancel,
        RetryCancel,
    };

    enum class Icon
    {
        None,
        Info,
        Warning,
        Error,
    };

#pragma push_macro("GetMessage") // windows stole my method name
#undef GetMessage
    /// <summary>
    /// The <see cref="ModelProperty" /> for the message.
    /// </summary>
    static const StringModelProperty MessageProperty;

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
    static const StringModelProperty HeaderProperty;

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
    static const IntModelProperty IconProperty;

    /// <summary>
    /// Gets the icon to display.
    /// </summary>
    const Icon GetIcon() const { return static_cast<Icon>(GetValue(IconProperty)); }

    /// <summary>
    /// Sets the icon to display (default: None).
    /// </summary>
    void SetIcon(Icon nValue) { SetValue(IconProperty, static_cast<int>(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the buttons to display.
    /// </summary>
    static const IntModelProperty ButtonsProperty;

    /// <summary>
    /// Gets the buttons to display.
    /// </summary>
    const Buttons GetButtons() const { return static_cast<Buttons>(GetValue(ButtonsProperty)); }

    /// <summary>
    /// Sets the buttons to display (default: OK).
    /// </summary>
    void SetButtons(Buttons nValue) { SetValue(ButtonsProperty, static_cast<int>(nValue)); }

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
        viewModel.SetIcon(Icon::Info);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows a warning message.
    /// </summary>
    static void ShowWarningMessage(const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetIcon(Icon::Warning);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows a warning message.
    /// </summary>
    static void ShowWarningMessage(std::wstring&& sMessage)
    {
        MessageBoxViewModel viewModel(std::move(sMessage));
        viewModel.SetIcon(Icon::Warning);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows a warning message.
    /// </summary>
    static void ShowWarningMessage(const std::wstring& sHeader, const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetHeader(sHeader);
        viewModel.SetIcon(Icon::Warning);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows a warning message.
    /// </summary>
    static void ShowWarningMessage(std::wstring&& sHeader, std::wstring&& sMessage)
    {
        const auto _sMessage = std::move(sMessage);
        MessageBoxViewModel viewModel(_sMessage);
        const auto _sHeader = std::move(sHeader);
        viewModel.SetHeader(_sHeader);
        viewModel.SetIcon(Icon::Warning);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows an error message.
    /// </summary>
    static void ShowErrorMessage(const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetIcon(Icon::Error);
        viewModel.ShowModal();
    }

    /// <summary>
    /// Shows an error message with a header message.
    /// </summary>
    static void ShowErrorMessage(const std::wstring& sHeader, const std::wstring& sMessage)
    {
        MessageBoxViewModel viewModel(sMessage);
        viewModel.SetHeader(sHeader);
        viewModel.SetIcon(Icon::Error);
        viewModel.ShowModal();
    }
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_MESSAGEBOX_VIEW_MODEL_H
