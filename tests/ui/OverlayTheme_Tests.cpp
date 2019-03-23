#include "ui\OverlayTheme.hh"

#include "tests\mocks\MockFileSystem.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace tests {

TEST_CLASS(OverlayTheme_Tests)
{
    class OverlayThemeHarness : public OverlayTheme
    {
    public:
        ra::services::mocks::MockFileSystem mockFileSystem;
    };

    std::map<const char*, std::function<ra::ui::Color(const OverlayTheme&)>> ColorProperties = {
        {"Background", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorBackground(); } },
        {"Border", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorBorder(); } },
        {"TextShadow", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorTextShadow(); } },
        {"Title", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorTitle(); } },
        {"Description", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorDescription(); } },
        {"Detail", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorDetail(); } },
        {"Error", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorError(); } },
        {"LeaderboardEntry", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorLeaderboardEntry(); } },
        {"LeaderboardPlayer", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorLeaderboardPlayer(); } },
    };

public:
    TEST_METHOD(TestLoadFromFileNoFile)
    {
        OverlayTheme defaultTheme;
        OverlayThemeHarness theme;
        theme.LoadFromFile();

        for (const auto& pair : ColorProperties)
        {
            const Color nDefaultColor = pair.second(defaultTheme);
            const Color nLoadedColor = pair.second(theme);
            Assert::AreEqual(nDefaultColor.ARGB, nLoadedColor.ARGB, ra::Widen(pair.first).c_str());
        }
    }

    TEST_METHOD(TestLoadFromFileInvalidJson)
    {
        OverlayTheme defaultTheme;
        OverlayThemeHarness theme;

        theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json", "Background:#FFFFFF");
        theme.LoadFromFile();

        for (const auto& pair : ColorProperties)
        {
            const Color nDefaultColor = pair.second(defaultTheme);
            const Color nLoadedColor = pair.second(theme);
            Assert::AreEqual(nDefaultColor.ARGB, nLoadedColor.ARGB, ra::Widen(pair.first).c_str());
        }
    }

    TEST_METHOD(TestLoadFromFileIndividualColors)
    {
        OverlayTheme defaultTheme;

        for (const auto& pair : ColorProperties)
        {
            OverlayThemeHarness theme;
            theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json",
                ra::StringPrintf("{\"Colors\":{\"%s\":\"#123456\"}}", pair.first));
            theme.LoadFromFile();

            for (const auto& pair2 : ColorProperties)
            {
                const Color nDefaultColor = pair2.second(defaultTheme);
                const Color nLoadedColor = pair2.second(theme);

                auto sMessage = ra::Widen(pair.first);
                if (pair.first == pair2.first)
                {
                    Assert::AreEqual((unsigned char)0xFF, nLoadedColor.Channel.A, sMessage.c_str());
                    Assert::AreEqual(0x123456U, nLoadedColor.ARGB & 0xFFFFFF, sMessage.c_str());
                }
                else
                {
                    Assert::AreEqual(nDefaultColor.ARGB, nLoadedColor.ARGB, sMessage.c_str());
                }
            }
        }
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
