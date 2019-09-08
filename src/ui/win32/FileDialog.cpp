#include "FileDialog.hh"

#include "RA_StringUtils.h"

#include "services/ServiceLocator.hh"

#include "ui/viewmodels/FileDialogViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"
#include "ui/win32/bindings/WindowBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

using ra::ui::viewmodels::FileDialogViewModel;

bool FileDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& oViewModel) noexcept
{
    return (dynamic_cast<const FileDialogViewModel*>(&oViewModel) != nullptr);
}

void FileDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    DoShowModal(oViewModel, nullptr);
}

void FileDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& oViewModel, HWND hParentWnd)
{
    DoShowModal(oViewModel, hParentWnd);
}

void FileDialog::Presenter::DoShowModal(ra::ui::WindowViewModelBase& oViewModel, HWND hParentWnd)
{
    auto& vmFileDialog = reinterpret_cast<FileDialogViewModel&>(oViewModel);

    const auto& sDefaultExtension = vmFileDialog.GetDefaultExtension();
    DWORD nFilterIndex = 0;
    DWORD nIndex = 1;
    std::wstring sFileTypes;
    for (const auto& pPair : vmFileDialog.GetFileTypes())
    {
        sFileTypes.append(pPair.second);
        sFileTypes.append(L" (");
        sFileTypes.append(pPair.first);
        sFileTypes.append(L")\0", 2);
        sFileTypes.append(pPair.first);
        sFileTypes.append(L"\0", 1);

        if (!sDefaultExtension.empty())
        {
            const wchar_t* pStart = pPair.first.data();
            while (*pStart)
            {
                while (*pStart && *pStart != '.')
                    ++pStart;

                if (!*pStart)
                    break;

                const wchar_t* pEnd = ++pStart;
                while (*pEnd && *pEnd != ';')
                    ++pEnd;

                if (memcmp(pStart, sDefaultExtension.data(), pEnd - pStart) == 0)
                    nFilterIndex = nIndex;

                pStart = pEnd;
            }
        }

        ++nIndex;
    }
    sFileTypes.append(L"All Files (*.*)");
    sFileTypes.append(L"\0", 1);
    sFileTypes.append(L"*.*");
    sFileTypes.append(L"\0\0", 2);

    if (nFilterIndex == 0)
        nFilterIndex = 1;

    std::wstring sFilenameBuffer = vmFileDialog.GetFileName();
    sFilenameBuffer.resize(MAX_PATH);

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hParentWnd;
    ofn.lpstrFilter = sFileTypes.data();
    ofn.nFilterIndex = nFilterIndex;
    ofn.lpstrFile = sFilenameBuffer.data();
    ofn.nMaxFile = sFilenameBuffer.size();
    ofn.lpstrTitle = vmFileDialog.GetWindowTitle().c_str();
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = sDefaultExtension.c_str();

    const auto& sInitialDirectory = vmFileDialog.GetInitialDirectory();
    if (!sInitialDirectory.empty())
        ofn.lpstrInitialDir = sInitialDirectory.c_str();

    bool bResult = false;
    switch (vmFileDialog.GetMode())
    {
        case FileDialogViewModel::Mode::Open:
            ofn.Flags |= OFN_FILEMUSTEXIST;
            bResult = (GetOpenFileNameW(&ofn) == TRUE);
            break;

        case FileDialogViewModel::Mode::Save:
            if (vmFileDialog.GetOverwritePrompt())
                ofn.Flags |= OFN_OVERWRITEPROMPT;

            bResult = (GetSaveFileNameW(&ofn) == TRUE);
            break;
    }

    if (bResult)
    {
        sFilenameBuffer.resize(wcslen(ofn.lpstrFile));
        vmFileDialog.SetFileName(sFilenameBuffer);
        vmFileDialog.SetDialogResult(ra::ui::DialogResult::OK);
    }
    else
    {
        vmFileDialog.SetDialogResult(ra::ui::DialogResult::Cancel);
    }
}

} // namespace win32
} // namespace ui
} // namespace ra
