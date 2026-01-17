#include "CppUnitTest.h"

#include "ui\viewmodels\MemoryWatchListViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\ui\UIAsserts.hh"

#include "tests\devkit\context\mocks\MockConsoleContext.hh"
#include "tests\devkit\context\mocks\MockEmulatorMemoryContext.hh"
#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockFileSystem.hh"
#include "tests\devkit\testutil\MemoryAsserts.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockFrameEventQueue.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(MemoryWatchListViewModel_Tests)
{
private:
    class MemoryWatchListViewModelHarness : public MemoryWatchListViewModel
    {
    public:
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorContext;
        ra::context::mocks::MockRcClient mockRcClient;
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        GSL_SUPPRESS_F6 MemoryWatchListViewModelHarness()
        {
            InitializeNotifyTargets();
        }

        void AddItem(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize = ra::data::Memory::Size::EightBit, ra::data::Memory::Format nFormat = ra::data::Memory::Format::Hex)
        {
            auto& pItem = Items().Add();
            pItem.BeginInitialization();
            pItem.SetAddress(nAddress);
            pItem.SetSize(nSize);
            pItem.SetFormat(nFormat);
            pItem.UpdateRealNote();
            pItem.EndInitialization();
        }

        void AddItem(const std::string& sIndirectAddress, ra::data::Memory::Format nFormat = ra::data::Memory::Format::Hex)
        {
            auto& pItem = Items().Add();
            pItem.BeginInitialization();
            pItem.SetIndirectAddress(sIndirectAddress);
            pItem.SetFormat(nFormat);
            pItem.UpdateRealNote();
            pItem.EndInitialization();
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryWatchListViewModelHarness watchList;

        Assert::AreEqual({0U}, watchList.Items().Count());

        Assert::AreEqual({ 17U }, watchList.Sizes().Count());
        Assert::AreEqual((int)ra::data::Memory::Size::EightBit, watchList.Sizes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L" 8-bit"), watchList.Sizes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::SixteenBit, watchList.Sizes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit"), watchList.Sizes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::TwentyFourBit, watchList.Sizes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"24-bit"), watchList.Sizes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::ThirtyTwoBit, watchList.Sizes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit"), watchList.Sizes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::SixteenBitBigEndian, watchList.Sizes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit BE"), watchList.Sizes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::TwentyFourBitBigEndian, watchList.Sizes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"24-bit BE"), watchList.Sizes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::ThirtyTwoBitBigEndian, watchList.Sizes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit BE"), watchList.Sizes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::BitCount, watchList.Sizes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"BitCount"), watchList.Sizes().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::NibbleLower, watchList.Sizes().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"Lower4"), watchList.Sizes().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::NibbleUpper, watchList.Sizes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"Upper4"), watchList.Sizes().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::Float, watchList.Sizes().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"Float"), watchList.Sizes().GetItemAt(10)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::FloatBigEndian, watchList.Sizes().GetItemAt(11)->GetId());
        Assert::AreEqual(std::wstring(L"Float BE"), watchList.Sizes().GetItemAt(11)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::Double32, watchList.Sizes().GetItemAt(12)->GetId());
        Assert::AreEqual(std::wstring(L"Double32"), watchList.Sizes().GetItemAt(12)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::Double32BigEndian, watchList.Sizes().GetItemAt(13)->GetId());
        Assert::AreEqual(std::wstring(L"Double32 BE"), watchList.Sizes().GetItemAt(13)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::MBF32, watchList.Sizes().GetItemAt(14)->GetId());
        Assert::AreEqual(std::wstring(L"MBF32"), watchList.Sizes().GetItemAt(14)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::MBF32LE, watchList.Sizes().GetItemAt(15)->GetId());
        Assert::AreEqual(std::wstring(L"MBF32 LE"), watchList.Sizes().GetItemAt(15)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Size::Text, watchList.Sizes().GetItemAt(16)->GetId());
        Assert::AreEqual(std::wstring(L"ASCII"), watchList.Sizes().GetItemAt(16)->GetLabel());

        Assert::AreEqual({ 2U }, watchList.Formats().Count());
        Assert::AreEqual((int)ra::data::Memory::Format::Hex, watchList.Formats().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Hex"), watchList.Formats().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ra::data::Memory::Format::Dec, watchList.Formats().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Dec"), watchList.Formats().GetItemAt(1)->GetLabel());
    }
    
    TEST_METHOD(TestCodeNoteChanged)
    {
        MemoryWatchListViewModelHarness watchList;
        watchList.mockGameContext.SetGameId(3U);
        watchList.mockGameContext.SetCodeNote(1234U, L"Note description");
        watchList.AddItem(1234U);

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        const auto* pItem = watchList.Items().GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"Note description"), pItem->GetRealNote());
        Assert::AreEqual(std::wstring(L"Note description"), pItem->GetDescription());
        Assert::AreEqual(1234U, pItem->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pItem->GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem->GetFormat());
        Assert::IsFalse(pItem->IsCustomDescription());

        watchList.mockGameContext.SetCodeNote(1234U, L"New description");

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        pItem = watchList.Items().GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"New description"), pItem->GetRealNote());
        Assert::AreEqual(std::wstring(L"New description"), pItem->GetDescription());
        Assert::AreEqual(1234U, pItem->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pItem->GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem->GetFormat());
        Assert::IsFalse(pItem->IsCustomDescription());
    }

    TEST_METHOD(TestCodeNoteChangedCustomDescription)
    {
        MemoryWatchListViewModelHarness watchList;
        watchList.mockGameContext.SetGameId(3U);
        watchList.mockGameContext.SetCodeNote(1234U, L"Note description");
        watchList.AddItem(1234U);
        watchList.Items().GetItemAt(0)->SetDescription(L"My Description");

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        const auto* pItem = watchList.Items().GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"Note description"), pItem->GetRealNote());
        Assert::AreEqual(std::wstring(L"My Description"), pItem->GetDescription());
        Assert::AreEqual(1234U, pItem->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pItem->GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem->GetFormat());
        Assert::IsTrue(pItem->IsCustomDescription());

        watchList.mockGameContext.SetCodeNote(1234U, L"New description");

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        pItem = watchList.Items().GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"New description"), pItem->GetRealNote());
        Assert::AreEqual(std::wstring(L"My Description"), pItem->GetDescription());
        Assert::AreEqual(1234U, pItem->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pItem->GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem->GetFormat());
        Assert::IsTrue(pItem->IsCustomDescription());

        watchList.mockGameContext.SetCodeNote(1234U, L"My Description");

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        pItem = watchList.Items().GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"My Description"), pItem->GetRealNote());
        Assert::AreEqual(std::wstring(L"My Description"), pItem->GetDescription());
        Assert::AreEqual(1234U, pItem->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pItem->GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem->GetFormat());
        Assert::IsFalse(pItem->IsCustomDescription());

        watchList.mockGameContext.SetCodeNote(1234U, L"New description");

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        pItem = watchList.Items().GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"New description"), pItem->GetRealNote());
        Assert::AreEqual(std::wstring(L"New description"), pItem->GetDescription());
        Assert::AreEqual(1234U, pItem->GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::EightBit, pItem->GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem->GetFormat());
        Assert::IsFalse(pItem->IsCustomDescription());
    }

    TEST_METHOD(TestDoFrame)
    {
        MemoryWatchListViewModelHarness watchList;
        watchList.mockGameContext.SetGameId(3U);

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.AddItem(18U);
        Assert::AreEqual({ 1U }, watchList.Items().Count());

        auto& pItem = *watchList.Items().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"12"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), pItem.GetPreviousValue());
        Assert::AreEqual(0U, pItem.GetChanges());

        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"12"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), pItem.GetPreviousValue());
        Assert::AreEqual(0U, pItem.GetChanges());

        memory.at(18) = 12;
        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"0c"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"12"), pItem.GetPreviousValue());
        Assert::AreEqual(1U, pItem.GetChanges());

        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"0c"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"12"), pItem.GetPreviousValue());
        Assert::AreEqual(1U, pItem.GetChanges());

        memory.at(18) = 15;
        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"0f"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0c"), pItem.GetPreviousValue());
        Assert::AreEqual(2U, pItem.GetChanges());
    }

    TEST_METHOD(TestOnEditMemory)
    {
        MemoryWatchListViewModelHarness watchList;
        watchList.mockGameContext.SetGameId(3U);

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.mockGameContext.NotifyActiveGameChanged();
        watchList.AddItem(1, ra::data::Memory::Size::ThirtyTwoBit);
        watchList.AddItem(2, ra::data::Memory::Size::TwentyFourBit);
        watchList.AddItem(3, ra::data::Memory::Size::SixteenBit);
        watchList.AddItem(4, ra::data::Memory::Size::EightBit);
        watchList.AddItem(4, ra::data::Memory::Size::SixteenBit);
        watchList.AddItem(5, ra::data::Memory::Size::EightBit);

        auto& pItem1 = *watchList.Items().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"04030201"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00000000"), pItem1.GetPreviousValue());
        Assert::AreEqual(0U, pItem1.GetChanges());

        auto& pItem2 = *watchList.Items().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"040302"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"000000"), pItem2.GetPreviousValue());
        Assert::AreEqual(0U, pItem2.GetChanges());

        auto& pItem3 = *watchList.Items().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0403"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), pItem3.GetPreviousValue());
        Assert::AreEqual(0U, pItem3.GetChanges());

        auto& pItem4a = *watchList.Items().GetItemAt(3);
        Assert::AreEqual(std::wstring(L"04"), pItem4a.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), pItem4a.GetPreviousValue());
        Assert::AreEqual(0U, pItem4a.GetChanges());

        auto& pItem4b = *watchList.Items().GetItemAt(4);
        Assert::AreEqual(std::wstring(L"0504"), pItem4b.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), pItem4b.GetPreviousValue());
        Assert::AreEqual(0U, pItem4b.GetChanges());

        auto& pItem5 = *watchList.Items().GetItemAt(5);
        Assert::AreEqual(std::wstring(L"05"), pItem5.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), pItem5.GetPreviousValue());
        Assert::AreEqual(0U, pItem5.GetChanges());

        //memory.at(4) = 0x40;
        watchList.mockEmulatorContext.WriteMemoryByte(4U, 0x40);

        Assert::AreEqual(std::wstring(L"40030201"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"04030201"), pItem1.GetPreviousValue());
        Assert::AreEqual(1U, pItem1.GetChanges());

        Assert::AreEqual(std::wstring(L"400302"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"040302"), pItem2.GetPreviousValue());
        Assert::AreEqual(1U, pItem2.GetChanges());

        Assert::AreEqual(std::wstring(L"4003"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0403"), pItem3.GetPreviousValue());
        Assert::AreEqual(1U, pItem3.GetChanges());

        Assert::AreEqual(std::wstring(L"40"), pItem4a.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"04"), pItem4a.GetPreviousValue());
        Assert::AreEqual(1U, pItem4a.GetChanges());

        Assert::AreEqual(std::wstring(L"0540"), pItem4b.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), pItem4b.GetPreviousValue());
        Assert::AreEqual(1U, pItem4b.GetChanges());

        Assert::AreEqual(std::wstring(L"05"), pItem5.GetCurrentValue()); // not affected by the edited memory, should not update
        Assert::AreEqual(std::wstring(L"00"), pItem5.GetPreviousValue());
        Assert::AreEqual(0U, pItem5.GetChanges());
    }

    TEST_METHOD(TestUpdateCurrentValueOnSizeChange)
    {
        MemoryWatchListViewModelHarness watchList;
        watchList.AddItem(4U, ra::data::Memory::Size::SixteenBit);
        auto& pItem1 = *watchList.Items().GetItemAt(0);

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 0; i < memory.size(); ++i)
            memory.at(i) = i;
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.DoFrame(); // initialize current and previous values

        Assert::AreEqual(std::wstring(L"0504"), pItem1.GetCurrentValue());
        pItem1.SetChanges(0U);

        memory.at(5) = 0x17;
        watchList.DoFrame();

        Assert::AreEqual(std::wstring(L"1704"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), pItem1.GetPreviousValue());
        Assert::AreEqual(1U, pItem1.GetChanges());

        pItem1.SetSize(ra::data::Memory::Size::EightBit);
        Assert::AreEqual(std::wstring(L"04"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), pItem1.GetPreviousValue());
        Assert::AreEqual(1U, pItem1.GetChanges());

        pItem1.SetSize(ra::data::Memory::Size::ThirtyTwoBit);
        Assert::AreEqual(std::wstring(L"07061704"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0504"), pItem1.GetPreviousValue());
        Assert::AreEqual(1U, pItem1.GetChanges());
    }

    TEST_METHOD(TestReadOnly)
    {
        MemoryWatchListViewModelHarness watchList;

        std::array<uint8_t, 64> memory = {};
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.AddItem(1U, ra::data::Memory::Size::EightBit);

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        auto& pItem = *watchList.Items().GetItemAt(0);
        Assert::IsFalse(pItem.IsReadOnly());

        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(watchList.Sizes().Count()); ++nIndex)
        {
            const auto nSize = ra::itoe<ra::data::Memory::Size>(watchList.Sizes().GetItemAt(nIndex)->GetId());
            pItem.SetSize(nSize);

            switch (nSize)
            {
                case ra::data::Memory::Size::BitCount:
                case ra::data::Memory::Size::Text:
                    Assert::IsTrue(pItem.IsReadOnly());
                    break;

                default:
                    Assert::IsFalse(pItem.IsReadOnly());
                    break;
            }
        }
    }

    TEST_METHOD(TestAddItemFloat)
    {
        MemoryWatchListViewModelHarness watchList;

        std::array<uint8_t, 64> memory = {};
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.AddItem(1U, ra::data::Memory::Size::Float);

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        const auto& pItem = *watchList.Items().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), pItem.GetRealNote());
        Assert::AreEqual(1U, pItem.GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::Float, pItem.GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem.GetFormat());
        Assert::AreEqual(std::wstring(L"0.0"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem.GetPreviousValue());
        Assert::AreEqual(0U, pItem.GetChanges());

        watchList.mockEmulatorContext.WriteMemoryByte(4U, 0xC0);
        Assert::AreEqual(std::wstring(L"-2.0"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem.GetPreviousValue());
        Assert::AreEqual(1U, pItem.GetChanges());

        memory.at(4) = 0x42; // 0x429A4492 => 77.133926
        memory.at(3) = 0x9A;
        memory.at(2) = 0x44;
        memory.at(1) = 0x92;

        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"77.133926"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"-2.0"), pItem.GetPreviousValue());
        Assert::AreEqual(2U, pItem.GetChanges());
    }

    TEST_METHOD(TestAddItemDouble32)
    {
        MemoryWatchListViewModelHarness watchList;

        std::array<uint8_t, 64> memory = {};
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.AddItem(4U, ra::data::Memory::Size::Double32);

        // without a code note, assume the user is pIteming the 4 significant bytes
        Assert::AreEqual({1U}, watchList.Items().Count());
        const auto& pItem = *watchList.Items().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), pItem.GetRealNote());
        Assert::AreEqual(4U, pItem.GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::Double32, pItem.GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem.GetFormat()); // default to user preference
        Assert::AreEqual(std::wstring(L"0.0"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem.GetPreviousValue());
        Assert::AreEqual(0U, pItem.GetChanges());

        watchList.mockEmulatorContext.WriteMemoryByte(7U, 0xC0);
        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"-2.0"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem.GetPreviousValue());
        Assert::AreEqual(1U, pItem.GetChanges());

        // with a code note, align the pItem to the 4 significant bytes
        watchList.mockGameContext.SetCodeNote(8U, L"[double] Note description");

        watchList.AddItem(8U, ra::data::Memory::Size::Double32);
        Assert::AreEqual({2U}, watchList.Items().Count());
        const auto& pItem2 = *watchList.Items().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"[double] Note description"), pItem2.GetRealNote());
        Assert::AreEqual(12U, pItem2.GetAddress()); // adjusted to significant bytes
        Assert::AreEqual(ra::data::Memory::Size::Double32, pItem2.GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem2.GetFormat());
        Assert::AreEqual(std::wstring(L"0.0"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem2.GetPreviousValue());
        Assert::AreEqual(0U, pItem2.GetChanges());

        watchList.mockEmulatorContext.WriteMemoryByte(15U, 0xC0);
        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"-2.0"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem2.GetPreviousValue());
        Assert::AreEqual(1U, pItem2.GetChanges());

        // does not exactly match code note address, assume the user is pIteming the most significant bytes
        watchList.AddItem(12U, ra::data::Memory::Size::Double32);
        Assert::AreEqual({3U}, watchList.Items().Count());
        const auto& pItem3 = *watchList.Items().GetItemAt(2);
        Assert::AreEqual(std::wstring(L""), pItem3.GetRealNote());
        Assert::AreEqual(12U, pItem3.GetAddress()); // adjusted to significant bytes
        Assert::AreEqual(ra::data::Memory::Size::Double32, pItem3.GetSize());
        Assert::AreEqual(ra::data::Memory::Format::Hex, pItem3.GetFormat()); // default to preference
        Assert::AreEqual(std::wstring(L"-2.0"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem3.GetPreviousValue());
        Assert::AreEqual(0U, pItem3.GetChanges());
    }

    TEST_METHOD(TestAddItemASCII)
    {
        MemoryWatchListViewModelHarness watchList;

        std::array<uint8_t, 64> memory = {};
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.AddItem(1U, ra::data::Memory::Size::Text);

        Assert::AreEqual({ 1U }, watchList.Items().Count());
        const auto& pItem = *watchList.Items().GetItemAt(0);
        Assert::AreEqual(std::wstring(L""), pItem.GetRealNote());
        Assert::AreEqual(1U, pItem.GetAddress());
        Assert::AreEqual(ra::data::Memory::Size::Text, pItem.GetSize());
        Assert::AreEqual(std::wstring(L""), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L""), pItem.GetPreviousValue());
        Assert::AreEqual(0U, pItem.GetChanges());

        memcpy(&memory.at(1), "Test", 5);
        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"Test"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L""), pItem.GetPreviousValue());
        Assert::AreEqual(1U, pItem.GetChanges());

        watchList.mockEmulatorContext.WriteMemoryByte(3U, 0x78);
        Assert::AreEqual(std::wstring(L"Text"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"Test"), pItem.GetPreviousValue());
        Assert::AreEqual(2U, pItem.GetChanges());

        memcpy(&memory.at(1), "EightCharacters", 15);
        watchList.DoFrame();
        Assert::AreEqual(std::wstring(L"EightCha"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"Text"), pItem.GetPreviousValue());
        Assert::AreEqual(3U, pItem.GetChanges());

        watchList.mockEmulatorContext.WriteMemoryByte(9U, 0x73);
        Assert::AreEqual(std::wstring(L"EightCha"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"Text"), pItem.GetPreviousValue());
        Assert::AreEqual(3U, pItem.GetChanges());

        watchList.mockEmulatorContext.WriteMemoryByte(8U, 0x73);
        Assert::AreEqual(std::wstring(L"EightChs"), pItem.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"EightCha"), pItem.GetPreviousValue());
        Assert::AreEqual(4U, pItem.GetChanges());
    }

    TEST_METHOD(TestSetCurrentValue)
    {
        MemoryWatchListViewModelHarness watchList;

        std::array<uint8_t, 64> memory = {};
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.AddItem(1U, ra::data::Memory::Size::EightBit);
        watchList.AddItem(1U, ra::data::Memory::Size::SixteenBit);
        watchList.AddItem(1U, ra::data::Memory::Size::Bit3);

        Assert::AreEqual({ 3U }, watchList.Items().Count());
        auto& pItem1 = *watchList.Items().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"00"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), pItem1.GetPreviousValue());
        auto& pItem2 = *watchList.Items().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"0000"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), pItem2.GetPreviousValue());
        auto& pItem3 = *watchList.Items().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), pItem3.GetPreviousValue());

        std::wstring sError;
        Assert::IsTrue(pItem1.SetCurrentValue(L"1C", sError));
        Assert::AreEqual(std::wstring(L"1c"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"00"), pItem1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"001c"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0000"), pItem2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"1"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), pItem3.GetPreviousValue());

        Assert::IsTrue(pItem2.SetCurrentValue(L"1234", sError));
        Assert::AreEqual(std::wstring(L"34"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"1c"), pItem1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"1234"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"001c"), pItem2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"0"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"1"), pItem3.GetPreviousValue());

        Assert::IsTrue(pItem3.SetCurrentValue(L"1", sError));
        Assert::AreEqual(std::wstring(L"3c"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"34"), pItem1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"123c"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"1234"), pItem2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"1"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0"), pItem3.GetPreviousValue());
    }

    TEST_METHOD(TestSetCurrentValueFloat)
    {
        MemoryWatchListViewModelHarness watchList;

        std::array<uint8_t, 64> memory = {};
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.AddItem(4U, ra::data::Memory::Size::Float);
        watchList.AddItem(8U, ra::data::Memory::Size::MBF32);
        watchList.AddItem(12U, ra::data::Memory::Size::Double32);

        Assert::AreEqual({ 3U }, watchList.Items().Count());
        auto& pItem1 = *watchList.Items().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"0.0"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem1.GetPreviousValue());
        auto& pItem2 = *watchList.Items().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"0.0"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem2.GetPreviousValue());
        auto& pItem3 = *watchList.Items().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0.0"), pItem3.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem3.GetPreviousValue());

        std::wstring sError;
        Assert::IsTrue(pItem1.SetCurrentValue(L"-2", sError));
        Assert::IsTrue(pItem2.SetCurrentValue(L"-3", sError));
        Assert::IsTrue(pItem3.SetCurrentValue(L"68.1234", sError));
        Assert::AreEqual(std::wstring(L"-2.0"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"-3.0"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"0.0"), pItem2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"68.123352"), pItem3.GetCurrentValue()); // conversion through double is inprecise
        Assert::AreEqual(std::wstring(L"0.0"), pItem3.GetPreviousValue());

        Assert::AreEqual({ 0xC0 }, memory.at(7)); // -2 => 0xC0000000

        Assert::AreEqual({ 0x82 }, memory.at(8)); // -3 => 0x82C00000
        Assert::AreEqual({ 0xC0 }, memory.at(9));

        Assert::AreEqual({ 0x40 }, memory.at(15)); // 68.123 => 0x405107E5
        Assert::AreEqual({ 0x51 }, memory.at(14));
        Assert::AreEqual({ 0x07 }, memory.at(13));
        Assert::AreEqual({ 0xE5 }, memory.at(12));

        Assert::IsTrue(pItem1.SetCurrentValue(L"3.14", sError));
        Assert::IsTrue(pItem2.SetCurrentValue(L"6.022", sError));
        Assert::IsTrue(pItem3.SetCurrentValue(L"-4.56", sError));
        Assert::AreEqual(std::wstring(L"3.14"), pItem1.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"-2.0"), pItem1.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"6.022"), pItem2.GetCurrentValue());
        Assert::AreEqual(std::wstring(L"-3.0"), pItem2.GetPreviousValue());
        Assert::AreEqual(std::wstring(L"-4.559998"), pItem3.GetCurrentValue()); // conversion through double is inprecise
        Assert::AreEqual(std::wstring(L"68.123352"), pItem3.GetPreviousValue());
    }

    TEST_METHOD(TestUpdateCurrentValue)
    {
        MemoryWatchListViewModelHarness watchList;
        std::array<uint8_t, 64> memory = {};
        memory.at(4) = 8;
        // 0x42883EFA => 68.123
        memory.at(0x10) = 0xFA;
        memory.at(0x11) = 0x3E;
        memory.at(0x12) = 0x88;
        memory.at(0x13) = 0x42;
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.mockGameContext.SetGameId(3U);

        watchList.AddItem(0x0010, ra::data::Memory::Size::ThirtyTwoBit);

        Assert::AreEqual({1U}, watchList.Items().Count());
        auto& pItem = *watchList.Items().GetItemAt(0);

        pItem.UpdateCurrentValue();
        Assert::AreEqual(std::wstring(L"42883efa"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::Float);
        Assert::AreEqual(std::wstring(L"68.123001"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::ThirtyTwoBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e8842"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::TwentyFourBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e88"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::SixteenBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::EightBit);
        Assert::AreEqual(std::wstring(L"fa"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::SixteenBit);
        Assert::AreEqual(std::wstring(L"3efa"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::TwentyFourBit);
        Assert::AreEqual(std::wstring(L"883efa"), pItem.GetCurrentValue());
    }

    TEST_METHOD(TestUpdateCurrentValueIndirect)
    {
        MemoryWatchListViewModelHarness watchList;
        std::array<uint8_t, 64> memory = {};
        memory.at(4) = 8;
        // 0x42883EFA => 68.123
        memory.at(0x10) = 0xFA;
        memory.at(0x11) = 0x3E;
        memory.at(0x12) = 0x88;
        memory.at(0x13) = 0x42;
        watchList.mockEmulatorContext.MockMemory(memory);

        watchList.mockGameContext.SetGameId(3U);

        watchList.AddItem("I:0xX0004_M:0xX0008"); // 4=8 $(8+8)= $16

        Assert::AreEqual({1U}, watchList.Items().Count());
        auto& pItem = *watchList.Items().GetItemAt(0);

        pItem.UpdateCurrentValue();
        Assert::AreEqual(std::wstring(L"42883efa"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::Float);
        Assert::AreEqual(std::wstring(L"68.123001"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::ThirtyTwoBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e8842"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::TwentyFourBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e88"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::SixteenBitBigEndian);
        Assert::AreEqual(std::wstring(L"fa3e"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::EightBit);
        Assert::AreEqual(std::wstring(L"fa"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::SixteenBit);
        Assert::AreEqual(std::wstring(L"3efa"), pItem.GetCurrentValue());

        pItem.SetSize(ra::data::Memory::Size::TwentyFourBit);
        Assert::AreEqual(std::wstring(L"883efa"), pItem.GetCurrentValue());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
