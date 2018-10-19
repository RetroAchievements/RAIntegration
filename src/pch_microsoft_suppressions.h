#ifndef PCH_MICROSOFT_SUPPRESSIONS_H
#define PCH_MICROSOFT_SUPPRESSIONS_H
#pragma once

/* These were triggered from "Microsoft All Rules" */
/* Currently only applies to RapidJSON, this file must not be included in an project file */

/* Ambiguous Intent */
#pragma warning(disable : 6313) // Incorrect operator:  zero-valued flag cannot be tested with
                                // bitwise-and.Use an equality test to check for zero-valued flags.

/* Miscellaneous */
#pragma warning(disable : 26455) // Default constructor may not throw.Declare it 'noexcept' (f.6).

/*
    C++ Core Guidelines
    -------------------
    These did not caught from C++ Core Guideline rules (All rules).
    For those it sometimes has to be a specific rule set.
*/
#pragma warning(disable : 26429) // Symbol 'x' is never tested for nullness, it can be marked
// as not_null(f.23).

#pragma warning(disable : 26440) // Function 'fn' can be declared 'noexcept' (f.6).
#pragma warning(disable : 26477) // Use 'nullptr' rather than 0 or NULL(es.47).
#pragma warning(disable : 26481) // Don't use pointer arithmetic. Use span instead (bounds.1).
#pragma warning(disable : 26482) // Only index into arrays using constant expressions(bounds.2).


#pragma warning(disable : 26485) // Expression 'exp': No array to pointer decay(bounds.3).

#pragma warning(disable : 26486) // Don't pass a pointer that may be invalid to a function.
                                 // Parameter n 'param' in call to 'function' may be
                                 // invalid (lifetime.1).

#pragma warning(disable : 26487) // Don't return a pointer that may be invalid (lifetime.1).

#pragma warning(disable : 26489) // Don't dereference a pointer that may be invalid: 'copy.s'.
                                 // 's' may have been invalidated at line 'line' (lifetime.1).

#pragma warning(disable : 26493) // Don't use C-style casts (type.4).
#pragma warning(disable : 26494) // Variable 'x' is uninitialized. Always initialize an object(type.5)
#pragma warning(disable : 26496) // The variable 'x' is assigned only once, mark it as const (con.4).

#endif /* !PCH_MICROSOFT_SUPPRESSIONS_H */
