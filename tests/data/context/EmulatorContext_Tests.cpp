#include "CppUnitTest.h"

#include "data\context\EmulatorContext.hh"

#include "tests\data\DataAsserts.hh"
#include "tests\ui\UIAsserts.hh"

#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\devkit\services\mocks\MockFileSystem.hh"
#include "tests\devkit\services\mocks\MockHttpRequester.hh"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
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

    class EmulatorContextNotifyHarness : public EmulatorContext::NotifyTarget
    {
    public:
        void OnByteWritten(ra::ByteAddress nAddress, uint8_t nValue) noexcept override
        {
            nLastByteWrittenAddress = nAddress;
            nLastByteWritten = nValue;
        }

        ra::ByteAddress nLastByteWrittenAddress = 0xFFFFFFFFU;
        uint8_t nLastByteWritten = 0xFF;
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

    static void WriteMemory0(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(nAddress) = nValue; }
    static void WriteMemory1(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 10) = nValue; }
    static void WriteMemory2(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 20) = nValue; }
    static void WriteMemory3(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 30) = nValue; }

    static void InitializeMemory()
    {
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);
    }

    TEST_METHOD(TestIsValidAddress)
    {
        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::IsTrue(emulator.IsValidAddress(12U));
        Assert::IsTrue(emulator.IsValidAddress(25U));
        Assert::IsTrue(emulator.IsValidAddress(29U));
        Assert::IsFalse(emulator.IsValidAddress(30U));
    }

    TEST_METHOD(TestReadMemoryByte)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(25, static_cast<int>(emulator.ReadMemoryByte(25U)));
        Assert::AreEqual(4, static_cast<int>(emulator.ReadMemoryByte(4U)));
        Assert::AreEqual(29, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(30U)));
    }

    TEST_METHOD(TestAddMemoryBlocksOutOfOrder)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(25, static_cast<int>(emulator.ReadMemoryByte(25U)));
        Assert::AreEqual(4, static_cast<int>(emulator.ReadMemoryByte(4U)));
        Assert::AreEqual(29, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(30U)));
    }

    TEST_METHOD(TestAddMemoryBlockDoesNotOverwrite)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        emulator.AddMemoryBlock(0, 20, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(25, static_cast<int>(emulator.ReadMemoryByte(25U)));
        Assert::AreEqual(4, static_cast<int>(emulator.ReadMemoryByte(4U)));
        Assert::AreEqual(29, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(30U)));
    }

    TEST_METHOD(TestClearMemoryBlocks)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        emulator.ClearMemoryBlocks();
        emulator.AddMemoryBlock(0, 20, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 20U }, emulator.TotalMemorySize());

        Assert::AreEqual(32, static_cast<int>(emulator.ReadMemoryByte(2U)));
        Assert::AreEqual(41, static_cast<int>(emulator.ReadMemoryByte(11U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(55U)));
    }

    TEST_METHOD(TestInvalidMemoryBlock)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 40U }, emulator.TotalMemorySize());

        Assert::IsTrue(emulator.IsValidAddress(12U));
        Assert::IsTrue(emulator.IsValidAddress(19U));
        Assert::IsFalse(emulator.IsValidAddress(20U));
        Assert::IsFalse(emulator.IsValidAddress(29U));
        Assert::IsTrue(emulator.IsValidAddress(30U));
        Assert::IsTrue(emulator.IsValidAddress(39U));
        Assert::IsFalse(emulator.IsValidAddress(40U));

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(19, static_cast<int>(emulator.ReadMemoryByte(19U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(20U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(30, static_cast<int>(emulator.ReadMemoryByte(30U)));
        Assert::AreEqual(39, static_cast<int>(emulator.ReadMemoryByte(39U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(40U)));
    }

    TEST_METHOD(TestReadMemory)
    {
        memory.at(4) = 0xA8;
        memory.at(5) = 0x00;
        memory.at(6) = 0x37;
        memory.at(7) = 0x2E;

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);

        // ReadMemory calls ReadMemoryByte for small sizes
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_0)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_1)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_2)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_3)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_4)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_5)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_6)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_7)));
        Assert::AreEqual(3, static_cast<int>(emulator.ReadMemory(4U, MemSize::BitCount)));
        Assert::AreEqual(8, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Lower)));
        Assert::AreEqual(10, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Upper)));
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, MemSize::EightBit)));
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, MemSize::SixteenBit)));
        Assert::AreEqual(0x2E37, static_cast<int>(emulator.ReadMemory(6U, MemSize::SixteenBit)));
        Assert::AreEqual(0x2E3700, static_cast<int>(emulator.ReadMemory(5U, MemSize::TwentyFourBit)));
        Assert::AreEqual(0x2E3700A8, static_cast<int>(emulator.ReadMemory(4U, MemSize::ThirtyTwoBit)));
        Assert::AreEqual(0x372E, static_cast<int>(emulator.ReadMemory(6U, MemSize::SixteenBitBigEndian)));
        Assert::AreEqual(0x00372E, static_cast<int>(emulator.ReadMemory(5U, MemSize::TwentyFourBitBigEndian)));
        Assert::AreEqual(0xA800372EU, emulator.ReadMemory(4U, MemSize::ThirtyTwoBitBigEndian));

        memory.at(4) ^= 0xFF; // toggle all bits and verify again
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_0)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_1)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_2)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_3)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_4)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_5)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_6)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_7)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, MemSize::BitCount)));
        Assert::AreEqual(7, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Lower)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Upper)));
        Assert::AreEqual(0x57, static_cast<int>(emulator.ReadMemory(4U, MemSize::EightBit)));
    }

    TEST_METHOD(TestReadMemoryBlock)
    {
        for (size_t i = 0; i < memory.size(); i++)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

        memory.at(4) = 0xA8;
        memory.at(5) = 0x00;
        memory.at(6) = 0x37;
        memory.at(7) = 0x2E;

        memory.at(14) = 0x57;

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory1, &WriteMemory0); // purposefully use ReadMemory1 to detect using byte reader for non-byte reads
        emulator.AddMemoryBlockReader(0, &ReadMemoryBlock0);

        // ReadMemory calls ReadMemoryByte for small sizes - should call ReadMemory1 ($4 => $14)
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_0)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_1)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_2)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_3)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_4)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_5)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_6)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_7)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, MemSize::BitCount)));
        Assert::AreEqual(7, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Lower)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Upper)));
        Assert::AreEqual(0x57, static_cast<int>(emulator.ReadMemory(4U, MemSize::EightBit)));

        // sizes larger than 8 bits should use the block reader ($4 => $4)
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, MemSize::SixteenBit)));
        Assert::AreEqual(0x2E37, static_cast<int>(emulator.ReadMemory(6U, MemSize::SixteenBit)));
        Assert::AreEqual(0x2E3700, static_cast<int>(emulator.ReadMemory(5U, MemSize::TwentyFourBit)));
        Assert::AreEqual(0x2E3700A8, static_cast<int>(emulator.ReadMemory(4U, MemSize::ThirtyTwoBit)));
        Assert::AreEqual(0x372E, static_cast<int>(emulator.ReadMemory(6U, MemSize::SixteenBitBigEndian)));
        Assert::AreEqual(0x00372E, static_cast<int>(emulator.ReadMemory(5U, MemSize::TwentyFourBitBigEndian)));
        Assert::AreEqual(0xA800372EU, emulator.ReadMemory(4U, MemSize::ThirtyTwoBitBigEndian));

        // test the block reader directly
        uint8_t buffer[16];
        emulator.ReadMemory(0U, buffer, sizeof(buffer));
        for (size_t i = 0; i < sizeof(buffer); i++)
            Assert::AreEqual(gsl::at(buffer, i), memory.at(i));

        emulator.ReadMemory(4U, buffer, 1);
        Assert::AreEqual(gsl::at(buffer, 0), memory.at(4));
        Assert::AreEqual(gsl::at(buffer, 1), memory.at(1));
    }

    TEST_METHOD(TestReadMemoryBuffer)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        uint8_t buffer[32];

        // simple read within a block
        emulator.ReadMemory(6U, buffer, 12);
        Assert::IsTrue(memcmp(buffer, &memory.at(6), 11) == 0);

        // read across block
        emulator.ReadMemory(19U, buffer, 4);
        Assert::IsTrue(memcmp(buffer, &memory.at(19), 4) == 0);

        // read end of block
        emulator.ReadMemory(13U, buffer, 7);
        Assert::IsTrue(memcmp(buffer, &memory.at(13), 7) == 0);

        // read start of block
        emulator.ReadMemory(20U, buffer, 5);
        Assert::IsTrue(memcmp(buffer, &memory.at(20), 5) == 0);

        // read passed end of total memory
        emulator.ReadMemory(28U, buffer, 4);
        Assert::AreEqual(28, static_cast<int>(buffer[0]));
        Assert::AreEqual(29, static_cast<int>(buffer[1]));
        Assert::AreEqual(0, static_cast<int>(buffer[2]));
        Assert::AreEqual(0, static_cast<int>(buffer[3]));

        // read outside memory
        buffer[0] = buffer[1] = buffer[2] = buffer[3] = 0xFF;
        emulator.ReadMemory(37U, buffer, 4);
        Assert::AreEqual(0, static_cast<int>(buffer[0]));
        Assert::AreEqual(0, static_cast<int>(buffer[1]));
        Assert::AreEqual(0, static_cast<int>(buffer[2]));
        Assert::AreEqual(0, static_cast<int>(buffer[3]));
    }

    TEST_METHOD(TestReadMemoryBufferInvalidMemoryBlock)
    {
        constexpr uint8_t zero = 0;
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 40U }, emulator.TotalMemorySize());

        uint8_t buffer[32];

        // simple read within valid block
        emulator.ReadMemory(6U, buffer, 12);
        Assert::IsTrue(memcmp(buffer, &memory.at(6), 11) == 0);

        // simple read within invalid block
        emulator.ReadMemory(22U, buffer, 4);
        Assert::AreEqual(zero, buffer[0]);
        Assert::AreEqual(zero, buffer[1]);
        Assert::AreEqual(zero, buffer[2]);
        Assert::AreEqual(zero, buffer[3]);

        // read across block (valid -> invalid)
        emulator.ReadMemory(16U, buffer, 8);
        Assert::AreEqual(memory.at(16), buffer[0]);
        Assert::AreEqual(memory.at(17), buffer[1]);
        Assert::AreEqual(memory.at(18), buffer[2]);
        Assert::AreEqual(memory.at(19), buffer[3]);
        Assert::AreEqual(zero, buffer[4]);
        Assert::AreEqual(zero, buffer[5]);
        Assert::AreEqual(zero, buffer[6]);
        Assert::AreEqual(zero, buffer[7]);

        // read across block (invalid -> valid)
        emulator.ReadMemory(26U, buffer, 8);
        Assert::AreEqual(zero, buffer[0]);
        Assert::AreEqual(zero, buffer[1]);
        Assert::AreEqual(zero, buffer[2]);
        Assert::AreEqual(zero, buffer[3]);
        Assert::AreEqual(memory.at(30), buffer[4]);
        Assert::AreEqual(memory.at(31), buffer[5]);
        Assert::AreEqual(memory.at(32), buffer[6]);
        Assert::AreEqual(memory.at(33), buffer[7]);
    }

    TEST_METHOD(TestWriteMemoryByte)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        // attempt to write all 64 bytes
        for (ra::ByteAddress i = 0; i < memory.size(); ++i)
            emulator.WriteMemoryByte(i, gsl::narrow_cast<uint8_t>(i + 4));

        // only the 30 mapped bytes should be updated
        for (uint8_t i = 0; i < emulator.TotalMemorySize(); ++i)
            Assert::AreEqual(static_cast<uint8_t>(i + 4), memory.at(i));

        // the others should have their original values
        for (uint8_t i = gsl::narrow_cast<uint8_t>(emulator.TotalMemorySize()); i < memory.size(); ++i)
            Assert::AreEqual(i, memory.at(i));
    }

    TEST_METHOD(TestWriteMemoryByteEvents)
    {
        InitializeMemory();

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        EmulatorContextNotifyHarness notify;
        emulator.AddNotifyTarget(notify);

        // write to first block
        uint8_t nByte = 0xCE;
        emulator.WriteMemoryByte(6U, nByte);
        Assert::AreEqual(6U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);

        // write to second block
        nByte = 0xEE;
        emulator.WriteMemoryByte(26U, nByte);
        Assert::AreEqual(26U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);

        // write to invalid address
        emulator.WriteMemoryByte(36U, nByte + 3);
        Assert::AreEqual(26U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);

        // write partial value
        nByte = 0xC4;
        emulator.WriteMemory(6U, MemSize::Nibble_Lower, 0x04);
        Assert::AreEqual(6U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);
    }

    TEST_METHOD(TestWriteMemory)
    {
        memory.at(4) = 0xA8;
        memory.at(5) = 0x60;
        memory.at(6) = 0x37;
        memory.at(7) = 0x2E;
        memory.at(8) = 0x04;

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);

        emulator.WriteMemory(4U, MemSize::EightBit, 0xFC);
        Assert::AreEqual((uint8_t)0xFC, memory.at(4));
        Assert::AreEqual((uint8_t)0x60, memory.at(5));

        emulator.WriteMemory(4U, MemSize::EightBit, 0x12345678);
        Assert::AreEqual((uint8_t)0x78, memory.at(4));
        Assert::AreEqual((uint8_t)0x60, memory.at(5));

        emulator.WriteMemory(4U, MemSize::SixteenBit, 0xABCD);
        Assert::AreEqual((uint8_t)0xCD, memory.at(4));
        Assert::AreEqual((uint8_t)0xAB, memory.at(5));
        Assert::AreEqual((uint8_t)0x37, memory.at(6));

        emulator.WriteMemory(4U, MemSize::SixteenBit, 0x12345678);
        Assert::AreEqual((uint8_t)0x78, memory.at(4));
        Assert::AreEqual((uint8_t)0x56, memory.at(5));
        Assert::AreEqual((uint8_t)0x37, memory.at(6));

        emulator.WriteMemory(4U, MemSize::TwentyFourBit, 0x12345678);
        Assert::AreEqual((uint8_t)0x78, memory.at(4));
        Assert::AreEqual((uint8_t)0x56, memory.at(5));
        Assert::AreEqual((uint8_t)0x34, memory.at(6));
        Assert::AreEqual((uint8_t)0x2E, memory.at(7));

        emulator.WriteMemory(4U, MemSize::ThirtyTwoBit, 0x76543210);
        Assert::AreEqual((uint8_t)0x10, memory.at(4));
        Assert::AreEqual((uint8_t)0x32, memory.at(5));
        Assert::AreEqual((uint8_t)0x54, memory.at(6));
        Assert::AreEqual((uint8_t)0x76, memory.at(7));
        Assert::AreEqual((uint8_t)0x04, memory.at(8));

        emulator.WriteMemory(4U, MemSize::SixteenBitBigEndian, 0x12345678);
        Assert::AreEqual((uint8_t)0x56, memory.at(4));
        Assert::AreEqual((uint8_t)0x78, memory.at(5));
        Assert::AreEqual((uint8_t)0x54, memory.at(6));

        emulator.WriteMemory(4U, MemSize::TwentyFourBitBigEndian, 0x12345678);
        Assert::AreEqual((uint8_t)0x34, memory.at(4));
        Assert::AreEqual((uint8_t)0x56, memory.at(5));
        Assert::AreEqual((uint8_t)0x78, memory.at(6));
        Assert::AreEqual((uint8_t)0x76, memory.at(7));

        emulator.WriteMemory(4U, MemSize::ThirtyTwoBitBigEndian, 0x76543210);
        Assert::AreEqual((uint8_t)0x76, memory.at(4));
        Assert::AreEqual((uint8_t)0x54, memory.at(5));
        Assert::AreEqual((uint8_t)0x32, memory.at(6));
        Assert::AreEqual((uint8_t)0x10, memory.at(7));
        Assert::AreEqual((uint8_t)0x04, memory.at(8));

        memory.at(4) = 0xFF;
        emulator.WriteMemory(4U, MemSize::Bit_0, 0x00);
        Assert::AreEqual((uint8_t)0xFE, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_0, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Bit_1, 0x00);
        Assert::AreEqual((uint8_t)0xFD, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_1, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Bit_2, 0x00);
        Assert::AreEqual((uint8_t)0xFB, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_2, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Bit_3, 0x00);
        Assert::AreEqual((uint8_t)0xF7, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_3, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Bit_4, 0x00);
        Assert::AreEqual((uint8_t)0xEF, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_4, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Bit_5, 0x00);
        Assert::AreEqual((uint8_t)0xDF, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_5, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Bit_6, 0x00);
        Assert::AreEqual((uint8_t)0xBF, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_6, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Bit_7, 0x00);
        Assert::AreEqual((uint8_t)0x7F, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Bit_7, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Nibble_Lower, 0x00);
        Assert::AreEqual((uint8_t)0xF0, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Nibble_Lower, 0x0F);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::Nibble_Upper, 0x00);
        Assert::AreEqual((uint8_t)0x0F, memory.at(4));
        emulator.WriteMemory(4U, MemSize::Nibble_Upper, 0x0F);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, MemSize::BitCount, 0x00);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));
        emulator.WriteMemory(4U, MemSize::BitCount, 0x02);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));
    }

    TEST_METHOD(TestIsMemoryInsecureCached)
    {
        EmulatorContextHarness emulator;
        Assert::IsFalse(emulator.IsMemoryInsecure());

        emulator.mockDesktop.SetDebuggerPresent(true);
        Assert::IsFalse(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(5));
        Assert::IsFalse(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(6));
        Assert::IsTrue(emulator.IsMemoryInsecure());
    }

    TEST_METHOD(TestIsMemoryInsecureRememberedUntilReset)
    {
        EmulatorContextHarness emulator;
        emulator.mockDesktop.SetDebuggerPresent(true);
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.mockDesktop.SetDebuggerPresent(false);
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(100));
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.ResetMemoryModified();
        Assert::IsFalse(emulator.IsMemoryInsecure());
    }

    TEST_METHOD(TestIsMemoryInsecureRecheckedWhenReset)
    {
        EmulatorContextHarness emulator;
        emulator.mockDesktop.SetDebuggerPresent(true);
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(100));
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.ResetMemoryModified();
        Assert::IsTrue(emulator.IsMemoryInsecure());
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
            pBookmarks.AddBookmark(i, MemSize::EightBit);

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

    TEST_METHOD(TestFormatAddress)
    {
        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 0x80, nullptr, nullptr);

        Assert::AreEqual(std::string("0x0000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x00ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x0100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0xffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(1, 0x80, nullptr, nullptr); // total size = 0x100
        Assert::AreEqual(std::string("0x0000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x00ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x0100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0xffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(2, 0xFF00, nullptr, nullptr); // total size = 0x10000
        Assert::AreEqual(std::string("0x0000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x00ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x0100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0xffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(3, 1, nullptr, nullptr); // total size = 0x10001
        Assert::AreEqual(std::string("0x000000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x0000ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x000100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0x00ffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(4, 0xFEFFFF, nullptr, nullptr); // total size = 0x1000000
        Assert::AreEqual(std::string("0x000000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x0000ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x000100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0x00ffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(5, 1, nullptr, nullptr); // total size = 0x1000001
        Assert::AreEqual(std::string("0x00000000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x000000ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x00000100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0x0000ffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x00010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0x00ffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));
    }

    TEST_METHOD(TestCaptureMemory)
    {
        EmulatorContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::search::MemBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 0, 30, 0);
        Assert::AreEqual({ 2 }, vBlocks.size());

        Assert::AreEqual(20U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        for (size_t i = 0; i < 20; i++)
            Assert::AreEqual(memory.at(i), pBytes[i]);

        Assert::AreEqual(10U, vBlocks.at(1).GetBytesSize());
        pBytes = vBlocks.at(1).GetBytes();
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemoryOffset)
    {
        EmulatorContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::search::MemBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 8, 20, 0);
        Assert::AreEqual({ 2 }, vBlocks.size());

        Assert::AreEqual(12U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        for (size_t i = 0; i < 12; i++)
            Assert::AreEqual(memory.at(i + 8), pBytes[i]);

        Assert::AreEqual(8U, vBlocks.at(1).GetBytesSize());
        pBytes = vBlocks.at(1).GetBytes();
        for (size_t i = 0; i < 8; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemorySecondBlockOnly)
    {
        EmulatorContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::search::MemBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 20, 10, 0);
        Assert::AreEqual({ 1 }, vBlocks.size());

        Assert::AreEqual(10U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemoryGap)
    {
        EmulatorContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 10, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::search::MemBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 0, 30, 0);
        Assert::AreEqual({ 2 }, vBlocks.size());

        Assert::AreEqual(10U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i), pBytes[i]);

        Assert::AreEqual(10U, vBlocks.at(1).GetBytesSize());
        pBytes = vBlocks.at(1).GetBytes();
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemoryGapBoundary)
    {
        EmulatorContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 10, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::search::MemBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 20, 10, 0);
        Assert::AreEqual({ 1 }, vBlocks.size());

        Assert::AreEqual(10U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }
};

std::array<uint8_t, 64> EmulatorContext_Tests::memory;

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
