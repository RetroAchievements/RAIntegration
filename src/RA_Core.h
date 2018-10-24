#ifndef RA_CORE_H
#define RA_CORE_H
#pragma once

#include "RA_Defs.h"
#include "RA_Interface.h"

#include "Exports.hh"

//	Non-exposed:
extern std::wstring g_sHomeDir;

extern HINSTANCE g_hRAKeysDLL;
extern HMODULE g_hThisDLLInst;
extern HWND g_RAMainWnd;
extern EmulatorID g_EmulatorID;
extern ConsoleID g_ConsoleID;
extern const char* g_sClientVersion;
extern const char* g_sClientName;
extern bool g_bRAMTamperedWith;

//	Read a file to a malloc'd buffer. Returns nullptr on error. Owner MUST free() buffer if not nullptr.
extern char* _MallocAndBulkReadFileToBuffer(const wchar_t* sFilename, long& nFileSizeOut);

//  Read a file to a std::string. Returns false on error.
_Success_(return)
_NODISCARD bool _ReadBufferFromFile(_Out_ std::string& buffer, _In_ const wchar_t* const __restrict sFile);

//	Read file until reaching the end of the file, or the specified char.
extern BOOL _ReadTil(const char nChar, char buffer[], unsigned int nSize, DWORD* pCharsRead, FILE* pFile);

//	Read a string til the end of the string, or nChar. bTerminate==TRUE replaces that char with \0.
extern char* _ReadStringTil(char nChar, char*& pOffsetInOut, BOOL bTerminate);
extern void  _ReadStringTil(std::string& sValue, char nChar, const char*& pOffsetInOut);

//	Write out the buffer to a file
extern void _WriteBufferToFile(const std::wstring& sFileName, const std::string& sString);

//	Fetch various interim txt/data files
extern void _FetchGameHashLibraryFromWeb();
extern void _FetchGameTitlesFromWeb();
extern void _FetchMyProgressFromWeb();


extern BOOL _FileExists(const std::wstring& sFileName);

extern std::string _TimeStampToString(time_t nTime);

_NODISCARD std::string GetFolderFromDialog() noexcept;

void DownloadAndActivateAchievementData(unsigned int nGameID);
BOOL CanCausePause();

void RestoreWindowPosition(HWND hDlg, const char* sDlgKey, bool bToRight, bool bToBottom);
void RememberWindowPosition(HWND hDlg, const char* sDlgKey);
void RememberWindowSize(HWND hDlg, const char* sDlgKey);

#endif // !RA_CORE_H
