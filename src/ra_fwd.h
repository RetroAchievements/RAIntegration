#ifndef RA_FWD_H
#define RA_FWD_H
#pragma once

// Forward declaring namespace std caused problems
#include <xstring>


namespace ra {

#ifndef _TCHAR_DEFINED
#if _MBCS
using TCHAR = char;
#elif _UNICODE
using TCHAR = wchar_t;
#else 
#error Unknown character set detected, only MultiByte and Unicode are supported!
#endif // _MBCS
#define _TCHAR_DEFINED
#endif // !_TCHAR_DEFINED



using tstring = std::basic_string<TCHAR>;

using DWORD         = unsigned long;
using ARGB          = DWORD;
using ByteAddress   = std::size_t;
using DataPos       = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;
using GameID        = std::size_t;

} // namespace ra

#endif // !RA_FWD_H
