#include "CppUnitTest.h"

#include "ui\viewmodels\MemorySearchViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\ui\UIAsserts.hh"

#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::services::SearchType>(
    const ra::services::SearchType& nSearchType)
{
    switch (nSearchType)
    {
        case ra::services::SearchType::FourBit:
            return L"FourBit";
        case ra::services::SearchType::EightBit:
            return L"EightBit";
        case ra::services::SearchType::SixteenBit:
            return L"SixteenBit";
        case ra::services::SearchType::ThirtyTwoBit:
            return L"ThirtyTwoBit";
        case ra::services::SearchType::SixteenBitAligned:
            return L"SixteenBitAligned";
        case ra::services::SearchType::ThirtyTwoBitAligned:
            return L"ThirtyTwoBitAligned";
        case ra::services::SearchType::AsciiText:
            return L"AsciiText";
        default:
            return std::to_wstring(static_cast<int>(nSearchType));
    }
}

template<>
std::wstring ToString<ra::services::SearchFilterType>(
    const ra::services::SearchFilterType& nValueType)
{
    switch (nValueType)
    {
        case ra::services::SearchFilterType::Constant:
            return L"Constant";
        case ra::services::SearchFilterType::LastKnownValue:
            return L"LastKnownValue";
        case ra::services::SearchFilterType::LastKnownValuePlus:
            return L"LastKnownValuePlus";
        case ra::services::SearchFilterType::LastKnownValueMinus:
            return L"LastKnownValueMinus";
        default:
            return std::to_wstring(static_cast<int>(nValueType));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(MemorySearchViewModel_Tests)
{
private:
    class MemorySearchViewModelHarness : public MemorySearchViewModel
    {
    public:
        GSL_SUPPRESS_F6 MemorySearchViewModelHarness() : MemorySearchViewModel()
        {
            InitializeNotifyTargets();
        }

        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        std::array<unsigned char, 32> memory{};

        void InitializeMemory()
        {
            for (size_t i = 0; i < memory.size(); ++i)
                memory.at(i) = gsl::narrow_cast<unsigned char>(i);

            mockEmulatorContext.MockMemory(memory);
        }

        void InitializeAsciiMemory()
        {
            std::array<unsigned char, 36> text {
                'S', 'h', 'e', ' ', 's', 'e', 'l', 'l', 's', ' ', 's', 'e', 'a', 's', 'h', 'e',
                'l', 'l', 's', ' ', 'b', 'y', ' ', 't', 'h', 'e', ' ', 's', 'e', 'a', 's', 'h',
                'o', 'r', 'e', '.'
            };
            for (size_t i = 0; i < memory.size(); ++i)
                memory.at(i) = text.at(i);

            mockEmulatorContext.MockMemory(memory);
        }

        bool CanBeginNewSearch() const { return GetValue(CanBeginNewSearchProperty); }
        bool CanFilter() const { return GetValue(CanFilterProperty); }
        bool CanContinuousFilter() const { return GetValue(CanContinuousFilterProperty); }
        bool CanEditFilterValue() const { return GetValue(CanEditFilterValueProperty); }
        bool CanGoToPreviousPage() const { return GetValue(CanGoToPreviousPageProperty); }
        bool CanGoToNextPage() const { return GetValue(CanGoToNextPageProperty); }
        bool HasSelection() const { return GetValue(HasSelectionProperty); }

        void SetScrollOffset(int nValue) { SetValue(ScrollOffsetProperty, nValue); }

        const std::wstring& ContinuousFilterLabel() const { return GetValue(ContinuousFilterLabelProperty); }
    };

    void AssertRow(MemorySearchViewModelHarness& search, gsl::index nRow, ra::ByteAddress nAddress,
        const wchar_t* sAddress, const wchar_t* sCurrentValue)
    {
        auto* pRow = search.Results().GetItemAt(nRow);
        Assert::IsNotNull(pRow);
        Ensures(pRow != nullptr);

        Assert::AreEqual(std::wstring(sAddress), pRow->GetAddress());
        Assert::AreEqual(std::wstring(sCurrentValue), pRow->GetCurrentValue());
        Assert::AreEqual(nAddress, pRow->nAddress);
    }

public:
    TEST_METHOD(TestInitialValues)
    {
        MemorySearchViewModelHarness search;

        Assert::AreEqual({ 0U }, search.Results().Count());
        Assert::AreEqual(MemSize::Bit_0, search.ResultMemSize());
        Assert::AreEqual({ 0U }, search.GetResultCount());

        Assert::AreEqual(std::wstring(L""), search.GetFilterRange());
        Assert::AreEqual(std::wstring(L""), search.GetFilterValue());

        Assert::AreEqual({ 11U }, search.SearchTypes().Count());
        Assert::AreEqual((int)ra::services::SearchType::FourBit, search.SearchTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"4-bit"), search.SearchTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::EightBit, search.SearchTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"8-bit"), search.SearchTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::SixteenBit, search.SearchTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit"), search.SearchTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::ThirtyTwoBit, search.SearchTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit"), search.SearchTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::SixteenBitAligned, search.SearchTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit (aligned)"), search.SearchTypes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::ThirtyTwoBitAligned, search.SearchTypes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit (aligned)"), search.SearchTypes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::SixteenBitBigEndian, search.SearchTypes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit BE"), search.SearchTypes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::ThirtyTwoBitBigEndian, search.SearchTypes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit BE"), search.SearchTypes().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::Float, search.SearchTypes().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"Float"), search.SearchTypes().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::MBF32, search.SearchTypes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"MBF32"), search.SearchTypes().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchType::AsciiText, search.SearchTypes().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"ASCII Text"), search.SearchTypes().GetItemAt(10)->GetLabel());
        Assert::AreEqual(ra::services::SearchType::EightBit, search.GetSearchType());

        Assert::AreEqual({ 6U }, search.ComparisonTypes().Count());
        Assert::AreEqual((int)ComparisonType::Equals, search.ComparisonTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"="), search.ComparisonTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ComparisonType::LessThan, search.ComparisonTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"<"), search.ComparisonTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ComparisonType::LessThanOrEqual, search.ComparisonTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"<="), search.ComparisonTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ComparisonType::GreaterThan, search.ComparisonTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L">"), search.ComparisonTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)ComparisonType::GreaterThanOrEqual, search.ComparisonTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L">="), search.ComparisonTypes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)ComparisonType::NotEqualTo, search.ComparisonTypes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"!="), search.ComparisonTypes().GetItemAt(5)->GetLabel());
        Assert::AreEqual(ComparisonType::Equals, search.GetComparisonType());

        Assert::AreEqual({ 5U }, search.ValueTypes().Count());
        Assert::AreEqual((int)ra::services::SearchFilterType::Constant, search.ValueTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Constant"), search.ValueTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchFilterType::LastKnownValue, search.ValueTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Last Value"), search.ValueTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchFilterType::LastKnownValuePlus, search.ValueTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Last Value Plus"), search.ValueTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchFilterType::LastKnownValueMinus, search.ValueTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Last Value Minus"), search.ValueTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)ra::services::SearchFilterType::InitialValue, search.ValueTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Initial Value"), search.ValueTypes().GetItemAt(4)->GetLabel());

        Assert::AreEqual(ra::services::SearchFilterType::LastKnownValue, search.GetValueType());

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"0/0"), search.GetSelectedPage());

        Assert::AreEqual(std::wstring(L""), search.GetFilterSummary());

        Assert::IsFalse(search.CanBeginNewSearch());
        Assert::IsFalse(search.CanFilter());
        Assert::IsFalse(search.CanEditFilterValue());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());
        Assert::IsFalse(search.HasSelection());
    }

    TEST_METHOD(TestBeginNewSearchEightBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        Assert::IsFalse(search.CanFilter());

        search.BeginNewSearch();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 0 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 32U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 8-bit Search"), search.GetFilterSummary());
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());
    }

    TEST_METHOD(TestBeginNewSearchSixteenBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::SixteenBit);
        Assert::IsFalse(search.CanFilter());

        search.BeginNewSearch();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 0 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 31U }, search.GetResultCount());
        Assert::AreEqual(MemSize::SixteenBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 16-bit Search"), search.GetFilterSummary());
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());
    }

    TEST_METHOD(TestBeginNewSearchFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::FourBit);
        Assert::IsFalse(search.CanFilter());

        search.BeginNewSearch();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 0 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 64U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 4-bit Search"), search.GetFilterSummary());
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());
    }

    TEST_METHOD(TestApplyFilterEightBitConstantDecimal)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"12");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= 12"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0c");
    }

    TEST_METHOD(TestApplyFilterEightBitConstantHex)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"0x0C");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= 0x0C"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0c");
    }

    TEST_METHOD(TestApplyFilterEightBitConstantInvalid)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        Assert::IsTrue(search.CanEditFilterValue());
        search.SetFilterValue(L"banana");

        bool bSawDialog = false;
        search.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;
            Assert::AreEqual(std::wstring(L"Invalid filter value"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        search.ApplyFilter();
        Assert::IsTrue(bSawDialog);

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 32U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"32"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 8-bit Search"), search.GetFilterSummary());
        Assert::AreEqual({ 0U }, search.Results().Count());
    }

    TEST_METHOD(TestApplyFilterEightBitConstantNoValue)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        Assert::IsTrue(search.CanEditFilterValue());
        search.SetFilterValue(L"");

        bool bSawDialog = false;
        search.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;
            Assert::AreEqual(std::wstring(L"Invalid filter value"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        search.ApplyFilter();
        Assert::IsTrue(bSawDialog);

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 32U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"32"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"New 8-bit Search"), search.GetFilterSummary());
        Assert::AreEqual({ 0U }, search.Results().Count());
    }

    TEST_METHOD(TestApplyFilterEightBitLastKnown)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        Assert::IsFalse(search.CanEditFilterValue());
        search.memory.at(12) = 9;

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x09");

        search.memory.at(12) = 7;
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());

        AssertRow(search, 0, 12U, L"0x000c", L"0x07");
    }

    TEST_METHOD(TestApplyFilterEightBitLastKnownNoChange)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.memory.at(12) = 9;

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x09");

        // if results don't change and filter does change, generate a new page
        search.SetComparisonType(ComparisonType::Equals);
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x09");
    }

    TEST_METHOD(TestApplyFilterEightBitLastKnownPlus)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValuePlus);
        Assert::IsTrue(search.CanEditFilterValue());
        search.SetFilterValue(L"2");
        search.memory.at(12) = 14;

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= Last +2"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0e");
    }

    TEST_METHOD(TestApplyFilterEightBitLastKnownMinus)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValueMinus);
        Assert::IsTrue(search.CanEditFilterValue());
        search.SetFilterValue(L"2");
        search.memory.at(12) = 10;

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= Last -2"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0a");
    }

    TEST_METHOD(TestApplyFilterEightBitCompareToInitialValue)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::GreaterThan);
        search.SetValueType(ra::services::SearchFilterType::InitialValue);
        Assert::IsFalse(search.CanEditFilterValue());

        search.memory.at(10) = 20;
        search.memory.at(20) = 30;
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 2U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"2"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"> Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 2U }, search.Results().Count());
        AssertRow(search, 0, 10U, L"0x000a", L"0x14");
        AssertRow(search, 1, 20U, L"0x0014", L"0x1e");

        // 15 is still greater than initial value (10), but less than current value (20).
        search.memory.at(10) = 15;
        search.ApplyFilter();
        Assert::AreEqual({ 2U }, search.Results().Count());
        AssertRow(search, 0, 10U, L"0x000a", L"0x0f");
        AssertRow(search, 1, 20U, L"0x0014", L"0x1e");

        // 5 is less than initial value and should be filtered out.
        search.memory.at(10) = 5;
        search.ApplyFilter();
        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 20U, L"0x0014", L"0x1e");
    }

    TEST_METHOD(TestApplyFilterAsciiTextConstant)
    {
        MemorySearchViewModelHarness search;
        search.InitializeAsciiMemory();
        search.SetSearchType(ra::services::SearchType::AsciiText);
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"sea");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 2U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Text, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= sea"), search.GetFilterSummary());

        Assert::AreEqual({ 2U }, search.Results().Count());
        AssertRow(search, 0, 10U, L"0x000a", L"seashells by the");
        AssertRow(search, 1, 27U, L"0x001b", L"seash");
    }

    TEST_METHOD(TestApplyFilterMaxPages)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        Assert::IsFalse(search.CanEditFilterValue());

        for (int i = 0; i < 100; ++i)
        {
            search.memory.at(12)++;
            search.ApplyFilter();
        }

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"50/50"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x70");

        // test initial value == 12 to check first buffered result
        // which should still be the initial capture, even if the history had to be cycled
        search.SetValueType(ra::services::SearchFilterType::InitialValue);
        search.SetComparisonType(ComparisonType::Equals);
        search.memory.at(12) = 12;
        search.ApplyFilter();

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"50/50"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0c");
    }

    TEST_METHOD(TestApplyFilterMaxPagesInitialValue)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(ra::services::SearchFilterType::InitialValue);
        Assert::IsFalse(search.CanEditFilterValue());

        for (int i = 0; i < 100; ++i)
        {
            search.memory.at(12)++;
            search.ApplyFilter();
        }

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"50/50"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x70");

        // test initial value == 12 to check first buffered result. since every step along
        // the way was an initial value check, should still be the original initial value
        search.SetValueType(ra::services::SearchFilterType::InitialValue);
        search.SetComparisonType(ComparisonType::Equals);
        search.memory.at(12) = 12;
        search.ApplyFilter();

        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"50/50"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x000c", L"0x0c");
    }

    TEST_METHOD(TestDoFrameEightBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        search.memory.at(2) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 32U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"32"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 0U, L"0x0000", L"0x00");
        AssertRow(search, 1, 1U, L"0x0001", L"0x01");
        AssertRow(search, 2, 2U, L"0x0002", L"0x09");
    }

    TEST_METHOD(TestDoFrameSixteenBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::SixteenBit);
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        search.memory.at(2) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 31U }, search.GetResultCount());
        Assert::AreEqual(MemSize::SixteenBit, search.ResultMemSize());

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 0U, L"0x0000", L"0x0100");
        AssertRow(search, 1, 1U, L"0x0001", L"0x0901");
        AssertRow(search, 2, 2U, L"0x0002", L"0x0309");
    }


    TEST_METHOD(TestApplyFilterFourBitLastKnownPlus)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::FourBit);
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValuePlus);
        Assert::IsTrue(search.CanEditFilterValue());
        search.SetFilterValue(L"2");
        search.memory.at(6) = 0x28;
        search.memory.at(12) = 14;

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 3U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"3"), search.GetResultCountText());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"= Last +2"), search.GetFilterSummary());

        Assert::AreEqual({ 3U }, search.Results().Count());
        AssertRow(search, 0, 12U, L"0x0006L", L"0x8");
        AssertRow(search, 1, 13U, L"0x0006U", L"0x2");
        AssertRow(search, 2, 24U, L"0x000cL", L"0xe");
    }

    TEST_METHOD(TestDoFrameFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::FourBit);
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        search.memory.at(1) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 64U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 0U, L"0x0000L", L"0x0");
        AssertRow(search, 1, 1U, L"0x0000U", L"0x0");
        AssertRow(search, 2, 2U, L"0x0001L", L"0x9");
    }

    TEST_METHOD(TestNextPagePreviousPage)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"8");
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 8 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"< 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"8"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02");
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());

        search.SetComparisonType(ComparisonType::GreaterThan);
        search.SetFilterValue(L"3");
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 4 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"> 3"), search.GetFilterSummary());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"4"), search.GetResultCountText());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06");
        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetFilterValue(L"6");
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 3 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"3/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"!= 6"), search.GetFilterSummary());
        Assert::AreEqual({ 3U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"3"), search.GetResultCountText());
        Assert::AreEqual({ 3U }, search.Results().Count());
        AssertRow(search, 2, 7U, L"0x0007", L"0x07");
        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());

        // cannot go forward
        search.NextPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 3 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"3/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"!= 6"), search.GetFilterSummary());
        Assert::AreEqual({ 3U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"3"), search.GetResultCountText());
        Assert::AreEqual({ 3U }, search.Results().Count());
        AssertRow(search, 2, 7U, L"0x0007", L"0x07");
        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 4 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"> 3"), search.GetFilterSummary());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"4"), search.GetResultCountText());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06");
        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanGoToPreviousPage());
        Assert::IsTrue(search.CanGoToNextPage());

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 8 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"< 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"8"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02");
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsTrue(search.CanGoToNextPage());

        // cannot go back any further
        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 8 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"< 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"8"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02");
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsTrue(search.CanGoToNextPage());

        search.NextPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 4 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"> 3"), search.GetFilterSummary());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"4"), search.GetResultCountText());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06");
        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanGoToPreviousPage());
        Assert::IsTrue(search.CanGoToNextPage());

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 8 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/3"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"< 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"8"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02");
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsTrue(search.CanGoToNextPage());

        // not at last page - filter should replace any pages after current
        search.SetComparisonType(ComparisonType::Equals);
        search.SetFilterValue(L"5");
        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 1 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"= 5"), search.GetFilterSummary());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 5U, L"0x0005", L"0x05");
        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 8 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"1/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"< 8"), search.GetFilterSummary());
        Assert::AreEqual({ 8U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"8"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual({ 8U }, search.Results().Count());
        AssertRow(search, 2, 2U, L"0x0002", L"0x02");
        Assert::IsTrue(search.CanFilter());
        Assert::IsFalse(search.CanGoToPreviousPage());
        Assert::IsTrue(search.CanGoToNextPage());

        search.NextPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual({ 1 }, search.GetScrollMaximum());
        Assert::AreEqual(std::wstring(L"2/2"), search.GetSelectedPage());
        Assert::AreEqual(std::wstring(L"= 5"), search.GetFilterSummary());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 5U, L"0x0005", L"0x05");
        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanGoToPreviousPage());
        Assert::IsFalse(search.CanGoToNextPage());
    }

    TEST_METHOD(TestDoFramePreviousPage)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"8");
        search.ApplyFilter();
        Assert::AreEqual({ 8U }, search.GetResultCount());

        search.SetComparisonType(ComparisonType::GreaterThan);
        search.SetFilterValue(L"3");
        search.ApplyFilter();
        Assert::AreEqual({ 4U }, search.GetResultCount());

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetFilterValue(L"6");
        search.ApplyFilter();
        Assert::AreEqual({ 3U }, search.GetResultCount());

        search.PreviousPage();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x06");

        search.memory.at(6) = 9;
        search.DoFrame();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"2/3"), search.GetSelectedPage());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 2, 6U, L"0x0006", L"0x09");
    }

    TEST_METHOD(TestExcludeSelectedEightBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"5");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 5U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());

        Assert::AreEqual({ 5U }, search.Results().Count());
        Assert::IsFalse(search.HasSelection());
        search.Results().GetItemAt(2)->SetSelected(true);
        Assert::IsTrue(search.HasSelection());
        search.Results().GetItemAt(3)->SetSelected(true);
        Assert::IsTrue(search.HasSelection());

        search.ExcludeSelected();
        Assert::AreEqual({ 3U }, search.Results().Count());
        Assert::IsFalse(search.HasSelection());
        Assert::AreEqual(0U, search.Results().GetItemAt(0)->nAddress);
        Assert::AreEqual(1U, search.Results().GetItemAt(1)->nAddress);
        Assert::AreEqual(4U, search.Results().GetItemAt(2)->nAddress);
        Assert::IsFalse(search.Results().GetItemAt(2)->IsSelected());

        search.ExcludeSelected();
        Assert::IsFalse(search.HasSelection());
        Assert::AreEqual({ 3U }, search.Results().Count());
    }

    TEST_METHOD(TestExcludeSelectedFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::FourBit);
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"1");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 18U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());

        Assert::IsFalse(search.HasSelection());
        Assert::AreEqual(0U, search.Results().GetItemAt(0)->nAddress); // 0L
        search.Results().GetItemAt(0)->SetSelected(true);
        Assert::IsTrue(search.HasSelection());
        Assert::AreEqual(3U, search.Results().GetItemAt(2)->nAddress); // 1H
        search.Results().GetItemAt(2)->SetSelected(true); 
        Assert::IsTrue(search.HasSelection());

        search.ExcludeSelected();
        Assert::AreEqual({ 16U }, search.GetResultCount());
        Assert::IsFalse(search.HasSelection());
        Assert::AreEqual(1U, search.Results().GetItemAt(0)->nAddress); // 0H
        Assert::AreEqual(5U, search.Results().GetItemAt(1)->nAddress); // 2H
        Assert::AreEqual(7U, search.Results().GetItemAt(2)->nAddress); // 3H
        Assert::IsFalse(search.Results().GetItemAt(2)->IsSelected());

        search.ExcludeSelected();
        Assert::IsFalse(search.HasSelection());
        Assert::AreEqual({ 16U }, search.GetResultCount());
    }

    TEST_METHOD(TestBookmarkSelected)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::LessThan);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"5");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 5U }, search.GetResultCount());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());

        Assert::AreEqual({ 5U }, search.Results().Count());
        Assert::IsFalse(search.HasSelection());
        search.Results().GetItemAt(2)->SetSelected(true);
        Assert::IsTrue(search.HasSelection());
        search.Results().GetItemAt(3)->SetSelected(true);
        Assert::IsTrue(search.HasSelection());

        search.BookmarkSelected();
        Assert::AreEqual({ 5U }, search.Results().Count());
        Assert::IsTrue(search.HasSelection());

        const auto& pBookmarks = search.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 2U }, pBookmarks.Count());
        Assert::AreEqual({ 2U }, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual({ 3U }, pBookmarks.GetItemAt(1)->GetAddress());

        Assert::IsTrue(search.Results().GetItemAt(2)->IsSelected());
        Assert::IsTrue(search.Results().GetItemAt(3)->IsSelected());
    }

    TEST_METHOD(TestBookmarkSelectedFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.memory.at(30) = 0xFF;
        search.SetSearchType(ra::services::SearchType::FourBit);
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::GreaterThan);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"13");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 5U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Nibble_Lower, search.ResultMemSize());

        Assert::AreEqual({ 5U }, search.Results().Count());
        search.Results().GetItemAt(2)->SetSelected(true);
        search.Results().GetItemAt(3)->SetSelected(true);

        search.BookmarkSelected();
        Assert::AreEqual({ 5U }, search.Results().Count());

        const auto& pBookmarks = search.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 2U }, pBookmarks.Count());
        Assert::AreEqual({ 30U }, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual({ 30U }, pBookmarks.GetItemAt(1)->GetAddress());
        Assert::AreEqual(MemSize::Nibble_Lower, pBookmarks.GetItemAt(0)->GetSize());
        Assert::AreEqual(MemSize::Nibble_Upper, pBookmarks.GetItemAt(1)->GetSize());

        Assert::IsTrue(search.Results().GetItemAt(2)->IsSelected());
        Assert::IsTrue(search.Results().GetItemAt(3)->IsSelected());
    }

    TEST_METHOD(TestBookmarkSelectedText)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.memory.at(8) = 'T';
        search.memory.at(9) = 'E';
        search.memory.at(10) = 'S';
        search.memory.at(11) = 'T';
        search.SetSearchType(ra::services::SearchType::AsciiText);
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"TEST");

        search.ApplyFilter();
        Assert::AreEqual({ 0 }, search.GetScrollOffset());
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(MemSize::Text, search.ResultMemSize());

        Assert::AreEqual({ 1U }, search.Results().Count());
        search.Results().GetItemAt(0)->SetSelected(true);

        bool bSawDialog = false;
        search.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;
            Assert::AreEqual(std::wstring(L"Text bookmarks are not supported"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        search.BookmarkSelected();
        Assert::IsTrue(bSawDialog);
        Assert::AreEqual({ 1U }, search.Results().Count());

        const auto& pBookmarks = search.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 0U }, pBookmarks.Count());

        Assert::IsTrue(search.Results().GetItemAt(0)->IsSelected());
    }

    TEST_METHOD(TestPredefinedFilterRanges)
    {
        MemorySearchViewModelHarness search;

        // no memory ranges defined
        Assert::AreEqual({ 2U }, search.PredefinedFilterRanges().Count());
        Assert::AreEqual(0, search.PredefinedFilterRanges().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"All"), search.PredefinedFilterRanges().GetItemAt(0)->GetLabel());
        Assert::AreEqual(3, search.PredefinedFilterRanges().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Custom"), search.PredefinedFilterRanges().GetItemAt(1)->GetLabel());

        // only system range defined
        search.mockConsoleContext.AddMemoryRegion(0U, 0xFFFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockEmulatorContext.MockTotalMemorySizeChanged(0x10000U);

        Assert::AreEqual({ 3U }, search.PredefinedFilterRanges().Count());
        Assert::AreEqual(0, search.PredefinedFilterRanges().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"All"), search.PredefinedFilterRanges().GetItemAt(0)->GetLabel());
        Assert::AreEqual(1, search.PredefinedFilterRanges().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"System Memory (0x0000-0xffff)"), search.PredefinedFilterRanges().GetItemAt(1)->GetLabel());
        Assert::AreEqual(3, search.PredefinedFilterRanges().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Custom"), search.PredefinedFilterRanges().GetItemAt(2)->GetLabel());

        // system and game ranges defined
        search.mockConsoleContext.ResetMemoryRegions();
        search.mockConsoleContext.AddMemoryRegion(0U, 0xBFFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockConsoleContext.AddMemoryRegion(0xE000U, 0xEFFFU, ra::data::context::ConsoleContext::AddressType::SaveRAM);
        search.mockEmulatorContext.MockTotalMemorySizeChanged(0x10000U);

        Assert::AreEqual({ 4U }, search.PredefinedFilterRanges().Count());
        Assert::AreEqual(0, search.PredefinedFilterRanges().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"All"), search.PredefinedFilterRanges().GetItemAt(0)->GetLabel());
        Assert::AreEqual(1, search.PredefinedFilterRanges().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"System Memory (0x0000-0xbfff)"), search.PredefinedFilterRanges().GetItemAt(1)->GetLabel());
        Assert::AreEqual(2, search.PredefinedFilterRanges().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Game Memory (0xe000-0xefff)"), search.PredefinedFilterRanges().GetItemAt(2)->GetLabel());
        Assert::AreEqual(3, search.PredefinedFilterRanges().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Custom"), search.PredefinedFilterRanges().GetItemAt(3)->GetLabel());

        // multiple system and game ranges defined
        search.mockConsoleContext.ResetMemoryRegions();
        search.mockConsoleContext.AddMemoryRegion(0U, 0x7FFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockConsoleContext.AddMemoryRegion(0x8000U, 0xBFFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockConsoleContext.AddMemoryRegion(0xE000U, 0xEFFFU, ra::data::context::ConsoleContext::AddressType::SaveRAM);
        search.mockConsoleContext.AddMemoryRegion(0xF000U, 0xFFFFU, ra::data::context::ConsoleContext::AddressType::SaveRAM);
        search.mockEmulatorContext.MockTotalMemorySizeChanged(0x10000U);

        Assert::AreEqual({ 4U }, search.PredefinedFilterRanges().Count());
        Assert::AreEqual(0, search.PredefinedFilterRanges().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"All"), search.PredefinedFilterRanges().GetItemAt(0)->GetLabel());
        Assert::AreEqual(1, search.PredefinedFilterRanges().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"System Memory (0x0000-0xbfff)"), search.PredefinedFilterRanges().GetItemAt(1)->GetLabel());
        Assert::AreEqual(2, search.PredefinedFilterRanges().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Game Memory (0xe000-0xffff)"), search.PredefinedFilterRanges().GetItemAt(2)->GetLabel());
        Assert::AreEqual(3, search.PredefinedFilterRanges().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Custom"), search.PredefinedFilterRanges().GetItemAt(3)->GetLabel());
    }

    TEST_METHOD(TestPredefinedFilterRangeAll)
    {
        MemorySearchViewModelHarness search;
        search.mockConsoleContext.AddMemoryRegion(0U, 0x7FFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockConsoleContext.AddMemoryRegion(0xC000U, 0xCFFFU, ra::data::context::ConsoleContext::AddressType::SaveRAM);
        search.mockEmulatorContext.MockTotalMemorySizeChanged(0x10000U);
        search.SetFilterRange(L"0x0000-0x1FFF");

        search.SetPredefinedFilterRange(0);
        Assert::AreEqual(std::wstring(L""), search.GetFilterRange());
    }

    TEST_METHOD(TestPredefinedFilterRangeSystem)
    {
        MemorySearchViewModelHarness search;
        search.mockConsoleContext.AddMemoryRegion(0U, 0x7FFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockConsoleContext.AddMemoryRegion(0xC000U, 0xCFFFU, ra::data::context::ConsoleContext::AddressType::SaveRAM);
        search.mockEmulatorContext.MockTotalMemorySizeChanged(0x10000U);
        search.SetFilterRange(L"0x0000-0x1FFF");

        search.SetPredefinedFilterRange(1);
        Assert::AreEqual(std::wstring(L"0x0000-0x7fff"), search.GetFilterRange());
    }

    TEST_METHOD(TestPredefinedFilterRangeGame)
    {
        MemorySearchViewModelHarness search;
        search.mockConsoleContext.AddMemoryRegion(0U, 0x7FFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockConsoleContext.AddMemoryRegion(0xC000U, 0xCFFFU, ra::data::context::ConsoleContext::AddressType::SaveRAM);
        search.mockEmulatorContext.MockTotalMemorySizeChanged(0x10000U);
        search.SetFilterRange(L"0x0000-0x1FFF");

        search.SetPredefinedFilterRange(2);
        Assert::AreEqual(std::wstring(L"0xc000-0xcfff"), search.GetFilterRange());
    }

    TEST_METHOD(TestPredefinedFilterRangeCustom)
    {
        MemorySearchViewModelHarness search;
        search.mockConsoleContext.AddMemoryRegion(0U, 0x7FFFU, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        search.mockConsoleContext.AddMemoryRegion(0xC000U, 0xCFFFU, ra::data::context::ConsoleContext::AddressType::SaveRAM);
        search.mockEmulatorContext.MockTotalMemorySizeChanged(0x10000U);
        search.SetFilterRange(L"0x0000-0x1FFF");

        search.SetPredefinedFilterRange(0);
        Assert::AreEqual(std::wstring(L""), search.GetFilterRange());

        Assert::AreEqual(3, search.PredefinedFilterRanges().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Custom (0x0000-0x1fff)"), search.PredefinedFilterRanges().GetItemAt(3)->GetLabel());

        search.SetPredefinedFilterRange(3);
        Assert::AreEqual(std::wstring(L"0x0000-0x1fff"), search.GetFilterRange());
    }

    TEST_METHOD(TestToggleContinousFilter)
    {
        MemorySearchViewModelHarness search;

        Assert::IsFalse(search.CanFilter());
        Assert::IsFalse(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Continuous Filter"), search.ContinuousFilterLabel());

        search.InitializeMemory();
        search.BeginNewSearch();

        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Continuous Filter"), search.ContinuousFilterLabel());

        search.ToggleContinuousFilter();

        Assert::IsFalse(search.CanFilter());
        Assert::IsTrue(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Stop Filtering"), search.ContinuousFilterLabel());

        search.ToggleContinuousFilter();

        Assert::IsTrue(search.CanFilter());
        Assert::IsTrue(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Continuous Filter"), search.ContinuousFilterLabel());
    }

    TEST_METHOD(TestContinousFilterEightBitLastKnown)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);

        search.memory.at(10) = 9;
        search.memory.at(11) = 9;
        search.memory.at(12) = 9;
        search.memory.at(13) = 9;
        search.memory.at(14) = 9;
        search.memory.at(15) = 9;

        // enabling performs the first filter
        search.ToggleContinuousFilter();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 6U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"6"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 6U }, search.Results().Count());
        AssertRow(search, 0, 10U, L"0x000a", L"0x09");
        AssertRow(search, 1, 11U, L"0x000b", L"0x09");
        AssertRow(search, 2, 12U, L"0x000c", L"0x09");
        AssertRow(search, 3, 13U, L"0x000d", L"0x09");
        AssertRow(search, 4, 14U, L"0x000e", L"0x09");
        AssertRow(search, 5, 15U, L"0x000f", L"0x09");

        search.memory.at(10) = 7;
        search.memory.at(11) = 7;
        search.memory.at(12) = 7;
        search.memory.at(14) = 7;
        search.memory.at(15) = 7;

        // next filter is performed in DoFrame - should not create a new page
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 5U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"5"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 5U }, search.Results().Count());
        AssertRow(search, 0, 10U, L"0x000a", L"0x07");
        AssertRow(search, 1, 11U, L"0x000b", L"0x07");
        AssertRow(search, 2, 12U, L"0x000c", L"0x07");
        AssertRow(search, 3, 14U, L"0x000e", L"0x07");
        AssertRow(search, 4, 15U, L"0x000f", L"0x07");

        Assert::IsFalse(search.CanFilter());
        Assert::IsTrue(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Stop Filtering"), search.ContinuousFilterLabel());

        search.memory.at(11) = 4;

        // next filter is performed in DoFrame - should not create a new page
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 11U, L"0x000b", L"0x04");

        Assert::IsFalse(search.CanFilter());
        Assert::IsTrue(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Stop Filtering"), search.ContinuousFilterLabel());

        // when no results remain, continuous filtering is automatically disabled
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 0U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"0"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Last Known"), search.GetFilterSummary());

        Assert::AreEqual({ 0U }, search.Results().Count());

        Assert::IsFalse(search.CanFilter());
        Assert::IsFalse(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Continuous Filter"), search.ContinuousFilterLabel());
    }

    TEST_METHOD(TestContinousFilterEightBitInitialValue)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.SetValueType(ra::services::SearchFilterType::InitialValue);

        search.memory.at(10) = 9;
        search.memory.at(11) = 9;
        search.memory.at(12) = 9;
        search.memory.at(13) = 9;
        search.memory.at(14) = 9;
        search.memory.at(15) = 9;

        // enabling performs the first filter
        search.ToggleContinuousFilter();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 6U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"6"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 6U }, search.Results().Count());
        AssertRow(search, 0, 10U, L"0x000a", L"0x09");
        AssertRow(search, 1, 11U, L"0x000b", L"0x09");
        AssertRow(search, 2, 12U, L"0x000c", L"0x09");
        AssertRow(search, 3, 13U, L"0x000d", L"0x09");
        AssertRow(search, 4, 14U, L"0x000e", L"0x09");
        AssertRow(search, 5, 15U, L"0x000f", L"0x09");

        search.memory.at(10) = 10;
        search.memory.at(12) = 7;
        search.memory.at(14) = 14;
        search.memory.at(15) = 7;

        // next filter is performed in DoFrame - should not create a new page
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 4U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"4"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 4U }, search.Results().Count());
        AssertRow(search, 0, 11U, L"0x000b", L"0x09");
        AssertRow(search, 1, 12U, L"0x000c", L"0x07");
        AssertRow(search, 2, 13U, L"0x000d", L"0x09");
        AssertRow(search, 3, 15U, L"0x000f", L"0x07");

        Assert::IsFalse(search.CanFilter());
        Assert::IsTrue(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Stop Filtering"), search.ContinuousFilterLabel());

        search.memory.at(11) = 11;
        search.memory.at(12) = 12;
        search.memory.at(13) = 3;
        search.memory.at(15) = 15;

        // next filter is performed in DoFrame - should not create a new page
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 1U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"1"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 1U }, search.Results().Count());
        AssertRow(search, 0, 13U, L"0x000d", L"0x03");

        Assert::IsFalse(search.CanFilter());
        Assert::IsTrue(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Stop Filtering"), search.ContinuousFilterLabel());

        search.memory.at(13) = 13;

        // when no results remain, continuous filtering is automatically disabled
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"1/1"), search.GetSelectedPage());
        Assert::AreEqual({ 0U }, search.GetResultCount());
        Assert::AreEqual(std::wstring(L"0"), search.GetResultCountText());
        Assert::AreEqual(MemSize::EightBit, search.ResultMemSize());
        Assert::AreEqual(std::wstring(L"!= Initial"), search.GetFilterSummary());

        Assert::AreEqual({ 0U }, search.Results().Count());

        Assert::IsFalse(search.CanFilter());
        Assert::IsFalse(search.CanContinuousFilter());
        Assert::AreEqual(std::wstring(L"Continuous Filter"), search.ContinuousFilterLabel());
    }


    TEST_METHOD(TestOnCodeNoteChanged)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.mockConsoleContext.AddMemoryRegion({ 0U }, { 0xFFU }, ra::data::context::ConsoleContext::AddressType::SystemRAM, "System RAM");
        search.BeginNewSearch();

        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::Constant);
        search.SetFilterValue(L"12");

        search.ApplyFilter();

        Assert::AreEqual({ 1U }, search.Results().Count());
        auto* pRow = search.Results().GetItemAt(0);
        Assert::IsNotNull(pRow);
        Ensures(pRow != nullptr);

        Assert::AreEqual({ 12U }, pRow->nAddress);
        Assert::AreEqual(std::wstring(L"System RAM"), pRow->GetDescription());
        Assert::IsFalse(pRow->bHasCodeNote);

        search.mockGameContext.SetCodeNote({ 12U }, L"Note");
        Assert::AreEqual(std::wstring(L"Note"), pRow->GetDescription());
        Assert::IsTrue(pRow->bHasCodeNote);

        search.mockGameContext.SetCodeNote({ 12U }, L"Note 2");
        Assert::AreEqual(std::wstring(L"Note 2"), pRow->GetDescription());
        Assert::IsTrue(pRow->bHasCodeNote);

        search.mockGameContext.SetCodeNote({ 12U }, L"");
        Assert::AreEqual(std::wstring(L"System RAM"), pRow->GetDescription());
        Assert::IsFalse(pRow->bHasCodeNote);
    }

    TEST_METHOD(TestScrollDisplaysCurrentValue)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        search.memory.at(2) = 9;
        search.DoFrame();

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 0U, L"0x0000", L"0x00");
        AssertRow(search, 1, 1U, L"0x0001", L"0x01");
        AssertRow(search, 2, 2U, L"0x0002", L"0x09");

        search.SetScrollOffset(1U);

        Assert::IsTrue(search.Results().Count() > 3);
        AssertRow(search, 0, 1U, L"0x0001", L"0x01");
        AssertRow(search, 1, 2U, L"0x0002", L"0x09");
        AssertRow(search, 2, 3U, L"0x0003", L"0x03");
    }

    TEST_METHOD(TestGetTooltipEightBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        const auto& vmResult = *search.Results().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0x0002\n0x02 | Current\n0x02 | Last Filter\n0x02 | Initial"), search.GetTooltip(vmResult));

        search.memory.at(2) = 9;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\n0x09 | Current\n0x02 | Last Filter\n0x02 | Initial"), search.GetTooltip(vmResult));

        search.memory.at(2) = 0x0c;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\n0x0c | Current\n0x02 | Last Filter\n0x02 | Initial"), search.GetTooltip(vmResult));

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.ApplyFilter();

        Assert::AreEqual({ 1U }, search.Results().Count());
        const auto& vmResult2 = *search.Results().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"0x0002\n0x0c | Current\n0x0c | Last Filter\n0x02 | Initial"), search.GetTooltip(vmResult2));

        search.memory.at(2) = 0x35;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\n0x35 | Current\n0x0c | Last Filter\n0x02 | Initial"), search.GetTooltip(vmResult2));
    }

    TEST_METHOD(TestGetTooltipFourBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::FourBit);
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        const auto& vmResult = *search.Results().GetItemAt(4);
        Assert::AreEqual(std::wstring(L"0x0002L\n0x2 | Current\n0x2 | Last Filter\n0x2 | Initial"), search.GetTooltip(vmResult));

        search.memory.at(2) = 9;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002L\n0x9 | Current\n0x2 | Last Filter\n0x2 | Initial"), search.GetTooltip(vmResult));

        search.memory.at(2) = 0x0c;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002L\n0xc | Current\n0x2 | Last Filter\n0x2 | Initial"), search.GetTooltip(vmResult));

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.ApplyFilter();

        Assert::AreEqual({ 1U }, search.Results().Count());
        const auto& vmResult2 = *search.Results().GetItemAt(0);
        Assert::AreEqual(std::wstring(L"0x0002L\n0xc | Current\n0xc | Last Filter\n0x2 | Initial"), search.GetTooltip(vmResult2));

        search.memory.at(2) = 0x35;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002L\n0x5 | Current\n0xc | Last Filter\n0x2 | Initial"), search.GetTooltip(vmResult2));
    }

    TEST_METHOD(TestGetTooltipSixteenBit)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.SetSearchType(ra::services::SearchType::SixteenBit);
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        const auto& vmResult = *search.Results().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0x0002\n0x0302 | Current\n0x0302 | Last Filter\n0x0302 | Initial"), search.GetTooltip(vmResult));

        search.memory.at(2) = 9;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\n0x0309 | Current\n0x0302 | Last Filter\n0x0302 | Initial"), search.GetTooltip(vmResult));

        search.memory.at(3) = 0x0c;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\n0x0c09 | Current\n0x0302 | Last Filter\n0x0302 | Initial"), search.GetTooltip(vmResult));

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.ApplyFilter();

        Assert::AreEqual({ 3U }, search.Results().Count()); // 01=0901 02=0c09 03=030c
        const auto& vmResult2 = *search.Results().GetItemAt(1);
        Assert::AreEqual(std::wstring(L"0x0002\n0x0c09 | Current\n0x0c09 | Last Filter\n0x0302 | Initial"), search.GetTooltip(vmResult2));

        search.memory.at(2) = 0x35;
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\n0x0c35 | Current\n0x0c09 | Last Filter\n0x0302 | Initial"), search.GetTooltip(vmResult2));
    }

    TEST_METHOD(TestGetTooltipText)
    {
        MemorySearchViewModelHarness search;
        search.InitializeMemory();
        search.memory.at(2) = 'T';
        search.memory.at(3) = 'E';
        search.memory.at(4) = 'S';
        search.memory.at(5) = 'T';
        search.memory.at(6) = 0;
        search.SetSearchType(ra::services::SearchType::AsciiText);
        search.BeginNewSearch();
        search.SetComparisonType(ComparisonType::Equals);
        search.SetValueType(ra::services::SearchFilterType::LastKnownValue);
        search.ApplyFilter();

        const auto& vmResult = *search.Results().GetItemAt(2);
        Assert::AreEqual(std::wstring(L"0x0002\nTEST | Current\nTEST | Last Filter\nTEST | Initial"), search.GetTooltip(vmResult));

        search.memory.at(4) = 'X';
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\nTEXT | Current\nTEST | Last Filter\nTEST | Initial"), search.GetTooltip(vmResult));

        search.memory.at(2) = 'N';
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\nNEXT | Current\nTEST | Last Filter\nTEST | Initial"), search.GetTooltip(vmResult));

        search.SetComparisonType(ComparisonType::NotEqualTo);
        search.ApplyFilter();

        const auto& vmResult2 = *search.Results().GetItemAt(1); // 01=?NEXT 02=NEXT 03=EXT 04=XT
        Assert::AreEqual(std::wstring(L"0x0002\nNEXT | Current\nNEXT | Last Filter\nTEST | Initial"), search.GetTooltip(vmResult2));

        search.memory.at(4) = 'A';
        search.DoFrame();
        Assert::AreEqual(std::wstring(L"0x0002\nNEAT | Current\nNEXT | Last Filter\nTEST | Initial"), search.GetTooltip(vmResult2));
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
