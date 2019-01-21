#ifndef RA_FWD_H
#define RA_FWD_H
#pragma once

#define _NORETURN \
    [[noreturn]] /*only used on functions that 100% fail (like functions that solely throw or std::terminate) */

/* Maybe an extra check just in-case */
#if _HAS_CXX17
#define _DEPRECATED [[deprecated]]
#define _DEPRECATEDR(reason) [[deprecated(reason)]]
#define _FALLTHROUGH [[fallthrough]] //; you need ';' at the end
#define _UNUSED [[maybe_unused]]
#define _CONSTANT_VAR inline constexpr auto
#else
#define _NODISCARD _Check_return_
#define _DEPRECATED __declspec(deprecated)
#define _DEPRECATEDR(reason) _CRT_DEPRECATE_TEXT(reason)
#define _FALLTHROUGH __fallthrough //; you need ';' at the end
#define _UNUSED
#define _CONSTANT_VAR constexpr auto
#endif // _HAS_CXX17

#define _CONSTANT_LOC constexpr auto // local vars can't be inline
#define _CONSTANT_FN _CONSTANT_VAR

#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 0
#endif /* !__STDC_VERSION__ */

#if __STDC_VERSION__ >= 199901L
/* "restrict" is a keyword */
#elif defined(__GNUC__) || defined(__clang__)
#define restrict __restrict__
#elif defined(_MSC_VER)
#define restrict __restrict
#else
#define restrict
#endif /* __STDC_VERSION__ >= 199901L */

/* Use these for code analysis suppressions if needed */
/* clang-format off */
#define GSL_SUPPRESS_CON3 GSL_SUPPRESS(con.3)
#define GSL_SUPPRESS_CON4 GSL_SUPPRESS(con.4)
#define GSL_SUPPRESS_ES47 GSL_SUPPRESS(es.47)
#define GSL_SUPPRESS_F6 GSL_SUPPRESS(f.6)
#define GSL_SUPPRESS_F23 GSL_SUPPRESS(f.23)
#define GSL_SUPPRESS_IO5 GSL_SUPPRESS(io.5)
#define GSL_SUPPRESS_TYPE1 GSL_SUPPRESS(type.1)
#define GSL_SUPPRESS_TYPE6 GSL_SUPPRESS(type.6)
/* clang-format on */

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
struct HBITMAP__;
struct HDC__;
struct HPEN__;
struct HBRUSH__;
struct HWND__;
struct HINSTANCE__;

using HBITMAP = HBITMAP__*;
using HDC = HDC__*;
using HPEN = HPEN__*;
using HBRUSH = HBRUSH__*;

using HWND = HWND__*;
using HINSTANCE = HINSTANCE__*;
using HMODULE = HINSTANCE__*;

struct tagRECT;
struct tagSIZE;
using RECT = tagRECT;
using SIZE = tagSIZE;

using BOOL = int;
using BYTE = unsigned char;
using DWORD = unsigned long;
using HGDIOBJ = void*;
using LPVOID = void*;
using WORD = unsigned short;
#endif // !_WINDEF_

#ifndef _WINNT_
using HANDLE = void*;
using LPCTSTR = const TCHAR*;
using LPTSTR = TCHAR*;
#endif // !_WINNT_

namespace ra {

using tstring = std::basic_string<TCHAR>;

using ARGB = DWORD;
using ByteAddress = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;

} /* namespace ra */

#endif /* !RA_FWD_H */
