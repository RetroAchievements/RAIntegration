#include "CppUnitTest.h"

#include "data\context\EmulatorContext.hh"

#include "tests\data\DataAsserts.hh"
#include "tests\ui\UIAsserts.hh"

#include "tests\devkit\context\mocks\MockEmulatorMemoryContext.hh"
#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockClock.hh"
#include "tests\devkit\services\mocks\MockFileSystem.hh"
#include "tests\devkit\services\mocks\MockHttpRequester.hh"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace context {
namespace tests {

TEST_CLASS(EmulatorContext_Tests)
{
private:
    class EmulatorContextHarness : public EmulatorContext
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockRcClient mockRcClient;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::services::mocks::MockHttpRequester mockHttpRequester;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        GSL_SUPPRESS_F6 EmulatorContextHarness() noexcept
            : mockHttpRequester([](const ra::services::Http::Request&) { return ra::services::Http::Response(ra::services::Http::StatusCode::OK, ""); }),
              m_Override(this)
        {
            mockAchievementRuntime.MockGame();
        }

        void MockVersions(const std::string& sClientVersion, const std::string& sServerVersion, const std::string& sMinimumVersion)
        {
            SetClientVersion(sClientVersion);
            mockServer.HandleRequest<ra::api::LatestClient>([sServerVersion, sMinimumVersion](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
            {
                response.LatestVersion = sServerVersion;
                response.MinimumVersion = sMinimumVersion;
                response.Result = ra::api::ApiResult::Success;
                return true;
            });
        }

        void MockVersions(const std::string& sClientVersion, const std::string& sServerVersion)
        {
            MockVersions(sClientVersion, sServerVersion, sServerVersion);
        }

        ra::data::models::AchievementModel& MockAchievement()
        {
            auto& pAch = mockGameContext.Assets().NewAchievement();
            pAch.SetCategory(ra::data::models::AssetCategory::Core);
            pAch.SetID(1U);
            pAch.SetName(L"AchievementTitle");
            pAch.SetDescription(L"AchievementDescription");
            pAch.SetBadge(L"12345");
            pAch.SetPoints(5);
            pAch.SetState(ra::data::models::AssetState::Active);
            pAch.UpdateServerCheckpoint();
            return pAch;
        }

    private:
        ra::services::ServiceLocator::ServiceOverride<EmulatorContext> m_Override;
    };

public:
    TEST_METHOD(TestClientName)
    {
        EmulatorContextHarness emulator;

        emulator.Initialize(EmulatorID::RA_FCEUX, nullptr);
        Assert::AreEqual(std::string("RANes"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Gens, nullptr);
        Assert::AreEqual(std::string("RAGens_REWiND"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Libretro, nullptr);
        Assert::AreEqual(std::string("RALibRetro"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Meka, nullptr);
        Assert::AreEqual(std::string("RAMeka"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Nester, nullptr);
        Assert::AreEqual(std::string("RANes"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_PCE, nullptr);
        Assert::AreEqual(std::string("RAPCE"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Project64, nullptr);
        Assert::AreEqual(std::string("RAP64"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_QUASI88, nullptr);
        Assert::AreEqual(std::string("RAQUASI88"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        Assert::AreEqual(std::string("RASnes9X"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_VisualboyAdvance, nullptr);
        Assert::AreEqual(std::string("RAVisualBoyAdvance"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_AppleWin, nullptr);
        Assert::AreEqual(std::string("RAppleWin"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Oricutron, nullptr);
        Assert::AreEqual(std::string("RAOricutron"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::UnknownEmulator, "CustomName");
        Assert::AreEqual(std::string("CustomName"), emulator.GetClientName());
    }

    TEST_METHOD(TestValidateClientVersionCurrent)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("0.56");
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request& request, ra::api::LatestClient::Response& response)
        {
            Assert::AreEqual(static_cast<unsigned int>(EmulatorID::RA_Snes9x), request.EmulatorId);
            response.LatestVersion = "0.56";
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionNewer)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.MockVersions("0.57", "0.56");

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionApiErrorNonHardcore)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("0.57");
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
        {
            response.ErrorMessage = "Could not communicate with server.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not retrieve latest client version."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Could not communicate with server."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionApiErrorNonHardcoreLoggedIn)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("0.57");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
        {
            response.ErrorMessage = "Could not communicate with server.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not retrieve latest client version."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Could not communicate with server."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());
    }

    TEST_METHOD(TestValidateClientVersionApiErrorHardcore)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("0.57");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.mockConfiguration.SetApiToken("Token");
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
        {
            response.ErrorMessage = "Could not communicate with server.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not retrieve latest client version."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The latest client is required for hardcore mode. Login canceled.\nCould not communicate with server."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionApiErrorHardcoreLoginDisabled)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("0.57");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.mockUserContext.DisableLogin();
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
        {
            response.ErrorMessage = "Could not communicate with server.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not retrieve latest client version."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The latest client is required for hardcore mode.\nCould not communicate with server."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionApiErrorHardcoreLoggedIn)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("0.57");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
        {
            response.ErrorMessage = "Could not communicate with server.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not retrieve latest client version."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The latest client is required for hardcore mode. You will be logged out.\nCould not communicate with server."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::IsFalse(emulator.mockUserContext.IsLoggedIn());
    }

    TEST_METHOD(TestValidateClientVersionOlderNonHardcoreIgnore)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Would you like to update?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(), emulator.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestValidateClientVersionOlderNonHardcoreAcknowledge)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Would you like to update?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Yes;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());
    }

    TEST_METHOD(TestValidateClientVersionOlderHardcoreProceed)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionOlderHardcoreAcknowledge)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionAboveMinimumHardcoreProceed)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "0.58.0.0", "0.56.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Would you like to update?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionAboveMinimumHardcoreAcknowledge)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "0.58.0.0", "0.56.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Would you like to update?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Yes;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionAtMinimumHardcoreProceed)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "0.58.0.0", "0.57.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Would you like to update?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionBelowMinimumHardcoreAcknowledge)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.56.0.0", "0.58.0.0", "0.57.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.56\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());

        // ValidateClientVersion doesn't actually disable hardcore if the user chooses to upgrade.
        // That way it will still be enabled when they relaunch the emulator.
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionBelowMinimumHardcoreProceed)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockUserContext.SetUsername("User");
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.56.0.0", "0.58.0.0", "0.57.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.56\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });

        // login while hardcore is enabled
        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(), emulator.mockDesktop.LastOpenedUrl());
        // successful downgrade to softcore
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());

        // attempt to enable hardcore and abort
        emulator.mockDesktop.ResetExpectedWindows();
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.56\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });

        Assert::IsFalse(emulator.EnableHardcoreMode());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(), emulator.mockDesktop.LastOpenedUrl());
        // no change
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());

        // attempt to enable hardcore and agree
        emulator.mockDesktop.ResetExpectedWindows();
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.56\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.EnableHardcoreMode());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());
        // switch to hardcore didn't actually happen, and user is logged out
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(emulator.mockUserContext.IsLoggedIn());
    }

    TEST_METHOD(TestValidateClientVersionCurrentAboveMinimum)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.MockVersions("0.56", "0.56", "0.55");

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionTo1_0)
    {
        // ensures "1.0" doesn't get trimmed to "1"
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "1.0.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 1.0\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionFrom1_0)
    {
        // ensures "1.0" doesn't get trimmed to "1"
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("1.0.0.0", "1.0.1.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 1.0\n- New version: 1.0.1\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionUnknownClient)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("1.0");
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
        {
            response.ErrorMessage = "Unknown client";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not retrieve latest client version."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Unknown client"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionWithPrefix)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("v1.0", "1.0.1.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 1.0\n- New version: 1.0.1\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    static void ReplaceIntegrationVersion(std::string& sVersion)
    {
        const auto nIndex = sVersion.find("Integration/") + 12;
        const auto nIndex2 = sVersion.find(' ', nIndex);
        if (nIndex2 == std::string::npos)
            sVersion.replace(nIndex, std::string::npos, "VERSION");
        else
            sVersion.replace(nIndex, nIndex2 - nIndex, "VERSION");
    }

    TEST_METHOD(TestUserAgent)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("1.0");

        std::string sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("RASnes9X/1.0 (UnitTests) Integration/VERSION"), sVersion);

        emulator.Initialize(EmulatorID::RA_AppleWin, nullptr);
        emulator.SetClientVersion("1.1.1");

        sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("RAppleWin/1.1.1 (UnitTests) Integration/VERSION"), sVersion);

        emulator.Initialize(EmulatorID::RA_Libretro, nullptr);
        emulator.SetClientVersion("v1.3.8");

        sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("RALibRetro/1.3.8 (UnitTests) Integration/VERSION"), sVersion);

        emulator.Initialize(EmulatorID::UnknownEmulator, "CustomName");
        emulator.SetClientVersion("ver 3.1a");

        sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("CustomName/3.1a (UnitTests) Integration/VERSION"), sVersion);
    }

    TEST_METHOD(TestUserAgentClientDetail)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Libretro, nullptr);
        emulator.SetClientVersion("1.0.15");
        emulator.SetClientUserAgentDetail("fceumm_libretro/(SVN)_0a0fdb8");

        std::string sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("RALibRetro/1.0.15 (UnitTests) Integration/VERSION fceumm_libretro/(SVN)_0a0fdb8"), sVersion);
    }

    TEST_METHOD(TestWarnDisableHardcoreNotEnabled)
    {
        EmulatorContextHarness emulator;
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        Assert::IsTrue(emulator.WarnDisableHardcoreMode("test"));
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestWarnDisableHardcoreAborted)
    {
        EmulatorContextHarness emulator;
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        bool bWasShown = false;
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWasShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Disable Hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"You cannot test while Hardcore mode is active."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            bWasShown = true;

            return ra::ui::DialogResult::No;
        });

        Assert::IsFalse(emulator.WarnDisableHardcoreMode("test"));
        Assert::IsTrue(bWasShown);
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestWarnDisableHardcoreAllowed)
    {
        EmulatorContextHarness emulator;
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        bool bWasShown = false;
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWasShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Disable Hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"You cannot test while Hardcore mode is active."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            bWasShown = true;

            return ra::ui::DialogResult::Yes;
        });

        Assert::IsTrue(emulator.WarnDisableHardcoreMode("test"));
        Assert::IsTrue(bWasShown);
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestWarnDisableHardcoreNoActivity)
    {
        EmulatorContextHarness emulator;
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        Assert::IsTrue(emulator.WarnDisableHardcoreMode(""));
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestDisableHardcoreMode)
    {
        EmulatorContextHarness emulator;
        emulator.mockAchievementRuntime.MockGame();
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        emulator.mockAchievementRuntime.MockLeaderboardWithLboard(32U);
        emulator.mockAchievementRuntime.MockAchievementWithTrigger(44U);
        auto* pAchievement45 = emulator.mockAchievementRuntime.MockAchievementWithTrigger(45U);
        emulator.mockAchievementRuntime.UnlockAchievement(pAchievement45, RC_CLIENT_ACHIEVEMENT_UNLOCKED_SOFTCORE);
        auto* pAchievement46 = emulator.mockAchievementRuntime.MockAchievementWithTrigger(46U);
        emulator.mockAchievementRuntime.UnlockAchievement(pAchievement46, RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH);
        emulator.mockGameContext.InitializeFromAchievementRuntime();

        auto* vmAchievement44 = emulator.mockGameContext.Assets().FindAchievement(44U);
        auto* vmAchievement45 = emulator.mockGameContext.Assets().FindAchievement(45U);
        auto* vmAchievement46 = emulator.mockGameContext.Assets().FindAchievement(46U);
        auto* vmLeaderboard32 = emulator.mockGameContext.Assets().FindLeaderboard(32U);
        Expects(vmAchievement44 && vmAchievement45 && vmAchievement46 && vmLeaderboard32);

        Assert::IsTrue(vmAchievement44->IsActive());
        Assert::IsTrue(vmAchievement45->IsActive());
        Assert::IsFalse(vmAchievement46->IsActive());
        Assert::IsTrue(vmLeaderboard32->IsActive());

        emulator.DisableHardcoreMode();

        // ensure mode was deactivated
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()));

        // deactivating hardcore mode should not disable leaderboards
        Assert::IsTrue(vmLeaderboard32->IsActive());

        // deactivating hardcore mode should update active achievements
        Assert::IsTrue(vmAchievement44->IsActive());
        Assert::IsFalse(vmAchievement45->IsActive());
        Assert::IsFalse(vmAchievement46->IsActive());

        // deactivating hardcore mode should not reset the emulator
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentNoGame)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
        Assert::IsFalse(bWasReset); // if no game loaded, rc_client won't raise a reset event
    }

    TEST_METHOD(TestEnableHardcoreModeAlreadyEnabled)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U); // would normally show a dialog
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
                Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
                Assert::AreEqual(std::wstring(L"Enable hardcore mode?"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"Enabling hardcore mode will reset the emulator. You will lose any "
                                              L"progress that has not been saved through the game."),
                                 vmMessageBox.GetMessage());
                return ra::ui::DialogResult::Yes;
            });

        emulator.mockAchievementRuntime.MockLeaderboardWithLboard(32U);
        emulator.mockAchievementRuntime.MockAchievementWithTrigger(44U);
        auto* pAchievement45 = emulator.mockAchievementRuntime.MockAchievementWithTrigger(45U);
        emulator.mockAchievementRuntime.UnlockAchievement(pAchievement45, RC_CLIENT_ACHIEVEMENT_UNLOCKED_SOFTCORE);
        auto* pAchievement46 = emulator.mockAchievementRuntime.MockAchievementWithTrigger(46U);
        emulator.mockAchievementRuntime.UnlockAchievement(pAchievement46, RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH);
        emulator.mockGameContext.InitializeFromAchievementRuntime();

        auto* vmAchievement44 = emulator.mockGameContext.Assets().FindAchievement(44U);
        auto* vmAchievement45 = emulator.mockGameContext.Assets().FindAchievement(45U);
        auto* vmAchievement46 = emulator.mockGameContext.Assets().FindAchievement(46U);
        auto* vmLeaderboard32 = emulator.mockGameContext.Assets().FindLeaderboard(32U);
        Expects(vmAchievement44 && vmAchievement45 && vmAchievement46 && vmLeaderboard32);

        Assert::IsTrue(vmAchievement44->IsActive());
        Assert::IsFalse(vmAchievement45->IsActive());
        Assert::IsFalse(vmAchievement46->IsActive());
        Assert::IsFalse(vmLeaderboard32->IsActive());

        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsTrue(emulator.EnableHardcoreMode());

        // ensure hardcore mode was enabled
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()) != 0);

        // ensure prompt was shown
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());

        // enabling hardcore mode should enable leaderboards
        Assert::IsTrue(vmLeaderboard32->IsActive());

        // enabling hardcore mode should activate achievements not unlocked in hardcore
        Assert::IsTrue(vmAchievement44->IsActive());
        Assert::IsTrue(vmAchievement45->IsActive());
        Assert::IsFalse(vmAchievement46->IsActive());

        // enabling hardcore mode should reset the emulator
        Assert::IsTrue(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentGameLoadedCancel)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
            Assert::AreEqual(std::wstring(L"Enable hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Enabling hardcore mode will reset the emulator. You will lose any progress that has not been saved through the game."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsFalse(emulator.EnableHardcoreMode());

        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()) != 0);
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionNewerGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.58", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
            Assert::AreEqual(std::wstring(L"Enable hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Enabling hardcore mode will reset the emulator. You will lose any progress that has not been saved through the game."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Yes;
        });
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()) != 0);
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::IsTrue(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsFalse(emulator.EnableHardcoreMode());

        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()) != 0);
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());

        // user should be logged out if hardcore was enabled on an older version
        Assert::IsFalse(emulator.mockUserContext.IsLoggedIn());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedCancel)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsFalse(emulator.EnableHardcoreMode());

        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()) != 0);
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(""), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentGameLoadedNoPrompt)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;

        emulator.mockAchievementRuntime.MockLeaderboardWithLboard(32U);
        emulator.mockAchievementRuntime.MockAchievementWithTrigger(44U);
        auto* pAchievement45 = emulator.mockAchievementRuntime.MockAchievementWithTrigger(45U);
        emulator.mockAchievementRuntime.UnlockAchievement(pAchievement45, RC_CLIENT_ACHIEVEMENT_UNLOCKED_SOFTCORE);
        auto* pAchievement46 = emulator.mockAchievementRuntime.MockAchievementWithTrigger(46U);
        emulator.mockAchievementRuntime.UnlockAchievement(pAchievement46, RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH);
        emulator.mockGameContext.InitializeFromAchievementRuntime();

        auto* vmAchievement44 = emulator.mockGameContext.Assets().FindAchievement(44U);
        auto* vmAchievement45 = emulator.mockGameContext.Assets().FindAchievement(45U);
        auto* vmAchievement46 = emulator.mockGameContext.Assets().FindAchievement(46U);
        auto* vmLeaderboard32 = emulator.mockGameContext.Assets().FindLeaderboard(32U);
        Expects(vmAchievement44 && vmAchievement45 && vmAchievement46 && vmLeaderboard32);

        Assert::IsTrue(vmAchievement44->IsActive());
        Assert::IsFalse(vmAchievement45->IsActive());
        Assert::IsFalse(vmAchievement46->IsActive());
        Assert::IsFalse(vmLeaderboard32->IsActive());

        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsTrue(emulator.EnableHardcoreMode(false));

        // ensure hardcore mode was enabled
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()) != 0);

        // ensure prompt was not shown
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());

        // enabling hardcore mode should enable leaderboards
        Assert::IsTrue(vmLeaderboard32->IsActive());

        // enabling hardcore mode should activate achievements not unlocked in hardcore
        Assert::IsTrue(vmAchievement44->IsActive());
        Assert::IsTrue(vmAchievement45->IsActive());
        Assert::IsFalse(vmAchievement46->IsActive());

        // enabling hardcore mode should reset the emulator
        Assert::IsTrue(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedNoPrompt)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
            Assert::AreEqual(std::wstring(L"A newer client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsFalse(emulator.EnableHardcoreMode(false));

        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(rc_client_get_hardcore_enabled(emulator.mockAchievementRuntime.GetClient()) != 0);
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());

        // user should be logged out if hardcore was enabled on an older version
        Assert::IsFalse(emulator.mockUserContext.IsLoggedIn());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeClosesEditors)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.58", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            return ra::ui::DialogResult::Yes;
        });

        ra::services::mocks::MockLocalStorage mockLocalStorage; // required by RichPresenceMonitor.Show()

        emulator.mockWindowManager.RichPresenceMonitor.Show();
        emulator.mockWindowManager.AssetList.Show();
        emulator.mockWindowManager.AssetEditor.Show();
        emulator.mockWindowManager.MemoryBookmarks.Show();
        emulator.mockWindowManager.MemoryInspector.Show();
        emulator.mockWindowManager.CodeNotes.Show();

        Assert::IsTrue(emulator.mockWindowManager.RichPresenceMonitor.IsVisible());
        Assert::IsTrue(emulator.mockWindowManager.AssetList.IsVisible());
        Assert::IsTrue(emulator.mockWindowManager.AssetEditor.IsVisible());
        Assert::IsTrue(emulator.mockWindowManager.MemoryBookmarks.IsVisible());
        Assert::IsTrue(emulator.mockWindowManager.MemoryInspector.IsVisible());
        Assert::IsTrue(emulator.mockWindowManager.CodeNotes.IsVisible());

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockWindowManager.RichPresenceMonitor.IsVisible());
        Assert::IsTrue(emulator.mockWindowManager.AssetList.IsVisible());
        Assert::IsFalse(emulator.mockWindowManager.AssetEditor.IsVisible());
        Assert::IsFalse(emulator.mockWindowManager.MemoryBookmarks.IsVisible());
        Assert::IsFalse(emulator.mockWindowManager.MemoryInspector.IsVisible());
        Assert::IsFalse(emulator.mockWindowManager.CodeNotes.IsVisible());
    }

    TEST_METHOD(TestGetAppTitleDefault)
    {
        EmulatorContextHarness emulator;
        Assert::AreEqual(std::wstring(L" -  []"), emulator.GetAppTitle(""));

        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.SetClientVersion("0.57.0.0");
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleVersion)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("retroachievements.org");

        emulator.SetClientVersion("0.57.1.1");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));

        emulator.SetClientVersion("0.99");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.99"), emulator.GetAppTitle(""));

        emulator.SetClientVersion("1.0.12.0");
        Assert::AreEqual(std::wstring(L"RASnes9X - 1.0"), emulator.GetAppTitle(""));

        emulator.SetClientVersion("v2.5.6");
        Assert::AreEqual(std::wstring(L"RASnes9X - v2.5"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleUserName)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        emulator.SetClientVersion("0.57");

        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));

        emulator.mockUserContext.Initialize("User", "UserDisplay", "Token");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 - UserDisplay"), emulator.GetAppTitle(""));

        emulator.mockUserContext.Logout();
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleCustomMessage)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 - Test"), emulator.GetAppTitle("Test"));
    }

    TEST_METHOD(TestGetAppTitleCustomHost)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("localhost");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 [localhost]"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleEverything)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x, nullptr);
        emulator.mockConfiguration.SetHostName("localhost");
        emulator.mockUserContext.Initialize("User", "UserDisplay", "Token");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 - Test - UserDisplay [localhost]"), emulator.GetAppTitle("Test"));
    }

    TEST_METHOD(TestInitializeUnknownEmulator)
    {
        EmulatorContextHarness emulator;
        emulator.mockDesktop.SetRunningExecutable(L"C:\\Games\\Emulator\\RATest.exe");
        emulator.Initialize(ra::itoe<EmulatorID>(9999), nullptr);
        Assert::AreEqual(ra::etoi(EmulatorID::UnknownEmulator), ra::etoi(emulator.GetEmulatorId()));
        Assert::AreEqual(std::string("RATest"), emulator.GetClientName());
    }

    TEST_METHOD(TestValidateUnknownEmulator)
    {
        EmulatorContextHarness emulator;
        Assert::AreEqual(ra::etoi(EmulatorID::UnknownEmulator), ra::etoi(emulator.GetEmulatorId()));
        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    static std::array<uint8_t, 64> memory;

    static uint8_t ReadMemory0(uint32_t nAddress) noexcept { return memory.at(nAddress); }
    static uint8_t ReadMemory1(uint32_t nAddress) noexcept { return memory.at(gsl::narrow_cast<size_t>(nAddress) + 10); }
    static uint8_t ReadMemory2(uint32_t nAddress) noexcept { return memory.at(gsl::narrow_cast<size_t>(nAddress) + 20); }
    static uint8_t ReadMemory3(uint32_t nAddress) noexcept { return memory.at(gsl::narrow_cast<size_t>(nAddress) + 30); }

    static uint32_t ReadMemoryBlock0(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes) noexcept
    {
        memcpy(pBuffer, &memory.at(nAddress), nBytes);
        return nBytes;
    }

    static uint32_t ReadMemoryBlock1(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes) noexcept
    {
        memcpy(pBuffer, &memory.at(gsl::narrow_cast<size_t>(nAddress)) + 10, nBytes);
        return nBytes;
    }

    TEST_METHOD(TestEnableHardcoreModeUnfreezesBookmarks)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&emulator](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            return ra::ui::DialogResult::Yes;
        });

        constexpr int nBookmarks = 10;
        auto& pBookmarks = emulator.mockWindowManager.MemoryBookmarks;
        for (int i = 0; i < nBookmarks; ++i)
            pBookmarks.AddBookmark(i, Memory::Size::EightBit);

        pBookmarks.Bookmarks().Items()
            .GetItemAt<ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel>(3)
                ->SetBehavior(ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::Frozen);
        pBookmarks.Bookmarks().Items()
            .GetItemAt<ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel>(5)
                ->SetBehavior(ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::PauseOnChange);
        pBookmarks.Bookmarks().Items()
            .GetItemAt<ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel>(8)
                ->SetBehavior(ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::Frozen);

        Assert::IsTrue(emulator.EnableHardcoreMode());

        for (int i = 0; i < nBookmarks; ++i)
        {
            const auto* pBookmark = pBookmarks.Bookmarks().Items().GetItemAt<ra::ui::viewmodels::MemoryBookmarksViewModel::MemoryBookmarkViewModel>(i);
            Expects(pBookmark != nullptr);
            Assert::IsTrue(pBookmark->GetBehavior() == ra::ui::viewmodels::MemoryBookmarksViewModel::BookmarkBehavior::None);
        }
    }

    TEST_METHOD(TestEnableHardcoreModeDisablesModifiedAssets)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        emulator.mockAchievementRuntime.GetClient()->state.hardcore = 0;
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&emulator](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            return ra::ui::DialogResult::Yes;
        });

        emulator.mockAchievementRuntime.MockAchievementWithTrigger(44U);
        emulator.mockAchievementRuntime.MockAchievementWithTrigger(45U);
        emulator.mockAchievementRuntime.MockAchievementWithTrigger(46U);
        emulator.mockAchievementRuntime.MockAchievementWithTrigger(47U);
        emulator.mockGameContext.InitializeFromAchievementRuntime();

        auto* vmAchievement44 = emulator.mockGameContext.Assets().FindAchievement(44U);
        auto* vmAchievement45 = emulator.mockGameContext.Assets().FindAchievement(45U);
        vmAchievement45->SetName(L"New Name"); // modified
        auto* vmAchievement46 = emulator.mockGameContext.Assets().FindAchievement(46U);
        vmAchievement46->SetNew(); // new
        auto* vmAchievement47 = emulator.mockGameContext.Assets().FindAchievement(47U);
        vmAchievement47->SetName(L"New Name");
        vmAchievement47->UpdateLocalCheckpoint(); // unpublished

        Assert::AreEqual(ra::data::models::AssetChanges::None, vmAchievement44->GetChanges());
        Assert::AreEqual(ra::data::models::AssetChanges::Modified, vmAchievement45->GetChanges());
        Assert::AreEqual(ra::data::models::AssetChanges::New, vmAchievement46->GetChanges());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, vmAchievement47->GetChanges());

        Assert::AreEqual(ra::data::models::AssetState::Active, vmAchievement44->GetState());
        Assert::AreEqual(ra::data::models::AssetState::Active, vmAchievement45->GetState());
        Assert::AreEqual(ra::data::models::AssetState::Active, vmAchievement46->GetState());
        Assert::AreEqual(ra::data::models::AssetState::Active, vmAchievement47->GetState());

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::AreEqual(ra::data::models::AssetChanges::None, vmAchievement44->GetChanges());
        Assert::AreEqual(ra::data::models::AssetChanges::Modified, vmAchievement45->GetChanges());
        Assert::AreEqual(ra::data::models::AssetChanges::New, vmAchievement46->GetChanges());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, vmAchievement47->GetChanges());

        // enabling hardcore does not change the asset state to waiting. it issues a reset request
        // and resetting the runtime changes the asset state to waiting.
        Assert::AreEqual(ra::data::models::AssetState::Active, vmAchievement44->GetState());
        Assert::AreEqual(ra::data::models::AssetState::Inactive, vmAchievement45->GetState());
        Assert::AreEqual(ra::data::models::AssetState::Inactive, vmAchievement46->GetState());
        Assert::AreEqual(ra::data::models::AssetState::Inactive, vmAchievement47->GetState());
    }
};

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
