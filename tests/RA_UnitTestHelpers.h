#ifndef RA_UNITTESTHELPERS_H
#define RA_UNITTESTHELPERS_H
#pragma once

#include "ra_utility.h"

#include "RA_Condition.h"
#include "RA_StringUtils.h"

#include "RA_Achievement.h"

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

#ifndef RA_ANALYSIS
// this has to be defined when compiling against v141_xp, but not when compiling against v142.
// since it's impractical to detect the selected platform toolset, just key off the fact that
// the Analysis builds have to use the newer toolset.

template<>
std::wstring ToString<__int64>(const __int64& t)
{
    RETURN_WIDE_STRING(t);
}
#endif

template<>
std::wstring ToString<MemSize>(const MemSize& t)
{
    switch (t)
    {
        case MemSize::Text:
            return L"Text";
        default:
            if (ra::etoi(t) < MEMSIZE_STR.size())
                return MEMSIZE_STR.at(ra::etoi(t));
            return std::to_wstring(ra::etoi(t));
    }

}

template<>
std::wstring ToString<CompVariable::Type>(const CompVariable::Type& t)
{
    return ra::Widen(CompVariable::TYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString<ComparisonType>(const ComparisonType& t)
{
    return ra::Widen(COMPARISONTYPE_STR.at(ra::etoi(t)));
}

template<>
std::wstring ToString<Condition::Type>(const Condition::Type& t)
{
    return (Condition::TYPE_STR.at(ra::etoi(t)));
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

void AssertContains(const std::string& sHaystack, const std::string& sNeedle);
void AssertDoesNotContain(const std::string& sHaystack, const std::string& sNeedle);

#endif /* !RA_UNITTESTHELPERS_H */
