#ifndef RA_HTTP_ASSERTS_H
#define RA_HTTP_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "services\Http.hh"

#include "util\TypeCasts.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.
template<>
std::wstring ToString<ra::services::Http::StatusCode>(const ra::services::Http::StatusCode& t)
{
    return std::to_wstring(ra::etoi(t));
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_HTTP_ASSERTS_H */
