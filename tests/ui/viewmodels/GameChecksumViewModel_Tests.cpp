#include "CppUnitTest.h"

#include "ui\viewmodels\GameChecksumViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockClipboard.hh"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using ra::data::context::mocks::MockGameContext;
using ra::services::mocks::MockClipboard;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(GameChecksumViewModel_Tests)
{
public:
    TEST_METHOD(TestInitialValueFromGameContext)
    {
        MockGameContext mockGameContext;
        GameChecksumViewModel vmChecksum1;
        Assert::AreEqual(std::wstring(L"No game loaded."), vmChecksum1.GetChecksum());

        mockGameContext.SetGameId(123U);
        mockGameContext.SetGameHash("abc123");
        GameChecksumViewModel vmChecksum2;
        Assert::AreEqual(std::wstring(L"abc123"), vmChecksum2.GetChecksum());

        // NOTE: game ID does not have to be set - if the checksum doesn't match a known game, we still want to be able to view it.
        mockGameContext.SetGameId(0U);
        mockGameContext.SetGameHash("def345");
        GameChecksumViewModel vmChecksum3;
        Assert::AreEqual(std::wstring(L"def345"), vmChecksum3.GetChecksum());
    }

    TEST_METHOD(TestCopyToClipboard)
    {
        MockGameContext mockGameContext;
        MockClipboard mockClipboard;
        GameChecksumViewModel vmChecksum;

        Assert::AreEqual(std::wstring(L"No game loaded."), vmChecksum.GetChecksum());
        Assert::AreEqual(std::wstring(), mockClipboard.GetText());

        vmChecksum.CopyChecksumToClipboard();
        Assert::AreEqual(std::wstring(L"No game loaded."), vmChecksum.GetChecksum());
        Assert::AreEqual(std::wstring(L"No game loaded."), mockClipboard.GetText());

        vmChecksum.SetChecksum(L"abc123");
        Assert::AreEqual(std::wstring(L"abc123"), vmChecksum.GetChecksum());
        Assert::AreEqual(std::wstring(L"No game loaded."), mockClipboard.GetText());

        vmChecksum.CopyChecksumToClipboard();
        Assert::AreEqual(std::wstring(L"abc123"), vmChecksum.GetChecksum());
        Assert::AreEqual(std::wstring(L"abc123"), mockClipboard.GetText());

        vmChecksum.SetChecksum(L"def456");
        Assert::AreEqual(std::wstring(L"def456"), vmChecksum.GetChecksum());
        Assert::AreEqual(std::wstring(L"abc123"), mockClipboard.GetText());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
