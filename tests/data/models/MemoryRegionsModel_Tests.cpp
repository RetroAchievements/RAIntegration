#include "CppUnitTest.h"

#include "data\models\MemoryRegionsModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockEmulatorContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(MemoryRegionsModel_Tests)
{
private:
    class MemoryRegionsModelHarness : public MemoryRegionsModel
    {
    public:
        void AssertSerialize(const std::string& sExpected)
        {
            std::string sSerialized = "M0";
            ra::services::impl::StringTextWriter pTextWriter(sSerialized);
            Serialize(pTextWriter);

            Assert::AreEqual(sExpected, sSerialized);
        }

        void AssertRegion(ra::ByteAddress nStartAddress, ra::ByteAddress nEndAddress, const std::wstring& sLabel)
        {
            for (const auto& pRegion : CustomRegions())
            {
                if (pRegion.nStartAddress == nStartAddress && pRegion.nEndAddress == nEndAddress)
                {
                    Assert::AreEqual(sLabel, pRegion.sLabel);
                    return;
                }
            }

            Assert::Fail(ra::StringPrintf(L"Did not find region for %04x-%04x", nStartAddress, nEndAddress).c_str());
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryRegionsModelHarness regions;

        Assert::AreEqual(AssetType::MemoryRegions, regions.GetType());
        Assert::AreEqual(0U, regions.GetID());
        Assert::AreEqual(std::wstring(L"Memory Regions"), regions.GetName());
        Assert::AreEqual(std::wstring(L""), regions.GetDescription());
        Assert::AreEqual(AssetCategory::Core, regions.GetCategory());
        Assert::AreEqual(AssetState::Inactive, regions.GetState());
        Assert::AreEqual(AssetChanges::None, regions.GetChanges());
    }

    TEST_METHOD(TestSerialize)
    {
        MemoryRegionsModelHarness regions;
        Assert::AreEqual(AssetChanges::None, regions.GetChanges());

        regions.ResetCustomRegions();
        regions.AddCustomRegion(0x1234, 0x2345, L"My region");
        Assert::AreEqual(AssetChanges::Unpublished, regions.GetChanges());
        regions.AssertSerialize("M0:0x1234-0x2345:\"My region\"");
    }

    TEST_METHOD(TestSerializeEscaped)
    {
        MemoryRegionsModelHarness regions;
        Assert::AreEqual(AssetChanges::None, regions.GetChanges());

        regions.ResetCustomRegions();
        regions.AddCustomRegion(0x1234, 0x2345, L"My \"region\"");
        Assert::AreEqual(AssetChanges::Unpublished, regions.GetChanges());
        regions.AssertSerialize("M0:0x1234-0x2345:\"My \\\"region\\\"\"");
    }

    TEST_METHOD(TestDeserialize)
    {
        MemoryRegionsModelHarness regions;
        Assert::AreEqual(AssetChanges::None, regions.GetChanges());

        const std::string sSerialized = ":0x1234-0x2345:\"My region\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(regions.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::None, regions.GetChanges());
        regions.AssertRegion(0x1234, 0x2345, L"My region");
    }

    TEST_METHOD(TestDeserializeEscaped)
    {
        MemoryRegionsModelHarness regions;
        Assert::AreEqual(AssetChanges::None, regions.GetChanges());

        const std::string sSerialized = ":0x1234-0x2345:\"My \\\"region\\\"\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(regions.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::None, regions.GetChanges());
        regions.AssertRegion(0x1234, 0x2345, L"My \"region\"");
    }

    TEST_METHOD(TestParseFilterRangeEmpty)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0 }, nStartAddress);
        Assert::AreEqual({ 0xFFFF }, nEndAddress); // total memory size - 1
    }

    TEST_METHOD(TestParseFilterRangeHexPrefix)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"0xbeef-0xfeed", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBEEF }, nStartAddress);
        Assert::AreEqual({ 0xFEED }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeDollarPrefix)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"$beef-$feed", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBEEF }, nStartAddress);
        Assert::AreEqual({ 0xFEED }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNoPrefix)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"1234-face", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x1234 }, nStartAddress);
        Assert::AreEqual({ 0xFACE }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeWhitespace)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"0xbeef - 0xfeed", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBEEF }, nStartAddress);
        Assert::AreEqual({ 0xFEED }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNoHyphen)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"8765", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x8765 }, nStartAddress);
        Assert::AreEqual({ 0x8765 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNoStart)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(MemoryRegionsModel::ParseFilterRange(L"-8765", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0 }, nStartAddress);
        Assert::AreEqual({ 0 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNoEnd)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(MemoryRegionsModel::ParseFilterRange(L"8765-", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x8765 }, nStartAddress);
        Assert::AreEqual({ 0 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeNotHex)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(MemoryRegionsModel::ParseFilterRange(L"banana", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xBA }, nStartAddress);
        Assert::AreEqual({ 0 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeEndOutOfRange)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"0xF000-0x10000", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xF000 }, nStartAddress);
        Assert::AreEqual({ 0xFFFF }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeStartOutOfRange)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(MemoryRegionsModel::ParseFilterRange(L"0x10000-0x1FFFF", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x10000 }, nStartAddress);
        Assert::AreEqual({ 0x1FFFF }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeStartAfterEnd)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockTotalMemorySizeChanged(0x10000);

        ra::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"0x9876-0x1234", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x1234 }, nStartAddress);
        Assert::AreEqual({ 0x9876 }, nEndAddress);
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
