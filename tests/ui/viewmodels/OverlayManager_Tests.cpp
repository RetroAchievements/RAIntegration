#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayManager.hh"

#include "ui\OverlayTheme.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayTheme.hh"
#include "tests\mocks\MockSurface.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

#include "tests\ui\UIAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(OverlayManager_Tests)
{
private:
    class OverlayManagerHarness : public OverlayManager
    {
    public:
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::mocks::MockOverlayTheme mockTheme;
        ra::ui::drawing::mocks::MockSurfaceFactory mockSurfaceFactory;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        OverlayManagerHarness() noexcept
            : m_Override(this)
        {
            GSL_SUPPRESS_F6 SetRenderRequestHandler([this]()
            {
                m_bRenderRequested = true;
            });

            GSL_SUPPRESS_F6 SetShowRequestHandler([this]()
            {
                m_bShowRequested = true;
            });

            GSL_SUPPRESS_F6 SetHideRequestHandler([this]()
            {
                m_bHideRequested = true;
            });
        }

        bool WasRenderRequested() const noexcept { return m_bRenderRequested; }
        bool WasShowRequested() const noexcept { return m_bShowRequested; }
        bool WasHideRequested() const noexcept { return m_bHideRequested; }

        void ResetRenderRequested() noexcept
        {
            m_bRenderRequested = false;
            m_bShowRequested = false;
            m_bHideRequested = false;
        }

        int GetOverlayRenderX() const { return m_vmOverlay.GetHorizontalOffset(); }

        ra::ui::viewmodels::ProgressTrackerViewModel* GetProgressTracker() noexcept
        {
            return m_vmProgressTracker.get();
        }

    private:
        bool m_bRenderRequested = false;
        bool m_bShowRequested = false;
        bool m_bHideRequested = false;
        ra::services::ServiceLocator::ServiceOverride<OverlayManager> m_Override;
    };

public:
    TEST_METHOD(TestQueueMessageTitleDesc)
    {
        OverlayManagerHarness overlay;
        const auto nId = overlay.QueueMessage(L"Title", L"Description");

        const auto* pPopup = overlay.GetMessage(nId);
        Expects(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Title"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Description"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L""), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());

        Assert::IsTrue(overlay.WasRenderRequested());
    }

    TEST_METHOD(TestQueueMessageTitleDescDetail)
    {
        OverlayManagerHarness overlay;
        const auto nId = overlay.QueueMessage(L"Title", L"Description", L"Detail");

        const auto* pPopup = overlay.GetMessage(nId);
        Expects(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Title"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Description"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Detail"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::None, pPopup->GetImage().Type());

        Assert::IsTrue(overlay.WasRenderRequested());
    }

    TEST_METHOD(TestQueueMessageTitleDescImage)
    {
        OverlayManagerHarness overlay;
        const auto nId = overlay.QueueMessage(L"Title", L"Description", ra::ui::ImageType::Badge, "BadgeURI");

        const auto* pPopup = overlay.GetMessage(nId);
        Expects(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Title"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Description"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L""), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("BadgeURI"), pPopup->GetImage().Name());

        Assert::IsTrue(overlay.WasRenderRequested());
    }

    TEST_METHOD(TestQueueMessageTitleDescDetailImage)
    {
        OverlayManagerHarness overlay;
        const auto nId = overlay.QueueMessage(L"Title", L"Description", L"Detail", ra::ui::ImageType::Badge, "BadgeURI");

        const auto* pPopup = overlay.GetMessage(nId);
        Expects(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Title"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Description"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Detail"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::Badge, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("BadgeURI"), pPopup->GetImage().Name());

        Assert::IsTrue(overlay.WasRenderRequested());
    }

    TEST_METHOD(TestRenderPopup)
    {
        OverlayManagerHarness overlay;
        const auto nId = overlay.QueueMessage(L"Title", L"Description");
        const auto* pPopup = overlay.GetMessage(nId);
        Expects(pPopup != nullptr);

        // Queuing the message requests the overlay be shown and rendered
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsTrue(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());
        overlay.ResetRenderRequested();

        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);
        Assert::AreEqual(pPopup->GetVerticalOffset(), -74);
        // a queued popup still requests a render even if it's not visible. this ensures the Render
        // callback will be called to animate the popup
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());

        // 0.8 seconds to fully visualize, expect it to be partially onscreen after 0.5 seconds
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsTrue(pPopup->GetVerticalOffset() > -74);
        Assert::IsTrue(pPopup->GetVerticalOffset() < 10);
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());

        // after 1 second, it should be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(pPopup->GetVerticalOffset(), 10);
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());

        // after 2 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(pPopup->GetVerticalOffset(), 10);
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());

        // after 3 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(pPopup->GetVerticalOffset(), 10);
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());

        // after 4 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(pPopup->GetVerticalOffset(), 10);
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());

        // after 4.5 seconds, it should be starting to scroll offscreen
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsTrue(pPopup->GetVerticalOffset() > -74);
        Assert::IsTrue(pPopup->GetVerticalOffset() < 10);
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsFalse(overlay.WasHideRequested());
        Assert::IsNotNull(overlay.GetMessage(nId));

        // after 5 seconds, it should be fully offscreen and removed from the queue
        // since it's removed from the queue, we can't read RenderLocationY.
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsFalse(overlay.WasRenderRequested());
        Assert::IsFalse(overlay.WasShowRequested());
        Assert::IsTrue(overlay.WasHideRequested());
        Assert::IsNull(overlay.GetMessage(nId));
    }

    TEST_METHOD(TestAddRemoveScoreTrackerLeaderboardEnabled)
    {
        OverlayManagerHarness overlay;
        overlay.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker, ra::ui::viewmodels::PopupLocation::BottomRight);

        const auto& vmScoreTracker = overlay.AddScoreTracker(3);
        Assert::AreEqual(3, vmScoreTracker.GetPopupId());
        Assert::AreEqual(std::wstring(L"0"), vmScoreTracker.GetDisplayText());
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(vmScoreTracker.IsDestroyPending());
        Assert::AreEqual(10, vmScoreTracker.GetVerticalOffset());

        Assert::IsTrue(&vmScoreTracker == overlay.GetScoreTracker(3));

        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, vmScoreTracker.GetVerticalOffset());

        overlay.ResetRenderRequested();
        overlay.RemoveScoreTracker(3);
        Assert::IsTrue(vmScoreTracker.IsDestroyPending()); // has to erase itself in render
        Assert::IsFalse(overlay.WasRenderRequested());
        Assert::IsNotNull(overlay.GetScoreTracker(3));

        overlay.Render(mockSurface, false);
        Assert::IsNull(overlay.GetScoreTracker(3)); // render should erase and destroy
    }

    TEST_METHOD(TestAddRemoveScoreTrackerLeaderboardDisabled)
    {
        OverlayManagerHarness overlay;
        overlay.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker, ra::ui::viewmodels::PopupLocation::None);

        const auto& vmScoreTracker = overlay.AddScoreTracker(3);
        Assert::AreEqual(3, vmScoreTracker.GetPopupId());
        Assert::AreEqual(std::wstring(L"0"), vmScoreTracker.GetDisplayText());
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(vmScoreTracker.IsDestroyPending());
        Assert::AreEqual(10, vmScoreTracker.GetVerticalOffset());

        Assert::IsTrue(&vmScoreTracker == overlay.GetScoreTracker(3));

        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, vmScoreTracker.GetVerticalOffset());

        overlay.ResetRenderRequested();
        overlay.RemoveScoreTracker(3);
        Assert::IsTrue(vmScoreTracker.IsDestroyPending()); // has to erase itself in render
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsNotNull(overlay.GetScoreTracker(3));

        overlay.Render(mockSurface, false);
        Assert::IsNull(overlay.GetScoreTracker(3)); // render should erase and destroy
    }

    TEST_METHOD(TestQueueScoreboard)
    {
        OverlayManagerHarness overlay;
        {
            ScoreboardViewModel vmScoreboard;
            vmScoreboard.SetHeaderText(L"HeaderText");
            overlay.QueueScoreboard(3, std::move(vmScoreboard));
        }

        const auto* pScoreboard = overlay.GetScoreboard(3);
        Expects(pScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"HeaderText"), pScoreboard->GetHeaderText());
        Assert::IsTrue(overlay.WasRenderRequested());
    }

    TEST_METHOD(TestRenderScoreboard)
    {
        OverlayManagerHarness overlay;
        {
            ScoreboardViewModel vmScoreboard;
            vmScoreboard.SetHeaderText(L"HeaderText");
            overlay.QueueScoreboard(3, std::move(vmScoreboard));
        }

        const auto* pScoreboard = overlay.GetScoreboard(3);
        Expects(pScoreboard != nullptr);

        constexpr auto nScoreboardWidth = 288;

        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);
        Assert::AreEqual(pScoreboard->GetHorizontalOffset(), 10 - nScoreboardWidth);
        // no time has elapsed since the popup was queued, we're still waiting for the first render
        Assert::IsTrue(overlay.WasRenderRequested());

        // 0.8 seconds to fully visualize, expect it to be partially onscreen after 0.5 seconds
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsTrue(pScoreboard->GetHorizontalOffset() > 10 - nScoreboardWidth);
        Assert::IsTrue(pScoreboard->GetHorizontalOffset() < 10);
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 1 second, it should be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, pScoreboard->GetHorizontalOffset());
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 2 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, pScoreboard->GetHorizontalOffset());
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 3 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, pScoreboard->GetHorizontalOffset());
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 4 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, pScoreboard->GetHorizontalOffset());
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 5 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, pScoreboard->GetHorizontalOffset());
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 6 seconds, it should still be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::seconds(1));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, pScoreboard->GetHorizontalOffset());
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 6.5 seconds, it should be starting to scroll offscreen
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsTrue(pScoreboard->GetHorizontalOffset() > 10 - nScoreboardWidth);
        Assert::IsTrue(pScoreboard->GetHorizontalOffset() < 10);
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsNotNull(overlay.GetScoreboard(3));

        // after 7 seconds, it should be fully offscreen and removed from the queue
        // since it's removed from the queue, we can't read RenderLocationY.
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(500));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsFalse(overlay.WasRenderRequested());
        Assert::IsNull(overlay.GetScoreboard(3));
    }

    TEST_METHOD(TestAddRemoveChallengeIndicator)
    {
        OverlayManagerHarness overlay;
        overlay.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge, ra::ui::viewmodels::PopupLocation::BottomRight);

        const auto& vmIndicator = overlay.AddChallengeIndicator(6, ra::ui::ImageType::Badge, "12345");
        Assert::AreEqual(6, vmIndicator.GetPopupId());
        Assert::AreEqual(ra::ui::ImageType::Badge, vmIndicator.GetImage().Type());
        Assert::AreEqual(std::string("12345"), vmIndicator.GetImage().Name());
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(vmIndicator.IsDestroyPending());
        Assert::AreEqual(10, vmIndicator.GetVerticalOffset());

        Assert::IsTrue(&vmIndicator == overlay.GetChallengeIndicator(6));

        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, vmIndicator.GetVerticalOffset());

        overlay.ResetRenderRequested();
        overlay.RemoveChallengeIndicator(6);
        Assert::IsTrue(vmIndicator.IsDestroyPending()); // has to erase itself in render
        Assert::IsFalse(overlay.WasRenderRequested());
        Assert::IsNotNull(overlay.GetChallengeIndicator(6));

        overlay.Render(mockSurface, false);
        Assert::IsNull(overlay.GetChallengeIndicator(6)); // render should erase and destroy
    }

    TEST_METHOD(TestAddRemoveChallengeIndicatorDestroyPending)
    {
        OverlayManagerHarness overlay;
        overlay.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge, ra::ui::viewmodels::PopupLocation::BottomRight);

        const auto& vmIndicator = overlay.AddChallengeIndicator(6, ra::ui::ImageType::Badge, "12345");
        Assert::IsFalse(vmIndicator.IsDestroyPending());
        Assert::IsTrue(&vmIndicator == overlay.GetChallengeIndicator(6));

        overlay.RemoveChallengeIndicator(6);
        Assert::IsTrue(vmIndicator.IsDestroyPending());
        Assert::IsNotNull(overlay.GetChallengeIndicator(6));

        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);
        overlay.ResetRenderRequested();

        // request to redisplay indicator (it's currently queued to be destroyed)
        const auto& vmNewIndicator = overlay.AddChallengeIndicator(6, ra::ui::ImageType::Badge, "12345");
        Assert::AreEqual(6, vmNewIndicator.GetPopupId());
        Assert::AreEqual(ra::ui::ImageType::Badge, vmNewIndicator.GetImage().Type());
        Assert::AreEqual(std::string("12345"), vmNewIndicator.GetImage().Name());
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(vmNewIndicator.IsDestroyPending());

        overlay.Render(mockSurface, false);

        // indicator should exist
        Assert::IsTrue(&vmNewIndicator == overlay.GetChallengeIndicator(6));
    }

    TEST_METHOD(TestAddMultipleChallengeIndicator)
    {
        OverlayManagerHarness overlay;
        overlay.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge, ra::ui::viewmodels::PopupLocation::BottomRight);

        const auto& vmIndicator = overlay.AddChallengeIndicator(6, ra::ui::ImageType::Badge, "12345");
        const auto& vmIndicator2 = overlay.AddChallengeIndicator(7, ra::ui::ImageType::Badge, "22222");
        const auto& vmIndicator3 = overlay.AddChallengeIndicator(8, ra::ui::ImageType::Badge, "75319");

        Assert::AreEqual(6, vmIndicator.GetPopupId());
        Assert::AreEqual(ra::ui::ImageType::Badge, vmIndicator.GetImage().Type());
        Assert::AreEqual(std::string("12345"), vmIndicator.GetImage().Name());

        Assert::AreEqual(7, vmIndicator2.GetPopupId());
        Assert::AreEqual(ra::ui::ImageType::Badge, vmIndicator2.GetImage().Type());
        Assert::AreEqual(std::string("22222"), vmIndicator2.GetImage().Name());

        Assert::AreEqual(8, vmIndicator3.GetPopupId());
        Assert::AreEqual(ra::ui::ImageType::Badge, vmIndicator3.GetImage().Type());
        Assert::AreEqual(std::string("75319"), vmIndicator3.GetImage().Name());

        Assert::IsTrue(overlay.WasRenderRequested());
        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);

        constexpr int nSpacing = 10 + 32 + 2; // spacing + image + shadow
        Assert::AreEqual(10, vmIndicator.GetHorizontalOffset());
        Assert::AreEqual(10 + nSpacing, vmIndicator2.GetHorizontalOffset());
        Assert::AreEqual(10 + nSpacing * 2, vmIndicator3.GetHorizontalOffset());
        Assert::AreEqual(10, vmIndicator.GetVerticalOffset());
        Assert::AreEqual(10, vmIndicator2.GetVerticalOffset());
        Assert::AreEqual(10, vmIndicator3.GetVerticalOffset());

        overlay.RemoveChallengeIndicator(7);
        Assert::IsFalse(vmIndicator.IsDestroyPending());
        Assert::IsTrue(vmIndicator2.IsDestroyPending());
        Assert::IsFalse(vmIndicator3.IsDestroyPending());

        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, vmIndicator.GetHorizontalOffset());
        Assert::AreEqual(10 + nSpacing, vmIndicator3.GetHorizontalOffset());
        Assert::AreEqual(10, vmIndicator.GetVerticalOffset());
        Assert::AreEqual(10, vmIndicator3.GetVerticalOffset());
    }

    TEST_METHOD(TestAddExistingChallengeIndicator)
    {
        OverlayManagerHarness overlay;
        overlay.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Challenge, ra::ui::viewmodels::PopupLocation::BottomRight);

        const auto& vmIndicator = overlay.AddChallengeIndicator(6, ra::ui::ImageType::Badge, "12345");
        Assert::AreEqual(6, vmIndicator.GetPopupId());
        Assert::AreEqual(ra::ui::ImageType::Badge, vmIndicator.GetImage().Type());
        Assert::AreEqual(std::string("12345"), vmIndicator.GetImage().Name());
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(vmIndicator.IsDestroyPending());
        Assert::AreEqual(10, vmIndicator.GetVerticalOffset());

        Assert::IsTrue(&vmIndicator == overlay.GetChallengeIndicator(6));

        const auto& vmIndicator2 = overlay.AddChallengeIndicator(6, ra::ui::ImageType::Badge, "12345");
        Assert::IsTrue(&vmIndicator == &vmIndicator2);
    }

    TEST_METHOD(TestAddRemoveProgressIndicator)
    {
        OverlayManagerHarness overlay;
        overlay.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Progress, ra::ui::viewmodels::PopupLocation::BottomRight);

        overlay.UpdateProgressTracker(ra::ui::ImageType::Badge, "12345_lock", 3, 7, false);
        auto* pTracker = overlay.GetProgressTracker();
        Assert::IsNotNull(pTracker);
        Ensures(pTracker != nullptr);

        Assert::AreEqual(std::wstring(L"3/7"), pTracker->GetText());
        Assert::AreEqual(ra::ui::ImageType::Badge, pTracker->GetImage().Type());
        Assert::AreEqual(std::string("12345_lock"), pTracker->GetImage().Name());
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsFalse(pTracker->IsDestroyPending());
        Assert::AreEqual(10, pTracker->GetVerticalOffset());

        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        overlay.Render(mockSurface, false);
        Assert::AreEqual(10, pTracker->GetVerticalOffset());

        overlay.mockClock.AdvanceTime(std::chrono::seconds(3));
        overlay.Render(mockSurface, false);
        Assert::IsTrue(overlay.WasRenderRequested()); // destroy pending
        Assert::IsTrue(pTracker->IsDestroyPending());

        overlay.Render(mockSurface, false);
        pTracker = overlay.GetProgressTracker(); // destroy occurred
        Assert::IsNull(pTracker);
    }

    TEST_METHOD(TestShowHideOverlay)
    {
        OverlayManagerHarness overlay;
        ra::ui::drawing::mocks::MockSurface mockSurface(800, 600);
        Assert::IsFalse(overlay.IsOverlayFullyVisible());

        overlay.ShowOverlay();
        Assert::IsTrue(overlay.WasRenderRequested());
        Assert::IsTrue(overlay.WasShowRequested());

        // 0.4 seconds to fully visualize, expect it to be partially onscreen after 0.25 seconds
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(250));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsTrue(overlay.GetOverlayRenderX() < 0);
        Assert::IsTrue(overlay.GetOverlayRenderX() > -800);
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 0.5 seconds, it should be fully on screen
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(250));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(0, overlay.GetOverlayRenderX());
        Assert::IsFalse(overlay.WasRenderRequested());

        Assert::IsTrue(overlay.IsOverlayFullyVisible());

        overlay.HideOverlay();
        Assert::IsTrue(overlay.WasRenderRequested());
        overlay.Render(mockSurface, false); // force state change from FadeOutRequested to FadeOut

        // 0.4 seconds to fully collapse, expect it to be partially onscreen after 0.25 seconds
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(250));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::IsTrue(overlay.GetOverlayRenderX() < 0);
        Assert::IsTrue(overlay.GetOverlayRenderX() > -800);
        Assert::IsTrue(overlay.WasRenderRequested());

        // after 0.5 seconds, it should be fully off screen
        overlay.mockClock.AdvanceTime(std::chrono::milliseconds(250));
        overlay.ResetRenderRequested();
        overlay.Render(mockSurface, false);
        Assert::AreEqual(-800, overlay.GetOverlayRenderX());
        Assert::IsFalse(overlay.WasRenderRequested());
        Assert::IsTrue(overlay.WasHideRequested());

        Assert::IsFalse(overlay.IsOverlayFullyVisible());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
