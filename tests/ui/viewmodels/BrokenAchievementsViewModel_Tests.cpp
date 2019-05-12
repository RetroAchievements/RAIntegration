#include "CppUnitTest.h"

#include "ui\viewmodels\BrokenAchievementsViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockServer.hh"

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
        ra::data::mocks::MockGameContext mockGameContext;
        ra::ui::mocks::MockDesktop mockDesktop;

        void MockAchievements()
        {
            mockGameContext.SetGameId(1U);
            mockGameContext.SetGameTitle(L"GAME");
            mockGameContext.SetGameHash("HASH");

            auto& ach1 = mockGameContext.NewAchievement(AchievementSet::Type::Core);
            ach1.SetID(1U);
            ach1.SetTitle("Title1");
            ach1.SetActive(false);
            auto& ach2 = mockGameContext.NewAchievement(AchievementSet::Type::Core);
            ach2.SetID(2U);
            ach2.SetTitle("Title2");
            ach2.SetActive(true);
            auto& ach3 = mockGameContext.NewAchievement(AchievementSet::Type::Core);
            ach3.SetID(3U);
            ach3.SetTitle("Title3");
            ach3.SetActive(false);
            auto& ach4 = mockGameContext.NewAchievement(AchievementSet::Type::Core);
            ach4.SetID(4U);
            ach4.SetTitle("Title4");
            ach4.SetActive(true);
            auto& ach5 = mockGameContext.NewAchievement(AchievementSet::Type::Core);
            ach5.SetID(5U);
            ach5.SetTitle("Title5");
            ach5.SetActive(true);

            Assert::IsTrue(InitializeAchievements());
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        Assert::AreEqual(-1, vmBrokenAchievements.GetSelectedProblemId());
        Assert::AreEqual(std::wstring(L""), vmBrokenAchievements.GetComment());
        Assert::AreEqual(0U, vmBrokenAchievements.Achievements().Count());
    }

    TEST_METHOD(TestInitializeAchievementsNoGameLoaded)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You must load a game before you can report broken achievements."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsLocalAchievements)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);
        vmBrokenAchievements.mockGameContext.SetActiveAchievementType(AchievementSet::Type::Local);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You cannot report broken local achievements."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmBrokenAchievements.InitializeAchievements());
        Assert::IsTrue(bDialogSeen);
    }

    TEST_METHOD(TestInitializeAchievementsCompatibilityTestMode)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);
        vmBrokenAchievements.mockGameContext.SetMode(ra::data::GameContext::Mode::CompatibilityTest);

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"You cannot report broken achievements in compatibility test mode."), vmMessageBox.GetMessage());
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

    TEST_METHOD(TestInitializeAchievementsOnlyCore)
    {
        BrokenAchievementsViewModelHarness vmBrokenAchievements;
        vmBrokenAchievements.mockGameContext.SetGameId(1U);
        auto& ach3 = vmBrokenAchievements.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        ach3.SetID(3U);
        ach3.SetTitle("Title3");
        ach3.SetActive(false);
        auto& ach4 = vmBrokenAchievements.mockGameContext.NewAchievement(AchievementSet::Type::Unofficial);
        ach4.SetID(4U);
        ach4.SetTitle("Title4");
        ach4.SetActive(true);
        auto& ach5 = vmBrokenAchievements.mockGameContext.NewAchievement(AchievementSet::Type::Core);
        ach5.SetID(5U);
        ach5.SetTitle("Title5");
        ach5.SetActive(true);

        Assert::IsTrue(vmBrokenAchievements.InitializeAchievements());
        Assert::IsFalse(vmBrokenAchievements.mockDesktop.WasDialogShown());
        Assert::AreEqual(2U, vmBrokenAchievements.Achievements().Count());

        const auto* row1 = vmBrokenAchievements.Achievements().GetItemAt(0);
        Assert::IsNotNull(row1);
        Ensures(row1 != nullptr);
        Assert::AreEqual(3, row1->GetId());
        Assert::AreEqual(std::wstring(L"Title3"), row1->GetLabel());
        Assert::AreEqual(true, row1->IsAchieved());

        const auto* row2 = vmBrokenAchievements.Achievements().GetItemAt(1);
        Assert::IsNotNull(row2);
        Ensures(row2 != nullptr);
        Assert::AreEqual(5, row2->GetId());
        Assert::AreEqual(std::wstring(L"Title5"), row2->GetLabel());
        Assert::AreEqual(false, row2->IsAchieved());
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
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.SetComment(L"I tried.");

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to create a triggered at the wrong time ticket for Title3?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Achievement ID: 3\nGame Checksum: HASH\nComment: I tried."), vmMessageBox.GetMessage());

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
        vmBrokenAchievements.Achievements().GetItemAt(2)->SetSelected(true);
        vmBrokenAchievements.Achievements().GetItemAt(4)->SetSelected(true);
        vmBrokenAchievements.SetComment(L"I tried.");

        bool bDialogSeen = false;
        vmBrokenAchievements.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogSeen](const MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to create 2 did not trigger tickets for GAME?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Achievement IDs: 3,5\nGame Checksum: HASH\nComment: I tried."), vmMessageBox.GetMessage());

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
        vmBrokenAchievements.SetComment(L"I tried.");

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
            Assert::AreEqual(std::string("I tried."), request.Comment);
            Assert::AreEqual(1U, request.AchievementIds.size());
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
        vmBrokenAchievements.SetComment(L"I tried.");

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
            Assert::AreEqual(std::string("I tried."), request.Comment);
            Assert::AreEqual(2U, request.AchievementIds.size());
            Assert::IsTrue(request.AchievementIds.find(3U) != request.AchievementIds.end());
            Assert::IsTrue(request.AchievementIds.find(5U) != request.AchievementIds.end());

            response.Result = ra::api::ApiResult::Success;
            response.TicketsCreated = 2;
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
        vmBrokenAchievements.SetComment(L"I tried.");

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
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
