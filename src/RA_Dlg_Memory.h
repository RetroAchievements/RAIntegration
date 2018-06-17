#ifndef RA_DLG_MEMORY_H
#define RA_DLG_MEMORY_H
#pragma once

#include "RA_CodeNotes.h" // RA_Defs.h
#include "RA_MemManager.h"

// this doesn't need to be static but we'll work on that later (static == hang
// unless you can guarantee compile time speed/constexpr)

class MemoryViewerControl
{
public:
    static INT_PTR CALLBACK s_MemoryDrawProc(HWND, UINT, WPARAM, LPARAM);

public:
    static void RenderMemViewer(HWND hTarget);

    static void createEditCaret(int w, int h);
    static void destroyEditCaret();
    static void SetCaretPos();
    static void OnClick(POINT point);

    static bool OnKeyDown(UINT nChar);
    static bool OnEditInput(UINT c);

    static void setAddress(unsigned int nAddr);
    static void setWatchedAddress(unsigned int nAddr);
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    static unsigned int getWatchedAddress() { return m_nWatchedAddress; }
    static void SetDataSize(ComparisonVariableSize value) { m_nDataSize = value; Invalidate(); }
    static ComparisonVariableSize GetDataSize() { return m_nDataSize; }
#pragma warning(pop)

    
    static void moveAddress(int offset, int nibbleOff);
    static void editData(unsigned int nByteAddress, bool bLowerNibble, unsigned int value);
    static void Invalidate();

    

public:
    static unsigned short m_nActiveMemBank;
    static unsigned int m_nDisplayedLines;

private:
    static HFONT m_hViewerFont;
    static SIZE m_szFontSize;
    static unsigned int m_nDataStartXOffset;
    static unsigned int m_nAddressOffset;
    static unsigned int m_nWatchedAddress;
    static ComparisonVariableSize m_nDataSize;
    static unsigned int m_nEditAddress;
    static unsigned int m_nEditNibble;

    static bool m_bHasCaret;
    static unsigned int m_nCaretWidth;
    static unsigned int m_nCaretHeight;
};

#pragma pack(push, 1)
struct SearchResult
{
    std::vector<MemCandidate> m_ResultCandidate;
    unsigned int m_nCount ={};
    unsigned int m_nLastQueryVal ={};
    bool m_bUseLastValue ={};
    tstring m_sFirstLine;
    tstring m_sSecondLine;
    ComparisonType m_nCompareType ={};
};
#pragma pack(pop)

class Dlg_Memory
{
public:


public:
    void Init();

    void ClearLogOutput();

    static INT_PTR CALLBACK s_MemoryProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR MemoryProc(HWND, UINT, WPARAM, LPARAM);

#pragma warning(push)
    // unreferenced inline functions, do you see the pattern boss? What do all of these have in common? -Samer
#pragma warning(disable : 4514)
    void InstallHWND(HWND hWnd) { m_hWnd = hWnd; }
    HWND GetHWND() const { return m_hWnd; }

    // here's an alternative that won't trigger it (it will now since it's not
    // used), much better in an helper class though
    _NODISCARD inline constexpr auto operator()() const noexcept { return m_hWnd; } // get hwnd
    inline constexpr auto operator()(_In_ HWND hwnd) noexcept { m_hWnd = hwnd; } // set hwnd

    const CodeNotes& Notes() const { return m_CodeNotes; }
#pragma warning(pop)


    void OnLoad_NewRom();

    void OnWatchingMemChange();

    void RepopulateMemNotesFromFile();
    void Invalidate();

    void SetWatchingAddress(unsigned int nAddr);
    void UpdateBits() const;
    BOOL IsActive() const;

    

    void ClearBanks();
    void AddBank(size_t nBankID);
    void GenerateResizes(HWND hDlg);

private:
    bool GetSystemMemoryRange(ByteAddress& start, ByteAddress& end);
    bool GetGameMemoryRange(ByteAddress& start, ByteAddress& end);

    bool GetSelectedMemoryRange(ByteAddress& start, ByteAddress& end);

    void UpdateSearchResult(unsigned int index, unsigned int &nMemVal, TCHAR(&buffer)[1024]);
    bool CompareSearchResult(unsigned int nCurVal, unsigned int nPrevVal);

    static CodeNotes m_CodeNotes;
    static HWND m_hWnd;

    unsigned int m_nStart = 0;
    unsigned int m_nEnd = 0;
    ComparisonVariableSize m_nCompareSize;

    std::vector<SearchResult> m_SearchResults;
};

extern Dlg_Memory g_MemoryDialog;


#endif // !RA_DLG_MEMORY_H
