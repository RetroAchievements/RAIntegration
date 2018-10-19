#ifndef PCH_CPPCORECHECK_SUPPRESSIONS_H
#define PCH_CPPCORECHECK_SUPPRESSIONS_H
#pragma once

/*
    Currently these warnings only apply to RapidJSON and MD5, only the PCH can include this file.
    We should not be suppressing these warnings for our code.

    There is an exception for where md5.h is used.
    Some of these warnings are addressed in Microsoft's Native Rule-sets.
*/

#pragma warning(disable : 26401) // Do not delete a raw pointer that is not an owner<T>(i.11).

#pragma warning(disable : 26408) // Avoid malloc() and free(), prefer the nothrow version of new
                                 // with delete (r.10).

#pragma warning(disable : 26409) // Avoid calling new and delete explicitly, use std::make_unique<T>
                                 // instead (r.11).

#pragma warning(disable : 26429) // Symbol 'symbol' is never tested for nullness, it can be marked
                                 // as gsl::not_null(f.23).

#pragma warning(disable : 26432) // If you define or delete any default operation in the type 'type',
                                 // define or delete them all(c.21).

#pragma warning(disable : 26440) // Function 'function' can be declared 'noexcept' (f.6).

// Remarks: We can make our own "at" function if GSL is not desired
#pragma warning(disable : 26446) // Prefer to use gsl::at() instead of unchecked subscript
                                 // operator (bounds.4).

#pragma warning(disable : 26447) // The function is declared 'noexcept' but calls function
                                 // 'function' which may throw exceptions(f.6).

// Remarks: This basically means you are using a narrower type than the return type as the return
//          value.
#pragma warning(disable : 26451) // Arithmetic overflow: Using operator '-' on a 4 byte value and
                                 // then casting the result to a 8 byte value. Cast the value to
                                 // the wider type before calling operator '-' to avoid
                                 // overflow (io.2).

#pragma warning(disable : 26460) // The reference argument 'arg' for function 'fn' can be marked as
                                 // const (con.3).RA_Integration

// Example: "const char*":       pointer to const
//          "char* const":       const pointer
//          "const char* const": const pointer to const
// Remarks: Prefer "const pointer to const" when you can, if that isn't possible use pointer to const
//          This logic does not apply to references
#pragma warning(disable : 26461) // The pointer argument 'argument' for function 'function' can be
                                 // marked as a pointer to const (con.3).

#pragma warning(disable : 26462) // The value pointed to by 'pointer' is assigned only once, mark it
                                 // as a pointer to const (con.4).

#pragma warning(disable : 26465) // Don't use const_cast to cast away const.
                                 // const_cast is not required; constness or volatility is not being
                                 // removed by this conversion (type.3).

#pragma warning(disable : 26471) // Don't use reinterpret_cast. A cast from void* can use
                                 // static_cast (type.1).

// Remarks: There is a narrow_cast for RA that is similar to GSL's but it's not complete yet
//          GSL does not do unboxing, they just forward the parameter recursively, arithmetic types
//          shouldn't be passed by rvalue references anyway.
#pragma warning(disable : 26472) // Don't use a static_cast for arithmetic conversions.
                                 // Use brace initialization, gsl::narrow_cast or
                                 // gsl::narrow (type.1).

#pragma warning(disable : 26473) // Don't cast between pointer types where the source type and the
                                 // target type are the same (type.1).

#pragma warning(disable : 26475) // Do not use function style C-casts(es.49).
#pragma warning(disable : 26477) // Use 'nullptr' rather than 0 or NULL(es.47).

// Remarks: Pointer arithmetic makes no sense
#pragma warning(disable : 26481) // Don't use pointer arithmetic. Use gsl::span instead (bounds.1).
#pragma warning(disable : 26482) // Only index into arrays using constant expressions(bounds.2).

// Remarks: this happens a lot in RA where a static array is used instead of a dynamic on in
//          sprintf_s and it's variants.
#pragma warning(disable : 26485) // Expression 'expression': No array to pointer decay(bounds.3).
#pragma warning(disable : 26490) // Don't use reinterpret_cast (type.1).
#pragma warning(disable : 26493) // Don't use C-style casts (type.4).

#pragma warning(disable : 26494) // Variable 'variable' is uninitialized. Always initialize an
                                 // object(type.5).

#pragma warning(disable : 26495) // Variable 'variable' is uninitialized.
                                 // Always initialize a member variable(type.6).

#pragma warning(disable : 26496) // The variable 'variable' is assigned only once, mark it as
                                 // const (con.4).

#pragma warning(disable : 26497) // The function 'function' could be marked constexpr if compile-time
                                 // evaluation is desired(f.4).

#endif /* !PCH_CPPCORECHECK_SUPPRESSIONS_H */
