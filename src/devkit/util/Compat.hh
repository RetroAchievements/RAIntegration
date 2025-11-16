#ifndef RA_COMPAT_HH
#define RA_COMPAT_HH
#pragma once

/* ===== function prototype macros ===== */

//#define _DEPRECATED [[deprecated]]
//#define _DEPRECATEDR(reason) [[deprecated(reason)]]
//#define _FALLTHROUGH [[fallthrough]] //; you need ';' at the end
#define _UNUSED [[maybe_unused]]
#define _CONSTANT_VAR inline constexpr auto
//
#define _CONSTANT_LOC constexpr auto // local vars can't be inline
#define _CONSTANT_FN _CONSTANT_VAR

#ifndef __STDC_VERSION__
 #define __STDC_VERSION__ 0
#endif /* !__STDC_VERSION__ */

#if __STDC_VERSION__ >= 199901L
 /* "restrict" is a keyword */
 #define _RESTRICT restrict 
#elif defined(__GNUC__) || defined(__clang__)
 #define _RESTRICT __restrict__
#elif defined(_MSC_VER)
 #define _RESTRICT __restrict
#else
 #define _RESTRICT restrict
#endif /* __STDC_VERSION__ >= 199901L */

#endif /* !RA_COMPAT_HH */
