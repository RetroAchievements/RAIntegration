#ifndef RA_CORE_H
#define RA_CORE_H
#pragma once

#include "RA_Defs.h"
#include "RA_Interface.h"

#include "Exports.hh"

// Non-exposed:
extern std::wstring g_sHomeDir;

extern HINSTANCE g_hRAKeysDLL;
extern HMODULE g_hThisDLLInst;
extern HWND g_RAMainWnd;
extern ConsoleID g_ConsoleID;
extern bool g_bRAMTamperedWith;

// Read a file to a malloc'd buffer. Returns nullptr on error. Owner MUST free() buffer if not nullptr.
extern char* _MallocAndBulkReadFileToBuffer(_In_z_ const wchar_t* sFilename, _Out_ long& nFileSizeOut) noexcept;

// Read a file to a std::string. Returns false on error.
_Success_(return ) _NODISCARD bool _ReadBufferFromFile(_Out_ std::string& buffer, _In_ const wchar_t* restrict sFile);

// Read file until reaching the end of the file, or the specified char.
extern BOOL _ReadTil(const char nChar, char* restrict buffer, unsigned int nSize,
                     gsl::not_null<DWORD* restrict> pCharsReadOut, gsl::not_null<FILE* restrict> pFile);

GSL_SUPPRESS_F23 char* _ReadStringTil(char nChar, char* restrict& pOffsetInOut, BOOL bTerminate);
GSL_SUPPRESS_F23 void _ReadStringTil(std::string& sValue, char nChar, const char* restrict& pOffsetInOut);

// Write out the buffer to a file
extern void _WriteBufferToFile(const std::wstring& sFileName, const std::string& sString);

// Fetch various interim txt/data files
extern void _FetchGameHashLibraryFromWeb();
extern void _FetchGameTitlesFromWeb();
extern void _FetchMyProgressFromWeb();

extern std::string _TimeStampToString(time_t nTime);

_NODISCARD std::string GetFolderFromDialog();

BOOL CanCausePause() noexcept;

void RestoreWindowPosition(HWND hDlg, const char* sDlgKey, bool bToRight, bool bToBottom);
void RememberWindowPosition(HWND hDlg, const char* sDlgKey);
void RememberWindowSize(HWND hDlg, const char* sDlgKey);

#endif // !RA_CORE_H
