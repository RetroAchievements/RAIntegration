#ifndef RA_GSL_HH
#define RA_GSL_HH
#pragma once

#define GSL_THROW_ON_CONTRACT_VIOLATION
#include <gsl\gsl>

// the default GSL Expects() macro throws an exception, which we don't try to catch anywhere.
// replace it with our custom handler, which will log and report the error before throwing it.
#ifndef RA_UTEST
 #ifdef NDEBUG
  extern void __gsl_contract_handler(const char* const file, unsigned int line);
  #undef Expects
  #define Expects(cond) (GSL_LIKELY(cond) ? static_cast<void>(0) : __gsl_contract_handler(__FILE__, __LINE__))
 #else
  extern void __gsl_contract_handler(const char* const file, unsigned int line, const char* const error);
  #undef Expects
  #define Expects(cond) (GSL_LIKELY(cond) ? static_cast<void>(0) : __gsl_contract_handler(__FILE__, __LINE__, GSL_STRINGIFY(cond)))
 #endif
#endif

/* Use these for code analysis suppressions if needed */
/* clang-format off */
#define GSL_SUPPRESS_BOUNDS4 GSL_SUPPRESS(bounds.4)
#define GSL_SUPPRESS_C128 GSL_SUPPRESS(c.128)
#define GSL_SUPPRESS_CON3 GSL_SUPPRESS(con.3)
#define GSL_SUPPRESS_CON4 GSL_SUPPRESS(con.4)
#define GSL_SUPPRESS_ENUM3 GSL_SUPPRESS(enum.3)
#define GSL_SUPPRESS_ES47 GSL_SUPPRESS(es.47)
#define GSL_SUPPRESS_F4 GSL_SUPPRESS(f.4)
#define GSL_SUPPRESS_F6 GSL_SUPPRESS(f.6)
#define GSL_SUPPRESS_F23 GSL_SUPPRESS(f.23)
#define GSL_SUPPRESS_IO5 GSL_SUPPRESS(io.5)
#define GSL_SUPPRESS_R3 GSL_SUPPRESS(r.3)
#define GSL_SUPPRESS_R30 GSL_SUPPRESS(r.30)
#define GSL_SUPPRESS_R32 GSL_SUPPRESS(r.32)
#define GSL_SUPPRESS_TYPE1 GSL_SUPPRESS(type.1)
#define GSL_SUPPRESS_TYPE3 GSL_SUPPRESS(type.3)
#define GSL_SUPPRESS_TYPE4 GSL_SUPPRESS(type.4)
#define GSL_SUPPRESS_TYPE6 GSL_SUPPRESS(type.6)
/* clang-format on */

#endif /* !RA_GSL_HH */
