#pragma once

#include <WTypes.h>

class Dlg_RichPresence
{
public:
	~Dlg_RichPresence() noexcept;
	Dlg_RichPresence(const Dlg_RichPresence&) = delete;
	Dlg_RichPresence& operator=(const Dlg_RichPresence&) = delete;
	Dlg_RichPresence(Dlg_RichPresence&&) noexcept = default;
	Dlg_RichPresence& operator=(Dlg_RichPresence&&) noexcept = default;
	Dlg_RichPresence() = default; // must be at the end, might throw because of HFONT

	static INT_PTR CALLBACK s_RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR CALLBACK RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);

	void InstallHWND(HWND hWnd) { m_hRichPresenceDialog = hWnd; }
	HWND GetHWND() const { return m_hRichPresenceDialog; }

	void StartMonitoring();

private:
	void StartTimer();
	void StopTimer();

	HFONT m_hFont{ 
		CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, nullptr) 
	};
	HWND m_hRichPresenceDialog{ nullptr };
	bool m_bTimerActive{ false };
};

extern Dlg_RichPresence g_RichPresenceDialog;


