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

_Success_(return == 0)
_NODISCARD static auto CALLBACK
BrowseCallbackProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ _UNUSED LPARAM lParam, _In_ LPARAM lpData) noexcept // it might
{
    if (uMsg == BFFM_INITIALIZED)
    {
        LPCTSTR path{};
        GSL_SUPPRESS_TYPE1 path = reinterpret_cast<LPCTSTR>(lpData);
        GSL_SUPPRESS_TYPE1 ::SendMessage(hwnd, ra::to_unsigned(BFFM_SETSELECTION), 0U, reinterpret_cast<LPARAM>(path));
    }
    return 0;
}

static void ShowFolder(FileDialogViewModel& vmFileDialog, HWND hParentWnd)
{
    IFileDialog* pFileDialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))))
    {
        pFileDialog->SetTitle(vmFileDialog.GetWindowTitle().c_str());

        const std::wstring& sInitialLocation = vmFileDialog.GetInitialDirectory();
        IShellItem* pShellItem = nullptr;
        if (!sInitialLocation.empty())
        {
            LPITEMIDLIST pItemIdList = nullptr;
            DWORD nAttrs = 0;
            if (SHILCreateFromPath(sInitialLocation.c_str(), &pItemIdList, &nAttrs) == 0)
            {
                if (SHCreateShellItem(nullptr, nullptr, pItemIdList, &pShellItem) == 0)
                {
                    pFileDialog->SetFolder(pShellItem);
                    pShellItem->Release();
                }
            }
        }

        DWORD dwOptions;
        if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions)))
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);

        const HRESULT hr = pFileDialog->Show(hParentWnd);
        pFileDialog->Release();

        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            vmFileDialog.SetDialogResult(ra::ui::DialogResult::Cancel);
            return;
        }

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(pFileDialog->GetResult(&pShellItem)))
            {
                wchar_t* sPath;
                if (SUCCEEDED(pShellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &sPath)))
                {
                    vmFileDialog.SetFileName(sPath);
                    vmFileDialog.SetDialogResult(ra::ui::DialogResult::OK);
                }

                pShellItem->Release();
            }

            return;
        }
    }

    // fall back to BrowseFolderDialog
    if (::OleInitialize(nullptr) == S_OK)
    {
        BROWSEINFOW bi{};
        bi.hwndOwner = hParentWnd;

        std::wstring sDisplayName;
        sDisplayName.reserve(512);
        bi.pszDisplayName = sDisplayName.data();
        bi.lpszTitle = vmFileDialog.GetWindowTitle().c_str();

        bi.ulFlags = BIF_USENEWUI | BIF_VALIDATE;
        bi.lpfn = BrowseCallbackProc;

        if (!vmFileDialog.GetInitialDirectory().empty())
        {
            GSL_SUPPRESS_TYPE1 bi.lParam = reinterpret_cast<LPARAM>(vmFileDialog.GetInitialDirectory().c_str());
        }

        const auto idlist_deleter = [](LPITEMIDLIST lpItemIdList) noexcept
        {
            ::CoTaskMemFree(lpItemIdList);
            lpItemIdList = nullptr;
        };

        using ItemListOwner = std::unique_ptr<ITEMIDLIST __unaligned, decltype(idlist_deleter)>;
        ItemListOwner owner{ ::SHBrowseForFolderW(&bi), idlist_deleter };
        if (owner)
        {
            if (::SHGetPathFromIDListW(owner.get(), bi.pszDisplayName) == 0)
            {
                vmFileDialog.SetDialogResult(ra::ui::DialogResult::Cancel);
            }
            else
            {
                vmFileDialog.SetFileName(bi.pszDisplayName);
                vmFileDialog.SetDialogResult(ra::ui::DialogResult::OK);
            }
        }

        ::OleUninitialize();
    }
}

void FileDialog::Presenter::DoShowModal(ra::ui::WindowViewModelBase& oViewModel, HWND hParentWnd)
{
    auto& vmFileDialog = reinterpret_cast<FileDialogViewModel&>(oViewModel);

    if (vmFileDialog.GetMode() == FileDialogViewModel::Mode::Folder)
    {
        ShowFolder(vmFileDialog, hParentWnd);
        return;
    }

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
            Expects(pStart != nullptr);
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
    ofn.nMaxFile = gsl::narrow_cast<DWORD>(sFilenameBuffer.size());
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
