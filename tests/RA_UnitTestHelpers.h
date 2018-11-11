#ifndef RA_UNITTESTHELPERS_H
#define RA_UNITTESTHELPERS_H
#pragma once

#include "ra_utility.h"

#include "RA_Condition.h"
#include "RA_StringUtils.h"

#include "api\ApiCall.hh"

#include "services\Http.hh"

#include "ui\WindowViewModelBase.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

template<> static std::wstring ToString<MemSize>(const MemSize& t)
{
    return MEMSIZE_STR.at(ra::etoi(t));
}

template<> static std::wstring ToString<CompVariable::Type>(const CompVariable::Type& t)
{
    return ra::Widen(CompVariable::TYPE_STR.at(ra::etoi(t)));
}

template<> static std::wstring ToString<ComparisonType>(const ComparisonType& t)
{
    return ra::Widen(COMPARISONTYPE_STR[(int)t]);
}

template<> static std::wstring ToString<Condition::Type>(const Condition::Type& t)
{
    return (Condition::TYPE_STR.at(ra::etoi(t)));
}

template<> static std::wstring ToString<ra::services::Http::StatusCode>(const ra::services::Http::StatusCode& t)
{
    return std::to_wstring(ra::etoi(t));
}

template<> static std::wstring ToString<ra::ui::DialogResult>(const ra::ui::DialogResult& result)
{
    switch (result)
    {
        case ra::ui::DialogResult::None: return L"None";
        case ra::ui::DialogResult::OK: return L"OK";
        case ra::ui::DialogResult::Cancel: return L"Cancel";
        case ra::ui::DialogResult::Yes: return L"Yes";
        case ra::ui::DialogResult::No: return L"No";
        case ra::ui::DialogResult::Retry: return L"Retry";
        default: return std::to_wstring(ra::etoi(result));
    }
}

template<> static std::wstring ToString<ra::api::ApiResult>(const ra::api::ApiResult& result)
{
    switch (result)
    {
        case ra::api::ApiResult::None: return L"None";
        case ra::api::ApiResult::Success: return L"Success";
        case ra::api::ApiResult::Error: return L"Error";
        case ra::api::ApiResult::Failed: return L"Failed";
        case ra::api::ApiResult::Unsupported: return L"Unsupported";
        default: return std::to_wstring(ra::etoi(result));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft


// Loads memory into the MemoryManager
void InitializeMemory(unsigned char* pMemory, size_t szMemorySize);

void AssertContains(const std::string& sHaystack, const std::string& sNeedle);


#endif /* !RA_UNITTESTHELPERS_H */
