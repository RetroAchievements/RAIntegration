#ifndef _DLG_MEMORY_H_
#define _DLG_MEMORY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <wtypes.h>
#include <assert.h>

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

private:
	//static HWND m_hControl;
	static HFONT m_hViewerFont;
	static SIZE m_szFontSize;
	static unsigned int m_nDataStartXOffset;
	static unsigned int m_nAddressOffset;
	static unsigned int m_nAddressSize;
	static unsigned int m_nDataSize;
	static unsigned int m_nEditAddress;
	static unsigned int m_nEditNibble;
	static unsigned int m_nDisplayedLines;

	static bool m_bHasCaret;
	static unsigned int m_nCaretWidth;
	static unsigned int m_nCaretHeight;
};

class Dlg_Memory
{
public:
	Dlg_Memory();
	~Dlg_Memory();

public:
	void Init();

	static INT_PTR CALLBACK s_MemoryProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR MemoryProc(HWND, UINT, WPARAM, LPARAM);

	void InstallHWND( HWND hWnd )	{ m_hWnd = hWnd; }
	HWND GetHWND() const			{ return m_hWnd; }

	void OnLoad_NewRom();

	void OnWatchingMemChange();

	void RepopulateMemNotesFromFile();
	void Invalidate();
	
	void SetWatchingAddress( unsigned int nAddr );

public:
	CodeNotes* m_pCodeNotes;
	HWND m_hWnd;
};
extern Dlg_Memory g_MemoryDialog;

#ifdef __cplusplus
};
#endif


#endif //_DLG_ACHIEVEMENT_H_