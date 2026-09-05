#include "CppUnitTest.h"

#include "ui\viewmodels\PointerFinderViewModel.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\devkit\context\mocks\MockConsoleContext.hh"
#include "tests\devkit\context\mocks\MockEmulatorMemoryContext.hh"
#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockConfiguration.hh"
#include "tests\devkit\services\mocks\MockFileSystem.hh"

#include "tests\mocks\MockClipboard.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(PointerFinderViewModel_Tests)
{
private:
    class PointerFinderViewModelHarness : public PointerFinderViewModel
    {
    public:
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        GSL_SUPPRESS_F6 PointerFinderViewModelHarness() : PointerFinderViewModel()
        {
            SetValue(ShowProgressDialogProperty, false);
        }

        ~PointerFinderViewModelHarness()
        {
        }

        PointerFinderViewModelHarness(const PointerFinderViewModelHarness&) noexcept = delete;
        PointerFinderViewModelHarness& operator=(const PointerFinderViewModelHarness&) noexcept = delete;
        PointerFinderViewModelHarness(PointerFinderViewModelHarness&&) noexcept = delete;
        PointerFinderViewModelHarness& operator=(PointerFinderViewModelHarness&&) noexcept = delete;

        template <size_t N>
        void MockMemory(std::array<uint8_t, N>& pMemory)
        {
            mockEmulatorMemoryContext.MockMemory(pMemory);
            mockConsoleContext.AddMemoryRegion(0, gsl::narrow_cast<ra::data::ByteAddress>(pMemory.size()),
                ra::data::MemoryRegion::Type::SystemRAM);
        }

        void AssertRow(gsl::index nIndex, const std::wstring& sPointerAddress, const std::wstring& sOffset,
            const std::wstring& sPointerValue1, const std::wstring& sPointerValue2,
            const std::wstring& sPointerValue3, const std::wstring& sPointerValue4)
        {
            const auto* pPointer = PotentialPointers().GetItemAt(nIndex);
            Assert::IsNotNull(pPointer);
            Ensures(pPointer != nullptr);
            Assert::AreEqual(sPointerAddress, pPointer->GetPointerAddress());
            Assert::AreEqual(sOffset, pPointer->GetOffset());
            Assert::AreEqual(sPointerValue1, pPointer->GetPointerValue1());
            Assert::AreEqual(sPointerValue2, pPointer->GetPointerValue2());
            Assert::AreEqual(sPointerValue3, pPointer->GetPointerValue3());
            Assert::AreEqual(sPointerValue4, pPointer->GetPointerValue4());
        }

        using PointerFinderViewModel::AddPotentialPointer;
    };

public:
    TEST_METHOD(TestCaptureNoGame)
    {
        PointerFinderViewModelHarness vmPointerFinder;

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](MessageBoxViewModel& vmMessageBox) {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"No game loaded."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Cannot capture memory without a loaded game."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        for (auto& pState : vmPointerFinder.States())
        {
            bDialogSeen = false;

            Assert::IsTrue(pState.CanCapture());
            Assert::AreEqual(std::wstring(L""), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Capture"), pState.GetCaptureButtonText());

            pState.ToggleCapture();

            Assert::IsTrue(bDialogSeen);

            Assert::IsTrue(pState.CanCapture());
            Assert::AreEqual(std::wstring(L""), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Capture"), pState.GetCaptureButtonText());
        }
    }

    TEST_METHOD(TestCaptureNoAddress)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        std::array<unsigned char, 256> pMemory{};
        vmPointerFinder.MockMemory(pMemory);

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](MessageBoxViewModel& vmMessageBox) {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Invalid address."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        for (auto& pState : vmPointerFinder.States())
        {
            bDialogSeen = false;

            Assert::IsTrue(pState.CanCapture());
            Assert::AreEqual(std::wstring(L""), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Capture"), pState.GetCaptureButtonText());

            pState.ToggleCapture();
            Assert::IsTrue(bDialogSeen);
            Assert::IsTrue(pState.CanCapture());

            bDialogSeen = false;
            pState.SetAddress(L"banana");
            Assert::AreEqual(std::wstring(L"banana"), pState.GetAddress());

            pState.ToggleCapture();
            Assert::IsTrue(bDialogSeen);
            Assert::IsTrue(pState.CanCapture());

            bDialogSeen = false;
            pState.SetAddress(L"0xyz");
            Assert::AreEqual(std::wstring(L"0xyz"), pState.GetAddress());

            pState.ToggleCapture();
            Assert::IsTrue(bDialogSeen);
            Assert::IsTrue(pState.CanCapture());

            bDialogSeen = false;
            pState.SetAddress(L"0x00");
            Assert::AreEqual(std::wstring(L"0x00"), pState.GetAddress());

            pState.ToggleCapture();
            Assert::IsFalse(bDialogSeen);
            Assert::IsFalse(pState.CanCapture());
        }
    }

    TEST_METHOD(TestCaptureAndRelease)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.mockEmulatorMemoryContext.MockTotalMemorySizeChanged(0x10000);

        for (auto& pState : vmPointerFinder.States())
        {
            pState.SetAddress(L"0x1234");
            Assert::IsTrue(pState.Viewer().IsAddressFixed());
            Assert::AreEqual({ 0x1234U }, pState.Viewer().GetAddress());
            Assert::AreEqual({ 0x1230U }, pState.Viewer().GetFirstAddress());

            Assert::IsTrue(pState.CanCapture());
            Assert::AreEqual(std::wstring(L"0x1234"), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Capture"), pState.GetCaptureButtonText());

            pState.ToggleCapture();

            Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());

            Assert::IsFalse(pState.CanCapture());
            Assert::AreEqual(std::wstring(L"0x1234"), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Release"), pState.GetCaptureButtonText());

            pState.ToggleCapture();

            Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());

            Assert::IsTrue(pState.CanCapture());
            Assert::AreEqual(std::wstring(L"0x1234"), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Capture"), pState.GetCaptureButtonText());
        }
    }

    TEST_METHOD(TestFindNoCaptures)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](MessageBoxViewModel& vmMessageBox) {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Cannot find."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"At least two unique addresses must be captured before potential pointers can be located."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        vmPointerFinder.Find();

        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual({ 0U }, vmPointerFinder.PotentialPointers().Count());
    }

    TEST_METHOD(TestFindOneCapture)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        std::array<unsigned char, 256> pMemory{};
        vmPointerFinder.MockMemory(pMemory);

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](MessageBoxViewModel& vmMessageBox) {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Cannot find."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"At least two unique addresses must be captured before potential pointers can be located."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        vmPointerFinder.States().at(0).SetAddress(L"0x1234");
        vmPointerFinder.States().at(0).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual({ 0U }, vmPointerFinder.PotentialPointers().Count());
    }

    TEST_METHOD(TestFindTwoCapturesSameAddress)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        std::array<unsigned char, 256> pMemory{};
        vmPointerFinder.MockMemory(pMemory);

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](MessageBoxViewModel& vmMessageBox) {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Cannot find."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"At least two unique addresses must be captured before potential pointers can be located."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        vmPointerFinder.States().at(0).SetAddress(L"0x1234");
        vmPointerFinder.States().at(0).ToggleCapture();
        vmPointerFinder.States().at(1).SetAddress(L"0x1234");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsTrue(bDialogSeen);
        Assert::AreEqual({ 0U }, vmPointerFinder.PotentialPointers().Count());
    }

    TEST_METHOD(TestFindOffsetZero)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x1c) = 0x55;
        pMemory.at(0x1d) = 0x46;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x1c");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x34;
        pMemory.at(0x34) = 0x55;
        pMemory.at(0x35) = 0x46;

        vmPointerFinder.States().at(1).SetAddress(L"0x34");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 1U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x00", L"001c", L"0034", L"", L"");
        Assert::AreEqual(std::wstring(L"1"), vmPointerFinder.GetResultCountText());
    }

    TEST_METHOD(TestFindOffsetPositive)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x1c) = 0x55;
        pMemory.at(0x1d) = 0x46;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x20");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x34;
        pMemory.at(0x34) = 0x55;
        pMemory.at(0x35) = 0x46;

        vmPointerFinder.States().at(1).SetAddress(L"0x38");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 1U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        Assert::AreEqual(std::wstring(L"1"), vmPointerFinder.GetResultCountText());
    }
    
    TEST_METHOD(TestFindOffsetNegative)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x1c) = 0x55;
        pMemory.at(0x1d) = 0x46;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x18");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x34;
        pMemory.at(0x34) = 0x55;
        pMemory.at(0x35) = 0x46;

        vmPointerFinder.States().at(1).SetAddress(L"0x30");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 1U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0xFFFFFFFC", L"001c", L"0034", L"", L""); // 1c-04=>18, 34-04=>30
        Assert::AreEqual(std::wstring(L"1"), vmPointerFinder.GetResultCountText());
    }

    TEST_METHOD(TestFindOffsetMultiplePointers)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x1c) = 0x55;
        pMemory.at(0x1d) = 0x46;
        pMemory.at(0x70) = 0x1c;
        pMemory.at(0x9c) = 0x20;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x20");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x34;
        pMemory.at(0x34) = 0x55;
        pMemory.at(0x35) = 0x46;
        pMemory.at(0x70) = 0x34;
        pMemory.at(0x9c) = 0x38;

        vmPointerFinder.States().at(1).SetAddress(L"0x38");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 3U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x009c", L"+0x00", L"0020", L"0038", L"", L""); // 20+00=>20, 38+00=>38
        vmPointerFinder.AssertRow(1, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(2, L"0x0070", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        Assert::AreEqual(std::wstring(L"3"), vmPointerFinder.GetResultCountText());
    }
    
    TEST_METHOD(TestFindOffsetMultipleStates)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x70) = 0x1c;
        pMemory.at(0x9c) = 0x20;
        pMemory.at(0xa4) = 0x20;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x20");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x70) = 0x20; // pointer at $70 not valid
        vmPointerFinder.States().at(1).SetAddress(L"0x20");
        vmPointerFinder.States().at(1).ToggleCapture();

        pMemory.at(0x08) = 0x34; // all pointers valid
        pMemory.at(0x70) = 0x34;
        pMemory.at(0x9c) = 0x38;
        pMemory.at(0xa4) = 0x38;

        vmPointerFinder.States().at(2).SetAddress(L"0x38");
        vmPointerFinder.States().at(2).ToggleCapture();

        pMemory.at(0x9c) = 0x20; // pointer at $9c not valid
        vmPointerFinder.States().at(3).SetAddress(L"0x38");
        vmPointerFinder.States().at(3).ToggleCapture();

        vmPointerFinder.Find();
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 2U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x00a4", L"+0x00", L"0020", L"0020", L"0038", L"0038"); // 20+00=>20, 38+00=>38
        vmPointerFinder.AssertRow(1, L"0x0008", L"+0x04", L"001c", L"001c", L"0034", L"0034"); // 1c+04=>20, 34+04=>38
        Assert::AreEqual(std::wstring(L"2"), vmPointerFinder.GetResultCountText());
    }

    TEST_METHOD(TestFindOffsetNoMatches)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x18");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x38;

        vmPointerFinder.States().at(1).SetAddress(L"0x30");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 1U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"No pointers found.", L"", L"", L"", L"", L"");
        Assert::AreEqual(std::wstring(L"0"), vmPointerFinder.GetResultCountText());
    }

    TEST_METHOD(TestFindOffsetNested)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        // 0008 + 4 + 12
        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c; // $0008 = 001C +  4 => 0020
        pMemory.at(0x20) = 0x44; // $0020 = 0044 + 12 => 0x50
        pMemory.at(0x21) = 0x00;
        pMemory.at(0x50) = 0x55; // $0050
        pMemory.at(0x51) = 0x46;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x50");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x34; // $0008 = 0034 +  4 => 0038
        pMemory.at(0x38) = 0x68; // $0038 = 0068 + 12 => 0074
        pMemory.at(0x39) = 0x00;
        pMemory.at(0x74) = 0x55;
        pMemory.at(0x75) = 0x46;

        vmPointerFinder.States().at(1).SetAddress(L"0x74");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 2U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(1, L"", L"+0x0C", L"0044", L"0068", L"", L""); // 44+0C=>50, 68+0C=>74
        Assert::AreEqual(std::wstring(L"1"), vmPointerFinder.GetResultCountText());


    }

    TEST_METHOD(TestBookmarkSelected)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        ra::context::mocks::MockRcClient mockRcClient;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        // no results - bookmarks window will still be shown
        vmPointerFinder.BookmarkSelected();
        Assert::IsTrue(vmPointerFinder.mockWindowManager.MemoryBookmarks.IsVisible());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();

        const auto& pBookmarks = vmPointerFinder.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 0U }, pBookmarks.Items().Count());

        // initialize results
        ra::services::PointerFinder::PotentialPointer pPointer;
        pPointer.nRootAddress = 0x009c;
        pPointer.vOffsets.at(0) = 0;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0008;
        pPointer.vOffsets.at(0) = 4;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0020;
        pPointer.vOffsets.at(0) = 8;
        pPointer.vOffsets.at(1) = 0;
        pPointer.vOffsets.at(2) = 4;
        pPointer.nOffsetLength = 3;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0070;
        pPointer.vOffsets.at(0) = 4;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        // no selection
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 0U }, pBookmarks.Items().Count());

        // selection
        vmPointerFinder.PotentialPointers().GetItemAt(1)->SetSelected(true);
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 1U }, pBookmarks.Items().Count());
        Assert::AreEqual(std::string("I:0x 0008_M:0x 0004"), pBookmarks.Items().GetItemAt(0)->GetIndirectAddress());
        vmPointerFinder.PotentialPointers().GetItemAt(1)->SetSelected(false);

        // first part of chain
        vmPointerFinder.PotentialPointers().GetItemAt(2)->SetSelected(true);
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 2U }, pBookmarks.Items().Count());
        Assert::AreEqual(std::string("I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), pBookmarks.Items().GetItemAt(1)->GetIndirectAddress());
        vmPointerFinder.PotentialPointers().GetItemAt(2)->SetSelected(false);

        // middle part of chain
        vmPointerFinder.PotentialPointers().GetItemAt(3)->SetSelected(true);
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 3U }, pBookmarks.Items().Count());
        Assert::AreEqual(std::string("I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), pBookmarks.Items().GetItemAt(2)->GetIndirectAddress());
        vmPointerFinder.PotentialPointers().GetItemAt(3)->SetSelected(false);

        // end part of chain
        vmPointerFinder.PotentialPointers().GetItemAt(4)->SetSelected(true);
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 4U }, pBookmarks.Items().Count());
        Assert::AreEqual(std::string("I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), pBookmarks.Items().GetItemAt(3)->GetIndirectAddress());

        // entire chain
        vmPointerFinder.PotentialPointers().GetItemAt(3)->SetSelected(false);
        vmPointerFinder.PotentialPointers().GetItemAt(2)->SetSelected(false);
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 5U }, pBookmarks.Items().Count());
        Assert::AreEqual(std::string("I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), pBookmarks.Items().GetItemAt(4)->GetIndirectAddress());
    }

    TEST_METHOD(TestCopySelectedToClipboard)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        ra::services::mocks::MockClipboard mockClipboard;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        // no results - nothing copied, no dialog shown
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(), mockClipboard.GetText());
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();

        // initialize results
        ra::services::PointerFinder::PotentialPointer pPointer;
        pPointer.nRootAddress = 0x009c;
        pPointer.vOffsets.at(0) = 0;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0008;
        pPointer.vOffsets.at(0) = 4;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0020;
        pPointer.vOffsets.at(0) = 8;
        pPointer.vOffsets.at(1) = 0;
        pPointer.vOffsets.at(2) = 4;
        pPointer.nOffsetLength = 3;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0070;
        pPointer.vOffsets.at(0) = 4;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        // no selection
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(), mockClipboard.GetText());
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();

        // selection
        vmPointerFinder.PotentialPointers().GetItemAt(1)->SetSelected(true);
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(L"I:0x 0008_M:0x 0004"), mockClipboard.GetText());
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();
        vmPointerFinder.PotentialPointers().GetItemAt(1)->SetSelected(false);

        // first part of chain
        vmPointerFinder.PotentialPointers().GetItemAt(2)->SetSelected(true);
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(L"I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), mockClipboard.GetText());
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();
        vmPointerFinder.PotentialPointers().GetItemAt(2)->SetSelected(false);
        mockClipboard.SetText(L"");

        // middle part of chain
        vmPointerFinder.PotentialPointers().GetItemAt(3)->SetSelected(true);
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(L"I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), mockClipboard.GetText());
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();
        vmPointerFinder.PotentialPointers().GetItemAt(3)->SetSelected(false);
        mockClipboard.SetText(L"");

        // end part of chain
        vmPointerFinder.PotentialPointers().GetItemAt(4)->SetSelected(true);
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(L"I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), mockClipboard.GetText());
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();
        mockClipboard.SetText(L"");

        // entire chain
        vmPointerFinder.PotentialPointers().GetItemAt(3)->SetSelected(false);
        vmPointerFinder.PotentialPointers().GetItemAt(2)->SetSelected(false);
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(L"I:0x 0020_I:0x 0008_I:0x 0000_M:0x 0004"), mockClipboard.GetText());
        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();
    }

    TEST_METHOD(TestCopySelectedToClipboardMultipleSelections)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        ra::services::mocks::MockClipboard mockClipboard;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        // initialize results
        ra::services::PointerFinder::PotentialPointer pPointer;
        pPointer.nRootAddress = 0x009c;
        pPointer.vOffsets.at(0) = 0;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0008;
        pPointer.vOffsets.at(0) = 4;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0020;
        pPointer.vOffsets.at(0) = 8;
        pPointer.vOffsets.at(1) = 0;
        pPointer.vOffsets.at(2) = 4;
        pPointer.nOffsetLength = 3;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        pPointer.nRootAddress = 0x0070;
        pPointer.vOffsets.at(0) = 4;
        pPointer.nOffsetLength = 1;
        vmPointerFinder.AddPotentialPointer(pPointer, ra::data::Memory::Size::SixteenBit);

        // selection
        vmPointerFinder.PotentialPointers().GetItemAt(1)->SetSelected(true);
        vmPointerFinder.PotentialPointers().GetItemAt(4)->SetSelected(true);

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogSeen](const ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
                Assert::AreEqual(std::wstring(L"Multiple items selected"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"Only one item can be copied at a time."), vmMessageBox.GetMessage());

                bDialogSeen = true;
                return ra::ui::DialogResult::OK;
            }
        );
        vmPointerFinder.CopySelectedToClipboard();
        Assert::AreEqual(std::wstring(L""), mockClipboard.GetText());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestExportResults)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(3U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        // initialize results
        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x70) = 0x1c;
        pMemory.at(0x9c) = 0x20;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x20");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x34;
        pMemory.at(0x70) = 0x34;
        pMemory.at(0x9c) = 0x38;

        vmPointerFinder.States().at(1).SetAddress(L"0x38");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 3U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x009c", L"+0x00", L"0020", L"0038", L"", L""); // 20+00=>20, 38+00=>38
        vmPointerFinder.AssertRow(1, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(2, L"0x0070", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog) {
                bDialogSeen = true;

                Assert::AreEqual(std::wstring(L"Export Pointer Results"), vmFileDialog.GetWindowTitle());
                Assert::AreEqual({1U}, vmFileDialog.GetFileTypes().size());
                Assert::AreEqual(std::wstring(L"csv"), vmFileDialog.GetDefaultExtension());
                Assert::AreEqual(std::wstring(L"3-Pointers.csv"), vmFileDialog.GetFileName());

                vmFileDialog.SetFileName(L"E:\\Data\\3-Pointers.csv");

                return DialogResult::OK;
            });

        vmPointerFinder.ExportResults();

        Assert::IsTrue(bDialogSeen);
        const std::string& sContents = vmPointerFinder.mockFileSystem.GetFileContents(L"E:\\Data\\3-Pointers.csv");
        Assert::AreEqual(std::string("Address,Offset,State1,State2\n0x009c,+0x00,0020,0038\n0x0008,+0x04,001c,0034\n0x0070,+0x04,001c,0034\n"),
                         sContents);
    }

    TEST_METHOD(TestExportResultsCancel)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(3U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        // initialize results
        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x70) = 0x1c;
        pMemory.at(0x9c) = 0x20;
        vmPointerFinder.MockMemory(pMemory);

        vmPointerFinder.States().at(0).SetAddress(L"0x20");
        vmPointerFinder.States().at(0).ToggleCapture();

        pMemory.at(0x08) = 0x34;
        pMemory.at(0x70) = 0x34;
        pMemory.at(0x9c) = 0x38;

        vmPointerFinder.States().at(1).SetAddress(L"0x38");
        vmPointerFinder.States().at(1).ToggleCapture();
        vmPointerFinder.Find();

        Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 3U }, vmPointerFinder.PotentialPointers().Count());
        vmPointerFinder.AssertRow(0, L"0x009c", L"+0x00", L"0020", L"0038", L"", L""); // 20+00=>20, 38+00=>38
        vmPointerFinder.AssertRow(1, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(2, L"0x0070", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel& vmFileDialog) {
                bDialogSeen = true;

                Assert::AreEqual(std::wstring(L"Export Pointer Results"), vmFileDialog.GetWindowTitle());
                Assert::AreEqual({1U}, vmFileDialog.GetFileTypes().size());
                Assert::AreEqual(std::wstring(L"csv"), vmFileDialog.GetDefaultExtension());
                Assert::AreEqual(std::wstring(L"3-Pointers.csv"), vmFileDialog.GetFileName());

                vmFileDialog.SetFileName(L"E:\\Data\\3-Pointers.csv");

                return DialogResult::Cancel;
            });

        vmPointerFinder.ExportResults();

        Assert::IsTrue(bDialogSeen);
        const std::string& sContents = vmPointerFinder.mockFileSystem.GetFileContents(L"E:\\Data\\3-Pointers.csv");
        Assert::AreEqual(std::string(), sContents);
    }

    TEST_METHOD(TestExportResultsNone)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(3U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        bool bMessageSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
                bMessageSeen = true;
                Assert::AreEqual(std::wstring(L"Nothing to export"), vmMessageBox.GetMessage());
                return ra::ui::DialogResult::OK;
            });

        bool bDialogSeen = false;
        vmPointerFinder.mockDesktop.ExpectWindow<ra::ui::viewmodels::FileDialogViewModel>(
            [&bDialogSeen](ra::ui::viewmodels::FileDialogViewModel&) {
                bDialogSeen = true;
                return DialogResult::Cancel;
            });

        // search not started
        vmPointerFinder.ExportResults();

        Assert::IsTrue(bMessageSeen);
        Assert::IsFalse(bDialogSeen);
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
