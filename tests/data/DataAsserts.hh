#ifndef RA_UNITTEST_DATAHELPERS_H
#define RA_UNITTEST_DATAHELPERS_H
#pragma once

#include "data\context\GameContext.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::context::GameContext::Mode>(const ra::data::context::GameContext::Mode& nMode)
{
    switch (nMode)
    {
        case ra::data::context::GameContext::Mode::Normal:
            return L"Normal";
        case ra::data::context::GameContext::Mode::CompatibilityTest:
            return L"CompatibilityTest";
        default:
            return std::to_wstring(static_cast<int>(nMode));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_UNITTEST_DATAHELPERS_H */
