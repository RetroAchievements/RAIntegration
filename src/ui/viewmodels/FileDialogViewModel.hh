#ifndef RA_UI_FILEDIALOG_VIEW_MODEL_H
#define RA_UI_FILEDIALOG_VIEW_MODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class FileDialogViewModel : public WindowViewModelBase
{
public:
    FileDialogViewModel() noexcept(std::is_nothrow_default_constructible_v<WindowViewModelBase>) = default;
    ~FileDialogViewModel() noexcept = default;
    FileDialogViewModel(const FileDialogViewModel&) = delete;
    FileDialogViewModel& operator=(const FileDialogViewModel&) = delete;

    FileDialogViewModel(FileDialogViewModel&&) 
        noexcept(std::is_nothrow_move_constructible_v<WindowViewModelBase>) = default;

    FileDialogViewModel& operator=(FileDialogViewModel&&) noexcept = default;

    /// <summary>
    /// Registers a file type for the dialog.
    /// </summary>
    /// <param name="sType">Description of the type</param>
    /// <param name="sExtension">Semi-colon separated list of extensions for the type: i.e. "*.jpg;*.jpeg"</param>
    void AddFileType(const std::wstring& sType, const std::wstring& sExtensions);

    /// <summary>
    /// Gets the file types for the dialog.
    /// </summary>
    const std::map<std::wstring, std::wstring>& GetFileTypes() const noexcept { return m_mFileTypes; }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the default extension.
    /// </summary>
    static const StringModelProperty DefaultExtensionProperty;

    /// <summary>
    /// Gets the default file extension.
    /// </summary>
    const std::wstring& GetDefaultExtension() const { return GetValue(DefaultExtensionProperty); }

    /// <summary>
    /// Sets the default file extension.
    /// </summary>
    void SetDefaultExtension(const std::wstring& sValue) { SetValue(DefaultExtensionProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the name of the selected file.
    /// </summary>
    static const StringModelProperty FileNameProperty;

    /// <summary>
    /// Gets the name of the selected file.
    /// </summary>
    const std::wstring& GetFileName() const { return GetValue(FileNameProperty); }

    /// <summary>
    /// Sets the name of the selected file.
    /// </summary>
    void SetFileName(const std::wstring& sValue) { SetValue(FileNameProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the initial directory for the dialog.
    /// </summary>
    static const StringModelProperty InitialDirectoryProperty;

    /// <summary>
    /// Gets the initial directory for the dialog.
    /// </summary>
    const std::wstring& GetInitialDirectory() const { return GetValue(InitialDirectoryProperty); }

    /// <summary>
    /// Sets the initial directory for the dialog.
    /// </summary>
    void SetInitialDirectory(const std::wstring& sValue) { SetValue(InitialDirectoryProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether the user should be prompted to overwrite an existing file.
    /// </summary>
    static const BoolModelProperty OverwritePromptProperty;

    /// <summary>
    /// Gets whether the user should be prompted to overwrite an existing file.
    /// </summary>
    bool GetOverwritePrompt() const { return GetValue(OverwritePromptProperty); }

    /// <summary>
    /// Sets whether the user should be prompted to overwrite an existing file.
    /// </summary>
    void SetOverwritePrompt(bool bValue) { SetValue(OverwritePromptProperty, bValue); }


    enum Mode
    {
        None,
        Open,
        Save,
        Folder,
    };

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether the user should be prompted to overwrite an existing file.
    /// </summary>
    static const IntModelProperty ModeProperty;

    /// <summary>
    /// Gets whether the user should be prompted to overwrite an existing file.
    /// </summary>
    Mode GetMode() const { return static_cast<Mode>(GetValue(ModeProperty)); }

    /// <summary>
    /// Sets whether the user should be prompted to overwrite an existing file.
    /// </summary>
    void SetMode(Mode nValue) { SetValue(ModeProperty, static_cast<int>(nValue)); }

    /// <summary>
    /// Show the Save file dialog
    /// </summary>
    DialogResult ShowOpenFileDialog()
    {
        SetMode(Mode::Open);
        return ShowModal();
    }

    /// <summary>
    /// Show the Save file dialog
    /// </summary>
    DialogResult ShowOpenFileDialog(const WindowViewModelBase& vmParentWindow)
    {
        SetMode(Mode::Open);
        return ShowModal(vmParentWindow);
    }

    /// <summary>
    /// Show the Save file dialog
    /// </summary>
    DialogResult ShowSaveFileDialog()
    {
        SetMode(Mode::Save);
        return ShowModal();
    }

    /// <summary>
    /// Show the Save file dialog
    /// </summary>
    DialogResult ShowSaveFileDialog(const WindowViewModelBase& vmParentWindow)
    {
        SetMode(Mode::Save);
        return ShowModal(vmParentWindow);
    }

    /// <summary>
    /// Show the select folder dialog
    /// </summary>
    DialogResult ShowSelectFolderDialog()
    {
        SetMode(Mode::Folder);
        return ShowModal();
    }

    /// <summary>
    /// Show the select folder dialog
    /// </summary>
    DialogResult ShowSelectFolderDialog(const WindowViewModelBase& vmParentWindow)
    {
        SetMode(Mode::Folder);
        return ShowModal(vmParentWindow);
    }

protected:
    std::map<std::wstring, std::wstring> m_mFileTypes;

};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_FILEDIALOG_VIEW_MODEL_H
