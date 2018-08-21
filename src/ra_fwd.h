#ifndef RA_FWD_H
#define RA_FWD_H
#pragma once

#include <xstring> /* Too much trouble, might refactor this file to get rid of it */

#define _NORETURN [[noreturn]]

#if _HAS_CXX17
#define _DEPRECATED          [[deprecated]]
#define _DEPRECATEDR(reason) [[deprecated(reason)]]
#define _FALLTHROUGH         [[fallthrough]] /* ; you need ';' at the end */
#define _UNUSED              [[maybe_unused]]
#define _CONSTANT_VAR        inline constexpr auto
#else
#define _NODISCARD           _Check_return_
#define _DEPRECATED          __declspec(deprecated)
#define _DEPRECATEDR(reason) _CRT_DEPRECATE_TEXT(reason)
#define _FALLTHROUGH         __fallthrough /* ; you need ';' at the end */
#define _UNUSED              
#define _CONSTANT_VAR        constexpr auto
#endif /* _HAS_CXX17 */        

#define _CONSTANT_LOC constexpr auto /* local vars can't be inline */
#define _CONSTANT_FN  _CONSTANT_VAR

#ifndef CALLBACK
#define CALLBACK __stdcall
#endif /* !CALLBACK */

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
#endif /* _MBCS */
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */

#ifndef _WINNT_
using HANDLE = void*;
#endif /* !_WINNT_ */

#ifndef _VCRUNTIME_H
#if _WIN32
using uintptr_t = unsigned int;
using intptr_t  = int;
#elif _WIN64
using uintptr_t = unsigned long long;
using intptr_t  = long long;
#else
#error Only Windows is currently supported
#endif /* _WIN32 */
namespace std {

using ::uintptr_t;
using ::intptr_t;

} /* namespace std */
#endif /* !_VCRUNTIME_H */

#ifndef _BASETSD_H_
#if _WIN32
using LONG_PTR = long;
#elif _WIN64
using LONG_PTR = long long;
#else
#error Only Windows is currently supported
#endif /* _WIN32 */
#endif /* !_BASETSD_H_ */

#ifndef _WINDEF_
using BOOL   = int;
using BYTE   = unsigned char;
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

using UINT    = unsigned int;
using LPARAM  = LONG_PTR;
using LRESULT = LONG_PTR;
using WPARAM  = std::uintptr_t;
using INT_PTR = std::intptr_t;
#endif /* !_WINDEF_ */

/* This needs to be forward declared to get rid of a conformance error */
struct IUnknown;

using time_t = long long;
using size_t = std::uintptr_t;

namespace std {

using ::time_t;
using ::size_t;

} /* namespace std */

namespace ra {

/* usings/typedefs matter in C++ */
#if _MBCS
using tstring = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
#elif  _UNICODE
using tstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;
#else
#error Unknown Character set
#endif /* _MBCS */

using ARGB          = ::DWORD;
using ByteAddress   = std::size_t;
using DataPos       = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;
using GameID        = std::size_t;

} /* namespace ra */

#endif /* !RA_FWD_H */
