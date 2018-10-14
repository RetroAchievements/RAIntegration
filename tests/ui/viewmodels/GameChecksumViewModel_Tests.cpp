#include "CppUnitTest.h"

#include "ui\viewmodels\GameChecksumViewModel.hh"

#include "services\ServiceLocator.hh"
#include "services\IClipboard.hh"

#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

std::string g_sCurrentROMMD5;

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
        const std::wstring& GetText() const { return m_sText; }

    private:
        ra::services::ServiceLocator::ServiceOverride<ra::services::IClipboard> m_Override;
        mutable std::wstring m_sText;
    };

public:
    TEST_METHOD(TestCopyToClipboard)
    {
        MockClipboard mockClipboard;
        GameChecksumViewModel vmChecksum;

        Assert::AreEqual(std::wstring(), vmChecksum.GetChecksum());
        Assert::AreEqual(std::wstring(), mockClipboard.GetText());

        vmChecksum.SetChecksum(L"abc123");
        Assert::AreEqual(std::wstring(L"abc123"), vmChecksum.GetChecksum());
        Assert::AreEqual(std::wstring(), mockClipboard.GetText());

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
