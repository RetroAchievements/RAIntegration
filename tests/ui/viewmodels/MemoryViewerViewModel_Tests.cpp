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

        MemoryViewerViewModelHarness() : MemoryViewerViewModel()
        {
            mockGameContext.AddNotifyTarget(*this);
        }

        int GetSelectedNibble() const { return m_nSelectedNibble; }

        size_t GetTotalMemorySize() const { return m_nTotalMemorySize; }

        unsigned char GetByte(ra::ByteAddress nAddress) const
        {
            return m_pMemory[nAddress - GetFirstAddress()];
        }

        unsigned char GetColor(ra::ByteAddress nAddress) const
        {
            return gsl::narrow_cast<unsigned char>(ra::itoe<TextColor>(m_pColor[nAddress - GetFirstAddress()]));
        }

        void InitializeMemory(size_t nSize)
        {
            unsigned char* pBytes = new unsigned char[nSize];
            m_pBytes.reset(pBytes);
            for (size_t i = 0; i < nSize; ++i)
                pBytes[i] = gsl::narrow_cast<unsigned char>(i & 0xFF);

            mockEmulatorContext.MockMemory(pBytes, nSize);

            DoFrame(); // populates m_nTotalMemorySize, m_pMemory, and m_pColor
        }

        void MockRender()
        {
            for (size_t i = 0; i < m_nTotalMemorySize; ++i)
                m_pColor[i] &= 0x0F;

            m_bNeedsRedraw = false;
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
        for (size_t i = 1; i < 128; ++i)
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
        for (size_t i = 1; i < 128; ++i)
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
        for (size_t i = 1; i < 128; ++i)
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

};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
