#include "data/models/MemoryRegionsModel.hh"

#include "services/impl/StringTextWriter.hh"

#include "tests/devkit/context/mocks/MockEmulatorMemoryContext.hh"

#include "testutil/AssetAsserts.hh"
#include "testutil/CppUnitTest.hh"

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
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;

        void AssertSerialize(const std::string& sExpected)
        {
            std::string sSerialized = "M0";
            ra::services::impl::StringTextWriter pTextWriter(sSerialized);
            Serialize(pTextWriter);

            Assert::AreEqual(sExpected, sSerialized);
        }

        void AssertRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, const std::wstring& sLabel)
        {
            for (const auto& pRegion : CustomRegions())
            {
                if (pRegion.GetStartAddress() == nStartAddress && pRegion.GetEndAddress() == nEndAddress)
                {
                    Assert::AreEqual(sLabel, pRegion.GetDescription());
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
        Assert::AreEqual(AssetChanges::Unpublished, regions.GetChanges());
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
        Assert::AreEqual(AssetChanges::Unpublished, regions.GetChanges());
        regions.AssertRegion(0x1234, 0x2345, L"My \"region\"");
    }

    TEST_METHOD(TestParseFilterRangeEmpty)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockEmulatorMemoryContext.MockTotalMemorySizeChanged(0x10000);

        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0 }, nStartAddress);
        Assert::AreEqual({ 0xFFFF }, nEndAddress); // total memory size - 1
    }

    TEST_METHOD(TestParseFilterRangeNoHyphen)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockEmulatorMemoryContext.MockTotalMemorySizeChanged(0x10000);

        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"8765", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x8765 }, nStartAddress);
        Assert::AreEqual({ 0x8765 }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeEndOutOfRange)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockEmulatorMemoryContext.MockTotalMemorySizeChanged(0x10000);

        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"0xF000-0x10000", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0xF000 }, nStartAddress);
        Assert::AreEqual({ 0xFFFF }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeStartOutOfRange)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockEmulatorMemoryContext.MockTotalMemorySizeChanged(0x10000);

        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsFalse(MemoryRegionsModel::ParseFilterRange(L"0x10000-0x1FFFF", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x10000 }, nStartAddress);
        Assert::AreEqual({ 0x1FFFF }, nEndAddress);
    }

    TEST_METHOD(TestParseFilterRangeStartAfterEnd)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        mockEmulatorMemoryContext.MockTotalMemorySizeChanged(0x10000);

        ra::data::ByteAddress nStartAddress, nEndAddress;
        Assert::IsTrue(MemoryRegionsModel::ParseFilterRange(L"0x9876-0x1234", nStartAddress, nEndAddress));
        Assert::AreEqual({ 0x1234 }, nStartAddress);
        Assert::AreEqual({ 0x9876 }, nEndAddress);
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
