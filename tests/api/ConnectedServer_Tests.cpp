#include "CppUnitTest.h"

#include "api\impl\ConnectedServer.hh"

#include "api\impl\DisconnectedServer.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\api\ApiAsserts.hh"
#include "tests\devkit\context\mocks\MockUserContext.hh"
#include "tests\devkit\services\mocks\MockHttpRequester.hh"
#include "tests\devkit\services\mocks\MockLocalStorage.hh"
#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\mocks\MockServer.hh"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::impl::ConnectedServer;
using ra::api::mocks::MockServer;
using ra::context::mocks::MockUserContext;
using ra::services::mocks::MockHttpRequester;
using ra::services::mocks::MockLocalStorage;
using ra::services::mocks::MockThreadPool;
using ra::services::Http;

namespace ra {
namespace api {
namespace tests {

TEST_CLASS(ConnectedServer_Tests)
{
public:
    // ConnectedServer manages converting the Http error codes to Incomplete responses, this just tests the handling
    // of Incomplete responses by the ApiRequestBase.

    // ====================================================
    // These tests validate the generic handling of errors - they just happen to be written using the LatestClient API

    TEST_METHOD(TestLatestClientInvalid)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::OK, "{\"Success\":false,\"Error\":\"Unknown client\"}");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("Unknown client"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientFailed)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::OK, "{\"Success\":false}");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        // "Success:false" without error message results in Failed call, not Error call
        Assert::AreEqual(ApiResult::Failed, response.Result);
        Assert::AreEqual(std::string(""), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientFailed401)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::Unauthorized, "");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("HTTP error code: 401 (err401)"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientFailed401WithMessage)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::Unauthorized, "{\"Success\":false,\"Error\":\"Unknown client\"}");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("Unknown client"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientUnknownServer)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::NotFound, "");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("HTTP error code: 404 (err404)"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientEmptyResponse)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::OK, "");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        Assert::AreEqual(ApiResult::Failed, response.Result);
        Assert::AreEqual(std::string("Empty JSON response"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientInvalidJson)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::OK, "You do not have access to that resource");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("You do not have access to that resource"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientHtmlResponse)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::OK, "<b>You do not have access to that resource</b>");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("<b>You do not have access to that resource</b>"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    TEST_METHOD(TestLatestClientNoRequiredFields)
    {
        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::OK, "{\"Success\":true}");
        });

        ConnectedServer server("host.com");

        LatestClient::Request request;
        request.EmulatorId = 1;

        auto response = server.LatestClient(request);

        // only the first missing required field is reported
        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("LatestVersion not found in response"), response.ErrorMessage);
        Assert::AreEqual(std::string(""), response.LatestVersion);
        Assert::AreEqual(std::string(""), response.MinimumVersion);
    }

    // End of generic validation tests
    // ====================================================
};

} // namespace tests
} // namespace api
} // namespace ra
