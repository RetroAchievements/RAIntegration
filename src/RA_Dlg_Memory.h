#ifndef RA_DLG_MEMORY_H
#define RA_DLG_MEMORY_H
#pragma once

#include "data/GameContext.hh"

#include "ui/ViewModelBase.hh"

#include "ui/win32/bindings/GridBinding.hh"

class MemoryViewerControl
{
public:
    static LRESULT CALLBACK s_MemoryDrawProc(HWND, UINT, WPARAM, LPARAM);

public:
    static void RenderMemViewer(HWND hTarget);

    static void OnClick(POINT point);

    static bool OnKeyDown(UINT nChar);
    static bool OnEditInput(UINT c);

    static void Invalidate();

    static MemSize GetDataSize();
};

class Dlg_Memory : protected ra::ui::ViewModelBase::NotifyTarget,
                   protected ra::ui::ViewModelCollectionBase::NotifyTarget,
                   protected ra::data::GameContext::NotifyTarget
{
public:
    void Init() noexcept;
    void Shutdown() noexcept;

    static INT_PTR CALLBACK s_MemoryProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR MemoryProc(HWND, UINT, WPARAM, LPARAM);

    void InstallHWND(HWND hWnd) noexcept { m_hWnd = hWnd; }
    HWND GetHWND() const noexcept { return m_hWnd; }

    void OnLoad_NewRom();

    void OnWatchingMemChange();

    void Invalidate();

    void UpdateMemoryRegions();

    void SetWatchingAddress(unsigned int nAddr);
    void GoToAddress(unsigned int nAddr);
    void UpdateBits() const;
    BOOL IsActive() const noexcept;

    void GenerateResizes(HWND hDlg);

protected:
    // ViewModelBase::NotifyTarget
    void OnViewModelIntValueChanged(const ra::ui::IntModelProperty::ChangeArgs& args) override;

    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const ra::ui::BoolModelProperty::ChangeArgs& args) override;

    // GameContext::NotifyTarget
    void OnEndGameLoad() override;

private:
    void SetAddressRange();
    ra::ByteAddress m_nSystemRamStart{}, m_nSystemRamEnd{}, m_nGameRamStart{}, m_nGameRamEnd{};

    static HWND m_hWnd;

    unsigned int m_nStart = 0;
    unsigned int m_nEnd = 0;
    MemSize m_nCompareSize = MemSize{};

    std::unique_ptr<ra::ui::win32::bindings::GridBinding> m_pSearchGridBinding;
};

extern Dlg_Memory g_MemoryDialog;


#endif // !RA_DLG_MEMORY_H
