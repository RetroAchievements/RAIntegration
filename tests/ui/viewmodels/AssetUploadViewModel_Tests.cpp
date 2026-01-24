#include "CppUnitTest.h"

#include "ui\viewmodels\AssetUploadViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "data\context\GameAssets.hh"
#include "data\models\AchievementModel.hh"
#include "data\models\LeaderboardModel.hh"

#include "tests\data\DataAsserts.hh"
#include "tests\ui\UIAsserts.hh"
#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\devkit\testutil\AssetAsserts.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

using ra::data::models::AchievementModel;
using ra::data::models::AssetCategory;
using ra::data::models::AssetChanges;
using ra::data::models::LeaderboardModel;

TEST_CLASS(AssetUploadViewModel_Tests)
{
private:
    class AssetUploadViewModelHarness : public AssetUploadViewModel
    {
    public:
        AssetUploadViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockGameContext.SetGameId(GameId);
        }

        static constexpr unsigned GameId = 22U;

        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::mocks::MockDesktop mockDesktop;

        void DoUpload(int nExpectedProgress = 100)
        {
            Assert::AreEqual(std::wstring(), GetMessage());
            Assert::AreEqual(0, GetProgress());
            Assert::AreEqual({ 0U }, mockThreadPool.PendingTasks());

            // setting visible starts the task threads
            SetIsVisible(true);
            Assert::AreEqual(DialogResult::None, GetDialogResult());

            Assert::AreEqual(ra::StringPrintf(L"Uploading %d items...", TaskCount()), GetMessage());
            Assert::AreEqual(0, GetProgress());

            // "run" the background thread - all pending tasks will be processed in one worker because the threads are virtual
            Assert::AreNotEqual({ 0U }, mockThreadPool.PendingTasks());
            mockThreadPool.ExecuteNextTask();

            // clean up any other workers that were spawned
            for (size_t i = mockThreadPool.PendingTasks(); i > 0; --i)
                mockThreadPool.ExecuteNextTask();

            // expect all tasks to have been processed
            Assert::AreEqual(nExpectedProgress, GetProgress());

            // dialog should automatically close once all tasks are completed
            if (nExpectedProgress == 100)
                Assert::AreEqual(DialogResult::OK, GetDialogResult());
            else
                Assert::AreEqual(DialogResult::Cancel, GetDialogResult());

            // simulate the hiding - tasks should not restart
            SetIsVisible(false);
            Assert::AreEqual({ 0U }, mockThreadPool.PendingTasks());

        }

        AchievementModel& AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto vmAchievement = std::make_unique<AchievementModel>();
            if (nCategory == AssetCategory::Local)
            {
                vmAchievement->SetID(gsl::narrow_cast<unsigned>(m_pAssets.Count()) + ra::data::context::GameAssets::FirstLocalId);
                vmAchievement->CreateServerCheckpoint();
            }
            else
            {
                vmAchievement->SetID(gsl::narrow_cast<unsigned>(m_pAssets.Count()) + 1);
            }

            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetDescription(sDescription);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetBadge(sBadge);
            vmAchievement->SetTrigger(sTrigger);

            if (nCategory != AssetCategory::Local)
                vmAchievement->CreateServerCheckpoint();

            // set name after creating the server checkpoint so the model appears Unpublished
            vmAchievement->SetName(sTitle);

            vmAchievement->CreateLocalCheckpoint();

            return dynamic_cast<AchievementModel&>(m_pAssets.Append(std::move(vmAchievement)));
        }

        LeaderboardModel& AddLeaderboard(AssetCategory nCategory, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::string& sStartTrigger, const std::string& sSubmitTrigger,
            const std::string& sCancelTrigger, const std::string& sValueDefinition, ra::data::ValueFormat nFormat)
        {
            auto vmLeaderboard = std::make_unique<LeaderboardModel>();
            if (nCategory == AssetCategory::Local)
            {
                vmLeaderboard->SetID(gsl::narrow_cast<unsigned>(m_pAssets.Count()) + ra::data::context::GameAssets::FirstLocalId);
                vmLeaderboard->CreateServerCheckpoint();
            }
            else
            {
                vmLeaderboard->SetID(gsl::narrow_cast<unsigned>(m_pAssets.Count()) + 1);
            }

            vmLeaderboard->SetCategory(nCategory);
            vmLeaderboard->SetName(sTitle);
            vmLeaderboard->SetDescription(sDescription);
            vmLeaderboard->SetStartTrigger(sStartTrigger);
            vmLeaderboard->SetSubmitTrigger(sSubmitTrigger);
            vmLeaderboard->SetCancelTrigger(sCancelTrigger);
            vmLeaderboard->SetValueDefinition(sValueDefinition);
            vmLeaderboard->SetValueFormat(nFormat);

            if (nCategory != AssetCategory::Local)
                vmLeaderboard->CreateServerCheckpoint();

            vmLeaderboard->CreateLocalCheckpoint();

            return dynamic_cast<LeaderboardModel&>(m_pAssets.Append(std::move(vmLeaderboard)));
        }

        ra::data::models::RichPresenceModel& RichPresence()
        {
            auto* pRichPresence = m_pAssets.FindRichPresence();
            if (pRichPresence == nullptr)
            {
                auto pNewRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
                pNewRichPresence->SetScript("Display:\nTest\n");
                pNewRichPresence->CreateServerCheckpoint();
                pNewRichPresence->CreateLocalCheckpoint();
                m_pAssets.Append(std::move(pNewRichPresence));
                pRichPresence = m_pAssets.FindRichPresence();
            }

            return *pRichPresence;
        }

        ra::data::models::CodeNotesModel& CodeNotes()
        {
            auto* pNotes = m_pAssets.FindCodeNotes();
            if (pNotes == nullptr)
            {
                auto pCodeNotes = std::make_unique<ra::data::models::CodeNotesModel>();
                m_pAssets.Append(std::move(pCodeNotes));
                pNotes = m_pAssets.FindCodeNotes();
            }

            return *pNotes;
        }

        void AssertSuccess(int nItems)
        {
            bool bDialogSeen = false;
            mockDesktop.ResetExpectedWindows();
            mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen, nItems](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
            {
                bDialogSeen = true;

                Assert::AreEqual(std::wstring(L"Publish succeeded."), vmMessageBox.GetHeader());

                const auto sMessage = ra::StringPrintf(L"%d items successfully uploaded.", nItems);
                Assert::AreEqual(sMessage, vmMessageBox.GetMessage());

                return DialogResult::OK;
            });

            ShowResults();

            Assert::IsTrue(bDialogSeen);
        }

        void AssertFailed(int nSuccessfulItems, int nFailedItems, const std::wstring& sFailedDetail)
        {
            bool bDialogSeen = false;
            const auto sMessage = ra::StringPrintf(L"%d items successfully uploaded.\n\n%d items failed:\n%s", nSuccessfulItems, nFailedItems, sFailedDetail);

            mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen, &sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
            {
                bDialogSeen = true;

                Assert::AreEqual(std::wstring(L"Publish failed."), vmMessageBox.GetHeader());
                Assert::AreEqual(sMessage, vmMessageBox.GetMessage());

                return DialogResult::OK;
            });

            ShowResults();

            Assert::IsTrue(bDialogSeen);
        }

        void AssertAbort(int nSuccessfulItems, int nFailedItems, const std::wstring& sFailedDetail)
        {
            bool bDialogSeen = false;
            auto sMessage = ra::StringPrintf(L"%d items successfully uploaded.", nSuccessfulItems);
            if (nFailedItems)
                sMessage += ra::StringPrintf(L"\n\n%d items failed:\n%s", nFailedItems, sFailedDetail);

            mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen, &sMessage](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
            {
                bDialogSeen = true;

                Assert::AreEqual(std::wstring(L"Publish canceled."), vmMessageBox.GetHeader());
                Assert::AreEqual(sMessage, vmMessageBox.GetMessage());

                return DialogResult::OK;
            });

            ShowResults();

            Assert::IsTrue(bDialogSeen);
        }

    protected:
        void Rest() const noexcept override {}

    private:
        ra::data::context::GameAssets m_pAssets;
    };

public:
    TEST_METHOD(TestSingleUnofficialAchievement)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement = vmUpload.AddAchievement(AssetCategory::Unofficial, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&bApiCalled]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
            Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
            Assert::AreEqual(std::string("0xH1234=1"), pRequest.Trigger);
            Assert::AreEqual(5U, pRequest.Points);
            Assert::AreEqual(std::string("12345"), pRequest.Badge);
            Assert::AreEqual(5U, pRequest.Category);
            Assert::AreEqual(1U, pRequest.AchievementId);

            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, pAchievement.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCoreAchievement)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&bApiCalled]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
            Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
            Assert::AreEqual(std::string("0xH1234=1"), pRequest.Trigger);
            Assert::AreEqual(5U, pRequest.Points);
            Assert::AreEqual(std::string("12345"), pRequest.Badge);
            Assert::AreEqual(3U, pRequest.Category);
            Assert::AreEqual(1U, pRequest.AchievementId);
            Assert::AreEqual(0U, pRequest.Type);

            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, pAchievement.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCoreAchievementMissable)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        pAchievement.SetAchievementType(ra::data::models::AchievementType::Missable);
        pAchievement.UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&bApiCalled]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
            Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
            Assert::AreEqual(std::string("0xH1234=1"), pRequest.Trigger);
            Assert::AreEqual(5U, pRequest.Points);
            Assert::AreEqual(std::string("12345"), pRequest.Badge);
            Assert::AreEqual(3U, pRequest.Category);
            Assert::AreEqual(1U, pRequest.AchievementId);
            Assert::AreEqual(1U, pRequest.Type);

            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, pAchievement.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleLocalAchievement)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement = vmUpload.AddAchievement(AssetCategory::Local, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&bApiCalled]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
            Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
            Assert::AreEqual(std::string("0xH1234=1"), pRequest.Trigger);
            Assert::AreEqual(5U, pRequest.Points);
            Assert::AreEqual(std::string("12345"), pRequest.Badge);
            Assert::AreEqual(5U, pRequest.Category);
            Assert::AreEqual(0U, pRequest.AchievementId);

            pResponse.AchievementId = 7716U;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);

        // published local achievement should be changed to unofficial and have it's ID updated
        Assert::AreEqual(AssetCategory::Unofficial, pAchievement.GetCategory());
        Assert::AreEqual(7716U, pAchievement.GetID());
        Assert::AreEqual(AssetChanges::None, pAchievement.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleLocalAchievementSubset)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.mockGameContext.SetGameId(11U);
        vmUpload.mockGameContext.MockSubset(33, 22, "Subset");
        auto& pAchievement = vmUpload.AddAchievement(AssetCategory::Local, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        pAchievement.SetSubsetID(22U);
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({1U}, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>(
            [&bApiCalled](const ra::api::UpdateAchievement::Request& pRequest,
                          ra::api::UpdateAchievement::Response& pResponse) {
                bApiCalled = true;
                Assert::AreEqual(33U, pRequest.GameId);
                Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
                Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
                Assert::AreEqual(std::string("0xH1234=1"), pRequest.Trigger);
                Assert::AreEqual(5U, pRequest.Points);
                Assert::AreEqual(std::string("12345"), pRequest.Badge);
                Assert::AreEqual(5U, pRequest.Category);
                Assert::AreEqual(0U, pRequest.AchievementId);

                pResponse.AchievementId = 7716U;
                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);

        // published local achievement should be changed to unofficial and have it's ID updated
        Assert::AreEqual(AssetCategory::Unofficial, pAchievement.GetCategory());
        Assert::AreEqual(7716U, pAchievement.GetID());
        Assert::AreEqual(AssetChanges::None, pAchievement.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCoreAchievementError)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&bApiCalled]
                (const ra::api::UpdateAchievement::Request&, ra::api::UpdateAchievement::Response& pResponse)
        {
            bApiCalled = true;
            pResponse.Result = ra::api::ApiResult::Error;
            pResponse.ErrorMessage = "You must be a developer to modify values in Core!";
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.AssertFailed(0, 1, L"* Title1: You must be a developer to modify values in Core!");
    }

    TEST_METHOD(TestSingleCoreAchievementApiErrorTrigger)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement =
            vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        pAchievement.UpdateServerCheckpoint();
        Assert::AreEqual(AssetChanges::None, pAchievement.GetChanges());

        pAchievement.SetTrigger(""); // trigger is a required field at the API level
        pAchievement.UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({1U}, vmUpload.TaskCount());

        // mockServer doesn't call the API that would return Error/Invalid state, so update the mockServer
        // to behave as if the failure happened at the API level.
        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>(
            [&bApiCalled](const ra::api::UpdateAchievement::Request&, ra::api::UpdateAchievement::Response& pResponse) {
                bApiCalled = true;
                pResponse.Result = ra::api::ApiResult::Error;
                pResponse.ErrorMessage = "Invalid state";
                return true;
            });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.AssertFailed(0, 1, L"* Title1: At least one condition is required");

        // This simulates the failure code in AssetListViewModel::Publish. The test can't be in
        // AssetListViewModel_Tests, because AssetListViewModel_Tests overrides Publish to avoid
        // using the AssetUploadViewModel.
        pAchievement.RestoreLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());
    }

    TEST_METHOD(TestSingleCoreAchievementApiErrorDescription)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement = // description is a required field at the API level
            vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({1U}, vmUpload.TaskCount());

        // mockServer doesn't call the API that would return Error/Invalid state, so update the mockServer
        // to behave as if the failure happened at the API level.
        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>(
            [&bApiCalled](const ra::api::UpdateAchievement::Request&, ra::api::UpdateAchievement::Response& pResponse) {
                bApiCalled = true;
                pResponse.Result = ra::api::ApiResult::Error;
                pResponse.ErrorMessage = "Invalid state";
                return true;
            });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.AssertFailed(0, 1, L"* Title1: Description is required");
    }

    TEST_METHOD(TestMultipleCoreAchievements)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"12345", "0xH1234=1");
        auto& pAchievement3 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title3", L"Desc3", L"12345", "0xH1234=1");
        auto& pAchievement4 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title4", L"Desc4", L"12345", "0xH1234=1");
        auto& pAchievement5 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title5", L"Desc5", L"12345", "0xH1234=1");
        auto& pAchievement6 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title6", L"Desc6", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement3.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement4.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement5.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement6.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        vmUpload.QueueAsset(pAchievement3);
        vmUpload.QueueAsset(pAchievement4);
        vmUpload.QueueAsset(pAchievement5);
        vmUpload.QueueAsset(pAchievement6);
        Assert::AreEqual({ 6U }, vmUpload.TaskCount());

        int nApiCount = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nApiCount]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nApiCount;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(3U, pRequest.Category);

            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(6, nApiCount);
        Assert::AreEqual(AssetChanges::None, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement2.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement3.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement4.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement5.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement6.GetChanges());

        vmUpload.AssertSuccess(6);
    }

    TEST_METHOD(TestMultipleCoreAchievementsPartialFailure)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"12345", "0xH1234=1");
        auto& pAchievement3 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title3", L"Desc3", L"12345", "0xH1234=1");
        auto& pAchievement4 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title4", L"Desc4", L"12345", "0xH1234=1");
        auto& pAchievement5 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title5", L"Desc5", L"12345", "0xH1234=1");
        auto& pAchievement6 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title6", L"Desc6", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement3.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement4.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement5.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement6.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        vmUpload.QueueAsset(pAchievement3);
        vmUpload.QueueAsset(pAchievement4);
        vmUpload.QueueAsset(pAchievement5);
        vmUpload.QueueAsset(pAchievement6);
        Assert::AreEqual({ 6U }, vmUpload.TaskCount());

        int nApiCount = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nApiCount]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nApiCount;

            if (nApiCount == 4)
            {
                pResponse.Result = ra::api::ApiResult::Failed;
                pResponse.ErrorMessage = "Timeout";
            }
            else
            {
                pResponse.AchievementId = pRequest.AchievementId;
                pResponse.Result = ra::api::ApiResult::Success;
            }

            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(6, nApiCount);
        Assert::AreEqual(AssetChanges::None, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement2.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement3.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement4.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement5.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement6.GetChanges());

        vmUpload.AssertFailed(5, 1, L"* Title4: Timeout");
    }

    TEST_METHOD(TestMultipleCoreAchievementsAborted)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"12345", "0xH1234=1");
        auto& pAchievement3 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title3", L"Desc3", L"12345", "0xH1234=1");
        auto& pAchievement4 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title4", L"Desc4", L"12345", "0xH1234=1");
        auto& pAchievement5 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title5", L"Desc5", L"12345", "0xH1234=1");
        auto& pAchievement6 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title6", L"Desc6", L"12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement3.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement4.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement5.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement6.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        vmUpload.QueueAsset(pAchievement3);
        vmUpload.QueueAsset(pAchievement4);
        vmUpload.QueueAsset(pAchievement5);
        vmUpload.QueueAsset(pAchievement6);
        Assert::AreEqual({ 6U }, vmUpload.TaskCount());

        int nApiCount = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&vmUpload, &nApiCount]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nApiCount;

            if (nApiCount == 4)
                vmUpload.SetDialogResult(DialogResult::Cancel);

            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload(50); // fourth item will complete, but isn't tallied

        Assert::AreEqual(4, nApiCount);
        Assert::AreEqual(AssetChanges::None, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement2.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement3.GetChanges());
        Assert::AreEqual(AssetChanges::None, pAchievement4.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement5.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement6.GetChanges());

        vmUpload.AssertAbort(4, 0, L"");
    }

    TEST_METHOD(TestSingleCoreAchievementWithImage)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"local\\12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement.GetChanges());

        vmUpload.QueueAsset(pAchievement);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bImageUploaded = false;
        vmUpload.mockServer.HandleRequest<ra::api::UploadBadge>([&bImageUploaded]
                (const ra::api::UploadBadge::Request& pRequest, ra::api::UploadBadge::Response& pResponse)
        {
            bImageUploaded = true;
            Assert::AreEqual(std::wstring(L"RACache\\Badges\\local\\12345"), pRequest.ImageFilePath);

            pResponse.BadgeId = "76543";
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        bool bAchievementUploaded = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&bAchievementUploaded]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            bAchievementUploaded = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
            Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
            Assert::AreEqual(std::string("0xH1234=1"), pRequest.Trigger);
            Assert::AreEqual(5U, pRequest.Points);
            Assert::AreEqual(std::string("76543"), pRequest.Badge);
            Assert::AreEqual(3U, pRequest.Category);
            Assert::AreEqual(1U, pRequest.AchievementId);

            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bImageUploaded);
        Assert::IsTrue(bAchievementUploaded);
        Assert::AreEqual(AssetChanges::None, pAchievement.GetChanges());
        Assert::AreEqual(std::wstring(L"76543"), pAchievement.GetBadge());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestMultipleCoreAchievementsWithImages)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"local\\12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"local\\22222", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        Assert::AreEqual({ 2U }, vmUpload.TaskCount());

        int nImagesUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UploadBadge>([&nImagesUploaded]
                (const ra::api::UploadBadge::Request& pRequest, ra::api::UploadBadge::Response& pResponse)
        {
            ++nImagesUploaded;

            if (pRequest.ImageFilePath == L"RACache\\Badges\\local\\12345")
                pResponse.BadgeId = "76543";
            else
                pResponse.BadgeId = "55555";

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        int nAchievementsUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nAchievementsUploaded]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nAchievementsUploaded;

            if (pRequest.Title == L"Title1")
                Assert::AreEqual(std::string("76543"), pRequest.Badge);
            else
                Assert::AreEqual(std::string("55555"), pRequest.Badge);

            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(2, nImagesUploaded);
        Assert::AreEqual(2, nAchievementsUploaded);
        Assert::AreEqual(AssetChanges::None, pAchievement1.GetChanges());
        Assert::AreEqual(std::wstring(L"76543"), pAchievement1.GetBadge());
        Assert::AreEqual(AssetChanges::None, pAchievement2.GetChanges());
        Assert::AreEqual(std::wstring(L"55555"), pAchievement2.GetBadge());

        vmUpload.AssertSuccess(2);
    }

    TEST_METHOD(TestMultipleCoreAchievementsWithImages429)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"local\\12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"local\\22222", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        Assert::AreEqual({ 2U }, vmUpload.TaskCount());

        int nImagesUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UploadBadge>([&nImagesUploaded]
                (const ra::api::UploadBadge::Request& pRequest, ra::api::UploadBadge::Response& pResponse)
        {
            ++nImagesUploaded;

            if (pRequest.ImageFilePath == L"RACache\\Badges\\local\\12345")
                pResponse.BadgeId = "76543";
            else
                pResponse.BadgeId = "55555";

            if (nImagesUploaded % 2 == 0)
                pResponse.Result = ra::api::ApiResult::Incomplete;
            else
                pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        int nAchievementsUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nAchievementsUploaded]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nAchievementsUploaded;

            if (pRequest.Title == L"Title1")
                Assert::AreEqual(std::string("76543"), pRequest.Badge);
            else
                Assert::AreEqual(std::string("55555"), pRequest.Badge);

            pResponse.AchievementId = pRequest.AchievementId;

            if (nAchievementsUploaded % 2 == 0)
                pResponse.Result = ra::api::ApiResult::Incomplete;
            else
                pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(3, nImagesUploaded);
        Assert::AreEqual(3, nAchievementsUploaded);
        Assert::AreEqual(AssetChanges::None, pAchievement1.GetChanges());
        Assert::AreEqual(std::wstring(L"76543"), pAchievement1.GetBadge());
        Assert::AreEqual(AssetChanges::None, pAchievement2.GetChanges());
        Assert::AreEqual(std::wstring(L"55555"), pAchievement2.GetBadge());

        vmUpload.AssertSuccess(2);
    }

    TEST_METHOD(TestMultipleCoreAchievementsWithSameImage)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"local\\12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"local\\12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount()); // single task for image upload - that will queue the two achievement tasks

        int nImagesUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UploadBadge>([&nImagesUploaded]
                (const ra::api::UploadBadge::Request& pRequest, ra::api::UploadBadge::Response& pResponse)
        {
            ++nImagesUploaded;

            if (pRequest.ImageFilePath == L"RACache\\Badges\\local\\12345")
                pResponse.BadgeId = "76543";
            else
                pResponse.BadgeId = "55555";

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        int nAchievementsUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nAchievementsUploaded]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nAchievementsUploaded;
            Assert::AreEqual(std::string("76543"), pRequest.Badge);
            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(1, nImagesUploaded);
        Assert::AreEqual(2, nAchievementsUploaded);
        Assert::AreEqual(AssetChanges::None, pAchievement1.GetChanges());
        Assert::AreEqual(std::wstring(L"76543"), pAchievement1.GetBadge());
        Assert::AreEqual(AssetChanges::None, pAchievement2.GetChanges());
        Assert::AreEqual(std::wstring(L"76543"), pAchievement2.GetBadge());

        vmUpload.AssertSuccess(2);
    }

    TEST_METHOD(TestMultipleCoreAchievementsWithFailedImages)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"local\\12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"local\\22222", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        Assert::AreEqual({ 2U }, vmUpload.TaskCount());

        int nImagesUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UploadBadge>([&nImagesUploaded]
                (const ra::api::UploadBadge::Request&, ra::api::UploadBadge::Response& pResponse)
        {
            ++nImagesUploaded;
            pResponse.Result = ra::api::ApiResult::Error;
            pResponse.ErrorMessage = "Unsupported image format";
            return true;
        });

        int nAchievementsUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nAchievementsUploaded]
                (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nAchievementsUploaded;
            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(2, nImagesUploaded);
        Assert::AreEqual(0, nAchievementsUploaded);
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(std::wstring(L"local\\12345"), pAchievement1.GetBadge());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());
        Assert::AreEqual(std::wstring(L"local\\22222"), pAchievement2.GetBadge());

        vmUpload.AssertFailed(0, 2, L"* Title1: Unsupported image format\n* Title2: Unsupported image format");
    }

    TEST_METHOD(TestMultipleCoreAchievementsWithSameFailedImage)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"local\\12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"local\\12345", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        int nImagesUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UploadBadge>([&nImagesUploaded]
        (const ra::api::UploadBadge::Request&, ra::api::UploadBadge::Response& pResponse)
        {
            ++nImagesUploaded;
            pResponse.Result = ra::api::ApiResult::Error;
            pResponse.ErrorMessage = "Unsupported image format";
            return true;
        });

        int nAchievementsUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nAchievementsUploaded]
        (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nAchievementsUploaded;
            pResponse.AchievementId = pRequest.AchievementId;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(1, nImagesUploaded);
        Assert::AreEqual(0, nAchievementsUploaded);
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(std::wstring(L"local\\12345"), pAchievement1.GetBadge());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());
        Assert::AreEqual(std::wstring(L"local\\12345"), pAchievement2.GetBadge());

        vmUpload.AssertFailed(0, 2, L"* Title1: Unsupported image format\n* Title2: Unsupported image format");
    }

    TEST_METHOD(TestMultipleCoreAchievementsWithImagesOneFailed)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pAchievement1 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title1", L"Desc1", L"local\\12345", "0xH1234=1");
        auto& pAchievement2 = vmUpload.AddAchievement(AssetCategory::Core, 5, L"Title2", L"Desc2", L"local\\22222", "0xH1234=1");
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement1.GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pAchievement2.GetChanges());

        vmUpload.QueueAsset(pAchievement1);
        vmUpload.QueueAsset(pAchievement2);
        Assert::AreEqual({ 2U }, vmUpload.TaskCount());

        int nImagesUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UploadBadge>([&nImagesUploaded]
        (const ra::api::UploadBadge::Request& pRequest, ra::api::UploadBadge::Response& pResponse)
        {
            ++nImagesUploaded;

            if (pRequest.ImageFilePath == L"RACache\\Badges\\local\\12345")
                pResponse.BadgeId = "76543";
            else
                pResponse.BadgeId = "55555";

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        int nAchievementsUploaded = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateAchievement>([&nAchievementsUploaded]
        (const ra::api::UpdateAchievement::Request& pRequest, ra::api::UpdateAchievement::Response& pResponse)
        {
            ++nAchievementsUploaded;

            if (pRequest.Title == L"Title1")
            {
                Assert::AreEqual(std::string("76543"), pRequest.Badge);
                pResponse.AchievementId = pRequest.AchievementId;
                pResponse.Result = ra::api::ApiResult::Success;
            }
            else
            {
                Assert::AreEqual(std::string("55555"), pRequest.Badge);
                pResponse.Result = ra::api::ApiResult::Error;
                pResponse.ErrorMessage = "Timeout";
            }

            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(2, nImagesUploaded);
        Assert::AreEqual(2, nAchievementsUploaded);
        Assert::AreEqual(AssetChanges::None, pAchievement1.GetChanges());
        Assert::AreEqual(std::wstring(L"76543"), pAchievement1.GetBadge());

        // badge was successfully uploaded, but achievement failed so it should remain modified
        // the AssetListViewModel will follow up with a save local, which will change the state to Unpublished
        Assert::AreEqual(AssetChanges::Modified, pAchievement2.GetChanges());
        Assert::AreEqual(std::wstring(L"55555"), pAchievement2.GetBadge());

        vmUpload.AssertFailed(1, 1, L"* Title2: Timeout");
    }

    TEST_METHOD(TestSingleLocalLeaderboard)
    {
        AssetUploadViewModelHarness vmUpload;
        auto& pLeaderboard = vmUpload.AddLeaderboard(AssetCategory::Local, L"Title1", L"Desc1", "0xH1234=1", "0xH1234=2", "0xH1234=3", "0xH2345", ra::data::ValueFormat::Score);
        Assert::AreEqual(AssetChanges::Unpublished, pLeaderboard.GetChanges());

        vmUpload.QueueAsset(pLeaderboard);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateLeaderboard>([&bApiCalled]
        (const ra::api::UpdateLeaderboard::Request& pRequest, ra::api::UpdateLeaderboard::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
            Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
            Assert::AreEqual(std::string("0xH1234=1"), pRequest.StartTrigger);
            Assert::AreEqual(std::string("0xH1234=2"), pRequest.SubmitTrigger);
            Assert::AreEqual(std::string("0xH1234=3"), pRequest.CancelTrigger);
            Assert::AreEqual(std::string("0xH2345"), pRequest.ValueDefinition);
            Assert::AreEqual(ra::data::ValueFormat::Score, pRequest.Format);
            Assert::IsFalse(pRequest.LowerIsBetter);
            Assert::AreEqual(0U, pRequest.LeaderboardId);

            pResponse.LeaderboardId = 7716U;
            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);

        // published local leaderboard should be changed to core and have it's ID updated
        Assert::AreEqual(AssetCategory::Core, pLeaderboard.GetCategory());
        Assert::AreEqual(7716U, pLeaderboard.GetID());
        Assert::AreEqual(AssetChanges::None, pLeaderboard.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleLocalLeaderboardSubset)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.mockGameContext.MockSubset(33, 22, "Subset");
        auto& pLeaderboard = vmUpload.AddLeaderboard(AssetCategory::Local, L"Title1", L"Desc1", "0xH1234=1",
                                                     "0xH1234=2", "0xH1234=3", "0xH2345", ra::data::ValueFormat::Score);
        pLeaderboard.SetSubsetID(22U);
        Assert::AreEqual(AssetChanges::Unpublished, pLeaderboard.GetChanges());

        vmUpload.QueueAsset(pLeaderboard);
        Assert::AreEqual({1U}, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateLeaderboard>(
            [&bApiCalled](const ra::api::UpdateLeaderboard::Request& pRequest,
                          ra::api::UpdateLeaderboard::Response& pResponse) {
                bApiCalled = true;
                Assert::AreEqual(33U, pRequest.GameId);
                Assert::AreEqual(std::wstring(L"Title1"), pRequest.Title);
                Assert::AreEqual(std::wstring(L"Desc1"), pRequest.Description);
                Assert::AreEqual(std::string("0xH1234=1"), pRequest.StartTrigger);
                Assert::AreEqual(std::string("0xH1234=2"), pRequest.SubmitTrigger);
                Assert::AreEqual(std::string("0xH1234=3"), pRequest.CancelTrigger);
                Assert::AreEqual(std::string("0xH2345"), pRequest.ValueDefinition);
                Assert::AreEqual(ra::data::ValueFormat::Score, pRequest.Format);
                Assert::IsFalse(pRequest.LowerIsBetter);
                Assert::AreEqual(0U, pRequest.LeaderboardId);

                pResponse.LeaderboardId = 7716U;
                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);

        // published local leaderboard should be changed to core and have it's ID updated
        Assert::AreEqual(AssetCategory::Core, pLeaderboard.GetCategory());
        Assert::AreEqual(7716U, pLeaderboard.GetID());
        Assert::AreEqual(AssetChanges::None, pLeaderboard.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestRichPresence)
    {
        AssetUploadViewModelHarness vmUpload;
        ra::context::mocks::MockRcClient mockRcClient;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockLocalStorage mockStorage;
        mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, std::to_wstring(vmUpload.GameId), "Display:\nThis is a test.");

        auto& pRichPresence = vmUpload.RichPresence();
        pRichPresence.ReloadRichPresenceScript();
        Assert::AreEqual(AssetChanges::Unpublished, pRichPresence.GetChanges());

        vmUpload.QueueAsset(pRichPresence);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateRichPresence>(
            [&bApiCalled](const ra::api::UpdateRichPresence::Request& pRequest, ra::api::UpdateRichPresence::Response& pResponse)
            {
                bApiCalled = true;
                Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
                Assert::AreEqual(std::string("Display:\nThis is a test.\n"), pRequest.Script);

                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, pRichPresence.GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCodeNoteNew)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());
        Assert::IsFalse(vmUpload.mockDesktop.WasDialogShown());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateCodeNote>([&bApiCalled]
                (const ra::api::UpdateCodeNote::Request& pRequest, ra::api::UpdateCodeNote::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(0x1234U, pRequest.Address);
            Assert::AreEqual(std::wstring(L"This is a note."), pRequest.Note);

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCodeNoteNewSubset)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.mockGameContext.MockSubset(33, 22, "Subset");
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::AreEqual({1U}, vmUpload.TaskCount());
        Assert::IsFalse(vmUpload.mockDesktop.WasDialogShown());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateCodeNote>(
            [&bApiCalled](const ra::api::UpdateCodeNote::Request& pRequest,
                          ra::api::UpdateCodeNote::Response& pResponse) {
                bApiCalled = true;
                Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
                Assert::AreEqual(0x1234U, pRequest.Address);
                Assert::AreEqual(std::wstring(L"This is a note."), pRequest.Note);

                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCodeNoteDeleted)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.CodeNotes().SetServerCodeNote(0x1234, L"This is a note.");
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());
        Assert::IsFalse(vmUpload.mockDesktop.WasDialogShown());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::DeleteCodeNote>([&bApiCalled]
                (const ra::api::DeleteCodeNote::Request& pRequest, ra::api::DeleteCodeNote::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(0x1234U, pRequest.Address);

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCodeNoteDifferentAuthor)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.mockUserContext.SetUsername("Author");
        vmUpload.CodeNotes().SetServerCodeNote(0x1234, L"Test");
        vmUpload.mockUserContext.SetUsername("Me");
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"Test2");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        bool bWindowSeen = false;
        vmUpload.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Overwrite note for address 0x1234?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to replace Author's note:\n\nTest\n\nWith your note:\n\nTest2"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::IsTrue(bWindowSeen);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateCodeNote>([&bApiCalled]
                (const ra::api::UpdateCodeNote::Request& pRequest, ra::api::UpdateCodeNote::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(0x1234U, pRequest.Address);
            Assert::AreEqual(std::wstring(L"Test2"), pRequest.Note);

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCodeNoteDeletedDifferentAuthor)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.mockUserContext.SetUsername("Author");
        vmUpload.CodeNotes().SetServerCodeNote(0x1234, L"Test");
        vmUpload.mockUserContext.SetUsername("Me");
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        bool bWindowSeen = false;
        vmUpload.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Delete note for address 0x1234?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to delete Author's note:\n\nTest"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::IsTrue(bWindowSeen);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::DeleteCodeNote>([&bApiCalled]
                (const ra::api::DeleteCodeNote::Request& pRequest, ra::api::DeleteCodeNote::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(0x1234U, pRequest.Address);

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestSingleCodeNoteDifferentAuthorCancel)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.mockUserContext.SetUsername("Author");
        vmUpload.CodeNotes().SetServerCodeNote(0x1234, L"Test");
        vmUpload.mockUserContext.SetUsername("Me");
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"Test2");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        bool bWindowSeen = false;
        vmUpload.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Overwrite note for address 0x1234?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to replace Author's note:\n\nTest\n\nWith your note:\n\nTest2"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::IsTrue(bWindowSeen);
        Assert::AreEqual({ 0U }, vmUpload.TaskCount());

        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());
    }

    TEST_METHOD(TestSingleCodeNoteDifferentLong)
    {
        std::wstring sLongNote;
        for (int i = 0; i < 48; ++i)
            sLongNote += L"Test" + std::to_wstring(i) + L" ";

        AssetUploadViewModelHarness vmUpload;
        vmUpload.mockUserContext.SetUsername("Author");
        vmUpload.CodeNotes().SetServerCodeNote(0x1234, sLongNote);
        vmUpload.mockUserContext.SetUsername("Me");
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"Test");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        bool bWindowSeen = false;
        vmUpload.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Overwrite note for address 0x1234?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to replace Author's note:\n\n" \
                "Test0 Test1 Test2 Test3 Test4 Test5 Test6 Test7 Test8 Test9 Test10 Test11 Test12 Test13 " \
                "Test14 Test15 Test16 Test17 Test18 Test19 Test20 Test21 Test22 Test23 Test24 Test25 " \
                "Test26 Test27 Test28 Test29 Test30 Test31 Test32 Test33 Test34 Test35 Test36 Test..." \
                "\n\nWith your note:\n\nTest"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::IsTrue(bWindowSeen);
        Assert::AreEqual({ 1U }, vmUpload.TaskCount());

        bool bApiCalled = false;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateCodeNote>([&bApiCalled]
                (const ra::api::UpdateCodeNote::Request& pRequest, ra::api::UpdateCodeNote::Response& pResponse)
        {
            bApiCalled = true;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            Assert::AreEqual(0x1234U, pRequest.Address);
            Assert::AreEqual(std::wstring(L"Test"), pRequest.Note);

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::IsTrue(bApiCalled);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(1);
    }

    TEST_METHOD(TestMultipleCodeNotes)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"This is a note.");
        vmUpload.CodeNotes().SetCodeNote(0x1235, L"This is another note.");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::AreEqual({ 2U }, vmUpload.TaskCount());

        int nApiCount = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateCodeNote>([&nApiCount]
                (const ra::api::UpdateCodeNote::Request& pRequest, ra::api::UpdateCodeNote::Response& pResponse)
        {
            nApiCount++;
            Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
            if (pRequest.Address == 0x1234U)
            {
                Assert::AreEqual(0x1234U, pRequest.Address);
                Assert::AreEqual(std::wstring(L"This is a note."), pRequest.Note);
            }
            else
            {
                Assert::AreEqual(0x1235U, pRequest.Address);
                Assert::AreEqual(std::wstring(L"This is another note."), pRequest.Note);
            }

            pResponse.Result = ra::api::ApiResult::Success;
            return true;
        });

        vmUpload.DoUpload();

        Assert::AreEqual(2, nApiCount);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(2);
    }

    TEST_METHOD(TestMultipleCodeNotes429)
    {
        AssetUploadViewModelHarness vmUpload;
        vmUpload.CodeNotes().SetCodeNote(0x1234, L"This is a note.");
        vmUpload.CodeNotes().SetCodeNote(0x1235, L"This is another note.");
        Assert::AreEqual(AssetChanges::Unpublished, vmUpload.CodeNotes().GetChanges());

        vmUpload.QueueAsset(vmUpload.CodeNotes());
        Assert::AreEqual({2U}, vmUpload.TaskCount());

        int nApiCount = 0;
        vmUpload.mockServer.HandleRequest<ra::api::UpdateCodeNote>(
            [&nApiCount](const ra::api::UpdateCodeNote::Request& pRequest,
                         ra::api::UpdateCodeNote::Response& pResponse) {
                nApiCount++;
                Assert::AreEqual(AssetUploadViewModelHarness::GameId, pRequest.GameId);
                if (pRequest.Address == 0x1234U)
                {
                    Assert::AreEqual(0x1234U, pRequest.Address);
                    Assert::AreEqual(std::wstring(L"This is a note."), pRequest.Note);
                }
                else
                {
                    Assert::AreEqual(0x1235U, pRequest.Address);
                    Assert::AreEqual(std::wstring(L"This is another note."), pRequest.Note);
                }

                if (nApiCount % 2 == 0)
                    pResponse.Result = ra::api::ApiResult::Incomplete;
                else
                    pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

        vmUpload.DoUpload();

        // 0 = 1234, success
        // 1 = 1235, delayed
        // 2 = 1235, success
        Assert::AreEqual(3, nApiCount);
        Assert::AreEqual(AssetChanges::None, vmUpload.CodeNotes().GetChanges());

        vmUpload.AssertSuccess(2);
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
