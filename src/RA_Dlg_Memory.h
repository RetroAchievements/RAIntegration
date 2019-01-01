#ifndef RA_DLG_MEMORY_H
#define RA_DLG_MEMORY_H
#pragma once

#include "RA_CodeNotes.h" // RA_Defs.h
#include "RA_MemManager.h"
#include "services/SearchResults.h"

class MemoryViewerControl
{
public:
    static LRESULT CALLBACK s_MemoryDrawProc(HWND, UINT, WPARAM, LPARAM);

public:
    static void RenderMemViewer(HWND hTarget);

    static void createEditCaret(int w, int h) noexcept;
    static void destroyEditCaret() noexcept;
    static void SetCaretPos();
    static void OnClick(POINT point);

    static bool OnKeyDown(UINT nChar);
    static bool OnEditInput(UINT c);

    static void setAddress(unsigned int nAddr);
    static void setWatchedAddress(unsigned int nAddr);
    static unsigned int getWatchedAddress() noexcept { return m_nWatchedAddress; }
    static void moveAddress(int offset, int nibbleOff);
    static void editData(unsigned int nByteAddress, bool bLowerNibble, unsigned int value);
    static void Invalidate();

    static void SetDataSize(MemSize value)
    {
        m_nDataSize = value;
        Invalidate();
    }
    static MemSize GetDataSize() noexcept { return m_nDataSize; }

public:
    static unsigned short m_nActiveMemBank;
    static unsigned int m_nDisplayedLines;

private:
    static HFONT m_hViewerFont;
    static SIZE m_szFontSize;
    static unsigned int m_nDataStartXOffset;
    static unsigned int m_nAddressOffset;
    static unsigned int m_nWatchedAddress;
    static MemSize m_nDataSize;
    static unsigned int m_nEditAddress;
    static unsigned int m_nEditNibble;

    static bool m_bHasCaret;
    static unsigned int m_nCaretWidth;
    static unsigned int m_nCaretHeight;
};

struct SearchResult
{
    ra::services::SearchResults m_results;

    std::vector<unsigned int> m_modifiedAddresses;
    bool m_bUseLastValue = false;
    unsigned int m_nLastQueryVal = 0;
    ComparisonType m_nCompareType = ComparisonType::Equals;

    bool WasModified(unsigned int nAddress)
    {
        return std::find(m_modifiedAddresses.begin(), m_modifiedAddresses.end(), nAddress) != m_modifiedAddresses.end();
    }
};

class Dlg_Memory
{
public:
    void Init() noexcept;

    void ClearLogOutput() noexcept;

    static INT_PTR CALLBACK s_MemoryProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR MemoryProc(HWND, UINT, WPARAM, LPARAM);

    void InstallHWND(HWND hWnd) noexcept { m_hWnd = hWnd; }
    HWND GetHWND() const noexcept { return m_hWnd; }

    void OnLoad_NewRom();

    void OnWatchingMemChange();

    void RepopulateMemNotesFromFile();
    void Invalidate();

    void SetWatchingAddress(unsigned int nAddr);
    void UpdateBits() const;
    BOOL IsActive() const noexcept;

    const CodeNotes& Notes() const noexcept { return m_CodeNotes; }

    void ClearBanks() noexcept;
    void AddBank(size_t nBankID);
    void GenerateResizes(HWND hDlg);

private:
    bool GetSystemMemoryRange(ra::ByteAddress& start, ra::ByteAddress& end) noexcept;
    bool GetGameMemoryRange(ra::ByteAddress& start, ra::ByteAddress& end) noexcept;

    bool GetSelectedMemoryRange(ra::ByteAddress& start, ra::ByteAddress& end);

    void UpdateSearchResult(const ra::services::SearchResults::Result& result, _Out_ unsigned int& nMemVal, TCHAR(&buffer)[1024]);
    bool CompareSearchResult(unsigned int nCurVal, unsigned int nPrevVal);

    static CodeNotes m_CodeNotes;
    static HWND m_hWnd;

    unsigned int m_nStart = 0;
    unsigned int m_nEnd = 0;
    MemSize m_nCompareSize = MemSize{};

    std::vector<SearchResult> m_SearchResults;
};

extern Dlg_Memory g_MemoryDialog;


#endif // !RA_DLG_MEMORY_H
