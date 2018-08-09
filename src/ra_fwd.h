#ifndef RA_FWD_H
#define RA_FWD_H
#pragma once

// Forward declaring namespace std caused problems
#include <xstring>

#ifndef _TCHAR_DEFINED
#if _MBCS
using TCHAR  = char;
using PTCHAR = char*;
using TBYTE  = unsigned char;
using PTBYTE = unsigned char*;
#elif _UNICODE
using TCHAR  = wchar_t;
using PTCHAR = wchar_t*;
using TBYTE  = wchar_t;
using PTBYTE = wchar_t*;
#else 
#error Unknown character set detected, only MultiByte and Unicode are supported!
#endif // _MBCS
#define _TCHAR_DEFINED
#endif // !_TCHAR_DEFINED

#ifndef _WINDEF_
using BOOL   = int;
using DWORD  = unsigned long;
using LPVOID = void*;
#endif // !_WINDEF_

#ifndef _WINNT_
using HANDLE = void*;
#endif // !_WINNT_

namespace ra {

using tstring = std::basic_string<TCHAR>;

using ARGB          = DWORD;
using ByteAddress   = std::size_t;
using DataPos       = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;
using GameID        = std::size_t;

} // namespace ra

#endif // !RA_FWD_H
