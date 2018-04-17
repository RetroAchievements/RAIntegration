#pragma once

#include <WTypes.h>

class Dlg_RichPresence
{
public:
	Dlg_RichPresence();
	~Dlg_RichPresence();

	static INT_PTR CALLBACK s_RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR CALLBACK RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);

	void InstallHWND(HWND hWnd) { m_hRichPresenceDialog = hWnd; }
	HWND GetHWND() const { return m_hRichPresenceDialog; }

	void StartMonitoring();

private:
	void StartTimer();
	void StopTimer();

	HFONT m_hFont;
	HWND m_hRichPresenceDialog;
	bool m_bTimerActive;
};

extern Dlg_RichPresence g_RichPresenceDialog;


