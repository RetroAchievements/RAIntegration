#ifndef RA_UNITTEST_SERVICESHELPERS_H
#define RA_UNITTEST_SERVICESHELPERS_H
#pragma once

#include "util\TypeCasts.hh"

#include "services\Http.hh"

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

#endif /* !RA_UNITTEST_SERVICESHELPERS_H */
