#include "CppUnitTest.h"

#include "ui\viewmodels\BrokenAchievementsViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::ui::viewmodels::MessageBoxViewModel;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(BrokenAchievementsViewModel_Tests)
{
private:
    class BrokenAchievementsViewModelHarness : public BrokenAchievementsViewModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        BrokenAchievementsViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockWindowManager.AssetList.InitializeNotifyTargets();
        }


        void SetDefaultComment()
        {
            SetComment(L"The achievement failed to trigger when I jumped.");
        }

        void MockAchievements()
        {
            mockGameContext.SetGameId(1U);
            mockGameContext.SetGameTitle(L"GAME");
            mockGameContext.SetGameHash("HASH");

            auto& ach1 = mockGameContext.Assets().NewAchievement();
            ach1.SetCategory(ra::data::models::AssetCategory::Core);
            ach1.SetID(1U);
            ach1.SetName(L"Title1");
            ach1.SetState(ra::data::models::AssetState::Inactive);
            auto& ach2 = mockGameContext.Assets().NewAchievement();
            ach2.SetCategory(ra::data::models::AssetCategory::Core);
            ach2.SetID(2U);
            ach2.SetName(L"Title2");
            ach2.SetState(ra::data::models::AssetState::Active);
            auto& ach3 = mockGameContext.Assets().NewAchievement();
            ach3.SetCategory(ra::data::models::AssetCategory::Core);
            ach3.SetID(3U);
            ach3.SetName(L"Title3");
            ach3.SetState(ra::data::models::AssetState::Inactive);
            auto& ach4 = mockGameContext.Assets().NewAchievement();
            ach4.SetCategory(ra::data::models::AssetCategory::Core);
            ach4.SetID(4U);
            ach4.SetName(L"Title4");
            ach4.SetState(ra::data::models::AssetState::Active);
            auto& ach5 = mockGameContext.Assets().NewAchievement();
            ach5.SetCategory(ra::data::models::AssetCategory::Core);
            ach5.SetID(5U);
            ach5.SetName(L"Title5");
            ach5.SetState(ra::data::models::AssetState::Active);
            auto& ach6 = mockGameContext.Assets().NewAchievement();
            ach6.SetCategory(ra::data::models::AssetCategory::Core);
            ach6.SetID(6U);
            ach6.SetName(L"Title6");
            ach6.SetState(ra::data::models::AssetState::Active);

            Assert::IsTrue(InitializeAchievements());
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        Assert::AreEqual(0, vmBrokenAchievements.GetSelectedProblemId());
        Assert::AreEqual(std::wstring(L""), vmBrokenAchievements.GetComment());
        Assert::AreEqual({ 0U }, vmBrokenAchievements.Achievements().Count());
    }

    TEST_METHOD(TestInitializeAchievementsNoGameLoaded)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You must load a game before you can report achievement problems."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsLocalAchievements)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);
        vmBrokenAchievements.mockGameContext.Assets().NewAchievement().SetCategory(ra::data::models::AssetCategory::Local);
        vmBrokenAchievements.mockWindowManager.AssetList.SetFilterCategory(AssetListViewModel::FilterCategory::Local);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You cannot report local achievement problems."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsCompatibilityTestMode)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);
        vmBrokenAchievements.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You cannot report achievement problems in compatibility test mode."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsNoAchievements)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"There are no active achievements to report."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitNoProblemType)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Please select a problem type."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitNoAchievementsSelected)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.SetDefaultComment();

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You must select at least one achievement to submit a ticket."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitDidNotTriggerAsTriggeredWrongTimeCancel)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.SetDefaultComment();
        vmBrokenAchievements.Achievements().GetItemAt(1)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"The achievement you have selected has not triggered, but you have selected 'Triggered at the wrong time'."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitDidNotTriggerAsTriggeredWrongTimeProceed)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.SetDefaultComment();
        vmBrokenAchievements.Achievements().GetItemAt(1)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            if (!bDialogSeen)
            {
                // first dialog is warning we want to capture, second dialog is confirmation
                bDialogSeen = true;
                Assert::AreEqual(std::wstring(L"The achievement you have selected has not triggered, but you have selected 'Triggered at the wrong time'."), vmMessageBox.GetMessage());
            }
            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request&, ra::api::SubmitTicket::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 1;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitTriggeredWrongTimeAsDidNotTriggerCancel)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(2);
        vmBrokenAchievements.SetDefaultComment();
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"The achievement you have selected has triggered, but you have selected 'Did not trigger'."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitTriggeredWrongTimeAsDidNotTriggerProceed)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(2);
        vmBrokenAchievements.SetDefaultComment();
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            if (!bDialogSeen)
            {
                // first dialog is warning we want to capture, second dialog is confirmation
                bDialogSeen = true;
                Assert::AreEqual(std::wstring(L"The achievement you have selected has triggered, but you have selected 'Did not trigger'."), vmMessageBox.GetMessage());
            }
            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request&, ra::api::SubmitTicket::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 1;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitTriggeredWrongTimeNoComment)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Please describe when the achievement did trigger."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitOneAchievementsCancel)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.SetDefaultComment();
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to create a triggered at the wrong time ticket for Title3?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Achievement ID: 3\nRetroAchievements Hash: HASH\nComment: The achievement failed to trigger when I jumped."), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::No;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitMultipleAchievementsCancel)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(2);
        vmBrokenAchievements.SetDefaultComment();
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(4)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to create 2 did not trigger tickets for GAME?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Achievement IDs: 3,5\nRetroAchievements Hash: HASH\nComment: The achievement failed to trigger when I jumped."), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::No;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitOneAchievement)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::OK)
            {
                bDialogSeen = true;
                Assert::AreEqual(std::wstring(L"1 ticket created"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"Thank you for reporting the issue. The development team will investigate and you will be notified when the ticket is updated or resolved."), vmMessageBox.GetMessage());
                return ra::ui::DialogResult::OK;
            }

            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request& request, ra::api::SubmitTicket::Response& response)
        {
            Assert::AreEqual(1, ra::etoi(request.Problem));
            Assert::AreEqual(std::string("HASH"), request.GameHash);
            Assert::AreEqual(std::string("The achievement failed to trigger when I jumped."), request.Comment);
            Assert::AreEqual({ 1U }, request.AchievementIds.size());
            Assert::IsTrue(request.AchievementIds.find(3U) != request.AchievementIds.end());

            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 1;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitMultipleAchievements)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(2);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(4)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::OK)
            {
                bDialogSeen = true;
                Assert::AreEqual(std::wstring(L"2 tickets created"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"Thank you for reporting the issues. The development team will investigate and you will be notified when the tickets are updated or resolved."), vmMessageBox.GetMessage());
                return ra::ui::DialogResult::OK;
            }

            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request& request, ra::api::SubmitTicket::Response& response)
        {
            Assert::AreEqual(2, ra::etoi(request.Problem));
            Assert::AreEqual(std::string("HASH"), request.GameHash);
            Assert::AreEqual(std::string("The achievement failed to trigger when I jumped."), request.Comment);
            Assert::AreEqual({ 2U }, request.AchievementIds.size());
            Assert::IsTrue(request.AchievementIds.find(3U) != request.AchievementIds.end());
            Assert::IsTrue(request.AchievementIds.find(5U) != request.AchievementIds.end());

            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 2;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitMultipleAchievementsDidNotTriggerLimit)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(2);
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(1)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(3)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(4)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(5)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>(
            [&bDialogSeen](const MessageBoxViewModel& vmMessageBox) {
                bDialogSeen = true;
                Assert::AreEqual(std::wstring(L"Too many achievements selected."), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"You cannot report more than 5 achievements at a time for not triggering. Please provide separate details for each achievement that did not trigger for you."), vmMessageBox.GetMessage());
                return ra::ui::DialogResult::OK;
            });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitMultipleAchievementsWrongTimeNoLimit)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(1)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(3)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(4)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(5)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::OK)
            {
                bDialogSeen = true;
                Assert::AreEqual(std::wstring(L"6 tickets created"), vmMessageBox.GetHeader());
                return ra::ui::DialogResult::OK;
            }

            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request& request, ra::api::SubmitTicket::Response& response)
        {
            Assert::AreEqual(1, ra::etoi(request.Problem));
            Assert::AreEqual({ 6U }, request.AchievementIds.size());

            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 6;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitOneAchievementFailed)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(2);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(4)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();


        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::OK)
            {
                bDialogSeen = true;
                Assert::AreEqual(std::wstring(L"Failed to submit tickets"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"API ERROR 3"), vmMessageBox.GetMessage());
                return ra::ui::DialogResult::OK;
            }

            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request&, ra::api::SubmitTicket::Response& response)
        {
            response.Result = ra::api::ApiResult::Error;
            response.ErrorMessage = "API ERROR 3";
            return true;
        });

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestSubmitOneAchievementWithRichPresence)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();

        vmBrokenAchievements.mockGameContext.Assets().FindAchievement(1U)->SetUnlockRichPresence(L"Nowhere, 2 Lives");

        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([](const MessageBoxViewModel& vmMessageBox)
        {
            if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::OK)
                return ra::ui::DialogResult::OK;
            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request& request, ra::api::SubmitTicket::Response& response)
        {
            Assert::AreEqual(1, ra::etoi(request.Problem));
            Assert::AreEqual(std::string("HASH"), request.GameHash);
            Assert::AreEqual(std::string("The achievement failed to trigger when I jumped.\n\nRich Presence at time of trigger:\nNowhere, 2 Lives"), request.Comment);
            Assert::AreEqual({ 1U }, request.AchievementIds.size());

            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 1;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
    }

    TEST_METHOD(TestSubmitMultipleAchievementsWithSameRichPresence)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();

        vmBrokenAchievements.mockGameContext.Assets().FindAchievement(1U)->SetUnlockRichPresence(L"Nowhere, 2 Lives");
        vmBrokenAchievements.mockGameContext.Assets().FindAchievement(3U)->SetUnlockRichPresence(L"Nowhere, 2 Lives");

        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([](const MessageBoxViewModel& vmMessageBox)
        {
            if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::OK)
                return ra::ui::DialogResult::OK;
            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request& request, ra::api::SubmitTicket::Response& response)
        {
            Assert::AreEqual(1, ra::etoi(request.Problem));
            Assert::AreEqual(std::string("HASH"), request.GameHash);
            Assert::AreEqual(std::string("The achievement failed to trigger when I jumped.\n\nRich Presence at time of trigger:\nNowhere, 2 Lives"), request.Comment);
            Assert::AreEqual({ 2U }, request.AchievementIds.size());

            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 2;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
    }

    TEST_METHOD(TestSubmitMultipleAchievementsWithDifferingRichPresence)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(1);
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.SetDefaultComment();

        vmBrokenAchievements.mockGameContext.Assets().FindAchievement(1U)->SetUnlockRichPresence(L"Nowhere, 2 Lives");
        vmBrokenAchievements.mockGameContext.Assets().FindAchievement(3U)->SetUnlockRichPresence(L"Somewhere, 2 Lives");

        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([](const MessageBoxViewModel& vmMessageBox)
        {
            if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::OK)
                return ra::ui::DialogResult::OK;
            return ra::ui::DialogResult::Yes;
        });

        vmBrokenAchievements.mockServer.HandleRequest<ra::api::SubmitTicket>([](const ra::api::SubmitTicket::Request& request, ra::api::SubmitTicket::Response& response)
        {
            Assert::AreEqual(1, ra::etoi(request.Problem));
            Assert::AreEqual(std::string("HASH"), request.GameHash);
            Assert::AreEqual(std::string("The achievement failed to trigger when I jumped.\n\nRich Presence at time of trigger:\n1: Nowhere, 2 Lives\n3: Somewhere, 2 Lives"), request.Comment);
            Assert::AreEqual({ 2U }, request.AchievementIds.size());

            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 2;
            return true;
        });

        Assert::IsTrue(vmBrokenAchievements.Submit());
    }

    TEST_METHOD(TestAutoSelectProblemType)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();

        Assert::AreEqual(0, vmBrokenAchievements.GetSelectedProblemId());

        // a non-active achievement has been triggered, set problem type to WrongTime
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);
        Assert::AreEqual(ra::etoi(ra::api::SubmitTicket::ProblemType::WrongTime), vmBrokenAchievements.GetSelectedProblemId());

        // when nothing is selected, problem type should be reset
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(false);
        Assert::AreEqual(0, vmBrokenAchievements.GetSelectedProblemId());

        // an active achievement has not been triggered, set problem type to DidNotTrigger
        vmBrokenAchievements.Achievements().GetItemAt(1)->SetSelected(true);
        Assert::AreEqual(ra::etoi(ra::api::SubmitTicket::ProblemType::DidNotTrigger), vmBrokenAchievements.GetSelectedProblemId());

        // don't change problem type if it's already set
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(true);
        Assert::AreEqual(ra::etoi(ra::api::SubmitTicket::ProblemType::DidNotTrigger), vmBrokenAchievements.GetSelectedProblemId());

        // don't reset problem type if anything is still selected
        vmBrokenAchievements.Achievements().GetItemAt(1)->SetSelected(false);
        Assert::AreEqual(ra::etoi(ra::api::SubmitTicket::ProblemType::DidNotTrigger), vmBrokenAchievements.GetSelectedProblemId());

        // do reset problem type after everything is deselected
        vmBrokenAchievements.Achievements().GetItemAt(0)->SetSelected(false);
        Assert::AreEqual(0, vmBrokenAchievements.GetSelectedProblemId());
    }

    TEST_METHOD(TestSubmitNoComment)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.MockAchievements();
        vmBrokenAchievements.SetSelectedProblemId(2);
        vmBrokenAchievements.Achievements().GetItemAt(1)->SetSelected(true);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Please be more specific."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Any information that you can provide to help the developer reproduce the issue will make it easier to fix."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        vmBrokenAchievements.mockServer.ExpectUncalled<ra::api::SubmitTicket>();

        Assert::IsFalse(vmBrokenAchievements.Submit());
        Assert::IsTrue(bDialogSeen);
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
