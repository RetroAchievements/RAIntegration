#ifndef RA_CORE_H
#define RA_CORE_H
#pragma once

#include "ra_fwd.h"

extern HMODULE g_hThisDLLInst;
extern HWND g_RAMainWnd;

GSL_SUPPRESS_F23 void _ReadStringTil(std::string& sValue, char nChar, const char* restrict& pOffsetInOut);

void RestoreWindowPosition(HWND hDlg, const char* sDlgKey, bool bToRight, bool bToBottom);
void RememberWindowPosition(HWND hDlg, const char* sDlgKey);
void RememberWindowSize(HWND hDlg, const char* sDlgKey);

#endif // !RA_CORE_H
