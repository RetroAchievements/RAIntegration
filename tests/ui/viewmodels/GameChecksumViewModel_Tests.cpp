#include "CppUnitTest.h"

#include "ui\viewmodels\GameChecksumViewModel.hh"

#include "services\ServiceLocator.hh"
#include "services\IClipboard.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using ra::data::mocks::MockGameContext;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(GameChecksumViewModel_Tests)
{
private:
    class MockClipboard : public ra::services::IClipboard
    {
    public:
        MockClipboard() noexcept
            : m_Override(this)
        {
        }

        void SetText(const std::wstring& sValue) const override { m_sText = sValue; }
        const std::wstring& GetText() const noexcept { return m_sText; }

    private:
        ra::services::ServiceLocator::ServiceOverride<ra::services::IClipboard> m_Override;
        mutable std::wstring m_sText;
    };

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
