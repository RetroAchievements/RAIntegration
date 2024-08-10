#include "CppUnitTest.h"

#include "data\context\ConsoleContext.hh"

#include "tests\data\DataAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace context {
namespace tests {

TEST_CLASS(ConsoleContext_Tests)
{
private:
    static void AssertRegion(const ConsoleContext& context, ra::ByteAddress nAddress,
        const ConsoleContext::MemoryRegion* pExpectedRegion)
    {
        const auto* pFoundRegion = context.GetMemoryRegion(nAddress);
        if (pFoundRegion != pExpectedRegion)
        {
            if (pExpectedRegion == nullptr)
                Assert::Fail(ra::StringPrintf(L"Found region for address %08x", nAddress).c_str());
            else if (pFoundRegion == nullptr)
                Assert::Fail(ra::StringPrintf(L"Did not find region for address %08x", nAddress).c_str());
            else
                Assert::Fail(ra::StringPrintf(L"Found wrong region for address %08x", nAddress).c_str());
        }
    }

public:
    TEST_METHOD(TestInitializeAtari2600)
    {
        ConsoleContext context(ConsoleID::Atari2600);

        Assert::AreEqual(25, (int)context.Id());
        Assert::AreEqual(std::wstring(L"Atari 2600"), context.Name());
        Assert::AreEqual(0x7FU, context.MaxAddress());

        const auto& vRegions = context.MemoryRegions();
        Assert::AreEqual({1}, vRegions.size());
        Assert::AreEqual(0x00U, vRegions.at(0).StartAddress);
        Assert::AreEqual(0x7FU, vRegions.at(0).EndAddress);
        Assert::AreEqual(0x80U, vRegions.at(0).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SystemRAM, (int)vRegions.at(0).Type);
        Assert::AreEqual(std::string("System RAM"), vRegions.at(0).Description);

        AssertRegion(context, 0x00U, &vRegions.at(0));
        AssertRegion(context, 0x7FU, &vRegions.at(0));
        AssertRegion(context, 0x80U, nullptr);

        Assert::AreEqual(0x14U, context.ByteAddressFromRealAddress(0x94U));
        Assert::AreEqual(0xFFFFFFFFU, context.ByteAddressFromRealAddress(0x14U));
    }

    TEST_METHOD(TestInitializeGameboyAdvance)
    {
        ConsoleContext context(ConsoleID::GBA);

        Assert::AreEqual(5, (int)context.Id());
        Assert::AreEqual(std::wstring(L"GameBoy Advance"), context.Name());
        Assert::AreEqual(0x00057FFFU, context.MaxAddress());

        const auto& vRegions = context.MemoryRegions();
        Assert::AreEqual({3}, vRegions.size());
        Assert::AreEqual(0x00000000U, vRegions.at(0).StartAddress);
        Assert::AreEqual(0x00007FFFU, vRegions.at(0).EndAddress);
        Assert::AreEqual(0x03000000U, vRegions.at(0).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SystemRAM, (int)vRegions.at(0).Type);
        Assert::AreEqual(std::string("System RAM"), vRegions.at(0).Description);
        Assert::AreEqual(0x00008000U, vRegions.at(1).StartAddress);
        Assert::AreEqual(0x00047FFFU, vRegions.at(1).EndAddress);
        Assert::AreEqual(0x02000000U, vRegions.at(1).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SystemRAM, (int)vRegions.at(1).Type);
        Assert::AreEqual(std::string("System RAM"), vRegions.at(1).Description);
        Assert::AreEqual(0x00048000U, vRegions.at(2).StartAddress);
        Assert::AreEqual(0x00057FFFU, vRegions.at(2).EndAddress);
        Assert::AreEqual(0x0E000000U, vRegions.at(2).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SaveRAM, (int)vRegions.at(2).Type);
        Assert::AreEqual(std::string("Save RAM"), vRegions.at(2).Description);

        AssertRegion(context, 0x00000000U, &vRegions.at(0));
        AssertRegion(context, 0x00007FFFU, &vRegions.at(0));
        AssertRegion(context, 0x00008000U, &vRegions.at(1));
        AssertRegion(context, 0x00047FFFU, &vRegions.at(1));
        AssertRegion(context, 0x00048000U, &vRegions.at(2));
        AssertRegion(context, 0x00057FFFU, &vRegions.at(2));
        AssertRegion(context, 0x00058000U, nullptr);

        Assert::AreEqual(0x00019234U, context.ByteAddressFromRealAddress(0x02011234U));
    }

    TEST_METHOD(TestInitializePlayStationPortable)
    {
        ConsoleContext context(ConsoleID::PSP);

        Assert::AreEqual(41, (int)context.Id());
        Assert::AreEqual(std::wstring(L"PlayStation Portable"), context.Name());
        Assert::AreEqual(0x01FFFFFFU, context.MaxAddress());

        const auto& vRegions = context.MemoryRegions();
        Assert::AreEqual({2}, vRegions.size());
        Assert::AreEqual(0x00000000U, vRegions.at(0).StartAddress);
        Assert::AreEqual(0x007FFFFFU, vRegions.at(0).EndAddress);
        Assert::AreEqual(0x08000000U, vRegions.at(0).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SystemRAM, (int)vRegions.at(0).Type);
        Assert::AreEqual(std::string("Kernel RAM"), vRegions.at(0).Description);
        Assert::AreEqual(0x00800000U, vRegions.at(1).StartAddress);
        Assert::AreEqual(0x01FFFFFFU, vRegions.at(1).EndAddress);
        Assert::AreEqual(0x08800000U, vRegions.at(1).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SystemRAM, (int)vRegions.at(1).Type);
        Assert::AreEqual(std::string("System RAM"), vRegions.at(1).Description);

        AssertRegion(context, 0x00000000U, &vRegions.at(0));
        AssertRegion(context, 0x007FFFFFU, &vRegions.at(0));
        AssertRegion(context, 0x00800000U, &vRegions.at(1));
        AssertRegion(context, 0x01FFFFFFU, &vRegions.at(1));
        AssertRegion(context, 0x02000000U, nullptr);

        Assert::AreEqual(0x01123456U, context.ByteAddressFromRealAddress(0x09123456U));
    }

    TEST_METHOD(TestByteAddressFromRealAddressDreamcast)
    {
        ConsoleContext context(ConsoleID::Dreamcast);

        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x0C001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x8C001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0xAC001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x9C001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x00001234U));
    }

    TEST_METHOD(TestByteAddressFromRealAddressGameCube)
    {
        ConsoleContext context(ConsoleID::GameCube);

        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x80001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0xC0001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0xA0001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x00001234U));
    }

    TEST_METHOD(TestByteAddressFromRealAddressDSi)
    {
        ConsoleContext context(ConsoleID::DSi);

        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x02001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x0C001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x0A001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x00001234U));
    }

    TEST_METHOD(TestByteAddressFromRealAddressPlayStation)
    {
        ConsoleContext context(ConsoleID::PlayStation);

        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x00001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x80001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0xA0001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x90001234U));
    }

    TEST_METHOD(TestByteAddressFromRealAddressPlayStation2)
    {
        ConsoleContext context(ConsoleID::PlayStation2);

        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x00001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x20001234U));
        Assert::AreEqual({ 0x00101234 }, context.ByteAddressFromRealAddress(0x30101234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x30001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x80001234U));
    }

    TEST_METHOD(TestByteAddressFromRealAddressWII)
    {
        ConsoleContext context(ConsoleID::WII);

        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0x80001234U));
        Assert::AreEqual({ 0x00001234 }, context.ByteAddressFromRealAddress(0xC0001234U));
        Assert::AreEqual({ 0x01801234 }, context.ByteAddressFromRealAddress(0x90001234U));
        Assert::AreEqual({ 0x01801234 }, context.ByteAddressFromRealAddress(0xD0001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0xA0001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0xB0001234U));
        Assert::AreEqual({ 0xFFFFFFFF }, context.ByteAddressFromRealAddress(0x00001234U));
    }
};

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
