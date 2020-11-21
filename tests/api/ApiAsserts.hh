#ifndef RA_UNITTEST_APIHELPERS_H
#define RA_UNITTEST_APIHELPERS_H
#pragma once

#include "api\ApiCall.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::api::ApiResult>(const ra::api::ApiResult& result)
{
    switch (result)
    {
        case ra::api::ApiResult::None:
            return L"None";
        case ra::api::ApiResult::Success:
            return L"Success";
        case ra::api::ApiResult::Error:
            return L"Error";
        case ra::api::ApiResult::Failed:
            return L"Failed";
        case ra::api::ApiResult::Unsupported:
            return L"Unsupported";
        case ra::api::ApiResult::Incomplete:
            return L"Incomplete";
        default:
            return std::to_wstring(ra::etoi(result));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_UNITTEST_APIHELPERS_H */
