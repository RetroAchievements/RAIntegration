#include "CppUnitTest.h"

#include "ui\viewmodels\MemoryViewerViewModel.hh"

#include "services\ServiceLocator.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockWindowManager.hh"

#undef GetMessage

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

constexpr unsigned char COLOR_RED = gsl::narrow_cast<unsigned char>(ra::etoi(MemoryViewerViewModel::TextColor::Selected));
constexpr unsigned char COLOR_BLACK = gsl::narrow_cast<unsigned char>(ra::etoi(MemoryViewerViewModel::TextColor::Default));
constexpr unsigned char COLOR_REDRAW = 0x80;

constexpr int CHAR_WIDTH = 8;
constexpr int CHAR_HEIGHT = 12;

TEST_CLASS(MemoryViewerViewModel_Tests)
{
private:
    class MemoryViewerViewModelHarness : public MemoryViewerViewModel
    {
    public:
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        GSL_SUPPRESS_F6 MemoryViewerViewModelHarness() : MemoryViewerViewModel()
        {
            InitializeNotifyTargets();

            s_szChar.Width = CHAR_WIDTH;
            s_szChar.Height = CHAR_HEIGHT;
        }

        ~MemoryViewerViewModelHarness()
        {
            // ensure we stop monitoring the MemoryBookmarksViewModel before the MockWindowManager is destroyed
            DetachNotifyTargets();
        }

        MemoryViewerViewModelHarness(const MemoryViewerViewModelHarness&) noexcept = delete;
        MemoryViewerViewModelHarness& operator=(const MemoryViewerViewModelHarness&) noexcept = delete;
        MemoryViewerViewModelHarness(MemoryViewerViewModelHarness&&) noexcept = delete;
        MemoryViewerViewModelHarness& operator=(MemoryViewerViewModelHarness&&) noexcept = delete;

        int GetSelectedNibble() const noexcept { return m_nSelectedNibble; }

        size_t GetTotalMemorySize() const noexcept { return m_nTotalMemorySize; }

        unsigned char GetByte(ra::ByteAddress nAddress) const
        {
            return m_pMemory[nAddress - GetFirstAddress()];
        }

        unsigned char GetColor(ra::ByteAddress nAddress) const
        {
            return gsl::narrow_cast<unsigned char>(ra::itoe<TextColor>(m_pColor[nAddress - GetFirstAddress()]));
        }

        bool IsReadOnly() const noexcept { return m_bReadOnly; }
        void SetReadOnly(bool value) noexcept { m_bReadOnly = value;  }

        bool IsAddressFixed() const noexcept { return m_bAddressFixed; }
        void SetAddressFixed(bool value) noexcept { m_bAddressFixed = value; }

        void InitializeMemory(size_t nSize)
        {
            unsigned char* pBytes = new unsigned char[nSize];
            m_pBytes.reset(pBytes);
            for (size_t i = 0; i < nSize; ++i)
                pBytes[i] = gsl::narrow_cast<unsigned char>(i & 0xFF);

            mockEmulatorContext.MockMemory(pBytes, nSize);

            DoFrame(); // populates m_pMemory and m_pColor
        }

        void MockRender() noexcept
        {
            for (size_t i = 0; i < m_nTotalMemorySize; ++i)
                m_pColor[i] &= 0x0F;

            m_nNeedsRedraw = 0;
        }

    private:
        std::unique_ptr<unsigned char[]> m_pBytes;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryViewerViewModelHarness viewer;

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetTotalMemorySize());
        Assert::AreEqual({ 8U }, viewer.GetNumVisibleLines());
        Assert::AreEqual(MemSize::EightBit, viewer.GetSize());
        Assert::IsTrue(viewer.NeedsRedraw());

        viewer.InitializeMemory(128);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 128U }, viewer.GetTotalMemorySize());
        Assert::AreEqual({ 8U }, viewer.GetNumVisibleLines());
        Assert::AreEqual(MemSize::EightBit, viewer.GetSize());
        Assert::IsTrue(viewer.NeedsRedraw());

        Assert::AreEqual({ 0U }, viewer.GetByte(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));

        for (int i = 1; i < 128; ++i)
        {
            Assert::AreEqual(gsl::narrow_cast<unsigned char>(i), viewer.GetByte(i));
            Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(i));
        }
    }

    TEST_METHOD(TestFirstAddressLimits)
    {
        MemoryViewerViewModelHarness viewer;

        viewer.InitializeMemory(1024);
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        viewer.SetFirstAddress(512);
        Assert::AreEqual({ 512U }, viewer.GetFirstAddress());

        viewer.SetFirstAddress(0xFFFFFFFF); // -1
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // 128 bytes visible in viewer, so maximum first address is 1024 - 128
        viewer.SetFirstAddress(0xFFFF);
        Assert::AreEqual({ 1024U - 128U }, viewer.GetFirstAddress());

        viewer.SetFirstAddress(0);
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        viewer.SetFirstAddress(1000);
        Assert::AreEqual({ 1024U - 128U }, viewer.GetFirstAddress());
    }

    TEST_METHOD(TestSetAddress)
    {
        MemoryViewerViewModelHarness viewer;

        viewer.InitializeMemory(1024);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual(COLOR_RED, viewer.GetColor(0U));
        Assert::AreEqual(COLOR_BLACK, viewer.GetColor(36U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // address in viewing window, first address should not change
        viewer.SetAddress(36U);
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 36U }, viewer.GetAddress());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(36U));
        for (ra::ByteAddress i = 1; i < 128; ++i)
        {
            if (i != 36)
                Assert::AreEqual(COLOR_BLACK, viewer.GetColor(i));
        }
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // address after viewing window, first address should change to center new address in viewing window
        viewer.SetAddress(444U);
        Assert::AreEqual({ 368U }, viewer.GetFirstAddress()); // (444 & ~0x0F) - 4 * 16
        Assert::AreEqual({ 444U }, viewer.GetAddress());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(444U));
        for (ra::ByteAddress i = 1; i < 128; ++i)
        {
            if (i != 444 - 368)
                Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(i + 368));
        }
        Assert::IsTrue(viewer.NeedsRedraw());

        // address before viewing window, first address should change to center new address in viewing window
        viewer.SetAddress(360U);
        Assert::AreEqual({ 288U }, viewer.GetFirstAddress()); // (360U & ~0x0F) - 4 * 16
        Assert::AreEqual({ 360U }, viewer.GetAddress());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(360U));
        for (ra::ByteAddress i = 1; i < 128; ++i)
        {
            if (i != 360 - 288)
                Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(i + 288));
        }
        Assert::IsTrue(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestSetAddressBeforeMemoryAvailable)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetAddress());

        // address not available, no memory registered
        viewer.SetAddress(444U);
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::IsFalse(viewer.NeedsRedraw());

        // memory becomes available, address should immediately update
        viewer.InitializeMemory(1024);
        Assert::AreEqual({ 444U }, viewer.GetAddress());
        Assert::AreEqual({ 368U }, viewer.GetFirstAddress()); // (444 & ~0x0F) - 4 * 16
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(444U));
        for (ra::ByteAddress i = 1; i < 128; ++i)
        {
            if (i != 444 - 368)
                Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(i + 368));
        }
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();
    }

    TEST_METHOD(TestSetAddressBeforeMemoryAvailableOutOfRange)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetAddress());

        // address not available, no memory registered
        viewer.SetAddress(4444U);
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::IsFalse(viewer.NeedsRedraw());

        // memory becomes available, address should immediately update (clamped at max address)
        viewer.InitializeMemory(1024);
        Assert::AreEqual({ 1023U }, viewer.GetAddress());
        Assert::AreEqual({ 896U }, viewer.GetFirstAddress()); // (1023 & ~0x0F) - 4 * 16
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1023U));
        for (ra::ByteAddress i = 1; i < 128; ++i)
        {
            if (i != 1023 - 896)
                Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(i + 896));
        }
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();
    }

    TEST_METHOD(TestAdvanceCursorEightBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 1U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 1U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // last visible byte
        viewer.SetAddress(127U);
        viewer.AdvanceCursor();
        Assert::AreEqual({ 127U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // beyond visible range, will scroll one line
        viewer.AdvanceCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());

        // jump to last address - cannot advance past last nibble
        viewer.SetAddress(255U);
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestRetreatCursorEightBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetAddress(130U);
        viewer.MockRender();

        Assert::AreEqual({ 130U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursor();
        Assert::AreEqual({ 129U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 129U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::IsTrue(viewer.NeedsRedraw());

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());

        // beyond visible range, will scroll one line
        viewer.RetreatCursor();
        Assert::AreEqual({ 127U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());

        // jump to first address - cannot retreat beyond first nibble
        viewer.SetAddress(0U);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestAdvanceCursorWordEightBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 1U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word jumps to start of next word
        viewer.AdvanceCursor();
        Assert::AreEqual({ 1U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // beyond visible range, will scroll one line
        viewer.SetAddress(127U);
        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());

        // jump to last address - cannot advance past last byte
        viewer.SetAddress(255U);
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestRetreatCursorWordEightBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetAddress(130U);
        viewer.MockRender();

        Assert::AreEqual({ 130U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 129U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word jumps to start of current word
        viewer.AdvanceCursor();
        Assert::AreEqual({ 129U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 129U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());

        // beyond visible range, will scroll one line
        viewer.RetreatCursorWord();
        Assert::AreEqual({ 127U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());

        // jump to first address - cannot retreat beyond first nibble
        viewer.SetAddress(0U);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestAdvanceCursorLine)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetAddress(4U);
        viewer.MockRender();

        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursorLine();
        Assert::AreEqual({ 20U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(4U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(20U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word is remembered
        viewer.AdvanceCursor();
        Assert::AreEqual({ 20U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursorLine();
        Assert::AreEqual({ 36U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // beyond visible range, will scroll one line
        viewer.SetAddress(120U);
        viewer.AdvanceCursorLine();
        Assert::AreEqual({ 136U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());

        // jump to last line - cannot advance past last line
        viewer.SetAddress(250U);
        Assert::AreEqual({ 250U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursorLine();
        Assert::AreEqual({ 250U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestRetreatCursorLine)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetAddress(170U);
        viewer.MockRender();

        Assert::AreEqual({ 170U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursorLine();
        Assert::AreEqual({ 154U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(154U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(170U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word is remembered
        viewer.AdvanceCursor();
        Assert::AreEqual({ 154U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorLine();
        Assert::AreEqual({ 138U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // beyond visible range, will scroll one line
        viewer.RetreatCursorLine();
        Assert::AreEqual({ 122U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());

        // jump to first line - cannot retreat beyond first nibble
        viewer.SetAddress(8U);
        Assert::AreEqual({ 8U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorLine();
        Assert::AreEqual({ 8U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestAdvanceCursorPage)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetAddress(4U);
        viewer.MockRender();

        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursorPage();
        Assert::AreEqual({ 116U }, viewer.GetAddress());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(116U));
        Assert::IsTrue(viewer.NeedsRedraw());

        // mid-word is remembered
        viewer.AdvanceCursor();
        Assert::AreEqual({ 116U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursorPage();
        Assert::AreEqual({ 228U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // max scroll reached
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());

        // cannot page past end of memory - stop at last line
        viewer.AdvanceCursorPage();
        Assert::AreEqual({ 244U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());
        viewer.MockRender();

        // at last line - do nothing
        viewer.AdvanceCursorPage();
        Assert::AreEqual({ 244U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestRetreatCursorPage)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetAddress(244U);
        viewer.MockRender();

        Assert::AreEqual({ 244U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursorPage();
        Assert::AreEqual({ 132U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(132U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word is remembered
        viewer.AdvanceCursor();
        Assert::AreEqual({ 132U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorPage();
        Assert::AreEqual({ 20U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // max scroll reached
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // cannot page past beginning of memory - stop at first line
        viewer.RetreatCursorLine();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        viewer.MockRender();

        // at first line - do nothing
        viewer.RetreatCursorLine();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestOnCharEightBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetByte(0U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // ignore if readonly
        Assert::IsTrue(viewer.IsReadOnly());
        Assert::IsFalse(viewer.OnChar('6'));

        viewer.SetReadOnly(false);
        Assert::IsFalse(viewer.IsReadOnly());

        // '6' should be set as upper nibble of selected byte
        Assert::IsTrue(viewer.OnChar('6'));
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x60U }, viewer.GetByte(0U));
        Assert::AreEqual({ 0x60U }, viewer.mockEmulatorContext.ReadMemoryByte(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // ignore invalid character
        Assert::IsFalse(viewer.OnChar('G'));
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x60U }, viewer.GetByte(0U));
        Assert::AreEqual({ 0x60U }, viewer.mockEmulatorContext.ReadMemoryByte(0U));
        Assert::AreEqual({ COLOR_RED }, viewer.GetColor(0U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // 'B' should be set as lower nibble of selected byte
        viewer.OnChar('B');
        Assert::AreEqual({ 1U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x6BU }, viewer.GetByte(0U));
        Assert::AreEqual({ 0x6BU }, viewer.mockEmulatorContext.ReadMemoryByte(0U));
        Assert::AreEqual({ 0x01U }, viewer.GetByte(1U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // 'F' should be set as upper nibble of next byte
        viewer.OnChar('F');
        Assert::AreEqual({ 1U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0xF1U }, viewer.GetByte(1U));
        Assert::AreEqual({ 0xF1U }, viewer.mockEmulatorContext.ReadMemoryByte(1U));

        // jump to last address - cannot advance past last nibble
        viewer.SetAddress(255U);
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0xFFU }, viewer.GetByte(255U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.OnChar('0');
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x0FU }, viewer.GetByte(255U));
        Assert::AreEqual({ 0x0FU }, viewer.mockEmulatorContext.ReadMemoryByte(255U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.OnChar('9');
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x09U }, viewer.GetByte(255U));
        Assert::AreEqual({ 0x09U }, viewer.mockEmulatorContext.ReadMemoryByte(255U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.OnChar('A');
        Assert::AreEqual({ 255U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x0AU }, viewer.GetByte(255U));
        Assert::AreEqual({ 0x0AU }, viewer.mockEmulatorContext.ReadMemoryByte(255U));
        Assert::IsTrue(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestAdvanceCursorSixteenBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetSize(MemSize::SixteenBit);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(2U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::IsTrue(viewer.NeedsRedraw());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // last visible word
        viewer.SetAddress(126U);
        viewer.AdvanceCursor();
        Assert::AreEqual({ 126U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 126U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 126U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // beyond visible range, will scroll one line
        viewer.AdvanceCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());

        // jump to last address - cannot advance past last nibble
        viewer.SetAddress(254U);
        Assert::AreEqual({ 254U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 254U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 254U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 254U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 254U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        // address not aligned - advance jumps to next aligned address
        viewer.SetAddress(251U);
        Assert::AreEqual({ 251U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
    }

    TEST_METHOD(TestRetreatCursorSixteenBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetSize(MemSize::SixteenBit);
        viewer.SetAddress(130U);
        viewer.MockRender();

        Assert::AreEqual({ 130U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(131U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());

        // beyond visible range, will scroll one line
        viewer.RetreatCursor();
        Assert::AreEqual({ 126U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());

        // jump to first address - cannot retreat beyond first nibble
        viewer.SetAddress(0U);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        // address not aligned - retreat jumps to aligned address
        viewer.SetAddress(3U);
        Assert::AreEqual({ 3U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        viewer.RetreatCursor();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
    }

    TEST_METHOD(TestAdvanceCursorWordSixteenBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetSize(MemSize::SixteenBit);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(2U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word jumps to start of next word
        viewer.AdvanceCursor();
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        viewer.SetAddress(5U);
        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 6U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // beyond visible range, will scroll one line
        viewer.SetAddress(126U);
        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());

        // jump to last address - cannot advance past last byte
        viewer.SetAddress(254U);
        Assert::AreEqual({ 254U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 254U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestRetreatCursorWordSixteenBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetSize(MemSize::SixteenBit);
        viewer.SetAddress(130U);
        viewer.MockRender();

        Assert::AreEqual({ 130U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(131U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word jumps to start of current word
        viewer.AdvanceCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.SetAddress(129U);
        viewer.RetreatCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // beyond visible range, will scroll one line
        viewer.RetreatCursorWord();
        Assert::AreEqual({ 126U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());

        // jump to first address - cannot retreat beyond first nibble
        viewer.SetAddress(0U);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestAdvanceCursorThirtyTwoBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetSize(MemSize::ThirtyTwoBit);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(2U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(2U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 4U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(2U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 5U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 6U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 7U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(2U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(4U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(5U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(6U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(7U));
        Assert::IsTrue(viewer.NeedsRedraw());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 4U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 5U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 6U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 7U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 8U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // last visible word
        viewer.SetAddress(124U);
        viewer.AdvanceCursor();
        Assert::AreEqual({ 124U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        Assert::AreEqual({ 124U }, viewer.GetAddress());
        Assert::AreEqual({ 7U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());

        // beyond visible range, will scroll one line
        viewer.AdvanceCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());

        // jump to last address - cannot advance past last nibble
        viewer.SetAddress(252U);
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 6U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 7U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursor();
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 7U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        // address not aligned - advance jumps to next aligned address
        viewer.SetAddress(250U);
        Assert::AreEqual({ 250U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursor();
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
    }

    TEST_METHOD(TestRetreatCursorThirtyTwoBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetSize(MemSize::ThirtyTwoBit);
        viewer.SetAddress(132U);
        viewer.MockRender();

        Assert::AreEqual({ 132U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 7U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(131U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(132U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(133U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(134U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(135U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 6U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 5U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 4U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(131U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(131U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());

        // beyond visible range, will scroll one line
        viewer.RetreatCursor();
        Assert::AreEqual({ 124U }, viewer.GetAddress());
        Assert::AreEqual({ 7U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());

        // jump to first address - cannot retreat beyond first nibble
        viewer.SetAddress(0U);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursor();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        // address not aligned - retreat jumps to aligned address
        viewer.SetAddress(7U);
        Assert::AreEqual({ 7U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        viewer.RetreatCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
    }

    TEST_METHOD(TestAdvanceCursorWordThirtyTwoBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetSize(MemSize::ThirtyTwoBit);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(2U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(4U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(5U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(6U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(7U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word jumps to start of next word
        viewer.AdvanceCursor();
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 8U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        viewer.SetAddress(10U);
        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 12U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // beyond visible range, will scroll one line
        viewer.SetAddress(124U);
        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());

        // jump to last address - cannot advance past last byte
        viewer.SetAddress(252U);
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.AdvanceCursorWord();
        Assert::AreEqual({ 252U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestRetreatCursorWordThirtyTwoBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128U);
        viewer.SetSize(MemSize::ThirtyTwoBit);
        viewer.SetAddress(132U);
        viewer.MockRender();

        Assert::AreEqual({ 132U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(128U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(129U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(131U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(132U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(133U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(134U));
        Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(135U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // mid-word jumps to start of current word
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        viewer.AdvanceCursor();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(131U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(130U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.SetAddress(131U);
        viewer.RetreatCursorWord();
        Assert::AreEqual({ 128U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // beyond visible range, will scroll one line
        viewer.RetreatCursorWord();
        Assert::AreEqual({ 124U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 112U }, viewer.GetFirstAddress());

        // jump to first address - cannot retreat beyond first nibble
        viewer.SetAddress(0U);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        viewer.MockRender();

        viewer.RetreatCursorWord();
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());
    }

    TEST_METHOD(TestOnCharSixteenBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetSize(MemSize::SixteenBit);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetByte(0U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // ignore if readonly
        Assert::IsTrue(viewer.IsReadOnly());
        Assert::IsFalse(viewer.OnChar('6'));

        viewer.SetReadOnly(false);
        Assert::IsFalse(viewer.IsReadOnly());

        // '6' should be set as upper nibble of selected byte
        Assert::IsTrue(viewer.OnChar('6'));
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x61U }, viewer.GetByte(1U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // ignore invalid character
        Assert::IsFalse(viewer.OnChar('G'));
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x61U }, viewer.GetByte(1U));
        Assert::AreEqual({ COLOR_RED }, viewer.GetColor(1U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // 'B' should be set as lower nibble of selected byte
        viewer.OnChar('B');
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x6BU }, viewer.GetByte(1U));
        Assert::AreEqual({ 0x00U }, viewer.GetByte(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(1U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // 'F' should be set as upper nibble of next byte
        viewer.OnChar('F');
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0xF0U }, viewer.GetByte(0U));

        // '2' should be set as lower nibble of selected byte
        viewer.OnChar('2');
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0xF2U }, viewer.GetByte(0U));

        // '3' should be set as upper nibble of next byte
        viewer.OnChar('3');
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x33U }, viewer.GetByte(3U));
    }

    TEST_METHOD(TestOnCharThirtyTwoBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetSize(MemSize::ThirtyTwoBit);
        viewer.MockRender();

        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0U }, viewer.GetByte(0U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // ignore if readonly
        Assert::IsTrue(viewer.IsReadOnly());
        Assert::IsFalse(viewer.OnChar('6'));

        viewer.SetReadOnly(false);
        Assert::IsFalse(viewer.IsReadOnly());

        // '6' should be set as upper nibble of selected byte
        Assert::IsTrue(viewer.OnChar('6'));
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x63U }, viewer.GetByte(3U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // ignore invalid character
        Assert::IsFalse(viewer.OnChar('G'));
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x63U }, viewer.GetByte(3U));
        Assert::AreEqual({ COLOR_RED }, viewer.GetColor(3U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // 'B' should be set as lower nibble of selected byte
        viewer.OnChar('B');
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x6BU }, viewer.GetByte(3U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(3U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // 'F' should be set as upper nibble of next byte
        viewer.OnChar('F');
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0xF2U }, viewer.GetByte(2U));

        viewer.OnChar('3');
        viewer.OnChar('D');
        viewer.OnChar('E');
        viewer.OnChar('6');
        viewer.OnChar('8');

        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0xF3U }, viewer.GetByte(2U));
        Assert::AreEqual({ 0xDEU }, viewer.GetByte(1U));
        Assert::AreEqual({ 0x68U }, viewer.GetByte(0U));

        viewer.OnChar('2');
        Assert::AreEqual({ 4U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x27U }, viewer.GetByte(7U));
    }

    TEST_METHOD(TestOnCharThirtyTwoBitBigEndian)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetSize(MemSize::ThirtyTwoBitBigEndian);
        viewer.MockRender();

        Assert::AreEqual({0U}, viewer.GetAddress());
        Assert::AreEqual({0U}, viewer.GetSelectedNibble());
        Assert::AreEqual({0U}, viewer.GetByte(0U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // ignore if readonly
        Assert::IsTrue(viewer.IsReadOnly());
        Assert::IsFalse(viewer.OnChar('6'));

        viewer.SetReadOnly(false);
        Assert::IsFalse(viewer.IsReadOnly());

        // '6' should be set as upper nibble of selected byte
        Assert::IsTrue(viewer.OnChar('6'));
        Assert::AreEqual({0U}, viewer.GetAddress());
        Assert::AreEqual({1U}, viewer.GetSelectedNibble());
        Assert::AreEqual({0x60U}, viewer.GetByte(0U));
        Assert::AreEqual({COLOR_RED | COLOR_REDRAW}, viewer.GetColor(0U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // ignore invalid character
        Assert::IsFalse(viewer.OnChar('G'));
        Assert::AreEqual({0U}, viewer.GetAddress());
        Assert::AreEqual({1U}, viewer.GetSelectedNibble());
        Assert::AreEqual({0x60U}, viewer.GetByte(0U));
        Assert::AreEqual({COLOR_RED}, viewer.GetColor(0U));
        Assert::IsFalse(viewer.NeedsRedraw());

        // 'B' should be set as lower nibble of selected byte
        viewer.OnChar('B');
        Assert::AreEqual({0U}, viewer.GetAddress());
        Assert::AreEqual({2U}, viewer.GetSelectedNibble());
        Assert::AreEqual({0x6BU}, viewer.GetByte(0U));
        Assert::AreEqual({COLOR_RED | COLOR_REDRAW}, viewer.GetColor(0U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // 'F' should be set as upper nibble of next byte
        viewer.OnChar('F');
        Assert::AreEqual({0U}, viewer.GetAddress());
        Assert::AreEqual({3U}, viewer.GetSelectedNibble());
        Assert::AreEqual({0xF1U}, viewer.GetByte(1U));

        viewer.OnChar('3');
        viewer.OnChar('D');
        viewer.OnChar('E');
        viewer.OnChar('6');
        viewer.OnChar('8');

        Assert::AreEqual({4U}, viewer.GetAddress());
        Assert::AreEqual({0U}, viewer.GetSelectedNibble());
        Assert::AreEqual({0xF3U}, viewer.GetByte(1U));
        Assert::AreEqual({0xDEU}, viewer.GetByte(2U));
        Assert::AreEqual({0x68U}, viewer.GetByte(3U));

        viewer.OnChar('2');
        Assert::AreEqual({4U}, viewer.GetAddress());
        Assert::AreEqual({1U}, viewer.GetSelectedNibble());
        Assert::AreEqual({0x24U}, viewer.GetByte(4U));
    }

    TEST_METHOD(TestTotalMemorySizeChanged)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.SetFirstAddress(128);
        viewer.SetAddress(160);

        // growth - no change necessary
        viewer.mockEmulatorContext.MockTotalMemorySizeChanged(256);
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 160U }, viewer.GetAddress());

        // shrink above max viewable address - no change necessary
        viewer.mockEmulatorContext.MockTotalMemorySizeChanged(384);
        Assert::AreEqual({ 128U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 160U }, viewer.GetAddress());

        // shrink below max viewable address, but above current address, first address should change
        viewer.mockEmulatorContext.MockTotalMemorySizeChanged(192);
        Assert::AreEqual({ 64U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 160U }, viewer.GetAddress());

        // shrink below current address (and max viewable address), both addresses should change
        viewer.mockEmulatorContext.MockTotalMemorySizeChanged(144);
        Assert::AreEqual({ 16U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 143U }, viewer.GetAddress()); // last visible byte

        // shrink below visiblelines, both addresses should change to 0
        viewer.mockEmulatorContext.MockTotalMemorySizeChanged(96);
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 95U }, viewer.GetAddress());

        // shrink to 0
        viewer.mockEmulatorContext.ClearMemoryBlocks();
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 0U }, viewer.GetAddress());
    }

    TEST_METHOD(TestOnClickEightBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256); // 16 rows of 16 bytes

        Assert::AreEqual(MemSize::EightBit, viewer.GetSize());
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // 14th character is second character of second byte
        viewer.OnClick(14 * CHAR_WIDTH + 4, CHAR_HEIGHT * 2 + 4);
        Assert::AreEqual({ 17U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // click in left margin should not change address
        viewer.OnClick(4, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 17U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // click in top margin should not change address
        viewer.OnClick(20 * CHAR_WIDTH + 4, 4);
        Assert::AreEqual({ 17U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // click in right margin should not change address
        viewer.OnClick(57 * CHAR_WIDTH + 4, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 17U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // more than half a char away from first valid column, ignore
        viewer.OnClick(9 * CHAR_WIDTH + 3, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 17U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // less than half a char away from first valid column, select first column
        viewer.OnClick(9 * CHAR_WIDTH + 4, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // rightmost pixel of first valid column, keep selection
        viewer.OnClick(11 * CHAR_WIDTH - 1, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // leftmost pixel of second valid column, update nibble
        viewer.OnClick(11 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // less than half a char into third column, keep selection
        viewer.OnClick(12 * CHAR_WIDTH + 3, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // half a char into third column, move to next byte
        viewer.OnClick(12 * CHAR_WIDTH + 4, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 1U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
    }

    TEST_METHOD(TestOnClickSixteenBit)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256); // 16 rows of 16 bytes

        viewer.SetSize(MemSize::SixteenBit);
        Assert::AreEqual(MemSize::SixteenBit, viewer.GetSize());
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // 17th character is third character of second word
        viewer.OnClick(17 * CHAR_WIDTH + 4, CHAR_HEIGHT * 2 + 4);
        Assert::AreEqual({ 18U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());

        // click in right margin should not change address
        viewer.OnClick(49 * CHAR_WIDTH + 4, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 18U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());

        // more than half a char away from first valid column, ignore
        viewer.OnClick(9 * CHAR_WIDTH + 3, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 18U }, viewer.GetAddress());
        Assert::AreEqual({ 2U }, viewer.GetSelectedNibble());

        // less than half a char away from first valid column, select first column
        viewer.OnClick(9 * CHAR_WIDTH + 4, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // rightmost pixel of first valid column, keep selection
        viewer.OnClick(11 * CHAR_WIDTH - 1, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // leftmost pixel of second valid column, update nibble
        viewer.OnClick(11 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());

        // leftmost pixel of fourth valid column, update nibble
        viewer.OnClick(13 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());

        // less than half a char into fifth column, keep selection
        viewer.OnClick(14 * CHAR_WIDTH + 3, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 3U }, viewer.GetSelectedNibble());

        // half a char into fifth column, move to next byte
        viewer.OnClick(14 * CHAR_WIDTH + 4, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 2U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
    }

    TEST_METHOD(TestScrollCursorOffscreen)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256); // 16 rows of 16 bytes

        viewer.SetFirstAddress(0x40U);

        // set cursor to last character of second line
        viewer.SetAddress(0x5FU);

        // simulates scroll wheel down - advance two lines
        viewer.SetFirstAddress(0x60U);

        // selected address is offscreen, but should not have changed
        // this also validates an edge case that was causing an index out of range exception
        Assert::AreEqual({ 0x5FU }, viewer.GetAddress());

        // reset and try again in 16-bit mode
        viewer.SetFirstAddress(0x40U);
        viewer.SetSize(MemSize::SixteenBit);
        viewer.SetFirstAddress(0x60U);
        Assert::AreEqual({ 0x5FU }, viewer.GetAddress());

        viewer.SetFirstAddress(0x40U);
        viewer.SetAddress(0x5EU);
        viewer.SetFirstAddress(0x60U);
        Assert::AreEqual({ 0x5EU }, viewer.GetAddress());

        // reset and try again in 32-bit mode
        viewer.SetFirstAddress(0x40U);
        viewer.SetAddress(0x5FU);
        viewer.SetSize(MemSize::ThirtyTwoBit);
        viewer.SetFirstAddress(0x60U);
        Assert::AreEqual({ 0x5FU }, viewer.GetAddress());

        viewer.SetFirstAddress(0x40U);
        viewer.SetAddress(0x5CU);
        viewer.SetFirstAddress(0x60U);
        Assert::AreEqual({ 0x5CU }, viewer.GetAddress());
    }

    TEST_METHOD(TestOnCharOffscreen)
    {
        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(256);
        viewer.MockRender();
        viewer.SetAddress(20U); // set address first, as it updates FirstAddress
        viewer.SetFirstAddress(64U); // showing $0040-$00BF (8 lines)
        viewer.SetReadOnly(false);
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        Assert::AreEqual({ 20U }, viewer.GetAddress());
        Assert::AreEqual({ 64U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 8U }, viewer.GetNumVisibleLines());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsFalse(viewer.NeedsRedraw());

        // cursor should be moved onscreen and '6' should be set as upper nibble of selected byte
        Assert::IsTrue(viewer.OnChar('6'));
        Assert::AreEqual({ 20U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x64U }, viewer.mockEmulatorContext.ReadMemoryByte(20U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(20U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        // jump to a later address, scroll it below the visible window
        viewer.SetAddress(0xE3U);
        viewer.SetFirstAddress(64U); // showing $0040-$00BF (8 lines)
        Assert::AreEqual({ 0xE3U }, viewer.GetAddress());
        Assert::AreEqual({ 64U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 8U }, viewer.GetNumVisibleLines());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();

        viewer.OnChar('0');
        Assert::AreEqual({ 0xE3U }, viewer.GetAddress());
        Assert::AreEqual({ 0x80U }, viewer.GetFirstAddress());
        Assert::AreEqual({ 1U }, viewer.GetSelectedNibble());
        Assert::AreEqual({ 0x03U }, viewer.mockEmulatorContext.ReadMemoryByte(0xE3U));
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(0xE3U));
        Assert::IsTrue(viewer.NeedsRedraw());
        viewer.MockRender();
    }

    TEST_METHOD(TestOnShiftClickEightBit)
    {
        ra::data::context::mocks::MockConsoleContext mockConsole(PlayStation, L"Playstation");

        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(128); // 8 rows of 16 bytes
        viewer.mockEmulatorContext.WriteMemoryByte(0U, 0x20);
        viewer.mockEmulatorContext.WriteMemoryByte(1U, 0xff);
        viewer.mockEmulatorContext.WriteMemoryByte(2U, 0x0);
        
        Assert::AreEqual(MemSize::EightBit, viewer.GetSize());
        Assert::AreEqual({ 0U }, viewer.GetAddress());
        Assert::AreEqual({ 0U }, viewer.GetSelectedNibble());

        // If fixed address, ignore
        viewer.SetAddressFixed(true);
        viewer.OnShiftClick(10 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({0x0U}, viewer.GetAddress());

        // If not fixed address and Shift click on the first byte containing 0x20 should lead to address 0x20
        viewer.SetAddressFixed(false);
        viewer.OnShiftClick(10 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({ 0x20U }, viewer.GetAddress());

        // Shift click on the second byte containing 0xFF should lead to address 0x7F as 0xFF is bigger than the last
        // address in memory
        viewer.OnShiftClick(13 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({0x7FU}, viewer.GetAddress());

        // Shift click on the third byte containing 0x0 should lead back to address 0x0
        viewer.OnShiftClick(16 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({0x0U}, viewer.GetAddress());
    }

    TEST_METHOD(TestOnShiftClickSixTeenBit)
    {
        ra::data::context::mocks::MockConsoleContext mockConsole(PlayStation, L"Playstation");

        MemoryViewerViewModelHarness viewer;
        viewer.InitializeMemory(512); // 32 rows of 16 bytes
        viewer.SetSize(MemSize::SixteenBit);
        viewer.mockEmulatorContext.WriteMemory(0U, MemSize::SixteenBit, 0x20);
        viewer.mockEmulatorContext.WriteMemory(2U, MemSize::SixteenBit, 0xff);
        viewer.mockEmulatorContext.WriteMemory(4U, MemSize::SixteenBit, 0xffff);

        Assert::AreEqual(MemSize::SixteenBit, viewer.GetSize());
        Assert::AreEqual({0U}, viewer.GetAddress());
        Assert::AreEqual({0U}, viewer.GetSelectedNibble());

        // If fixed address, ignore
        viewer.SetAddressFixed(true);
        viewer.OnShiftClick(10 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({0x0U}, viewer.GetAddress());

        // If not fixed address and Shift click on the first word containing 0x20 should lead to address 0x20
        viewer.SetAddressFixed(false);
        viewer.OnShiftClick(10 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({0x20U}, viewer.GetAddress());

        // Shift click on the second word containing 0x40 should lead to address 0xFF
        viewer.OnShiftClick(15 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({0xFFU}, viewer.GetAddress());

        // Shift click on the third word containing 0xFFFF should lead to address 0x1FF as 0xFFFF is bigger than the
        // last address in memory
        viewer.OnShiftClick(19 * CHAR_WIDTH, CHAR_HEIGHT + 4);
        Assert::AreEqual({0x1FFU}, viewer.GetAddress());
    }
};


} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
