#include "FileDialogViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty FileDialogViewModel::DefaultExtensionProperty("FileDialogViewModel", "DefaultExtension", L"");
const StringModelProperty FileDialogViewModel::FileNameProperty("FileDialogViewModel", "FileName", L"");
const StringModelProperty FileDialogViewModel::InitialDirectoryProperty("FileDialogViewModel", "InitialDirectory", L"");
const BoolModelProperty FileDialogViewModel::OverwritePromptProperty("FileDialogViewModel", "OverwritePrompt", true);
const IntModelProperty FileDialogViewModel::ModeProperty("FileDialogViewModel", "Mode", 0);

void FileDialogViewModel::AddFileType(const std::wstring& sType, const std::wstring& sExtensions)
{
    m_mFileTypes.emplace(sExtensions, sType);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
