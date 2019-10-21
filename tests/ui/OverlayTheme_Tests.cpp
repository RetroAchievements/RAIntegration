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

    std::map<const char*, std::function<ra::ui::Color(const OverlayTheme&)>> PopupColorProperties = {
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

    std::map<const char*, std::function<ra::ui::Color(const OverlayTheme&)>> OverlayColorProperties = {
        {"Panel", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlayPanel(); } },
        {"Text", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlayText(); } },
        {"DisabledText", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlayDisabledText(); } },
        {"SubText", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlaySubText(); } },
        {"DisabledSubText", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlayDisabledSubText(); } },
        {"SelectionBackground", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlaySelectionBackground(); } },
        {"SelectionText", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlaySelectionText(); } },
        {"SelectionDisabledText", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlaySelectionDisabledText(); } },
        {"ScrollBar", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlayScrollBar(); } },
        {"ScrollBarGripper", [](const OverlayTheme& pTheme) noexcept { return pTheme.ColorOverlayScrollBarGripper(); } },
    };

    std::map<const char*, std::function<int(const OverlayTheme&)>> PopupFontSizeProperties = {
        {"Title", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizePopupTitle(); } },
        {"Subtitle", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizePopupSubtitle(); } },
        {"Detail", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizePopupDetail(); } },
        {"LeaderboardTitle", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizePopupLeaderboardTitle(); } },
        {"LeaderboardEntry", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizePopupLeaderboardEntry(); } },
        {"LeaderboardTracker", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizePopupLeaderboardTracker(); } },
    };

    std::map<const char*, std::function<int(const OverlayTheme&)>> OverlayFontSizeProperties = {
        {"Title", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizeOverlayTitle(); } },
        {"Header", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizeOverlayHeader(); } },
        {"Summary", [](const OverlayTheme& pTheme) noexcept { return pTheme.FontSizeOverlaySummary(); } },
    };

public:
    TEST_METHOD(TestLoadFromFileNoFile)
    {
        OverlayTheme defaultTheme;
        OverlayThemeHarness theme;
        theme.LoadFromFile();

        for (const auto& pair : PopupColorProperties)
        {
            const Color nDefaultColor = pair.second(defaultTheme);
            const Color nLoadedColor = pair.second(theme);
            Assert::AreEqual(nDefaultColor.ARGB, nLoadedColor.ARGB, ra::Widen(pair.first).c_str());
        }

        for (const auto& pair : OverlayColorProperties)
        {
            const Color nDefaultColor = pair.second(defaultTheme);
            const Color nLoadedColor = pair.second(theme);
            Assert::AreEqual(nDefaultColor.ARGB, nLoadedColor.ARGB, ra::Widen(pair.first).c_str());
        }

        for (const auto& pair : PopupFontSizeProperties)
        {
            const auto nDefaultSize = pair.second(defaultTheme);
            const auto nLoadedSize = pair.second(theme);
            Assert::AreEqual(nDefaultSize, nLoadedSize, ra::Widen(pair.first).c_str());
        }

        for (const auto& pair : OverlayFontSizeProperties)
        {
            const auto nDefaultSize = pair.second(defaultTheme);
            const auto nLoadedSize = pair.second(theme);
            Assert::AreEqual(nDefaultSize, nLoadedSize, ra::Widen(pair.first).c_str());
        }

        Assert::AreEqual(defaultTheme.Transparent(), theme.Transparent());
    }

    TEST_METHOD(TestLoadFromFileInvalidJson)
    {
        OverlayTheme defaultTheme;
        OverlayThemeHarness theme;

        theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json", "Background:#FFFFFF");
        theme.LoadFromFile();

        for (const auto& pair : PopupColorProperties)
        {
            const Color nDefaultColor = pair.second(defaultTheme);
            const Color nLoadedColor = pair.second(theme);
            Assert::AreEqual(nDefaultColor.ARGB, nLoadedColor.ARGB, ra::Widen(pair.first).c_str());
        }

        for (const auto& pair : OverlayColorProperties)
        {
            const Color nDefaultColor = pair.second(defaultTheme);
            const Color nLoadedColor = pair.second(theme);
            Assert::AreEqual(nDefaultColor.ARGB, nLoadedColor.ARGB, ra::Widen(pair.first).c_str());
        }
    }

    TEST_METHOD(TestLoadFromFileIndividualColors)
    {
        OverlayTheme defaultTheme;

        for (const auto& pair : PopupColorProperties)
        {
            OverlayThemeHarness theme;
            theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json",
                ra::StringPrintf("{\"Popup\":{\"Colors\":{\"%s\":\"#123456\"}}}", pair.first));
            theme.LoadFromFile();

            for (const auto& pair2 : PopupColorProperties)
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

        for (const auto& pair : OverlayColorProperties)
        {
            OverlayThemeHarness theme;
            theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json",
                ra::StringPrintf("{\"Overlay\":{\"Colors\":{\"%s\":\"#123456\"}}}", pair.first));
            theme.LoadFromFile();

            for (const auto& pair2 : OverlayColorProperties)
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

    TEST_METHOD(TestLoadFromFileIndividualFonts)
    {
        {
            OverlayThemeHarness theme;
            theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json",
                "{\"Popup\":{\"Font\":\"Banana\"}}");
            theme.LoadFromFile();

            Assert::AreEqual(std::string("Banana"), theme.FontPopup());
        }

        {
            OverlayThemeHarness theme;
            theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json",
                "{\"Overlay\":{\"Font\":\"Banana\"}}");
            theme.LoadFromFile();

            Assert::AreEqual(std::string("Banana"), theme.FontOverlay());
        }
    }

    TEST_METHOD(TestLoadFromFileIndividualFontSizes)
    {
        OverlayTheme defaultTheme;

        for (const auto& pair : PopupFontSizeProperties)
        {
            OverlayThemeHarness theme;
            theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json",
                ra::StringPrintf("{\"Popup\":{\"FontSizes\":{\"%s\":99}}}", pair.first));
            theme.LoadFromFile();

            for (const auto& pair2 : PopupFontSizeProperties)
            {
                const auto nDefaultSize = pair2.second(defaultTheme);
                const auto nLoadedSize = pair2.second(theme);

                auto sMessage = ra::Widen(pair.first);
                if (pair.first == pair2.first)
                    Assert::AreEqual(99, nLoadedSize, sMessage.c_str());
                else
                    Assert::AreEqual(nDefaultSize, nLoadedSize, sMessage.c_str());
            }
        }

        for (const auto& pair : OverlayFontSizeProperties)
        {
            OverlayThemeHarness theme;
            theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json",
                ra::StringPrintf("{\"Overlay\":{\"FontSizes\":{\"%s\":99}}}", pair.first));
            theme.LoadFromFile();

            for (const auto& pair2 : OverlayFontSizeProperties)
            {
                const auto nDefaultSize = pair2.second(defaultTheme);
                const auto nLoadedSize = pair2.second(theme);

                auto sMessage = ra::Widen(pair.first);
                if (pair.first == pair2.first)
                    Assert::AreEqual(99, nLoadedSize, sMessage.c_str());
                else
                    Assert::AreEqual(nDefaultSize, nLoadedSize, sMessage.c_str());
            }
        }
    }

    TEST_METHOD(TestLoadFromFileTransparent)
    {
        OverlayThemeHarness theme;

        theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json", "{\"Transparent\":false}");
        theme.LoadFromFile();
        Assert::AreEqual(theme.Transparent(), false);

        theme.mockFileSystem.MockFile(L".\\Overlay\\theme.json", "{\"Transparent\":true}");
        theme.LoadFromFile();
        Assert::AreEqual(theme.Transparent(), true);
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
