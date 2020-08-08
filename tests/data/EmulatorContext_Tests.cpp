#include "CppUnitTest.h"

#include "data\EmulatorContext.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockHttpRequester.hh"
#include "tests\mocks\MockOverlayManager.hh"
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
        ra::services::mocks::MockFileSystem mockFileSystem;
        ra::services::mocks::MockHttpRequester mockHttpRequester;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        GSL_SUPPRESS_F6 EmulatorContextHarness() noexcept
            : mockHttpRequester([](const ra::services::Http::Request&) { return ra::services::Http::Response(ra::services::Http::StatusCode::OK, ""); })
        {
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

        emulator.Initialize(EmulatorID::RA_AppleWin);
        Assert::AreEqual(std::string("RAppleWin"), emulator.GetClientName());

        emulator.Initialize(EmulatorID::RA_Oricutron);
        Assert::AreEqual(std::string("RAOricutron"), emulator.GetClientName());
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestValidateClientVersionCurrentAboveMinimum)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.MockVersions("0.56", "0.56", "0.55");

        Assert::IsTrue(emulator.ValidateClientVersion());
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestValidateClientVersionTo1_0)
    {
        // ensures "1.0" doesn't get trimmed to "1"
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
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
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.SetClientVersion("1.0");

        std::string sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("RASnes9X/1.0 (UnitTests) Integration/VERSION"), sVersion);

        emulator.Initialize(EmulatorID::RA_AppleWin);
        emulator.SetClientVersion("1.1.1");

        sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("RAppleWin/1.1.1 (UnitTests) Integration/VERSION"), sVersion);
    }

    TEST_METHOD(TestUserAgentClientDetail)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Libretro);
        emulator.SetClientVersion("1.0.15");
        emulator.SetClientUserAgentDetail("fceumm_libretro/(SVN)_0a0fdb8");

        std::string sVersion = emulator.mockHttpRequester.GetUserAgent();
        ReplaceIntegrationVersion(sVersion);
        Assert::AreEqual(std::string("RALibRetro/1.0.15 (UnitTests) Integration/VERSION fceumm_libretro/(SVN)_0a0fdb8"), sVersion);
    }

    TEST_METHOD(TestDisableHardcoreMode)
    {
        EmulatorContextHarness emulator;
        emulator.mockGameContext.SetGameId(1234U);
        emulator.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        auto& pLeaderboard = emulator.mockGameContext.NewLeaderboard(32U);
        pLeaderboard.SetActive(true);
        Assert::IsTrue(pLeaderboard.IsActive());

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

        // ensure mode was deactivated
        Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        // deactivating hardcore mode should not disable leaderboards
        Assert::IsTrue(pLeaderboard.IsActive());

        // deactivating hardcore mode should request non-hardcore unlocks
        emulator.mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(bUnlocksRequested);

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
        Assert::IsTrue(bWasReset);
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
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::IsFalse(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
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

        const auto& pLeaderboard = emulator.mockGameContext.NewLeaderboard(32U);
        Assert::IsFalse(pLeaderboard.IsActive());

        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsTrue(emulator.EnableHardcoreMode());

        // ensure hardcore mode was enabled
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        // ensure prompt was shown
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());

        // enabling hardcore mode should enable leaderboards
        Assert::IsTrue(pLeaderboard.IsActive());

        // enabling hardcore mode should request hardcore unlocks
        emulator.mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(bUnlocksRequested);

        // enabling hardcore mode should reset the emulator
        Assert::IsTrue(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionCurrentGameLoadedCancel)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.57", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
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
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionNewerGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.MockVersions("0.58", "0.57");
        emulator.mockGameContext.SetGameId(1U);
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
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
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::IsTrue(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedAccept)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
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
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());

        // user should be logged out if hardcore was enabled on an older version
        Assert::IsFalse(emulator.mockUserContext.IsLoggedIn());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedCancel)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
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

        bool bUnlocksRequested = false;
        emulator.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([&bUnlocksRequested](const ra::api::FetchUserUnlocks::Request& request, ra::api::FetchUserUnlocks::Response& response)
        {
            bUnlocksRequested = true;
            Assert::AreEqual(1U, request.GameId);
            Assert::IsTrue(request.Hardcore);
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        const auto& pLeaderboard = emulator.mockGameContext.NewLeaderboard(32U);
        Assert::IsFalse(pLeaderboard.IsActive());

        bool bWasReset = false;
        emulator.SetResetFunction([&bWasReset]() { bWasReset = true; });

        Assert::IsTrue(emulator.EnableHardcoreMode(false));

        // ensure hardcore mode was enabled
        Assert::IsTrue(emulator.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));

        // ensure prompt was not shown
        Assert::IsFalse(emulator.mockDesktop.WasDialogShown());

        // enabling hardcore mode should enable leaderboards
        Assert::IsTrue(pLeaderboard.IsActive());

        // enabling hardcore mode should request hardcore unlocks
        emulator.mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(bUnlocksRequested);

        // enabling hardcore mode should reset the emulator
        Assert::IsTrue(bWasReset);
    }

    TEST_METHOD(TestEnableHardcoreModeVersionOlderGameLoadedNoPrompt)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("host");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.mockGameContext.SetGameId(1U);
        emulator.MockVersions("0.57.0.0", "0.58.0.0");
        emulator.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&emulator](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
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
        Assert::IsTrue(emulator.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::string("http://host/download.php"), emulator.mockDesktop.LastOpenedUrl());

        // user should be logged out if hardcore was enabled on an older version
        Assert::IsFalse(emulator.mockUserContext.IsLoggedIn());
        Assert::IsFalse(bWasReset);
    }

    TEST_METHOD(TestGetAppTitleDefault)
    {
        EmulatorContextHarness emulator;
        Assert::AreEqual(std::wstring(L" -  []"), emulator.GetAppTitle(""));

        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.SetClientVersion("0.57.0.0");
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleVersion)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("retroachievements.org");

        emulator.SetClientVersion("0.57.1.1");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));

        emulator.SetClientVersion("0.99");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.99"), emulator.GetAppTitle(""));

        emulator.SetClientVersion("1.0.12.0");
        Assert::AreEqual(std::wstring(L"RASnes9X - 1.0"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleUserName)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        emulator.SetClientVersion("0.57");

        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));

        emulator.mockUserContext.Initialize("User", "Token");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 - User"), emulator.GetAppTitle(""));

        emulator.mockUserContext.Logout();
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleCustomMessage)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("retroachievements.org");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 - Test"), emulator.GetAppTitle("Test"));
    }

    TEST_METHOD(TestGetAppTitleCustomHost)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("localhost");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 [localhost]"), emulator.GetAppTitle(""));
    }

    TEST_METHOD(TestGetAppTitleEverything)
    {
        EmulatorContextHarness emulator;
        emulator.Initialize(EmulatorID::RA_Snes9x);
        emulator.mockConfiguration.SetHostName("localhost");
        emulator.mockUserContext.Initialize("User", "Token");
        emulator.SetClientVersion("0.57");
        Assert::AreEqual(std::wstring(L"RASnes9X - 0.57 - Test - User [localhost]"), emulator.GetAppTitle("Test"));
    }

    TEST_METHOD(TestInitializeUnknownEmulator)
    {
        EmulatorContextHarness emulator;
        emulator.mockDesktop.SetRunningExecutable("C:\\Games\\Emulator\\RATest.exe");
        emulator.Initialize(ra::itoe<EmulatorID>(9999));
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

    static void WriteMemory0(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(nAddress) = nValue; }
    static void WriteMemory1(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 10) = nValue; }
    static void WriteMemory2(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 20) = nValue; }
    static void WriteMemory3(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 30) = nValue; }

    TEST_METHOD(TestReadMemoryByte)
    {
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

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
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

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
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

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
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

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

    TEST_METHOD(TestReadMemory)
    {
        memory.at(4) = 0xA8;
        memory.at(5) = 0x00;
        memory.at(6) = 0x37;
        memory.at(7) = 0x2E;

        EmulatorContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);

        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_0)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_1)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_2)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_3)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_4)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_5)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_6)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_7)));
        Assert::AreEqual(8, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Lower)));
        Assert::AreEqual(10, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Upper)));
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, MemSize::EightBit)));
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, MemSize::SixteenBit)));
        Assert::AreEqual(0x2E37, static_cast<int>(emulator.ReadMemory(6U, MemSize::SixteenBit)));
        Assert::AreEqual(0x2E3700, static_cast<int>(emulator.ReadMemory(5U, MemSize::TwentyFourBit)));
        Assert::AreEqual(0x2E3700A8, static_cast<int>(emulator.ReadMemory(4U, MemSize::ThirtyTwoBit)));

        memory.at(4) ^= 0xFF; // toggle all bits and verify again
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_0)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_1)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_2)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_3)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_4)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_5)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_6)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, MemSize::Bit_7)));
        Assert::AreEqual(7, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Lower)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, MemSize::Nibble_Upper)));
        Assert::AreEqual(0x57, static_cast<int>(emulator.ReadMemory(4U, MemSize::EightBit)));
    }

    TEST_METHOD(TestReadMemoryBuffer)
    {
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

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

    TEST_METHOD(TestWriteMemoryByte)
    {
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

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
    }
};

std::array<uint8_t, 64> EmulatorContext_Tests::memory;

} // namespace tests
} // namespace data
} // namespace ra
