#include "CppUnitTest.h"

#include "ui\viewmodels\PointerFinderViewModel.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\devkit\context\mocks\MockEmulatorMemoryContext.hh"
#include "tests\devkit\services\mocks\MockFileSystem.hh"
#include "tests\mocks\MockConfiguration.hh"
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
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        GSL_SUPPRESS_F6 PointerFinderViewModelHarness() : PointerFinderViewModel()
        {

        }

        ~PointerFinderViewModelHarness()
        {
        }

        PointerFinderViewModelHarness(const PointerFinderViewModelHarness&) noexcept = delete;
        PointerFinderViewModelHarness& operator=(const PointerFinderViewModelHarness&) noexcept = delete;
        PointerFinderViewModelHarness(PointerFinderViewModelHarness&&) noexcept = delete;
        PointerFinderViewModelHarness& operator=(PointerFinderViewModelHarness&&) noexcept = delete;

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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
            Assert::IsNull(pState.CapturedMemory());

            pState.ToggleCapture();

            Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());

            Assert::IsFalse(pState.CanCapture());
            Assert::AreEqual(std::wstring(L"0x1234"), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Release"), pState.GetCaptureButtonText());
            Assert::IsNotNull(pState.CapturedMemory());

            pState.ToggleCapture();

            Assert::IsFalse(vmPointerFinder.mockDesktop.WasDialogShown());

            Assert::IsTrue(pState.CanCapture());
            Assert::AreEqual(std::wstring(L"0x1234"), pState.GetAddress());
            Assert::AreEqual(std::wstring(L"Capture"), pState.GetCaptureButtonText());
            Assert::IsNull(pState.CapturedMemory());
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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(1, L"0x0070", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(2, L"0x009c", L"+0x00", L"0020", L"0038", L"", L""); // 20+00=>20, 38+00=>38
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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x04", L"001c", L"001c", L"0034", L"0034"); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(1, L"0x00a4", L"+0x00", L"0020", L"0020", L"0038", L"0038"); // 20+00=>20, 38+00=>38
        Assert::AreEqual(std::wstring(L"2"), vmPointerFinder.GetResultCountText());
    }

    TEST_METHOD(TestFindOffsetNoMatches)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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

    TEST_METHOD(TestBookmarkSelected)
    {
        PointerFinderViewModelHarness vmPointerFinder;
        vmPointerFinder.mockGameContext.SetGameId(1U);
        vmPointerFinder.SetSearchType(ra::services::SearchType::SixteenBitAligned);

        // no results - bookmarks window will still be shown
        vmPointerFinder.BookmarkSelected();
        Assert::IsTrue(vmPointerFinder.mockWindowManager.MemoryBookmarks.IsVisible());
        vmPointerFinder.mockDesktop.ResetExpectedWindows();

        const auto& pBookmarks = vmPointerFinder.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 0U }, pBookmarks.Items().Count());

        // initialize results
        std::array<unsigned char, 256> pMemory{};
        pMemory.at(0x08) = 0x1c;
        pMemory.at(0x70) = 0x1c;
        pMemory.at(0x9c) = 0x20;
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(1, L"0x0070", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(2, L"0x009c", L"+0x00", L"0020", L"0038", L"", L""); // 20+00=>20, 38+00=>38

        // no selection
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 0U }, pBookmarks.Items().Count());

        // selection
        vmPointerFinder.PotentialPointers().GetItemAt(1)->SetSelected(true);
        vmPointerFinder.BookmarkSelected();
        Assert::AreEqual({ 1U }, pBookmarks.Items().Count());
        Assert::AreEqual({ 0x0070U }, pBookmarks.Items().GetItemAt(0)->GetAddress());
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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(1, L"0x0070", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(2, L"0x009c", L"+0x00", L"0020", L"0038", L"", L""); // 20+00=>20, 38+00=>38

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
        Assert::AreEqual(std::string("Address,Offset,State1,State2\n0x0008,+0x04,001c,0034\n0x0070,+0x04,001c,0034\n0x009c,+0x00,0020,0038\n"),
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
        vmPointerFinder.mockEmulatorMemoryContext.MockMemory(pMemory);

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
        vmPointerFinder.AssertRow(0, L"0x0008", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(1, L"0x0070", L"+0x04", L"001c", L"0034", L"", L""); // 1c+04=>20, 34+04=>38
        vmPointerFinder.AssertRow(2, L"0x009c", L"+0x00", L"0020", L"0038", L"", L""); // 20+00=>20, 38+00=>38

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
