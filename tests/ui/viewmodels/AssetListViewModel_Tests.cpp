#include "CppUnitTest.h"

#include "data\models\LeaderboardModel.hh"
#include "data\models\LocalBadgesModel.hh"
#include "data\models\RichPresenceModel.hh"

#include "ui\viewmodels\AssetListViewModel.hh"
#include "ui\viewmodels\NewAssetViewModel.hh"

#include "services\impl\FileLocalStorage.hh"
#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\ui\UIAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockOverlayTheme.hh"
#include "tests\mocks\MockSurface.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::data::models::AssetType;
using ra::data::models::AssetCategory;
using ra::data::models::AssetState;
using ra::data::models::AssetChanges;
using ra::ui::viewmodels::MessageBoxViewModel;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::ui::viewmodels::AssetListViewModel::CategoryFilter>(
    const ra::ui::viewmodels::AssetListViewModel::CategoryFilter& category)
{
    switch (category)
    {
        case ra::ui::viewmodels::AssetListViewModel::CategoryFilter::Core:
            return L"Core";
        case ra::ui::viewmodels::AssetListViewModel::CategoryFilter::Unofficial:
            return L"Unofficial";
        case ra::ui::viewmodels::AssetListViewModel::CategoryFilter::Local:
            return L"Local";
        case ra::ui::viewmodels::AssetListViewModel::CategoryFilter::All:
            return L"All";
        default:
            return std::to_wstring(ra::etoi(category));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::AssetListViewModel::SpecialFilter>(
    const ra::ui::viewmodels::AssetListViewModel::SpecialFilter& filter)
{
    switch (filter)
    {
        case ra::ui::viewmodels::AssetListViewModel::SpecialFilter::Active:
            return L"Active";
        case ra::ui::viewmodels::AssetListViewModel::SpecialFilter::Inactive:
            return L"Inactive";
        case ra::ui::viewmodels::AssetListViewModel::SpecialFilter::Modified:
            return L"Modified";
        case ra::ui::viewmodels::AssetListViewModel::SpecialFilter::Unpublished:
            return L"Unpublished";
        case ra::ui::viewmodels::AssetListViewModel::SpecialFilter::Authored:
            return L"Authored";
        case ra::ui::viewmodels::AssetListViewModel::SpecialFilter::All:
            return L"All";
        default:
            return std::to_wstring(ra::etoi(filter));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft


namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(AssetListViewModel_Tests)
{
private:
    enum class ActivateButtonState
    {
        Activate,
        Deactivate,
        ActivateAll,
        ActivateDisabled,
        ActivateAllDisabled
    };

    enum class SaveButtonState
    {
        Save,
        Publish,
        Promote,
        Demote,
        SaveDisabled,
        PublishDisabled,
        PromoteDisabled,
        DemoteDisabled,
        SaveAll,
        PublishAll,
        PromoteAll,
        SaveAllDisabled,
        PublishAllDisabled,
        PromoteAllDisabled,
    };

    enum class ResetButtonState
    {
        Reset,
        ResetDisabled,
        ResetAll,
        ResetAllDisabled
    };

    enum class RevertButtonState
    {
        Revert,
        RevertAll,
        Delete,
        DeleteAll,
        RevertDisabled,
        RevertAllDisabled
    };

    enum class CreateButtonState
    {
        Enabled,
        Disabled
    };

    enum class CloneButtonState
    {
        Enabled,
        Disabled
    };

    class AssetListViewModelHarness : public AssetListViewModel
    {
    public:
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::mocks::MockOverlayTheme mockTheme;
        ra::ui::drawing::mocks::MockSurfaceFactory mockSurfaceFactory;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        mocks::MockWindowManager mockWindowManager;

        AssetListViewModelHarness() noexcept
        {
            mockRuntime.MockGame();

            GSL_SUPPRESS_F6 InitializeNotifyTargets();
        }

        void AssertButtonState(ActivateButtonState nActivateButtonState)
        {
            switch (nActivateButtonState)
            {
                case ActivateButtonState::Activate:
                    Assert::AreEqual(std::wstring(L"&Activate"), GetValue(ActivateButtonTextProperty));
                    Assert::IsTrue(CanActivate());
                    break;

                case ActivateButtonState::Deactivate:
                    Assert::AreEqual(std::wstring(L"De&activate"), GetValue(ActivateButtonTextProperty));
                    Assert::IsTrue(CanActivate());
                    break;

                case ActivateButtonState::ActivateAll:
                    Assert::AreEqual(std::wstring(L"&Activate All"), GetValue(ActivateButtonTextProperty));
                    Assert::IsTrue(CanActivate());
                    break;

                case ActivateButtonState::ActivateAllDisabled:
                    Assert::AreEqual(std::wstring(L"&Activate All"), GetValue(ActivateButtonTextProperty));
                    Assert::IsFalse(CanActivate());
                    break;

                case ActivateButtonState::ActivateDisabled:
                    Assert::AreEqual(std::wstring(L"&Activate"), GetValue(ActivateButtonTextProperty));
                    Assert::IsFalse(CanActivate());
                    break;
            }
        }

        void AssertButtonState(SaveButtonState nSaveButtonState)
        {
            switch (nSaveButtonState)
            {
                case SaveButtonState::Save:
                    Assert::AreEqual(std::wstring(L"&Save"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::Publish:
                    Assert::AreEqual(std::wstring(L"Publi&sh"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::Promote:
                    Assert::AreEqual(std::wstring(L"Pro&mote"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::Demote:
                    Assert::AreEqual(std::wstring(L"De&mote"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::SaveDisabled:
                    Assert::AreEqual(std::wstring(L"&Save"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;

                case SaveButtonState::PublishDisabled:
                    Assert::AreEqual(std::wstring(L"Publi&sh"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;

                case SaveButtonState::PromoteDisabled:
                    Assert::AreEqual(std::wstring(L"Pro&mote"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;

                case SaveButtonState::DemoteDisabled:
                    Assert::AreEqual(std::wstring(L"De&mote"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;

                case SaveButtonState::SaveAll:
                    Assert::AreEqual(std::wstring(L"&Save All"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::PublishAll:
                    Assert::AreEqual(std::wstring(L"Publi&sh All"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::PromoteAll:
                    Assert::AreEqual(std::wstring(L"Pro&mote All"), GetValue(SaveButtonTextProperty));
                    Assert::IsTrue(CanSave());
                    break;

                case SaveButtonState::SaveAllDisabled:
                    Assert::AreEqual(std::wstring(L"&Save All"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;

                case SaveButtonState::PublishAllDisabled:
                    Assert::AreEqual(std::wstring(L"Publi&sh All"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;

                case SaveButtonState::PromoteAllDisabled:
                    Assert::AreEqual(std::wstring(L"Pro&mote All"), GetValue(SaveButtonTextProperty));
                    Assert::IsFalse(CanSave());
                    break;
            }
        }

        void AssertButtonState(ResetButtonState nResetButtonState)
        {
            switch (nResetButtonState)
            {
                case ResetButtonState::Reset:
                    Assert::AreEqual(std::wstring(L"&Reset"), GetValue(ResetButtonTextProperty));
                    Assert::IsTrue(CanReset());
                    break;

                case ResetButtonState::ResetDisabled:
                    Assert::AreEqual(std::wstring(L"&Reset"), GetValue(ResetButtonTextProperty));
                    Assert::IsFalse(CanReset());
                    break;

                case ResetButtonState::ResetAll:
                    Assert::AreEqual(std::wstring(L"&Reset All"), GetValue(ResetButtonTextProperty));
                    Assert::IsTrue(CanReset());
                    break;

                case ResetButtonState::ResetAllDisabled:
                    Assert::AreEqual(std::wstring(L"&Reset All"), GetValue(ResetButtonTextProperty));
                    Assert::IsFalse(CanReset());
                    break;
            }
        }

        void AssertButtonState(RevertButtonState nRevertButtonState)
        {
            switch (nRevertButtonState)
            {
                case RevertButtonState::Revert:
                    Assert::AreEqual(std::wstring(L"Re&vert"), GetValue(RevertButtonTextProperty));
                    Assert::IsTrue(CanRevert());
                    break;

                case RevertButtonState::RevertAll:
                    Assert::AreEqual(std::wstring(L"Re&vert All"), GetValue(RevertButtonTextProperty));
                    Assert::IsTrue(CanRevert());
                    break;

                case RevertButtonState::Delete:
                    Assert::AreEqual(std::wstring(L"&Delete"), GetValue(RevertButtonTextProperty));
                    Assert::IsTrue(CanRevert());
                    break;

                case RevertButtonState::DeleteAll:
                    Assert::AreEqual(std::wstring(L"&Delete All"), GetValue(RevertButtonTextProperty));
                    Assert::IsTrue(CanRevert());
                    break;

                case RevertButtonState::RevertDisabled:
                    Assert::AreEqual(std::wstring(L"Re&vert"), GetValue(RevertButtonTextProperty));
                    Assert::IsFalse(CanRevert());
                    break;

                case RevertButtonState::RevertAllDisabled:
                    Assert::AreEqual(std::wstring(L"Re&vert All"), GetValue(RevertButtonTextProperty));
                    Assert::IsFalse(CanRevert());
                    break;
            }
        }

        void AssertButtonState(CreateButtonState nCreateButtonState)
        {
            switch (nCreateButtonState)
            {
                case CreateButtonState::Enabled:
                    Assert::IsTrue(CanCreate());
                    break;

                case CreateButtonState::Disabled:
                    Assert::IsFalse(CanCreate());
                    break;
            }
        }

        void AssertButtonState(CloneButtonState nCloneButtonState)
        {
            switch (nCloneButtonState)
            {
                case CloneButtonState::Enabled:
                    Assert::IsTrue(CanClone());
                    break;

                case CloneButtonState::Disabled:
                    Assert::IsFalse(CanClone());
                    break;
            }
        }

        void AssertButtonState(ActivateButtonState nActivateButtonState,
            SaveButtonState nSaveButtonState, ResetButtonState nResetButtonState,
            RevertButtonState nRevertButtonState, CreateButtonState nCreateButtonState,
            CloneButtonState nCloneButtonState)
        {
            AssertButtonState(nActivateButtonState);
            AssertButtonState(nSaveButtonState);
            AssertButtonState(nResetButtonState);
            AssertButtonState(nRevertButtonState);
            AssertButtonState(nCreateButtonState);
            AssertButtonState(nCloneButtonState);
        }

    private:
        void EnsureCoreSubset() const
        {
            auto* pGame = mockRuntime.GetClient()->game;
            if (!pGame->subsets)
            {
                // populate a minimal core subset just to prevent a null reference error
                auto* pSubset =
                    (rc_client_subset_info_t*)rc_buffer_alloc(&pGame->buffer, sizeof(rc_client_subset_info_t));
                memset(pSubset, 0, sizeof(*pSubset));
                pSubset->public_.id = pGame->public_.id;
                pSubset->public_.title = pGame->public_.title;
                pSubset->active = true;

                pGame->subsets = pSubset;
            }
        }

    public:
        void AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle)
        {
            auto vmAchievement = std::make_unique<MockAchievementModel>();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(mockGameContext.Assets().Count() + 1));
            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->CreateLocalCheckpoint();
            vmAchievement->AttachAndInitialize(*vmAchievement->m_pInfo);
            mockGameContext.Assets().Append(std::move(vmAchievement));

            EnsureCoreSubset();
        }

        void AddAchievement(AssetCategory nCategory, unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto vmAchievement = std::make_unique<MockAchievementModel>();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(mockGameContext.Assets().Count() + 1));
            vmAchievement->SetCategory(nCategory);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->SetDescription(sDescription);
            vmAchievement->SetBadge(sBadge);
            vmAchievement->SetTrigger(sTrigger);
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->CreateLocalCheckpoint();
            vmAchievement->AttachAndInitialize(*vmAchievement->m_pInfo);
            mockGameContext.Assets().Append(std::move(vmAchievement));

            EnsureCoreSubset();
        }

        void AddNewAchievement(unsigned nPoints, const std::wstring& sTitle,
            const std::wstring& sDescription, const std::wstring& sBadge, const std::string& sTrigger)
        {
            auto vmAchievement = std::make_unique<MockAchievementModel>();
            vmAchievement->CreateServerCheckpoint();
            vmAchievement->SetID(gsl::narrow_cast<unsigned int>(mockGameContext.Assets().Count() + 1));
            vmAchievement->SetCategory(AssetCategory::Local);
            vmAchievement->SetPoints(nPoints);
            vmAchievement->SetName(sTitle);
            vmAchievement->SetDescription(sDescription);
            vmAchievement->SetBadge(sBadge);
            vmAchievement->SetTrigger(sTrigger);
            vmAchievement->CreateLocalCheckpoint();
            vmAchievement->SetNew();
            vmAchievement->AttachAndInitialize(*vmAchievement->m_pInfo);
            mockGameContext.Assets().Append(std::move(vmAchievement));

            EnsureCoreSubset();
        }

        void AddThreeAchievements()
        {
            AddAchievement(AssetCategory::Core, 5, L"Ach1");
            AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");
            AddAchievement(AssetCategory::Core, 15, L"Ach3");
        }

        void AddLeaderboard(AssetCategory nCategory, const std::wstring& sTitle)
        {
            auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
            vmLeaderboard->SetID(gsl::narrow_cast<unsigned int>(mockGameContext.Assets().Count() + 1));
            vmLeaderboard->SetCategory(nCategory);
            vmLeaderboard->SetName(sTitle);
            vmLeaderboard->CreateServerCheckpoint();
            vmLeaderboard->CreateLocalCheckpoint();
            mockGameContext.Assets().Append(std::move(vmLeaderboard));

            EnsureCoreSubset();
        }

        void AddLeaderboard()
        {
            AddLeaderboard(AssetCategory::Core, L"Lboard1");
        }

        void AddRichPresence(const std::string& sScript)
        {
            auto vmRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
            vmRichPresence->SetScript(sScript);
            vmRichPresence->CreateServerCheckpoint();
            vmRichPresence->CreateLocalCheckpoint();
            mockGameContext.Assets().Append(std::move(vmRichPresence));

            EnsureCoreSubset();
        }

        void ForceUpdateButtons()
        {
            mockThreadPool.AdvanceTime(std::chrono::milliseconds(500));
        }

        void MockGameId(unsigned int nGameId)
        {
            mockGameContext.SetGameId(nGameId);
            mockGameContext.NotifyActiveGameChanged();
        }

        const std::string& GetUserFile(const std::wstring& sGameId)
        {
            return mockLocalStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, sGameId);
        }

        void MockUserFile(const std::string& sContents)
        {
            return mockLocalStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, std::to_wstring(GetGameId()), sContents);
        }

        void MockUserFileContents(const std::string& sContents)
        {
            MockUserFile("0.0.0.0\nGameName\n" + sContents);
        }

        void Publish(std::vector<ra::data::models::AssetModelBase*>& vAssets) override
        {
            PublishedAssets = vAssets;

            for (auto* pAsset : vAssets)
            {
                Expects(pAsset != nullptr);
                if (m_mPublishServerErrors.find(pAsset->GetID()) == m_mPublishServerErrors.end())
                    pAsset->UpdateServerCheckpoint();
                else
                    pAsset->RestoreLocalCheckpoint();
            }
        }

        std::vector<ra::data::models::AssetModelBase*> PublishedAssets;

        void SetValidationError(ra::AchievementID nId, const std::wstring& sError)
        {
            m_mValidationErrors.insert_or_assign(nId, sError);
        }

        void ValidateAchievementForCore(std::wstring& sError, const ra::data::models::AchievementModel& pAchievement) const override
        {
            if (m_mValidationErrors.empty())
            {
                AssetListViewModel::ValidateAchievementForCore(sError, pAchievement);
            }
            else
            {
                const auto pIter = m_mValidationErrors.find(pAchievement.GetID());
                if (pIter != m_mValidationErrors.end())
                    sError.append(ra::StringPrintf(L"\n* %s: %s", pAchievement.GetName(), pIter->second));
            }
        }

        void SetPublishServerError(ra::AchievementID nId, const std::string& sError)
        {
            m_mPublishServerErrors.insert_or_assign(nId, sError);
        }

        bool SelectionContainsInvalidAsset(const std::vector<ra::data::models::AssetModelBase*>& vSelectedAssets, _Out_ std::wstring& sErrorMessage) const override
        {
            if (vSelectedAssets.empty() && !m_mValidationErrors.empty())
            {
                sErrorMessage = m_mValidationErrors.begin()->second;
                return true;
            }

            for (const auto* pAsset : vSelectedAssets)
            {
                Expects(pAsset != nullptr);
                const auto pIter = m_mValidationErrors.find(pAsset->GetID());
                if (pIter != m_mValidationErrors.end())
                {
                    sErrorMessage = pIter->second;
                    return true;
                }
            }

            sErrorMessage.clear();
            return false;
        }

        ra::data::models::LocalBadgesModel* AddLocalBadgesModel()
        {
            auto pLocalBadges = std::make_unique<ra::data::models::LocalBadgesModel>();
            pLocalBadges->CreateServerCheckpoint();
            pLocalBadges->CreateLocalCheckpoint();
            mockGameContext.Assets().Append(std::move(pLocalBadges));

            return dynamic_cast<ra::data::models::LocalBadgesModel*>(mockGameContext.Assets().FindAsset(ra::data::models::AssetType::LocalBadges, 0));
        }

        class MockAchievementModel : public ra::data::models::AchievementModel
        {
        public:
            MockAchievementModel()
            {
                m_pInfo = std::make_unique<rc_client_achievement_info_t>();
                memset(m_pInfo.get(), 0, sizeof(rc_client_achievement_info_t));
            }

            std::unique_ptr<rc_client_achievement_info_t> m_pInfo;
        };

        std::map<ra::AchievementID, std::wstring> m_mValidationErrors;
        std::map<ra::AchievementID, std::string> m_mPublishServerErrors;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        AssetListViewModelHarness vmAssetList;

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual(0U, vmAssetList.GetGameId());
        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
        Assert::AreEqual(true, vmAssetList.IsProcessingActive());

        Assert::AreEqual({ 7U }, vmAssetList.States().Count());
        Assert::AreEqual((int)AssetState::Inactive, vmAssetList.States().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Inactive"), vmAssetList.States().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetState::Waiting, vmAssetList.States().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Waiting"), vmAssetList.States().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetState::Active, vmAssetList.States().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Active"), vmAssetList.States().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetState::Paused, vmAssetList.States().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Paused"), vmAssetList.States().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)AssetState::Primed, vmAssetList.States().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Primed"), vmAssetList.States().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)AssetState::Triggered, vmAssetList.States().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Triggered"), vmAssetList.States().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)AssetState::Disabled, vmAssetList.States().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"Disabled"), vmAssetList.States().GetItemAt(6)->GetLabel());

        Assert::AreEqual({ 4U }, vmAssetList.Categories().Count());
        Assert::AreEqual((int)AssetListViewModel::CategoryFilter::All, vmAssetList.Categories().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"All"), vmAssetList.Categories().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::CategoryFilter::Core, vmAssetList.Categories().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Core"), vmAssetList.Categories().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::CategoryFilter::Unofficial, vmAssetList.Categories().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Unofficial"), vmAssetList.Categories().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::CategoryFilter::Local, vmAssetList.Categories().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Local"), vmAssetList.Categories().GetItemAt(3)->GetLabel());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        Assert::AreEqual({ 6U }, vmAssetList.SpecialFilters().Count());
        Assert::AreEqual((int)AssetListViewModel::SpecialFilter::All, vmAssetList.SpecialFilters().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"All"), vmAssetList.SpecialFilters().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::SpecialFilter::Active, vmAssetList.SpecialFilters().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Active"), vmAssetList.SpecialFilters().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::SpecialFilter::Inactive, vmAssetList.SpecialFilters().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Inactive"), vmAssetList.SpecialFilters().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::SpecialFilter::Modified, vmAssetList.SpecialFilters().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Modified"), vmAssetList.SpecialFilters().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::SpecialFilter::Unpublished, vmAssetList.SpecialFilters().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Unpublished"), vmAssetList.SpecialFilters().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)AssetListViewModel::SpecialFilter::Authored, vmAssetList.SpecialFilters().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Authored"), vmAssetList.SpecialFilters().GetItemAt(5)->GetLabel());
        Assert::AreEqual(AssetListViewModel::SpecialFilter::All, vmAssetList.GetSpecialFilter());

        Assert::AreEqual({ 4U }, vmAssetList.AssetTypeFilters().Count());
        Assert::AreEqual((int)AssetType::None, vmAssetList.AssetTypeFilters().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"All"), vmAssetList.AssetTypeFilters().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetType::Achievement, vmAssetList.AssetTypeFilters().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Achievements"), vmAssetList.AssetTypeFilters().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetType::Leaderboard, vmAssetList.AssetTypeFilters().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Leaderboards"), vmAssetList.AssetTypeFilters().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetType::RichPresence, vmAssetList.AssetTypeFilters().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Rich Presence"), vmAssetList.AssetTypeFilters().GetItemAt(3)->GetLabel());
        Assert::AreEqual(AssetType::Achievement, vmAssetList.GetAssetTypeFilter());

        Assert::AreEqual({ 5U }, vmAssetList.Changes().Count());
        Assert::AreEqual((int)AssetChanges::None, vmAssetList.Changes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), vmAssetList.Changes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)AssetChanges::Modified, vmAssetList.Changes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Modified"), vmAssetList.Changes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)AssetChanges::Unpublished, vmAssetList.Changes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Unpublished"), vmAssetList.Changes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)AssetChanges::New, vmAssetList.Changes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"New"), vmAssetList.Changes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)AssetChanges::Deleted, vmAssetList.Changes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Deleted"), vmAssetList.Changes().GetItemAt(4)->GetLabel());
    }

    TEST_METHOD(TestLoadGameNoAchievements)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(1U);

        Assert::AreEqual(1U, vmAssetList.GetGameId());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());
        Assert::AreEqual({0U}, vmAssetList.FilteredAssets().Count());

        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.MockGameId(2U);
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual({0U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestLoadGameCoreAchievements)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach1");
        vmAssetList.MockGameId(1U);

        Assert::AreEqual(1U, vmAssetList.GetGameId());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestLoadGameUnofficialAchievements)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach1");
        vmAssetList.MockGameId(1U);

        Assert::AreEqual(1U, vmAssetList.GetGameId());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Unofficial, vmAssetList.GetCategoryFilter());
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestLoadGameLocalAchievements)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.AddAchievement(AssetCategory::Local, 10, L"Ach1");
        vmAssetList.MockGameId(1U);

        Assert::AreEqual(1U, vmAssetList.GetGameId());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestAddRemoveCoreAchievement)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach2");

        Assert::AreEqual(2, vmAssetList.GetAchievementCount());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 20, L"Ach3");

        Assert::AreEqual(3, vmAssetList.GetAchievementCount());
        Assert::AreEqual(35, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(1);

        Assert::AreEqual(2, vmAssetList.GetAchievementCount());
        Assert::AreEqual(25, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestAddRemoveUnofficialAchievement)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Ach1");

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach2");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 20, L"Ach3");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestAddRemoveLocalAchievement)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Ach1");

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach2");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Local, 20, L"Ach3");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual(0, vmAssetList.GetAchievementCount());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestPointsChange)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        vmAssetList.AddAchievement(AssetCategory::Local, 10, L"Ach2");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 20, L"Ach3");

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        auto ach = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(50);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());

        ach = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(40);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());

        ach = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(2));
        Assert::IsNotNull(ach);
        Ensures(ach != nullptr);
        ach->SetPoints(30);

        Assert::AreEqual(1, vmAssetList.GetAchievementCount());
        Assert::AreEqual(50, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestAddItemWithFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.AddAchievement(AssetCategory::Core, 15, L"Ach3");

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
    }

    TEST_METHOD(TestAddItemWithFilterUpdateSuspended)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.mockGameContext.Assets().BeginUpdate();

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.AddAchievement(AssetCategory::Core, 15, L"Ach3");

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().EndUpdate();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
    }

    TEST_METHOD(TestRemoveItemWithFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestRemoveItemWithFilterUpdateSuspended)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.mockGameContext.Assets().BeginUpdate();
        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().RemoveAt(0);

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());

        vmAssetList.mockGameContext.Assets().EndUpdate();

        Assert::AreEqual({ 0U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
    }

    TEST_METHOD(TestChangeItemForFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetCategory(AssetCategory::Unofficial);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());

        vmAssetList.mockGameContext.Assets().GetItemAt(1)->SetCategory(AssetCategory::Core);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(25, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(2, vmAssetList.FilteredAssets().GetItemAt(1)->GetId()); // item changed to match filter appears at end of list

        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetCategory(AssetCategory::Core);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 3U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(30, vmAssetList.GetTotalPoints());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(2, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(2)->GetId()); // item changed to match filter appears at end of list
    }

    TEST_METHOD(TestChangeFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 10, L"Ach2");
        vmAssetList.AddAchievement(AssetCategory::Core, 15, L"Ach3");

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(20, vmAssetList.GetTotalPoints());
        Assert::AreEqual(1, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual(3, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());

        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());
        Assert::AreEqual(2, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
    }

    TEST_METHOD(TestChangeAssetTypeFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.SetAssetTypeFilter(AssetType::None);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        vmAssetList.AddAchievement(AssetCategory::Core, 10, L"Ach2");
        vmAssetList.AddLeaderboard();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 3U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(AssetType::Achievement, vmAssetList.FilteredAssets().GetItemAt(0)->GetType());
        Assert::AreEqual(AssetType::Achievement, vmAssetList.FilteredAssets().GetItemAt(1)->GetType());
        Assert::AreEqual(AssetType::Leaderboard, vmAssetList.FilteredAssets().GetItemAt(2)->GetType());

        vmAssetList.SetAssetTypeFilter(AssetType::Leaderboard);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(0, vmAssetList.GetTotalPoints());
        Assert::AreEqual(AssetType::Leaderboard, vmAssetList.FilteredAssets().GetItemAt(0)->GetType());

        vmAssetList.SetAssetTypeFilter(AssetType::Achievement);

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(15, vmAssetList.GetTotalPoints());
        Assert::AreEqual(AssetType::Achievement, vmAssetList.FilteredAssets().GetItemAt(0)->GetType());
        Assert::AreEqual(AssetType::Achievement, vmAssetList.FilteredAssets().GetItemAt(1)->GetType());
    }

    TEST_METHOD(TestSpecialFilterActive)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        Assert::AreEqual({1U}, vmAssetList.mockGameContext.Assets().Count());
        auto* pAsset = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Expects(pAsset != nullptr);
        pAsset->SetState(AssetState::Active);

        vmAssetList.SetSpecialFilter(AssetListViewModel::SpecialFilter::Active);
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());

        pAsset->SetState(AssetState::Inactive);
        Assert::AreEqual({0U}, vmAssetList.FilteredAssets().Count());

        pAsset->SetState(AssetState::Waiting);
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestSpecialFilterInactive)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        Assert::AreEqual({1U}, vmAssetList.mockGameContext.Assets().Count());
        auto* pAsset = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Expects(pAsset != nullptr);
        pAsset->SetState(AssetState::Inactive);

        vmAssetList.SetSpecialFilter(AssetListViewModel::SpecialFilter::Inactive);
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());

        pAsset->SetState(AssetState::Waiting);
        Assert::AreEqual({0U}, vmAssetList.FilteredAssets().Count());

        pAsset->SetState(AssetState::Disabled);
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestSpecialFilterModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        Assert::AreEqual({1U}, vmAssetList.mockGameContext.Assets().Count());
        auto* pAsset = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Expects(pAsset != nullptr);
        pAsset->SetName(L"New Title");

        vmAssetList.SetSpecialFilter(AssetListViewModel::SpecialFilter::Modified);
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());

        pAsset->RestoreLocalCheckpoint();
        Assert::AreEqual({0U}, vmAssetList.FilteredAssets().Count());

        pAsset->SetDescription(L"New Description");
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestSpecialFilterUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        auto* pAsset = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Expects(pAsset != nullptr);
        pAsset->SetName(L"New Title");
        pAsset->UpdateLocalCheckpoint();

        vmAssetList.SetSpecialFilter(AssetListViewModel::SpecialFilter::Unpublished);
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        pAsset->RestoreServerCheckpoint();
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());

        pAsset->SetDescription(L"New Description");
        pAsset->UpdateLocalCheckpoint();
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestSpecialFilterAuthored)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.mockUserContext.Initialize("me", "Me", "ABCDEFGH");

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach2");
        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());

        auto* pAsset1 = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Expects(pAsset1 != nullptr);
        pAsset1->SetAuthor(L"Me");

        auto* pAsset2 = vmAssetList.mockGameContext.Assets().GetItemAt(1);
        Expects(pAsset2 != nullptr);
        pAsset2->SetAuthor(L"You");

        vmAssetList.SetSpecialFilter(AssetListViewModel::SpecialFilter::Authored);
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(std::wstring(L"Ach1"), vmAssetList.FilteredAssets().GetItemAt(0)->GetLabel());
    }

    TEST_METHOD(TestSubsetFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetSubsetFilter(0);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        // create core achievements first so core subset gets created
        vmAssetList.mockRuntime.MockAchievement(1, "Ach1")->public_.points = 5;
        vmAssetList.mockRuntime.MockAchievement(2, "Ach2")->public_.points = 10;
        // create subset and subset achievements
        vmAssetList.mockRuntime.MockSubset(555, "Bonus");
        vmAssetList.mockRuntime.MockSubsetAchievement(555, 3, "Ach3")->public_.points = 25;
        auto* pAch4 = vmAssetList.mockRuntime.MockSubsetAchievement(555, 4, "Ach4");
        pAch4->public_.points = 50;
        pAch4->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL;

        vmAssetList.mockGameContext.InitializeFromAchievementRuntime();
        vmAssetList.mockRuntime.SyncAssets();
        vmAssetList.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({2U}, vmAssetList.Subsets().Count());
        Assert::AreEqual({0U}, vmAssetList.Subsets().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Game Title"), vmAssetList.Subsets().GetItemAt(0)->GetLabel());
        Assert::AreEqual({555U}, vmAssetList.Subsets().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Bonus"), vmAssetList.Subsets().GetItemAt(1)->GetLabel());

        Assert::AreEqual({2U}, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual({2U}, vmAssetList.FilteredAssets().GetItemAt(1)->GetId());
        Assert::AreEqual({2U}, vmAssetList.GetAchievementCount());
        Assert::AreEqual({15U}, vmAssetList.GetTotalPoints());

        vmAssetList.SetSubsetFilter(555);
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual({3U}, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual({1U}, vmAssetList.GetAchievementCount());
        Assert::AreEqual({25U}, vmAssetList.GetTotalPoints());

        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);
        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual({4U}, vmAssetList.FilteredAssets().GetItemAt(0)->GetId());
        Assert::AreEqual({1U}, vmAssetList.GetAchievementCount());
        Assert::AreEqual({50U}, vmAssetList.GetTotalPoints());

        vmAssetList.SetSubsetFilter(0);
        Assert::AreEqual({0U}, vmAssetList.FilteredAssets().Count());
    }

    TEST_METHOD(TestSyncFilteredItem)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        auto pAchievement = std::make_unique<ra::data::models::AchievementModel>();
        pAchievement->SetID(1U);
        pAchievement->SetCategory(AssetCategory::Core);
        pAchievement->SetPoints(5);
        pAchievement->SetName(L"Title");
        pAchievement->SetState(AssetState::Inactive);
        pAchievement->CreateServerCheckpoint();
        pAchievement->CreateLocalCheckpoint();
        auto& vmAchievement = dynamic_cast<ra::data::models::AchievementModel&>(vmAssetList.mockGameContext.Assets().Append(std::move(pAchievement)));

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        const auto* pItem = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pItem != nullptr);

        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(AssetCategory::Core, pItem->GetCategory());
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(std::wstring(L"Title"), pItem->GetLabel());
        Assert::AreEqual(5, pItem->GetPoints());
        Assert::AreEqual(AssetState::Inactive, pItem->GetState());
        Assert::AreEqual(5, vmAssetList.GetTotalPoints());
        Assert::AreEqual(std::wstring(L""), pItem->GetWarning());

        vmAchievement.SetName(L"New Title");
        Assert::AreEqual(std::wstring(L"New Title"), pItem->GetLabel());
        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());

        vmAchievement.SetPoints(10);
        Assert::AreEqual(10, pItem->GetPoints());
        Assert::AreEqual(10, vmAssetList.GetTotalPoints());

        vmAchievement.SetState(AssetState::Active);
        Assert::AreEqual(AssetState::Active, pItem->GetState());

        vmAchievement.SetTrigger("A:0x1234");
        vmAchievement.UpdateLocalCheckpoint();
        Assert::IsFalse(vmAchievement.Validate());
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), pItem->GetWarning());
    }

    TEST_METHOD(TestSyncAddItem)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        // between this test and TestSyncFilteredItem, we can validate each of the synced properties are
        // correctly handled when an item is added to the list. TestSyncFilteredItem will also test to make
        // sure they update after they've been added.
        auto pAchievement = std::make_unique<ra::data::models::AchievementModel>();
        pAchievement->SetID(2U);
        pAchievement->SetCategory(AssetCategory::Core);
        pAchievement->SetPoints(10);
        pAchievement->SetName(L"Title2");
        pAchievement->SetState(AssetState::Active);
        pAchievement->CreateServerCheckpoint();
        pAchievement->CreateLocalCheckpoint();
        vmAssetList.mockGameContext.Assets().Append(std::move(pAchievement));

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        const auto* pItem = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pItem != nullptr);

        Assert::AreEqual(2, pItem->GetId());
        Assert::AreEqual(AssetCategory::Core, pItem->GetCategory());
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(std::wstring(L"Title2"), pItem->GetLabel());
        Assert::AreEqual(10, pItem->GetPoints());
        Assert::AreEqual(AssetState::Active, pItem->GetState());
    }

    TEST_METHOD(TestUpdateButtonsNoGame)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAllDisabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAllDisabled, RevertButtonState::RevertAllDisabled,
            CreateButtonState::Disabled, CloneButtonState::Disabled
        );

        vmAssetList.SetGameId(1U);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SetGameId(0U);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAllDisabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAllDisabled, RevertButtonState::RevertAllDisabled,
            CreateButtonState::Disabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsNoAssets)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);

        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAllDisabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsNoSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsInactiveSingleSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsActiveSingleSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsActiveAndInactiveSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnofficialSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::AreEqual(AssetChanges::None, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Promote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsChangeSelectionUpdatesButtons)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true); // active
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true); // inactive (both selected)
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false); // active (only inactive selected)
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(false); // inactive (nothing selected)
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsRemoveItems)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().RemoveAt(2); // remove selected item (which was active), nothing selected
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );

        vmAssetList.mockGameContext.Assets().RemoveAt(0); // remove unselected item (which was inactive), nothing remaining
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAllDisabled, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );

        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Ach1"); // add back one achievement (inactive, unselected)
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsChangeFilter)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Deactivate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateAll, SaveButtonState::PromoteAll,
            ResetButtonState::ResetAll, RevertButtonState::RevertAll,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsModifiedSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");       

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Save,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsModifiedUnselected)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetName(L"Modified");

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        // single unmodified selected record, show save button instead of save all, but disable it
        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsModifiedChanges)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Save,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Ach3");
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsNewSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.AddNewAchievement(5, L"Ach1", L"Test1", L"12345", "0xH1234=1");
        vmAssetList.AddNewAchievement(10, L"Ach2", L"Test2", L"12345", "0xH1234=2");

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::New, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Save,
            ResetButtonState::ResetDisabled, RevertButtonState::Delete,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->UpdateLocalCheckpoint();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Publish,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedUnselected)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetName(L"Modified");
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->UpdateLocalCheckpoint();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedChanges)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->UpdateLocalCheckpoint();
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Publish,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );

        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Ach3");
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->UpdateLocalCheckpoint();
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::Demote,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsInactiveSingleSelectionOffline)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);

        Assert::AreEqual({2U}, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::DemoteDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnpublishedSelectionOffline)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetName(L"Modified");
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->UpdateLocalCheckpoint();
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::PublishDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestUpdateButtonsUnofficialSelectionOffline)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);

        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::AreEqual(AssetChanges::None, vmAssetList.FilteredAssets().GetItemAt(0)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::Activate, SaveButtonState::PromoteDisabled,
            ResetButtonState::Reset, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Enabled
        );
    }

    TEST_METHOD(TestActivateSelectedSingle)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
    }

    TEST_METHOD(TestActivateSelectedMultiple)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true); // inactive
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true); // active
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
    }

    TEST_METHOD(TestActivateSelectedInvalid)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.SetValidationError(vmAssetList.FilteredAssets().GetItemAt(1)->GetId(), L"Error message goes here.");
        vmAssetList.ForceUpdateButtons();

        bool bMessageSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"Unable to activate"), vmMessage.GetHeader());
            Assert::AreEqual(std::wstring(L"Error message goes here."), vmMessage.GetMessage());

            return DialogResult::OK;
        });

        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ActivateSelected();
        Assert::IsTrue(bMessageSeen);
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
    }

    TEST_METHOD(TestActivateSelectedHardcoreDeactivateCore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Active);

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::No);
        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::string("deactivate core assets"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::Yes);
        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::string("deactivate core assets"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::IsFalse(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestActivateSelectedHardcoreDeactivateLocal)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetState(AssetState::Active);

        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::AreEqual(AssetState::Active, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::No);
        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::string(), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestActivateSelectedSingleHardcoreUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(AssetChanges::None, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.ActivateSelected();

        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestActivateSelectedSingleHardcoreModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        auto* pAsset = vmAssetList.mockGameContext.Assets().GetItemAt(2); // second achievement is unofficial
        Expects(pAsset != nullptr);
        pAsset->SetName(L"New Title");

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::No);
        vmAssetList.ActivateSelected();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(AssetChanges::Modified, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::Yes);
        vmAssetList.ActivateSelected();

        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        Assert::IsFalse(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestActivateSelectedSingleHardcoreUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        auto* pAsset = vmAssetList.mockGameContext.Assets().GetItemAt(2); // second achievement is unofficial
        Expects(pAsset != nullptr);
        pAsset->SetName(L"New Title");
        pAsset->UpdateLocalCheckpoint();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::No);
        vmAssetList.ActivateSelected();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(AssetChanges::Unpublished, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::Yes);
        vmAssetList.ActivateSelected();

        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        Assert::IsFalse(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestActivateSelectedSingleHardcoreNew)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();

        auto* pAsset = vmAssetList.mockGameContext.Assets().GetItemAt(2); // second achievement is unofficial
        Expects(pAsset != nullptr);
        pAsset->SetNew();

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(AssetChanges::New, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::No);
        vmAssetList.ActivateSelected();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(AssetChanges::New, vmAssetList.FilteredAssets().GetItemAt(1)->GetChanges());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::Yes);
        vmAssetList.ActivateSelected();

        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Waiting, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        Assert::IsFalse(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestDeactivatePrimed)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.AddThreeAchievements();
        vmAssetList.mockGameContext.Assets().GetItemAt(2)->SetState(AssetState::Primed);
        const auto nAchievementId = vmAssetList.mockGameContext.Assets().GetItemAt(2)->GetID();

        vmAssetList.mockOverlayManager.AddChallengeIndicator(nAchievementId, ra::ui::ImageType::Badge, "12345");
        const auto* pIndicator = vmAssetList.mockOverlayManager.GetChallengeIndicator(nAchievementId);
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);
        Assert::IsFalse(pIndicator->IsDestroyPending());

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        Assert::IsFalse(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
        Assert::AreEqual(AssetState::Primed, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(1)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        // RemoveChallengeIndicator only marks the item as to be destroyed.
        // it still exists until the next Render()
        Assert::IsTrue(pIndicator->IsDestroyPending());
    }

    TEST_METHOD(TestDeactivateStartedLeaderboard)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.SetAssetTypeFilter(AssetType::Leaderboard);
        vmAssetList.AddLeaderboard();
        vmAssetList.mockGameContext.Assets().GetItemAt(0)->SetState(AssetState::Primed);
        const auto nLeaderboardId = vmAssetList.mockGameContext.Assets().GetItemAt(0)->GetID();

        vmAssetList.mockOverlayManager.AddScoreTracker(nLeaderboardId);
        const auto* pIndicator = vmAssetList.mockOverlayManager.GetScoreTracker(nLeaderboardId);
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);
        Assert::IsFalse(pIndicator->IsDestroyPending());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::AreEqual(AssetState::Primed, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(std::wstring(L"De&activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        vmAssetList.ActivateSelected();
        vmAssetList.ForceUpdateButtons();
        Assert::AreEqual(AssetState::Inactive, vmAssetList.FilteredAssets().GetItemAt(0)->GetState());
        Assert::AreEqual(std::wstring(L"&Activate"), vmAssetList.GetActivateButtonText());
        Assert::IsTrue(vmAssetList.CanActivate());

        // RemoveScoreTracker only marks the item as to be destroyed.
        // it still exists until the next Render()
        Assert::IsTrue(pIndicator->IsDestroyPending());
    }

    TEST_METHOD(TestSaveSelectedLocalUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        vmAssetList.SaveSelected();

        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertContains(sText, "1:\"0xH1234=1\":Test1:Desc1:::::5:::::12345");
    }

    TEST_METHOD(TestSaveSelectedLocalModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddNewAchievement(7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetPoints(10);
        pItem->SetName(L"Test1b");
        pItem->SetDescription(L"Desc1b");
        pItem->SetBadge(L"54321");
        pItem->SetTrigger("0xH1234=2");

        auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);

        // when an item is selected, non-selected modified items should be written using their unmodified state
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText, "Test1");
        AssertContains(sText, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");

        // item 2 was saved, but item 1 should still be new
        Assert::AreEqual(AssetChanges::New, pItem->GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pItem2->GetChanges());

        // deselect all. when no items are selected, all items should be committed and written
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false);

        vmAssetList.SaveSelected();
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText2, "1:\"0xH1234=2\":Test1b:Desc1b:::::10:::::54321");
        AssertContains(sText2, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");

        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());
        Assert::AreEqual(AssetChanges::Unpublished, pItem2->GetChanges());

        // when an item is selected, non-selected modified items should be written using their unmodified state
        pItem->SetName(L"Test1c");
        pItem2->SetName(L"Test2b");

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText3 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText3, "1:\"0xH1234=2\":Test1c:Desc1b:::::10:::::54321");
        AssertContains(sText3, "2:\"0xH1111=1\":Test2:Desc2:::::7:::::11111");
    }

    TEST_METHOD(TestSaveSelectedUnofficialUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        vmAssetList.SaveSelected();

        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText, "1:\"0xH1234=1\":");
    }

    TEST_METHOD(TestSaveSelectedUnofficialModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetPoints(10);
        pItem->SetName(L"Test1b");
        pItem->SetDescription(L"Desc1b");
        pItem->SetBadge(L"54321");
        pItem->SetTrigger("0xH1234=2");

        // when an item is selected, non-selected modified items should be written using their unmodified state
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText, "1:\"0xH1234=1\":");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());

        // when no items are selected, all items should be committed and written
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(false);

        vmAssetList.SaveSelected();
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText2, "1:\"0xH1234=2\":Test1b:Desc1b:::::10:::::54321");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        // when an item is selected, non-selected modified items should be written using their unmodified state
        pItem->SetName(L"Test1c");
        auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);
        pItem2->SetName(L"Test2b");

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText3 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText3, "1:\"0xH1234=2\":Test1c:Desc1b:::::10:::::54321");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");
    }

    TEST_METHOD(TestSaveSelectedDiscardCoreChanges)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);

        vmAssetList.SaveSelected();
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertContains(sText, "1:\"0xH1234=1\":Test1b:Desc1:::::5:::::12345");
        AssertDoesNotContain(sText, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        // change the value back to match the server checkpoint, should no longer appear in local file
        pItem->SetName(L"Test1");
        vmAssetList.SaveSelected();
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertDoesNotContain(sText2, "1:\"0xH1234=1\":");
        AssertDoesNotContain(sText2, "2:\"0xH1111=1\":");

        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
    }

    TEST_METHOD(TestSaveSelectedInvalid)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(1U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.SetValidationError(pItem->GetID(), L"Error message goes here.");
        vmAssetList.ForceUpdateButtons();

        bool bMessageSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"Unable to save"), vmMessage.GetHeader());
            Assert::AreEqual(std::wstring(L"Error message goes here."), vmMessage.GetMessage());

            return DialogResult::OK;
        });

        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());
        vmAssetList.SaveSelected();
        Assert::IsTrue(bMessageSeen);
        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());
    }

    TEST_METHOD(TestSaveSelectedInvalidAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(1U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        vmAssetList.SetValidationError(pItem->GetID(), L"Error message goes here.");
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAll);

        bool bMessageSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"Unable to save"), vmMessage.GetHeader());
            Assert::AreEqual(std::wstring(L"Error message goes here."), vmMessage.GetMessage());

            return DialogResult::OK;
        });

        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());
        vmAssetList.SaveSelected();
        Assert::IsTrue(bMessageSeen);
        Assert::AreEqual(AssetChanges::Modified, pItem->GetChanges());
    }

    TEST_METHOD(TestSaveSelectedValidationWarning)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        pItem->SetTrigger("A:0x1234");
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        // item is not validated until saved
        Assert::AreEqual(std::wstring(), pItem->GetValidationError());

        vmAssetList.SaveSelected();

        // invalid item can still be saved
        const auto& sText = vmAssetList.GetUserFile(L"22");
        AssertContains(sText, "1:\"A:0x1234\":Test1b:Desc1:::::5:::::12345");

        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), pItem->GetValidationError());

        pItem->SetName(L"Test1c");
        vmAssetList.SaveSelected();

        // invalid item can still be saved
        const auto& sText2 = vmAssetList.GetUserFile(L"22");
        AssertContains(sText2, "1:\"A:0x1234\":Test1c:Desc1:::::5:::::12345");
    }

    TEST_METHOD(TestSaveSelectedPublishCoreModified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 1 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        pItem->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Demote);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreModifiedCancel)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 1 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::No;
        });

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        pItem->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 0U }, vmAssetList.PublishedAssets.size());
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreModifiedAbort)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetValidationError(1U, L"NewFeature is pre-release functionality");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Publish aborted."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following items could not be published:\n* Test1b: NewFeature is pre-release functionality"), vmMessageBox.GetMessage());
            return DialogResult::OK;
        });

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetName(L"Test1b");
        pItem->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 0U }, vmAssetList.PublishedAssets.size());
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreValidationError)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 1 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        pItem->SetTrigger("A:0x1234");
        pItem->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), pItem->GetValidationError());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(std::wstring(L"Final condition type expects another condition to follow"), pItem->GetValidationError());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Demote);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test2", L"Desc2", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test3", L"Desc3", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 3 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem1 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem1 != nullptr);
        pItem1->SetName(L"Test1b");
        pItem1->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem1->GetChanges());

        auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);
        pItem2->SetName(L"Test2b");
        pItem2->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem2->GetChanges());

        auto* pItem3 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(2));
        Expects(pItem3 != nullptr);
        pItem3->SetName(L"Test3b");
        pItem3->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::PublishAll);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 3U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem1 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::IsTrue(pItem2 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(1)));
        Assert::IsTrue(pItem3 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(2)));
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedPublishCoreAllOnlyUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test2", L"Desc2", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test3", L"Desc3", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 2 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        auto* pItem1 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem1 != nullptr);
        pItem1->SetName(L"Test1b");
        pItem1->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem1->GetChanges());

        const auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(1));
        Expects(pItem2 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());

        auto* pItem3 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(2));
        Expects(pItem3 != nullptr);
        pItem3->SetName(L"Test3b");
        pItem3->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::PublishAll);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 2U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem1 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::IsTrue(pItem3 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(1)));
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedPromoteToCore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to promote 1 items to core?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Items in core are officially available for players to earn."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        const auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Promote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pItem->GetCategory());

        // item will have been moved to core, so nothing will be selected
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedPromoteAllToCore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 6, L"Test2", L"Desc2", L"12345", "0xH1234=2");
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 7, L"Test3", L"Desc3", L"12345", "0xH1234=3");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to promote 3 items to core?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Items in core are officially available for players to earn."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        const auto* pItem1 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem1 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        const auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem2 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        const auto* pItem3 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem3 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::PromoteAll);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 3U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem1 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pItem1->GetCategory());
        Assert::IsTrue(pItem2 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pItem2->GetCategory());
        Assert::IsTrue(pItem3 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pItem3->GetCategory());

        // item will have been moved to core, so nothing will be selected
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedPromoteToCoreValidationError)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);
        vmAssetList.SetValidationError(1U, L"Test excuse");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Promote to core aborted."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following items could not be published:\n* Test1: Test excuse"), vmMessageBox.GetMessage());
            return DialogResult::OK;
        });

        const auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Promote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 0U }, vmAssetList.PublishedAssets.size());
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(AssetCategory::Unofficial, pItem->GetCategory());
    }

    TEST_METHOD(TestSaveSelectedPromoteToCoreServerError)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);
        vmAssetList.SetPublishServerError(1U, "Not allowed.");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to promote 1 items to core?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Items in core are officially available for players to earn."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        const auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Promote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());      // tried to publish
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(AssetCategory::Unofficial, pItem->GetCategory()); // didn't change category
    }

    TEST_METHOD(TestSaveSelectedPromoteToCoreParseError)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Unofficial, 5, L"Test1", L"Desc1", L"12345", "P:P:0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Unofficial);

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Promote to core aborted."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following items could not be published:\n* Test1: Parse Error -2: Invalid memory operand"), vmMessageBox.GetMessage());
            return DialogResult::OK;
        });

        const auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Promote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 0U }, vmAssetList.PublishedAssets.size());
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(AssetCategory::Unofficial, pItem->GetCategory());
    }

    TEST_METHOD(TestSaveSelectedDemoteToUnofficial)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to demote 1 items to unofficial?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Items in unofficial can no longer be earned by players."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        const auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Demote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(AssetCategory::Unofficial, pItem->GetCategory());

        // item will have been moved to core, so nothing will be selected
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedDemoteToUnofficialServerError)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.SetPublishServerError(1U, "Not allowed.");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to demote 1 items to unofficial?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Items in unofficial can no longer be earned by players."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        const auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Demote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());      // tried to publish
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pItem->GetCategory());       // didn't change category
    }

    TEST_METHOD(TestSaveSelectedDemoteAllToUnofficial)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 6, L"Test2", L"Desc2", L"12345", "0xH1234=2");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test3", L"Desc3", L"12345", "0xH1234=3");

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to demote 3 items to unofficial?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Items in unofficial can no longer be earned by players."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        const auto* pItem1 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem1 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        const auto* pItem2 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem2 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        const auto* pItem3 = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem3 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());

        // there is no "Demote All"
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(2)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Demote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 3U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem1 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        Assert::AreEqual(AssetCategory::Unofficial, pItem1->GetCategory());
        Assert::IsTrue(pItem2 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        Assert::AreEqual(AssetCategory::Unofficial, pItem2->GetCategory());
        Assert::IsTrue(pItem3 == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem3->GetChanges());
        Assert::AreEqual(AssetCategory::Unofficial, pItem3->GetCategory());

        // item will have been moved to unofficial, so nothing will be selected
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveAllDisabled);
    }

    TEST_METHOD(TestSaveSelectedDemoteLeaderboardToUnofficial)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::None);

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Leaderboards cannot be demoted."), vmMessageBox.GetMessage());
            return DialogResult::OK;
        });

        const auto* pItem1 = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Expects(pItem1 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());

        const auto* pItem2 = vmAssetList.mockGameContext.Assets().GetItemAt(1);
        Expects(pItem2 != nullptr);
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Demote);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual(AssetChanges::None, pItem1->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pItem1->GetCategory());
        Assert::AreEqual(AssetChanges::None, pItem2->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pItem2->GetCategory());

        // item will have been moved to core, so nothing will be selected
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Demote);
    }

    TEST_METHOD(TestSaveSelectedPublishLocal)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.mockGameContext.SetGameTitle(L"GameName");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);

        vmAssetList.MockUserFileContents("111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        std::vector<ra::data::models::AssetModelBase*> vEmptyList;
        vmAssetList.mockGameContext.Assets().ReloadAssets(vEmptyList);

        bool bDialogSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogSeen = true;
            Assert::AreEqual(std::wstring(L"Are you sure you want to publish 1 items?"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            return DialogResult::Yes;
        });

        const auto* pItem = dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Expects(pItem != nullptr);
        Assert::AreEqual(AssetChanges::Unpublished, pItem->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::Publish);
        vmAssetList.SaveSelected();

        Assert::IsTrue(bDialogSeen);

        Assert::AreEqual({ 1U }, vmAssetList.PublishedAssets.size());
        Assert::IsTrue(pItem == dynamic_cast<ra::data::models::AchievementModel*>(vmAssetList.PublishedAssets.at(0)));
        Assert::AreEqual(AssetChanges::None, pItem->GetChanges());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(SaveButtonState::SaveDisabled);

        // local file should be updated without the local achievement
        const auto& sText = vmAssetList.GetUserFile(L"22");
        Assert::AreEqual(std::string("0.0.0.0\nGameName\n"), sText);
    }

    TEST_METHOD(TestDoFrameUpdatesAssets)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        auto* pAchievement = dynamic_cast<AssetListViewModelHarness::MockAchievementModel*>(
            vmAssetList.mockGameContext.Assets().FindAchievement(1U));
        Expects(pAchievement != nullptr);
        pAchievement->Activate();
        Assert::AreEqual(AssetState::Waiting, pAchievement->GetState());

        auto* pTrigger = pAchievement->m_pInfo->trigger;
        Expects(pTrigger != nullptr);
        pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
        Assert::AreEqual(AssetState::Waiting, pAchievement->GetState());

        vmAssetList.mockGameContext.DoFrame();
        Assert::AreEqual(AssetState::Active, pAchievement->GetState());
    }

    TEST_METHOD(TestCreateNewAchievement)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;

        // second new Local achievement should be focused, deselecting the first
        vmAssetList.CreateNew();
        Assert::AreEqual({ 4U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsFalse(pAsset->IsSelected());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());

        // both achievements should be loaded in the runtime
        auto* pClient = vmAssetList.mockRuntime.GetClient();
        auto* pAch1 = (rc_client_achievement_info_t*)rc_client_get_achievement_info(pClient, 111000001U);
        Expects(pAch1 != nullptr);
        Assert::IsNull(pAch1->trigger); // trigger not set until activated
        Assert::AreEqual({0}, pAch1->public_.points);

        auto* pAch2 = (rc_client_achievement_info_t*)rc_client_get_achievement_info(pClient, 111000002U);
        Expects(pAch2 != nullptr);
        Assert::IsNull(pAch2->trigger);
        Assert::AreEqual({0}, pAch2->public_.points);
    }

    TEST_METHOD(TestCreateNewHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::No);

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        // new was aborted
        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsFalse(bEditorShown);

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::Yes);
        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;
    }

    TEST_METHOD(TestCreateNewFilterCategoryAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::All);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::All, vmAssetList.GetCategoryFilter());

        vmAssetList.CreateNew();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 3U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::All, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(2);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());
    }

    TEST_METHOD(TestCreateNewSpecialFilterActive)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.mockGameContext.Assets().FindAchievement(1)->SetState(AssetState::Active);
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        vmAssetList.mockGameContext.Assets().FindAchievement(2)->SetState(AssetState::Active);
        vmAssetList.SetSpecialFilter(AssetListViewModel::SpecialFilter::Active);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::SpecialFilter::Active, vmAssetList.GetSpecialFilter());

        vmAssetList.CreateNew();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::SpecialFilter::All, vmAssetList.GetSpecialFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());
    }

    TEST_METHOD(TestCreateNewAchievementFilterTypeAchievement)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::Achievement);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(ra::data::models::AssetType::Achievement, vmAssetList.GetAssetTypeFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAchievement());
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsLeaderboard());
        Assert::IsTrue(bEditorShown);
    }

    TEST_METHOD(TestCreateNewLeaderboardFilterTypeLeaderboard)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::Leaderboard);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        // new Local leaderboard should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(ra::data::models::AssetType::Leaderboard, vmAssetList.GetAssetTypeFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsAchievement());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsLeaderboard());
        Assert::IsTrue(bEditorShown);
    }

    TEST_METHOD(TestCreateNewAchievementFilterTypeAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::None);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());

        bool bPromptShown = false;
        vmAssetList.mockDesktop.ExpectWindow<NewAssetViewModel>([&bPromptShown](NewAssetViewModel& vmNewAsset)
        {
            bPromptShown = true;
            vmNewAsset.NewAchievement();
            return DialogResult::OK;
        });

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        Assert::IsTrue(bPromptShown);

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(ra::data::models::AssetType::None, vmAssetList.GetAssetTypeFilter());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAchievement());
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsLeaderboard());
        Assert::IsTrue(bEditorShown);
    }

    TEST_METHOD(TestCreateNewLeaderboardFilterTypeAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::None);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());

        bool bPromptShown = false;
        vmAssetList.mockDesktop.ExpectWindow<NewAssetViewModel>([&bPromptShown](NewAssetViewModel& vmNewAsset)
        {
            bPromptShown = true;
            vmNewAsset.NewLeaderboard();
            return DialogResult::OK;
        });

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        Assert::IsTrue(bPromptShown);

        // new Local leaderboard should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(ra::data::models::AssetType::None, vmAssetList.GetAssetTypeFilter());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsAchievement());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsLeaderboard());
        Assert::IsTrue(bEditorShown);
    }

    TEST_METHOD(TestCreateNewCancelFilterTypeAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::None);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());

        bool bPromptShown = false;
        vmAssetList.mockDesktop.ExpectWindow<NewAssetViewModel>([&bPromptShown](NewAssetViewModel&)
        {
            bPromptShown = true;
            return DialogResult::Cancel;
        });

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        Assert::IsTrue(bPromptShown);

        // no new item should have been created. existing filters should be maintained
        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(ra::data::models::AssetType::None, vmAssetList.GetAssetTypeFilter());

        // editor should not be opened
        Assert::IsFalse(bEditorShown);
    }

    TEST_METHOD(TestCreateNewRichPresenceFilterTypeRichPresence)
    {
        AssetListViewModelHarness vmAssetList;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::services::impl::FileLocalStorage storage(mockFileSystem);
        ra::services::ServiceLocator::ServiceOverride<ra::services::ILocalStorage> storageOverride(&storage);

        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::RichPresence);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 0U }, vmAssetList.FilteredAssets().Count());

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        // new Local rich presence should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(ra::data::models::AssetType::RichPresence, vmAssetList.GetAssetTypeFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Rich Presence"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 0 }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded externally
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        Assert::AreEqual(std::string("file://localhost/./RACache/Data/22-Rich.txt"), vmAssetList.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestCreateNewRichPresenceFilterTypeRichPresenceExisting)
    {
        AssetListViewModelHarness vmAssetList;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::services::impl::FileLocalStorage storage(mockFileSystem);
        ra::services::ServiceLocator::ServiceOverride<ra::services::ILocalStorage> storageOverride(&storage);

        vmAssetList.mockUserContext.Initialize("User1", "FOO");
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        vmAssetList.AddRichPresence("Display\nTest\n");
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::RichPresence);
        vmAssetList.ForceUpdateButtons();

        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        Assert::IsTrue(vmAssetList.CanCreate());
        vmAssetList.CreateNew();

        // existing Local rich presence should be focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(ra::data::models::AssetType::RichPresence, vmAssetList.GetAssetTypeFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Rich Presence"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
        Assert::AreEqual({ 0 }, pAsset->GetId());
        Assert::AreEqual(0, pAsset->GetPoints());

        // and loaded externally
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        Assert::AreEqual(std::string("file://localhost/./RACache/Data/22-Rich.txt"), vmAssetList.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestCloneSingle)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test2 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(7, pAsset->GetPoints());

        const auto* pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc2"), pAchievement->GetDescription());
        Assert::AreEqual(std::wstring(L"11111"), pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xH1111=1"), pAchievement->GetTrigger());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;

        // copying the copy should create another copy, deselecting the first
        vmAssetList.CloneSelected();
        Assert::AreEqual({ 4U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsFalse(pAsset->IsSelected());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
    }

    TEST_METHOD(TestCloneHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::No);
        Assert::IsTrue(vmAssetList.CanClone());
        vmAssetList.CloneSelected();

        // clone was aborted
        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());

        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsFalse(bEditorShown);

        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(DialogResult::Yes);
        Assert::IsTrue(vmAssetList.CanClone());
        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test2 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(7, pAsset->GetPoints());

        const auto* pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc2"), pAchievement->GetDescription());
        Assert::AreEqual(std::wstring(L"11111"), pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xH1111=1"), pAchievement->GetTrigger());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;
    }

    TEST_METHOD(TestCloneLocal)
    {
        AssetListViewModelHarness vmAssetList;
        const auto* pLocalBadges = vmAssetList.AddLocalBadgesModel();
        Expects(pLocalBadges != nullptr);
        const std::wstring sLocalBadgeName = L"local\\1234.png";

        vmAssetList.SetGameId(22U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        auto* pSourceAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(2);
        Expects(pSourceAchievement != nullptr);
        pSourceAchievement->SetBadge(sLocalBadgeName);

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test1 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(5, pAsset->GetPoints());

        const auto* pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc1"), pAchievement->GetDescription());
        Assert::AreEqual(sLocalBadgeName, pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xH1234=1"), pAchievement->GetTrigger());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);

        // image should have two references - one for source achievement, and one for clone
        Assert::AreEqual(2, pLocalBadges->GetReferenceCount(sLocalBadgeName, false));
    }

    TEST_METHOD(TestCloneSingleFilterCategoryAll)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::All);

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::All, vmAssetList.GetCategoryFilter());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused without changing the filter category
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 3U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::All, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::AreEqual({ 1U }, pAsset->GetId());
        Assert::IsFalse(pAsset->IsSelected());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(2);
        Expects(pAsset != nullptr);
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::IsTrue(pAsset->IsSelected());
    }

    TEST_METHOD(TestCloneSingleSpecialFilterActive)
    {
        AssetListViewModelHarness vmAssetList;

        vmAssetList.SetGameId(22U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        auto* pSourceAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(1);
        Expects(pSourceAchievement != nullptr);
        pSourceAchievement->SetState(ra::data::models::AssetState::Active);
        vmAssetList.SetSpecialFilter(AssetListViewModel::SpecialFilter::Active);
        Assert::AreEqual(AssetListViewModel::SpecialFilter::Active, vmAssetList.GetSpecialFilter());

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual(AssetListViewModel::SpecialFilter::All, vmAssetList.GetSpecialFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::AreEqual({ 1U }, pAsset->GetId());
        Assert::AreEqual(AssetState::Active, pAsset->GetState());
        Assert::IsFalse(pAsset->IsSelected());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::IsTrue(pAsset->IsSelected());
    }

    TEST_METHOD(TestCloneMultiple)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 9, L"Test3", L"Desc3", L"12321", "0xH5342=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 11, L"Test4", L"Desc4", L"55555", "0xHface=1");

        Assert::AreEqual({ 4U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 4U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        vmAssetList.FilteredAssets().GetItemAt(3)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.CloneSelected();

        // new Local achievements should be created and focused
        Assert::AreEqual({ 6U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test2 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(7, pAsset->GetPoints());

        auto* pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc2"), pAchievement->GetDescription());
        Assert::AreEqual(std::wstring(L"11111"), pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xH1111=1"), pAchievement->GetTrigger());

        pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test4 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000002U }, pAsset->GetId());
        Assert::AreEqual(11, pAsset->GetPoints());

        pAchievement = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchievement != nullptr);
        Assert::AreEqual(std::wstring(L"Desc4"), pAchievement->GetDescription());
        Assert::AreEqual(std::wstring(L"55555"), pAchievement->GetBadge());
        Assert::AreEqual(std::string("0xHface=1"), pAchievement->GetTrigger());

        // editor is not shown when multiple assets are cloned
        Assert::IsFalse(vmAssetList.mockDesktop.WasDialogShown());

        // both items should be selected
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmAssetList.FilteredAssets().GetItemAt(1)->IsSelected());
    }

    TEST_METHOD(TestCloneInvalid)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.SetValidationError(vmAssetList.FilteredAssets().GetItemAt(1)->GetId(), L"Error message goes here.");
        vmAssetList.ForceUpdateButtons();

        bool bMessageSeen = false;
        vmAssetList.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessage)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"Unable to clone"), vmMessage.GetHeader());
            Assert::AreEqual(std::wstring(L"Error message goes here."), vmMessage.GetMessage());

            return DialogResult::OK;
        });

        vmAssetList.CloneSelected();

        // achievement should not be created
        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::IsTrue(bMessageSeen);
    }

    TEST_METHOD(TestCloneReset)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual({ 3U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Test2 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(7, pAsset->GetPoints());

        // save the asset - if we reset without saving, it'll just get deleted
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SaveSelected();
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        // resetting the achievement should not change anything about it
        vmAssetList.ForceUpdateButtons();
        vmAssetList.ResetSelected();

        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::AreEqual(std::wstring(L"Test2 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());
        Assert::AreEqual(7, pAsset->GetPoints());
    }

    TEST_METHOD(TestCloneResetWithTemporaryImage)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        const auto* pLocalBadges = vmAssetList.AddLocalBadgesModel();
        Expects(pLocalBadges != nullptr);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        const std::wstring sBadge1 = L"local\\ABCD.png";
        const std::wstring sBadge2 = L"local\\EFGH.png";

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        pAsset->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        const auto* pAch = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAch != nullptr);

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        // new achievement should have updated uncommitted reference count to badge
        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        auto* pAchClone = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchClone != nullptr);

        // save the asset - if we reset without saving, it'll just get deleted
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SaveSelected();
        Assert::AreEqual(pAch->GetBadge(), pAchClone->GetBadge());
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));

        // resetting the achievement should not change anything about it
        vmAssetList.ForceUpdateButtons();
        vmAssetList.ResetSelected();
        Assert::AreEqual(pAch->GetBadge(), pAchClone->GetBadge());
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));

        // changing the badge should update reference counts
        pAchClone->SetBadge(sBadge1);
        Assert::AreEqual(sBadge1, pAchClone->GetBadge());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));

        // saving the asset should update reference counts
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SaveSelected();
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, true));

        // resetting the achievement should not change anything about it
        vmAssetList.ForceUpdateButtons();
        vmAssetList.ResetSelected();
        Assert::AreEqual(sBadge1, pAchClone->GetBadge());
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, true));

        // changing the badge should update reference counts
        pAchClone->SetBadge(sBadge2);
        Assert::AreEqual(sBadge2, pAchClone->GetBadge());
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));

        // resetting should update reference counts
        vmAssetList.ForceUpdateButtons();
        vmAssetList.ResetSelected();
        Assert::AreEqual(sBadge1, pAchClone->GetBadge());
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));
    }

    TEST_METHOD(TestCloneResetWithTemporaryImages)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        const auto* pLocalBadges = vmAssetList.AddLocalBadgesModel();
        Expects(pLocalBadges != nullptr);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");
        const std::wstring sBadge1 = L"local\\ABCD.png";
        const std::wstring sBadge2 = L"local\\EFGH.png";

        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pAsset != nullptr);
        pAsset->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        auto* pAch = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAch != nullptr);
        pAch->SetBadge(sBadge1);

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CloneSelected();

        // new Local achievement should be created and focused
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        // new achievement should have updated uncommitted reference count to badge
        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        auto* pAchClone = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAchClone != nullptr);
        Assert::AreEqual(pAch->GetBadge(), pAchClone->GetBadge());
        Assert::AreEqual(2, pLocalBadges->GetReferenceCount(sBadge1, false));

        // save the asset - if we reset without saving, it'll just get deleted
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SaveSelected();
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));

        // resetting the achievement should not change anything about it
        vmAssetList.ForceUpdateButtons();
        vmAssetList.ResetSelected();
        Assert::AreEqual(sBadge1, pAchClone->GetBadge());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));

        // changing the badge should update reference counts
        pAchClone->SetBadge(sBadge2);
        Assert::AreEqual(sBadge2, pAchClone->GetBadge());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));

        // saving the asset should update reference counts
        vmAssetList.ForceUpdateButtons();
        vmAssetList.SaveSelected();
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge2, true));

        // resetting the achievement should not change anything about it
        vmAssetList.ForceUpdateButtons();
        vmAssetList.ResetSelected();
        Assert::AreEqual(sBadge2, pAchClone->GetBadge());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge2, true));

        // changing the badge should update reference counts
        pAchClone->SetBadge(sBadge1);
        Assert::AreEqual(sBadge1, pAchClone->GetBadge());
        Assert::AreEqual(2, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));

        // resetting should update reference counts
        vmAssetList.ForceUpdateButtons();
        vmAssetList.ResetSelected();
        Assert::AreEqual(sBadge2, pAchClone->GetBadge());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge2, true));
    }

    TEST_METHOD(TestCloneLeaderboard)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::Leaderboard);
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Core, L"Leaderboard1");
        auto* vmLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(vmAssetList.mockGameContext.Assets().GetItemAt(0));
        Assert::IsNotNull(vmLeaderboard);
        Ensures(vmLeaderboard != nullptr);
        vmLeaderboard->SetDescription(L"Desc1");
        vmLeaderboard->SetStartTrigger("0=1");
        vmLeaderboard->SetSubmitTrigger("0xH1234=1");
        vmLeaderboard->SetCancelTrigger("0xH1234=2");
        vmLeaderboard->SetValueDefinition("0xH5555*2");
        vmLeaderboard->SetValueFormat(ra::data::ValueFormat::Score);
        vmLeaderboard->SetLowerIsBetter(true);

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        bool bEditorShown = false;
        vmAssetList.mockDesktop.ExpectWindow<AssetEditorViewModel>([&bEditorShown](AssetEditorViewModel&)
        {
            bEditorShown = true;
            return DialogResult::None;
        });

        vmAssetList.CloneSelected();

        // new Local leaderboard should be created and focused
        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Local, vmAssetList.GetCategoryFilter());

        const auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        Assert::IsTrue(pAsset->IsSelected());
        Assert::AreEqual(std::wstring(L"Leaderboard1 (copy)"), pAsset->GetLabel());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetState::Inactive, pAsset->GetState());
        Assert::AreEqual(AssetChanges::New, pAsset->GetChanges());
        Assert::AreEqual({ 111000001U }, pAsset->GetId());

        const auto* pLeaderboard = vmAssetList.mockGameContext.Assets().FindLeaderboard(pAsset->GetId());
        Expects(pLeaderboard != nullptr);
        Assert::AreEqual(std::wstring(L"Desc1"), pLeaderboard->GetDescription());
        Assert::AreEqual(std::string("0=1"), pLeaderboard->GetStartTrigger());
        Assert::AreEqual(std::string("0xH1234=1"), pLeaderboard->GetSubmitTrigger());
        Assert::AreEqual(std::string("0xH1234=2"), pLeaderboard->GetCancelTrigger());
        Assert::AreEqual(std::string("0xH5555*2"), pLeaderboard->GetValueDefinition());
        Assert::AreEqual(ra::data::ValueFormat::Score, pLeaderboard->GetValueFormat());
        Assert::IsTrue(pLeaderboard->IsLowerBetter());

        // and loaded in the editor, which should be shown (local achievement will always have ID 0)
        Assert::AreEqual({ 0U }, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(bEditorShown);
        bEditorShown = false;
    }

    TEST_METHOD(TestResetSelectedAllUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::ResetAll);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"This will discard all unsaved changes and reset the assets to the last locally saved state."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"This will discard any changes to the selected assets and reset them to the last locally saved state."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedUnmodifiedFromFile)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedFromFile)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedAbort)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0xH2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::No;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        Assert::AreEqual(std::string("0xH2345=1"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedInvalid)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.SetValidationError(vmAssetList.FilteredAssets().GetItemAt(0)->GetId(), L"Error message goes here.");
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        int nDialogShown = 0;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&nDialogShown](MessageBoxViewModel& vmMessage)
        {
            Assert::AreEqual(std::wstring(L"Reload from disk?"), vmMessage.GetHeader());
            ++nDialogShown;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        // invalid asset should be allowed to be reset, with only one confirmation dialog
        Assert::AreEqual(1, nDialogShown);

        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedCoreHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel&) { return DialogResult::Yes; });
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::Yes);
        vmAssetList.ResetSelected();
        Assert::AreEqual(std::string("reset core achievements"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());

        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedCoreHardcoreCancel)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel&) { return DialogResult::Yes; });
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::No);
        vmAssetList.ResetSelected();
        Assert::AreEqual(std::string("reset core achievements"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());

        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedModifiedFromFileHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetTrigger("0x2345=1");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.MockUserFileContents("1:\"0xH1234=0\":Test:::::User:0:0:0:::00000\n");

        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel&) { return DialogResult::Yes; });
        vmAssetList.ResetSelected();
        Assert::AreEqual(std::string(), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());

        Assert::AreEqual(std::string("0xH1234=0"), pAsset->GetTrigger());
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedNoLongerExistsInFileOpenInEditor)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        vmAssetList.mockWindowManager.AssetEditor.LoadAsset(pAsset);
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.GetAsset() == pAsset);

        vmAssetList.MockUserFileContents("\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // achievement should be removed from the editor
        Assert::IsNull(vmAssetList.mockGameContext.Assets().FindAchievement({ 1U }));
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsNull(vmAssetList.mockWindowManager.AssetEditor.GetAsset());
    }

    TEST_METHOD(TestResetSelectedNoLongerExistsInFilePrimedChallenge)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* vmAsset = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Assert::IsNotNull(vmAsset);
        Ensures(vmAsset != nullptr);
        vmAsset->SetState(ra::data::models::AssetState::Primed);

        vmAssetList.mockOverlayManager.AddChallengeIndicator(vmAsset->GetID(), ra::ui::ImageType::Badge, "12345");
        const auto* pIndicator = vmAssetList.mockOverlayManager.GetChallengeIndicator(vmAsset->GetID());
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);
        Assert::IsFalse(pIndicator->IsDestroyPending());

        vmAssetList.MockUserFileContents("\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // RemoveChallengeIndicator only marks the item as to be destroyed.
        // it still exists until the next Render()
        Assert::IsTrue(pIndicator->IsDestroyPending());
    }

    TEST_METHOD(TestResetSelectedNoLongerExistsInFilePrimedLeaderboard)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddLeaderboard(ra::data::models::AssetCategory::Local, L"Leaderboard1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.SetAssetTypeFilter(ra::data::models::AssetType::Leaderboard);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        auto* vmAsset = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Assert::IsNotNull(vmAsset);
        Ensures(vmAsset != nullptr);
        vmAsset->SetState(ra::data::models::AssetState::Primed);

        vmAssetList.mockOverlayManager.AddScoreTracker(vmAsset->GetID());
        const auto* pIndicator = vmAssetList.mockOverlayManager.GetScoreTracker(vmAsset->GetID());
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);
        Assert::IsFalse(pIndicator->IsDestroyPending());

        vmAssetList.MockUserFileContents("\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // RemoveScoreTracker only marks the item as to be destroyed.
        // it still exists until the next Render()
        Assert::IsTrue(pIndicator->IsDestroyPending());
    }

    TEST_METHOD(TestResetSelectedSomeNew)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Ach1", L"Test1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test2", L"Desc2", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);

        // if new and non-new items are selected, allow the user to reset. new items will be discarded
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.MockUserFileContents("2:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // item marked as new should be removed, and not restored from the file.
        // other item will be updated from file
        Assert::IsNull(vmAssetList.mockGameContext.Assets().FindAchievement({ 1U }));
        const auto* pAch2 = vmAssetList.mockGameContext.Assets().FindAchievement({ 2U });
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::AreEqual(std::string("0xH2345=0"), pAch2->GetTrigger());
    }

    TEST_METHOD(TestResetSelectedAllSomeNew)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddNewAchievement(5, L"Ach1", L"Test1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test2", L"Desc2", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);

        // if new and non-new items are selected, allow the user to reset. new items will be discarded
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::ResetAll);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.MockUserFileContents("2:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // item marked as new should be removed, and not restored from the file.
        // other item will be updated from file
        Assert::IsNull(vmAssetList.mockGameContext.Assets().FindAchievement({ 1U }));
        const auto* pAch2 = vmAssetList.mockGameContext.Assets().FindAchievement({ 2U });
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::AreEqual(std::string("0xH2345=0"), pAch2->GetTrigger());
    }

    TEST_METHOD(TestResetSelectedAllNewFromFile)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::ResetAll);

        vmAssetList.MockUserFileContents("111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // new item will be added when there is no selection.
        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
    }

    TEST_METHOD(TestResetSelectedNewFromFileWithSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::Reset);

        vmAssetList.MockUserFileContents("111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::00000\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // new item will be ignored when a selection is reset.
        Assert::IsNull(vmAssetList.mockGameContext.Assets().FindAchievement({ 111000001U }));
    }

    TEST_METHOD(TestResetSelectedAllNewFromFileLocalImage)
    {
        AssetListViewModelHarness vmAssetList;
        const auto* pLocalBadges = vmAssetList.AddLocalBadgesModel();
        Expects(pLocalBadges != nullptr);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(ResetButtonState::ResetAll);
        const std::wstring sBadge = L"local\\ABCD.png";

        vmAssetList.MockUserFileContents("111000001:\"0xH2345=0\":Test2:::::User:0:0:0:::\"local\\\\ABCD.png\"\n");

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.ResetSelected();

        Assert::IsTrue(bDialogShown);

        // new item will be added when there is no selection.
        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 111000001U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Local, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge, false));

        vmAssetList.ResetSelected();
        Assert::IsTrue(bDialogShown);

        // make sure reference counts remain unchanged
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge, false));
    }

    TEST_METHOD(TestRevertSelectedAllUnmodified)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::RevertAll);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert from server?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard all local work and revert the assets to the last state retrieved from the server."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.RevertSelected();

        Assert::IsTrue(bDialogShown);

        const auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        Assert::AreEqual(AssetCategory::Core, pAsset->GetCategory());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestRevertSelectedAllUncommitted)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::RevertAll);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert from server?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard all local work and revert the assets to the last state retrieved from the server."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.RevertSelected();

        Assert::IsTrue(bDialogShown);
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestRevertSelectedAllUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::RevertAll);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert from server?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard all local work and revert the assets to the last state retrieved from the server."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        pAsset->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        vmAssetList.RevertSelected();

        Assert::IsTrue(bDialogShown);
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestRevertSelectedAllLocalUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(RevertButtonState::DeleteAll);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Permanently delete?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Assets not available on the server will be permanently deleted."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        pAsset->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        vmAssetList.RevertSelected();

        Assert::IsTrue(bDialogShown);

        pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNull(pAsset);
    }

    TEST_METHOD(TestRevertSelectedLocalUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::Delete);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Permanently delete?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Assets not available on the server will be permanently deleted."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        pAsset->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pAsset->GetChanges());

        vmAssetList.RevertSelected();

        Assert::IsTrue(bDialogShown);

        pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNull(pAsset);
    }

    TEST_METHOD(TestRevertSelectedLocalOpenInEditor)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::Delete);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Permanently delete?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Assets not available on the server will be permanently deleted."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        vmAssetList.mockWindowManager.AssetEditor.LoadAsset(pAsset);
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.GetAsset() == pAsset);

        vmAssetList.RevertSelected();
        Assert::IsTrue(bDialogShown);

        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsAssetLoaded());
        Assert::IsNull(vmAssetList.mockWindowManager.AssetEditor.GetAsset());
    }

    TEST_METHOD(TestRevertSelectedLocalActiveChallenge)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);

        auto* vmAsset = vmAssetList.mockGameContext.Assets().GetItemAt(0);
        Assert::IsNotNull(vmAsset);
        Ensures(vmAsset != nullptr);
        vmAsset->SetState(ra::data::models::AssetState::Primed);

        vmAssetList.mockOverlayManager.AddChallengeIndicator(vmAsset->GetID(), ra::ui::ImageType::Badge, "12345");
        const auto* pIndicator = vmAssetList.mockOverlayManager.GetChallengeIndicator(vmAsset->GetID());
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);
        Assert::IsFalse(pIndicator->IsDestroyPending());

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::Delete);

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Permanently delete?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Assets not available on the server will be permanently deleted."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmAssetList.RevertSelected();
        Assert::IsTrue(bDialogShown);

        // RemoveChallengeIndicator only marks the item as to be destroyed.
        // it still exists until the next Render()
        Assert::IsTrue(pIndicator->IsDestroyPending());
    }

    TEST_METHOD(TestRevertSelectedLocalWithTemporaryImages)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(22U);
        const std::wstring sBadge1 = L"local\\ABCD.png";
        const std::wstring sBadge2 = L"local\\EFGH.png";
        const auto* pLocalBadges = vmAssetList.AddLocalBadgesModel();
        Expects(pLocalBadges != nullptr);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", sBadge1, "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Local, 7, L"Test2", L"Desc2", sBadge2, "0xH1234=1");

        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge2, true));

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Permanently delete?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Assets not available on the server will be permanently deleted."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        // deleting first asset should update badge reference counts
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        auto* pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        pAsset->SetSelected(true);

        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::Delete);

        vmAssetList.RevertSelected();
        Assert::IsTrue(bDialogShown);

        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge2, true));

        // select the other asset
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        pAsset = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pAsset != nullptr);
        pAsset->SetSelected(true);

        // change the badge for other asset
        auto* pAch = vmAssetList.mockGameContext.Assets().FindAchievement(pAsset->GetId());
        Expects(pAch != nullptr);
        pAch->SetBadge(sBadge1);

        Assert::AreEqual(1, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));

        // delete it
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::Delete);
        vmAssetList.RevertSelected();

        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge1, true));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, false));
        Assert::AreEqual(0, pLocalBadges->GetReferenceCount(sBadge2, true));
    }

    TEST_METHOD(TestRevertSelectedCore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();

        bool bDialogShown = false;
        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([&bDialogShown](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert from server?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard any local work for the selected assets and revert them to the last state retrieved from the server."), vmMessageBox.GetMessage());

            bDialogShown = true;
            return DialogResult::Yes;
        });

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::Revert);

        vmAssetList.RevertSelected();

        Assert::IsTrue(bDialogShown);
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestRevertSelectedCoreHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::RevertAll);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel&) { return DialogResult::Yes; });
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::Yes);
        vmAssetList.RevertSelected();

        Assert::AreEqual(std::string("revert core assets"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        Assert::AreEqual(AssetChanges::None, pAsset->GetChanges());
    }

    TEST_METHOD(TestRevertSelectedCoreHardcoreCancel)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddThreeAchievements();
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::RevertAll);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel&) { return DialogResult::Yes; });
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::No);
        vmAssetList.RevertSelected();

        Assert::AreEqual(std::string("revert core assets"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());
    }

    TEST_METHOD(TestRevertSelectedLocalHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.MockGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Local, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.ForceUpdateButtons();
        vmAssetList.AssertButtonState(RevertButtonState::DeleteAll);

        auto* pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNotNull(pAsset);
        Ensures(pAsset != nullptr);
        pAsset->SetName(L"Apple");
        Assert::AreEqual(AssetChanges::Modified, pAsset->GetChanges());

        vmAssetList.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel&) { return DialogResult::Yes; });
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::Yes);
        vmAssetList.RevertSelected();

        Assert::AreEqual(std::string(), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        pAsset = vmAssetList.mockGameContext.Assets().FindAchievement({ 1U });
        Assert::IsNull(pAsset);
    }

    TEST_METHOD(TestIsProcessingActive)
    {
        AssetListViewModelHarness vmAssetList;
        Assert::IsTrue(vmAssetList.IsProcessingActive());
        Assert::IsFalse(vmAssetList.mockRuntime.IsPaused());
        Assert::IsFalse(vmAssetList.mockEmulatorContext.WasMemoryModified());

        vmAssetList.SetProcessingActive(false);
        Assert::IsFalse(vmAssetList.IsProcessingActive());
        Assert::IsTrue(vmAssetList.mockRuntime.IsPaused());
        Assert::IsTrue(vmAssetList.mockEmulatorContext.WasMemoryModified());

        vmAssetList.SetProcessingActive(true);
        Assert::IsTrue(vmAssetList.IsProcessingActive());
        Assert::IsFalse(vmAssetList.mockRuntime.IsPaused());
        Assert::IsTrue(vmAssetList.mockEmulatorContext.WasMemoryModified());
    }

    TEST_METHOD(TestOpenEditorNonHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        auto* pItem = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pItem != nullptr);
        pItem->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        vmAssetList.OpenEditor(pItem);
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        Assert::AreEqual(ra::to_unsigned(pItem->GetId()), vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::AreEqual(std::string(), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
    }

    TEST_METHOD(TestOpenEditorHardcore)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::Yes);

        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        auto* pItem = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pItem != nullptr);
        pItem->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        vmAssetList.OpenEditor(pItem);
        Assert::IsTrue(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        Assert::AreEqual(ra::to_unsigned(pItem->GetId()), vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::AreEqual(std::string("edit assets"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        Assert::IsFalse(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestOpenEditorHardcoreCancel)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::No);

        vmAssetList.SetGameId(22U);
        vmAssetList.AddAchievement(AssetCategory::Core, 5, L"Test1", L"Desc1", L"12345", "0xH1234=1");
        vmAssetList.AddAchievement(AssetCategory::Core, 7, L"Test2", L"Desc2", L"11111", "0xH1111=1");

        Assert::AreEqual({ 2U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 2U }, vmAssetList.FilteredAssets().Count());
        Assert::AreEqual(AssetListViewModel::CategoryFilter::Core, vmAssetList.GetCategoryFilter());

        auto* pItem = vmAssetList.FilteredAssets().GetItemAt(1);
        Expects(pItem != nullptr);
        pItem->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        vmAssetList.OpenEditor(pItem);
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        Assert::AreEqual(0U, vmAssetList.mockWindowManager.AssetEditor.GetID());
        Assert::AreEqual(std::string("edit assets"), vmAssetList.mockEmulatorContext.GetDisableHardcoreWarningMessage());
        Assert::IsTrue(vmAssetList.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestOpenEditorRichPresence)
    {
        AssetListViewModelHarness vmAssetList;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::services::impl::FileLocalStorage storage(mockFileSystem);
        ra::services::ServiceLocator::ServiceOverride<ra::services::ILocalStorage> storageOverride(&storage);

        vmAssetList.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmAssetList.mockEmulatorContext.MockDisableHardcoreWarning(ra::ui::DialogResult::Yes);
        vmAssetList.SetGameId(22U);
        vmAssetList.AddRichPresence("Display\nTest\n");
        vmAssetList.SetAssetTypeFilter(AssetType::RichPresence);

        Assert::AreEqual({ 1U }, vmAssetList.mockGameContext.Assets().Count());
        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());

        auto* pItem = vmAssetList.FilteredAssets().GetItemAt(0);
        Expects(pItem != nullptr);
        pItem->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());
        vmAssetList.OpenEditor(pItem);
        Assert::IsFalse(vmAssetList.mockWindowManager.AssetEditor.IsVisible());

        Assert::AreEqual(std::string("file://localhost/./RACache/Data/0-Rich.txt"), vmAssetList.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestUpdateButtonsRichPresenceSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.SetAssetTypeFilter(AssetType::RichPresence);
        vmAssetList.AddRichPresence("Display:\nTest\n");

        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateDisabled, SaveButtonState::SaveDisabled,
            ResetButtonState::ResetDisabled, RevertButtonState::Revert,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsRichPresenceAndOtherSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.SetAssetTypeFilter(AssetType::None);
        vmAssetList.AddRichPresence("Display:\nTest\n");
        vmAssetList.AddThreeAchievements();

        Assert::AreEqual({ 3U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.FilteredAssets().GetItemAt(1)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateDisabled, SaveButtonState::SaveDisabled,
            ResetButtonState::ResetDisabled, RevertButtonState::RevertDisabled,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }
    
    TEST_METHOD(TestUpdateButtonsRichPresenceLocalSelection)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Local);
        vmAssetList.SetAssetTypeFilter(AssetType::RichPresence);
        vmAssetList.AddRichPresence("Display:\nTest\n");
        vmAssetList.mockGameContext.Assets().FindRichPresence()->SetCategory(AssetCategory::Local);

        Assert::AreEqual({ 1U }, vmAssetList.FilteredAssets().Count());
        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(
            ActivateButtonState::ActivateDisabled, SaveButtonState::SaveDisabled,
            ResetButtonState::ResetDisabled, RevertButtonState::RevertDisabled,
            CreateButtonState::Enabled, CloneButtonState::Disabled
        );
    }

    TEST_METHOD(TestUpdateButtonsRichPresenceUnpublished)
    {
        AssetListViewModelHarness vmAssetList;
        vmAssetList.SetGameId(1U);
        vmAssetList.SetCategoryFilter(AssetListViewModel::CategoryFilter::Core);
        vmAssetList.SetAssetTypeFilter(AssetType::RichPresence);

        vmAssetList.AddRichPresence("Display:\nTest\n");
        auto* pRichPresence = vmAssetList.mockGameContext.Assets().FindRichPresence();
        pRichPresence->SetScript("Display:\nTest2\n");
        pRichPresence->UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, pRichPresence->GetChanges());

        Assert::AreEqual({1U}, vmAssetList.FilteredAssets().Count());
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(ActivateButtonState::ActivateAll, SaveButtonState::SaveAllDisabled,
                                      ResetButtonState::ResetAll, RevertButtonState::RevertAll,
                                      CreateButtonState::Enabled, CloneButtonState::Disabled);

        vmAssetList.FilteredAssets().GetItemAt(0)->SetSelected(true);
        vmAssetList.ForceUpdateButtons();

        vmAssetList.AssertButtonState(ActivateButtonState::ActivateDisabled, SaveButtonState::SaveDisabled,
                                      ResetButtonState::ResetDisabled, RevertButtonState::Revert,
                                      CreateButtonState::Enabled, CloneButtonState::Disabled);
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
