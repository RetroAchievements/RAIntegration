#include "CppUnitTest.h"

#include "ui\viewmodels\MemoryViewerViewModel.hh"

#include "services\ServiceLocator.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockWindowManager.hh"

#undef GetMessage

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

constexpr unsigned char COLOR_RED = gsl::narrow_cast<unsigned char>(ra::etoi(MemoryViewerViewModel::TextColor::Red));
constexpr unsigned char COLOR_BLACK = gsl::narrow_cast<unsigned char>(ra::etoi(MemoryViewerViewModel::TextColor::Black));
constexpr unsigned char COLOR_REDRAW = 0x80;

TEST_CLASS(MemoryViewerViewModel_Tests)
{
private:
    class MemoryViewerViewModelHarness : public MemoryViewerViewModel
    {
    public:
        ra::data::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        GSL_SUPPRESS_F6 MemoryViewerViewModelHarness() : MemoryViewerViewModel()
        {
            InitializeNotifyTargets();
        }

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
        Assert::AreEqual({ 368U }, viewer.GetFirstAddress()); // (444 & 0x0F) - 4 * 16
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
        Assert::AreEqual({ 288U }, viewer.GetFirstAddress()); // (360U & 0x0F) - 4 * 16
        Assert::AreEqual({ 360U }, viewer.GetAddress());
        Assert::AreEqual({ COLOR_RED | COLOR_REDRAW }, viewer.GetColor(360U));
        for (ra::ByteAddress i = 1; i < 128; ++i)
        {
            if (i != 360 - 288)
                Assert::AreEqual({ COLOR_BLACK | COLOR_REDRAW }, viewer.GetColor(i + 288));
        }
        Assert::IsTrue(viewer.NeedsRedraw());
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
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
