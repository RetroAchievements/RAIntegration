#include "CppUnitTest.h"

#include "data\EmulatorContext.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(EmulatorContext_Tests)
{
private:
    class EmulatorContextHarness : public EmulatorContext
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::data::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;

        void MockVersions(const std::string& sClientVersion, const std::string& sServerVersion)
        {
            SetClientVersion(sClientVersion);
            mockServer.HandleRequest<ra::api::LatestClient>([sServerVersion](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
            {
                response.LatestVersion = sServerVersion;
                response.Result = ra::api::ApiResult::Success;
                return true;
            });
        }
    };

public:
    TEST_METHOD(TestClientName)
    {
        EmulatorContextHarness emulator;

        emulator.Initialize(EmulatorID::RA_FCEUX);
        Assert::AreEqual(std::string("RANes"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Gens);
        Assert::AreEqual(std::string("RAGens_REWiND"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Libretro);
        Assert::AreEqual(std::string("RALibRetro"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Meka);
        Assert::AreEqual(std::string("RAMeka"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Nester);
        Assert::AreEqual(std::string("RANes"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_PCE);
        Assert::AreEqual(std::string("RAPCE"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Project64);
        Assert::AreEqual(std::string("RAP64"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_QUASI88);
        Assert::AreEqual(std::string("RAQUASI88"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Snes9x);
        Assert::AreEqual(std::string("RASnes9X"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_VisualboyAdvance);
        Assert::AreEqual(std::string("RAVisualBoyAdvance"), emulator.GetClientName());
    }

    TEST_METHOD(TestValidateClientVersionCurrent)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.MockVersions("0.57", "0.56");

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionApiErrorNonHardcore)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"The latest client is required for hardcore mode."), vmMessageBox.GetHeader());
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"The latest client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionUnknownClient)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.SetClientVersion("1.0");
        emulator.mockServer.HandleRequest<ra::api::LatestClient>([](const ra::api::LatestClient::Request&, ra::api::LatestClient::Response& response)
        {
            response.ErrorMessage = "Unknown client! (EmulatorID: 999)";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not retrieve latest client version."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Unknown client! (EmulatorID: 999)"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestDisableHardcoreMode)
    {
        EmulatorContextHarness emulator;
        emulator.mockGameContext.SetGameId(1234U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        bool bUnlocksRequested = false;
        emulator.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([&bUnlocksRequested](const ra::api::FetchUserUnlocks::Request& request, ra::api::FetchUserUnlocks::Response& response)
        {
            bUnlocksRequested = true;
            Assert::AreEqual(1234U, request.GameId);
            Assert::IsFalse(request.Hardcore);
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        emulator.DisableHardcoreMode();

        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        emulator.mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(bUnlocksRequested);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentNoGame)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestEnableHardcoreModeAlreadyEnabled)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U); // would normally show a dialog
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Enable hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Enabling hardcore mode will reset the emulator. You will lose any progress that has not been saved through the game."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Yes;
        });

        bool bUnlocksRequested = false;
        emulator.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([&bUnlocksRequested](const ra::api::FetchUserUnlocks::Request& request, ra::api::FetchUserUnlocks::Response& response)
        {
            bUnlocksRequested = true;
            Assert::AreEqual(1U, request.GameId);
            Assert::IsTrue(request.Hardcore);
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());

        emulator.mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(bUnlocksRequested);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentGameLoadedCancel)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Enable hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Enabling hardcore mode will reset the emulator. You will lose any progress that has not been saved through the game."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        Assert::IsFalse(emulator.EnableHardcoreMode());

        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestEnableHardcoreModeVersionNewerGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.58", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Enable hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Enabling hardcore mode will reset the emulator. You will lose any progress that has not been saved through the game."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Yes;
        });

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"The latest client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());

        Assert::IsTrue(emulator.EnableHardcoreMode());

        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());

        // user should be logged out if hardcore was enabled on an older version
        Assert::IsFalse(emulator.mockUserContext.IsLoggedIn());
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedCancel)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"The latest client is required for hardcore mode."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"A new version of RASnes9X is available to download at host.\n\n- Current version: 0.57\n- New version: 0.58\n\nPress OK to logout and download the new version, or Cancel to disable hardcore mode and proceed."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Cancel;
        });
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());

        Assert::IsFalse(emulator.EnableHardcoreMode());

        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string(""), emulator.mockDesktop.LastOpenedUrl());
        Assert::IsTrue(emulator.mockUserContext.IsLoggedIn());
    }

    TEST_METHOD(TestGetAppTitleDefault)
    {
        EmulatorContextHarness emulator;
        Assert::AreEqual(std::string(" -  []"), emulator.GetAppTitle(""));

        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.SetClientVersion("0.57.0.0");
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual(std::string("RASnes9X - 0.57"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleVersion)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("retroachievements.org");

        emulator.SetClientVersion("0.57.1.1");
        Assert::AreEqual(std::string("RASnes9X - 0.57"), emulator.GetAppTitle(""));

        emulator.SetClientVersion("0.99");
        Assert::AreEqual(std::string("RASnes9X - 0.99"), emulator.GetAppTitle(""));

        emulator.SetClientVersion("1.0.12.0");
        Assert::AreEqual(std::string("RASnes9X - 1.0"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleUserName)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        emulator.SetClientVersion("0.57");

        Assert::AreEqual(std::string("RASnes9X - 0.57"), emulator.GetAppTitle(""));

        emulator.mockUserContext.Initialize("User", "Token");
        Assert::AreEqual(std::string("RASnes9X - 0.57 - User"), emulator.GetAppTitle(""));

        emulator.mockUserContext.Logout();
        Assert::AreEqual(std::string("RASnes9X - 0.57"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleCustomMessage)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::string("RASnes9X - 0.57 - Test"), emulator.GetAppTitle("Test"));
    }

    TEST_METHOD(TestGetAppTitleCustomHost)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("localhost");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::string("RASnes9X - 0.57 [localhost]"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleEverything)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("localhost");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::string("RASnes9X - 0.57 - Test - User [localhost]"), emulator.GetAppTitle("Test"));
    }
};

} // namespace tests
} // namespace data
} // namespace ra
