#ifndef RA_UNITTESTHELPERS_H
#define RA_UNITTESTHELPERS_H
#pragma once

#include "util\Strings.hh"

#include "data\Types.hh"

// The rcheevos nameless struct warning is only affecting the test project, for now we have
// to disable the warning in the project or pragmatically in rcheevos. Careful not to use nameless structs here.

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.
template<>
std::wstring ToString<std::chrono::milliseconds>(const std::chrono::milliseconds& t)
{
    return std::to_wstring(t.count()) + L"ms";
}

template<>
std::wstring ToString<ComparisonType>(const ComparisonType& t)
{
    switch (t)
    {
        case ComparisonType::Equals:
            return L"Equals";
        case ComparisonType::LessThan:
            return L"LessThan";
        case ComparisonType::LessThanOrEqual:
            return L"LessThanOrEqual";
        case ComparisonType::GreaterThan:
            return L"GreaterThan";
        case ComparisonType::GreaterThanOrEqual:
            return L"GreaterThanOrEqual";
        case ComparisonType::NotEqualTo:
            return L"NotEqualTo";
        default:
            return std::to_wstring(ra::etoi(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

void AssertContains(const std::string& sHaystack, const std::string& sNeedle);
void AssertDoesNotContain(const std::string& sHaystack, const std::string& sNeedle);

#endif /* !RA_UNITTESTHELPERS_H */
