#include "CppUnitTest.h"

#include "api\impl\DisconnectedServer.hh"

#include "api\impl\ConnectedServer.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockHttpRequester.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::impl::DisconnectedServer;
using ra::services::mocks::MockHttpRequester;
using ra::services::Http;

namespace ra {
namespace api {
namespace tests {

TEST_CLASS(DisconnectedServer_Tests)
{
public:
    TEST_METHOD(TestLoginSuccess)
    {
        // Login method calls ConnectedServer::Login. If successful, replaces regsitered instance
        // with the ConnectedServer
        ra::services::ServiceLocator::ServiceOverride<ra::api::IServer> serviceOverride(new DisconnectedServer("host.com"), true);
        auto& server = ra::services::ServiceLocator::GetMutable<ra::api::IServer>();

        MockHttpRequester mockHttp([](const Http::Request& request)
        {
            Assert::AreEqual(std::string("host.com/login_app.php"), request.GetUrl());
            Assert::AreEqual(std::string("u=User&p=pa%24%24w0rd"), request.GetPostData());
            return Http::Response(Http::StatusCode::OK, "{\"Success\":true,\"User\":\"User\",\"Token\":\"ApiTOKEN\",\"Score\":1234,\"Messages\":2}");
        });

        Login::Request request;
        request.Username = "User";
        request.Password = "pa$$w0rd";
        auto response = server.Login(request);

        // connected server response should have been forwarded out
        Assert::AreEqual(ApiResult::Success, response.Result);
        Assert::AreEqual(std::string(), response.ErrorMessage);
        Assert::AreEqual(std::string("User"), response.Username);
        Assert::AreEqual(std::string("ApiTOKEN"), response.ApiToken);
        Assert::AreEqual(1234U, response.Score);
        Assert::AreEqual(2U, response.NumUnreadMessages);

        // ensure registered instance is now the ConnectedServer
        const auto* pServer = &ra::services::ServiceLocator::Get<ra::api::IServer>();
        Assert::IsNotNull(pServer, L"IServer not found");
        const auto* pConnectedServer = dynamic_cast<const ra::api::impl::ConnectedServer*>(pServer);
        Assert::IsNotNull(pServer, L"Server not a ConnectedServer");
        Assert::AreEqual("host.com", pConnectedServer->Name());
    }

    TEST_METHOD(TestLoginInvalid)
    {
        // Login method calls ConnectedServer::Login. If successful, replaces regsitered instance
        // with the ConnectedServer
        ra::services::ServiceLocator::ServiceOverride<ra::api::IServer> serviceOverride(new DisconnectedServer("host.com"), true);
        auto& server = ra::services::ServiceLocator::GetMutable<ra::api::IServer>();

        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::OK, "{\"Success\":false,\"Error\":\"Invalid User/Password combination. Please try again\"}");
        });

        Login::Request request;
        request.Username = "User";
        request.Password = "pa$$w0rd";
        auto response = server.Login(request);

        // connected server response should have been forwarded out
        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("Invalid User/Password combination. Please try again"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.Username);
        Assert::AreEqual(std::string(""), response.ApiToken);
        Assert::AreEqual(0U, response.Score);
        Assert::AreEqual(0U, response.NumUnreadMessages);

        // ensure registered instance is still the DisconnectedServer
        const auto* pServer = &ra::services::ServiceLocator::Get<ra::api::IServer>();
        Assert::IsNotNull(pServer, L"IServer not found");
        Assert::IsTrue(&server == pServer);
    }

    TEST_METHOD(TestLogout)
    {
        // Logout method doesn't actually do anything
        ra::services::ServiceLocator::ServiceOverride<ra::api::IServer> serviceOverride(new DisconnectedServer("host.com"), true);
        auto& server = ra::services::ServiceLocator::GetMutable<ra::api::IServer>();

        Logout::Request request;
        auto response = server.Logout(request);

        Assert::AreEqual(ApiResult::Success, response.Result);
        Assert::AreEqual(std::string(), response.ErrorMessage);

        // ensure registered instance is still the DisconnectedServer
        const auto* pServer = &ra::services::ServiceLocator::Get<ra::api::IServer>();
        Assert::IsNotNull(pServer, L"IServer not found");
        Assert::IsTrue(&server == pServer);
    }
};

} // namespace tests
} // namespace api
} // namespace ra
