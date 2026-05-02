#ifndef RA_VALUE_ASSERTS_H
#define RA_VALUE_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data/Value.hh"

#include "util/TypeCasts.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::Value::Format>(const ra::data::Value::Format& nFormat)
{
    switch (nFormat)
    {
        case ra::data::Value::Format::Score:
            return L"Score";
        case ra::data::Value::Format::Value:
            return L"Value";
        case ra::data::Value::Format::Frames:
            return L"Frames";
        case ra::data::Value::Format::Centiseconds:
            return L"Centiseconds";
        case ra::data::Value::Format::Seconds:
            return L"Seconds";
        case ra::data::Value::Format::Minutes:
            return L"Minutes";
        case ra::data::Value::Format::SecondsAsMinutes:
            return L"SecondsAsMinutes";
        case ra::data::Value::Format::Float1:
            return L"Float1";
        case ra::data::Value::Format::Float2:
            return L"Float2";
        case ra::data::Value::Format::Float3:
            return L"Float3";
        case ra::data::Value::Format::Float4:
            return L"Float4";
        case ra::data::Value::Format::Float5:
            return L"Float5";
        case ra::data::Value::Format::Float6:
            return L"Float6";
        case ra::data::Value::Format::Fixed1:
            return L"Fixed1";
        case ra::data::Value::Format::Fixed2:
            return L"Fixed2";
        case ra::data::Value::Format::Fixed3:
            return L"Fixed3";
        case ra::data::Value::Format::Tens:
            return L"Tens";
        case ra::data::Value::Format::Hundreds:
            return L"Hundreds";
        case ra::data::Value::Format::Thousands:
            return L"Thousands";
        case ra::data::Value::Format::UnsignedValue:
            return L"UnsignedValue";
        case ra::data::Value::Format::Unformatted:
            return L"Unformatted";
        default:
            return std::to_wstring(ra::etoi(nFormat));
    }
}


#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_VALUE_ASSERTS_H */
