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
    TEST_METHOD(TestAtari2600)
    {
        ConsoleContext context(ConsoleID::Atari2600);

        Assert::AreEqual(25, (int)context.Id());
        Assert::AreEqual(std::wstring(L"Atari 2600"), context.Name());

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

    TEST_METHOD(TestGameboyAdvance)
    {
        ConsoleContext context(ConsoleID::GBA);

        Assert::AreEqual(5, (int)context.Id());
        Assert::AreEqual(std::wstring(L"GameBoy Advance"), context.Name());

        const auto& vRegions = context.MemoryRegions();
        Assert::AreEqual({2}, vRegions.size());
        Assert::AreEqual(0x00000000U, vRegions.at(0).StartAddress);
        Assert::AreEqual(0x00007FFFU, vRegions.at(0).EndAddress);
        Assert::AreEqual(0x03000000U, vRegions.at(0).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SaveRAM, (int)vRegions.at(0).Type);
        Assert::AreEqual(std::string("Cartridge RAM"), vRegions.at(0).Description);
        Assert::AreEqual(0x00008000U, vRegions.at(1).StartAddress);
        Assert::AreEqual(0x00047FFFU, vRegions.at(1).EndAddress);
        Assert::AreEqual(0x02000000U, vRegions.at(1).RealAddress);
        Assert::AreEqual((int)ConsoleContext::AddressType::SystemRAM, (int)vRegions.at(1).Type);
        Assert::AreEqual(std::string("System RAM"), vRegions.at(1).Description);

        AssertRegion(context, 0x00000000U, &vRegions.at(0));
        AssertRegion(context, 0x00007FFFU, &vRegions.at(0));
        AssertRegion(context, 0x00008000U, &vRegions.at(1));
        AssertRegion(context, 0x00047FFFU, &vRegions.at(1));
        AssertRegion(context, 0x00048000U, nullptr);

        Assert::AreEqual(0x00019234U, context.ByteAddressFromRealAddress(0x02011234U));
    }

    TEST_METHOD(TestPlayStationPortable)
    {
        ConsoleContext context(ConsoleID::PSP);

        Assert::AreEqual(41, (int)context.Id());
        Assert::AreEqual(std::wstring(L"PlayStation Portable"), context.Name());

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
};

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
