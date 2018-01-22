#pragma once

#include "RA_Defs.h"
#include "RA_CodeNotes.h"
#include "RA_MemManager.h"
#include "RA_Condition.h"

class CodeNotes;

class MemoryViewerControl
{
public:
	static INT_PTR CALLBACK s_MemoryDrawProc(HWND, UINT, WPARAM, LPARAM);

public:
	static void RenderMemViewer( HWND hTarget );

	static void createEditCaret( int w, int h );
	static void destroyEditCaret();
	static void SetCaretPos();
	static void OnClick( POINT point );

	static bool OnKeyDown( UINT nChar );
	static bool OnEditInput( UINT c );

	static void setAddress( unsigned int nAddr );
	static void moveAddress( int offset, int nibbleOff );
	static void editData( unsigned int nByteAddress, bool bLowerNibble, unsigned int value );
	static void Invalidate();

public:
	static unsigned short m_nActiveMemBank;
	static unsigned int m_nDisplayedLines;

private:
	static HFONT m_hViewerFont;
	static SIZE m_szFontSize;
	static unsigned int m_nDataStartXOffset;
	static unsigned int m_nAddressOffset;
	static unsigned int m_nDataSize;
	static unsigned int m_nEditAddress;
	static unsigned int m_nEditNibble;

	static bool m_bHasCaret;
	static unsigned int m_nCaretWidth;
	static unsigned int m_nCaretHeight;
};

class SearchResult
{
	public:
		SearchResult() {}
	public:
		std::vector<MemCandidate> m_ResultCandidate;
		unsigned int m_nCount = 0;
		unsigned int m_nLastQueryVal = 0;
		bool m_bUseLastValue;
		tstring m_sFirstLine;
		tstring m_sSecondLine;
		ComparisonType m_nCompareType;
};

class Dlg_Memory
{
public:
	Dlg_Memory() {}

public:
	void Init();
	
	void ClearLogOutput();

	static INT_PTR CALLBACK s_MemoryProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR MemoryProc(HWND, UINT, WPARAM, LPARAM);

	void InstallHWND( HWND hWnd )				{ m_hWnd = hWnd; }
	HWND GetHWND() const						{ return m_hWnd; }
	
	void OnLoad_NewRom();

	void OnWatchingMemChange();

	void RepopulateMemNotesFromFile();
	void Invalidate();
	
	void SetWatchingAddress( unsigned int nAddr );
	BOOL IsActive() const;

	const CodeNotes& Notes() const				{ return m_CodeNotes; }

	void ClearBanks();
	void AddBank( size_t nBankID );
	void GenerateResizes( HWND hDlg );

private:
	bool GetSystemMemoryRange( ByteAddress& start, ByteAddress& end );
	bool GetGameMemoryRange( ByteAddress& start, ByteAddress& end );

	bool GetSelectedMemoryRange( ByteAddress& start, ByteAddress& end );

	void UpdateSearchResult( unsigned int index, unsigned int &nMemVal, TCHAR ( &buffer )[ 1024 ] );
	bool CompareSearchResult ( unsigned int nCurVal, unsigned int nPrevVal );

	static CodeNotes m_CodeNotes;
	static HWND m_hWnd;

	unsigned int m_nStart = 0;
	unsigned int m_nEnd = 0;
	ComparisonVariableSize m_nCompareSize;

	std::vector<SearchResult> m_SearchResults;
};

extern Dlg_Memory g_MemoryDialog;
