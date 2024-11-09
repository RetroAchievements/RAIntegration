#include "CppUnitTest.h"

#include "api\DeleteCodeNote.hh"
#include "api\UpdateCodeNote.hh"

#include "ui\viewmodels\PointerInspectorViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(PointerInspectorViewModel_Tests)
{
private:
    class PointerInspectorViewModelHarness : public PointerInspectorViewModel
    {
    public:
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;

        std::array<unsigned char, 32> memory{};

        GSL_SUPPRESS_F6 PointerInspectorViewModelHarness()
        {
            InitializeNotifyTargets();

            for (size_t i = 0; i < memory.size(); ++i)
                memory.at(i) = gsl::narrow_cast<unsigned char>(i);

            mockEmulatorContext.MockMemory(memory);

            mockUserContext.SetUsername("Author");

            mockGameContext.InitializeCodeNotes();
        }

        ~PointerInspectorViewModelHarness() = default;

        PointerInspectorViewModelHarness(const PointerInspectorViewModelHarness&) noexcept = delete;
        PointerInspectorViewModelHarness& operator=(const PointerInspectorViewModelHarness&) noexcept = delete;
        PointerInspectorViewModelHarness(PointerInspectorViewModelHarness&&) noexcept = delete;
        PointerInspectorViewModelHarness& operator=(PointerInspectorViewModelHarness&&) noexcept = delete;

        const std::wstring* FindCodeNote(ra::ByteAddress nAddress) const
        {
            const auto* pCodeNotes = mockGameContext.Assets().FindCodeNotes();
            return (pCodeNotes != nullptr) ? pCodeNotes->FindCodeNote(nAddress) : nullptr;
        }
    };


public:
    TEST_METHOD(TestInitialValues)
    {
        PointerInspectorViewModelHarness inspector;

        Assert::AreEqual({ 0U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0000"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddress)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddress({ 3U });

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddressText)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddressText(L"3");

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"3"), inspector.GetCurrentAddressText()); /* don't update text when user types in partial address */
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddressWithNote)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddress({ 3U });
        inspector.mockGameContext.Assets().FindCodeNotes()->SetServerCodeNote({3U}, L"Note on 3");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({3U}, L"Note on 3");

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"Note on 3"), inspector.GetCurrentAddressNote());

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({3U}, L"Modified Note on 3");

        Assert::AreEqual(std::wstring(L"Modified Note on 3"), inspector.GetCurrentAddressNote());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
