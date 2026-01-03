#ifndef RA_MEMORYREGION_ASSERTS_H
#define RA_MEMORYREGION_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data\MemoryRegion.hh"
#include "util\TypeCasts.hh"

// The rcheevos nameless struct warning is only affecting the test project, for now we have
// to disable the warning in the project or pragmatically in rcheevos. Careful not to use nameless structs here.

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::MemoryRegion::Type>(const ra::data::MemoryRegion::Type& t)
{
    switch (t)
    {
        case ra::data::MemoryRegion::Type::Unknown:
            return L"Unknown";
        case ra::data::MemoryRegion::Type::SaveRAM:
            return L"SaveRAM";
        case ra::data::MemoryRegion::Type::SystemRAM:
            return L"SystemRAM";
        case ra::data::MemoryRegion::Type::VirtualRAM:
            return L"VirtualRAM";
        case ra::data::MemoryRegion::Type::VideoRAM:
            return L"VideoRAM";
        case ra::data::MemoryRegion::Type::HardwareController:
            return L"HardwareController";
        case ra::data::MemoryRegion::Type::ReadOnlyMemory:
            return L"ReadOnlyMemory";
        case ra::data::MemoryRegion::Type::Unused:
            return L"Unused";
        default:
            return std::to_wstring(ra::etoi(t));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_MEMORYREGION_ASSERTS_H */
