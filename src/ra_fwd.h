#ifndef RA_FWD_H
#define RA_FWD_H
#pragma once

/* Forward declaring namespace std caused problems */
#include <xstring>

#define _NORETURN            [[noreturn]]

/* Maybe an extra check just in-case */
#if _HAS_CXX17
#define _DEPRECATED          [[deprecated]]
#define _DEPRECATEDR(reason) [[deprecated(reason)]]
#define _FALLTHROUGH         [[fallthrough]]//; you need ';' at the end
#define _UNUSED              [[maybe_unused]]
#define _CONSTANT_VAR        inline constexpr auto
#else
#define _NODISCARD           _Check_return_
#define _DEPRECATED          __declspec(deprecated)
#define _DEPRECATEDR(reason) _CRT_DEPRECATE_TEXT(reason)
#define _FALLTHROUGH         __fallthrough//; you need ';' at the end
#define _UNUSED              
#define _CONSTANT_VAR        constexpr auto
#endif // _HAS_CXX17        

#define _CONSTANT_LOC constexpr auto // local vars can't be inline
#define _CONSTANT_FN  _CONSTANT_VAR

#ifndef _TCHAR_DEFINED
#if _MBCS
using TCHAR = char;
#elif _UNICODE
using TCHAR = wchar_t;
#else 
#error Unknown character set detected, only MultiByte and Unicode are supported!
#endif /* _MBCS */
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */

#ifndef _WINDEF_
using DWORD  = unsigned long;
using LPVOID = void*;
#endif // !_WINDEF_

#ifndef _WINNT_
using HANDLE  = void*;
using LPTSTR  = TCHAR*;
using LPCTSTR = const TCHAR*;
#endif // !_WINNT_

namespace ra {

#if _MBCS
using NativeStrType = std::string;
#elif _UNICODE
using NativeStrType = std::wstring;
#else
#error Unknown character set detected! Only MultiByte and Unicode are supported!
#endif // _MBCS

using Native_CStrType = NativeStrType::const_pointer;
using tstring         = NativeStrType;

using ARGB          = DWORD;
using ByteAddress   = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;
using GameID        = std::size_t;

} /* namespace ra */

#endif /* !RA_FWD_H */
