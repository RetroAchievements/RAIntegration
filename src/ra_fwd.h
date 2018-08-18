#ifndef RA_FWD_H
#define RA_FWD_H
#pragma once

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

#ifndef _WINNT_
using HANDLE = void*;
#endif // !_WINNT_

#ifndef _WINDEF_
using BOOL   = int;
using DWORD  = unsigned long;
using LPVOID = void*;

struct HBITMAP__;
struct HDC__;
struct HPEN__;
struct HFONT__;
struct HBRUSH__;
struct HINSTANCE__;
struct HWND__;
struct tagRECT;

using HBITMAP   = HBITMAP__*;
using HDC       = HDC__*;
using HPEN      = HPEN__*;
using HFONT     = HFONT__*;
using HBRUSH    = HBRUSH__*;
using HINSTANCE = HINSTANCE__*;
using HMODULE   = HINSTANCE;
using HWND      = HWND__*;
using RECT      = tagRECT;
#endif // !_WINDEF_

namespace std {

// Figured how to forward declare std
#ifndef _CTIME_
using ::time_t;
#endif // !_CTIME_

#ifndef _IOSFWD_
template struct char_traits<char>;
template struct char_traits<wchar_t>;
#endif // !_IOSFWD_

#ifndef _XMEMORY0_
template class allocator<char>;
template class allocator<wchar_t>;
#endif // !_XMEMORY0_

#ifndef _XSTRING_
template class basic_string<char, char_traits<char>, allocator<char>>;
template class basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>>;

using string  = basic_string<char, char_traits<char>, allocator<char>>;
using wstring = basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>>;
#endif // !_XSTRING_

} // namespace std

namespace ra {

using tstring = std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>>;

using ARGB          = ::DWORD;
using ByteAddress   = std::size_t;
using DataPos       = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;
using GameID        = std::size_t;

} // namespace ra

// Maybe an extra check just in-case

#define _NORETURN            [[noreturn]]

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

#endif // !RA_FWD_H
