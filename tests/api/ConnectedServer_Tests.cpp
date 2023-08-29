#include "CppUnitTest.h"

#include "api\impl\ConnectedServer.hh"

#include "api\impl\DisconnectedServer.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\api\ApiAsserts.hh"
#include "tests\mocks\MockHttpRequester.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::impl::ConnectedServer;
using ra::api::mocks::MockServer;
using ra::data::context::mocks::MockUserContext;
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
    TEST_METHOD(TestCallAsyncWithRetry)
    {
        MockThreadPool mockThreadPool;
        MockServer mockServer;
        bool bHandlerCalled = false;
        mockServer.HandleRequest<ra::api::AwardAchievement>([&bHandlerCalled](const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
        {
            bHandlerCalled = true;
            Assert::AreEqual(1234U, request.AchievementId);
            Assert::AreEqual(std::string("HASH"), request.GameHash);
            Assert::IsTrue(request.Hardcore);

            response.Result = ra::api::ApiResult::Incomplete;
            return true;
        });

        AwardAchievement::Request request;
        request.GameHash = "HASH";
        request.AchievementId = 1234U;
        request.Hardcore = true;

        bool bCallbackCalled = false;
        request.CallAsyncWithRetry([&bCallbackCalled](const ra::api::AwardAchievement::Response& response)
        {
            Assert::AreEqual(ra::api::ApiResult::Success, response.Result);
            bCallbackCalled = true;
        });

        // call should be async
        Assert::IsFalse(bCallbackCalled);
        Assert::IsFalse(bHandlerCalled);
        Assert::AreEqual({ 1U }, mockThreadPool.PendingTasks());

        // async call should be incomplete and requeued
        mockThreadPool.ExecuteNextTask();
        Assert::IsFalse(bCallbackCalled);
        Assert::IsTrue(bHandlerCalled);
        Assert::AreEqual({ 1U }, mockThreadPool.PendingTasks());
        Assert::AreEqual(std::chrono::milliseconds(500), mockThreadPool.NextTaskDelay());

        // callback called again, still incomplete, requeue with longer delay
        bHandlerCalled = false;
        mockThreadPool.AdvanceTime(std::chrono::milliseconds(500));
        Assert::IsFalse(bCallbackCalled);
        Assert::IsTrue(bHandlerCalled);
        Assert::AreEqual({ 1U }, mockThreadPool.PendingTasks());
        Assert::AreEqual(std::chrono::milliseconds(1000), mockThreadPool.NextTaskDelay());

        // response succeeded, should complete transaction
        mockServer.HandleRequest<ra::api::AwardAchievement>([&bHandlerCalled](const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
        {
            bHandlerCalled = true;
            Assert::AreEqual(1234U, request.AchievementId);
            Assert::AreEqual(std::string("HASH"), request.GameHash);
            Assert::IsTrue(request.Hardcore);

            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        bHandlerCalled = false;
        mockThreadPool.AdvanceTime(std::chrono::milliseconds(1000));
        Assert::IsTrue(bCallbackCalled);
        Assert::IsTrue(bHandlerCalled);
        Assert::AreEqual({ 0U }, mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestCallAsyncWithRetryMaxDelay)
    {
        MockThreadPool mockThreadPool;
        MockServer mockServer;
        mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.Result = ra::api::ApiResult::Incomplete;
            return true;
        });

        AwardAchievement::Request request;
        request.GameHash = "HASH";
        request.AchievementId = 1234U;
        request.Hardcore = true;

        request.CallAsyncWithRetry([](const ra::api::AwardAchievement::Response&) {});

        // call should be async
        mockThreadPool.ExecuteNextTask();
        for (int i = 0; i < 100; i++)
            mockThreadPool.AdvanceTime(mockThreadPool.NextTaskDelay());

        Assert::AreEqual(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(2)), mockThreadPool.NextTaskDelay());
    }

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

    TEST_METHOD(TestAwardAchievementFailed403)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Username", "ApiToken");

        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::Forbidden, "");
        });

        ConnectedServer server("host.com");

        AwardAchievement::Request request;
        request.GameHash = "HASH";
        request.AchievementId = 1234U;
        request.Hardcore = true;

        auto response = server.AwardAchievement(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("HTTP error code: 403 (err403)"), response.ErrorMessage);
    }

    TEST_METHOD(TestAwardAchievementFailed403WithMessage)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Username", "ApiToken");

        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::Forbidden, "{\"Success\":false, \"Error\":\"Invalid API token\"}");
        });

        ConnectedServer server("host.com");

        AwardAchievement::Request request;
        request.GameHash = "HASH";
        request.AchievementId = 1234U;
        request.Hardcore = true;

        auto response = server.AwardAchievement(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("Invalid API token"), response.ErrorMessage);
    }

    TEST_METHOD(TestAwardAchievementFailed429)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Username", "ApiToken");

        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::TooManyRequests,
                "<html>\n"
                "<head><title>429 Too Many Requests</title></head>\n"
                "<body>\n"
                "<center><h1>429 Too Many Requests</h1></center>\n"
                "<hr><center>nginx</center>\n"
                "</body>\n"
                "</html>");
        });

        ConnectedServer server("host.com");

        AwardAchievement::Request request;
        request.GameHash = "HASH";
        request.AchievementId = 1234U;
        request.Hardcore = true;

        auto response = server.AwardAchievement(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("HTTP error code: 429 (err429)"), response.ErrorMessage);
    }

    TEST_METHOD(TestAwardAchievementFailed429Json)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Username", "ApiToken");

        MockHttpRequester mockHttp([]([[maybe_unused]] const Http::Request& /*request*/)
        {
            return Http::Response(Http::StatusCode::TooManyRequests,
                "{\"Success\": false,\"Error\": \"Too Many Attempts\"}");
        });

        ConnectedServer server("host.com");

        AwardAchievement::Request request;
        request.GameHash = "HASH";
        request.AchievementId = 1234U;
        request.Hardcore = true;

        auto response = server.AwardAchievement(request);

        Assert::AreEqual(ApiResult::Error, response.Result);
        Assert::AreEqual(std::string("Too Many Attempts"), response.ErrorMessage);
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

    // ====================================================
    // SubmitLeaderboardEntry

    TEST_METHOD(TestSubmitLeaderboardEntrySigned)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Username", "ApiToken");

        MockHttpRequester mockHttp([](const Http::Request&)
        {
            return Http::Response(Http::StatusCode::OK,
                "{\"Success\":true,\"Response\":{\"Score\":1234,\"BestScore\":2345,"
                "\"TopEntries\":[{\"User\":\"Player1\",\"Score\":8765,\"Rank\":1},{\"User\":\"Player2\",\"Score\":7654,\"Rank\":2}],"
                "\"RankInfo\":{\"Rank\":5,\"NumEntries\":\"17\"}}}");
        });

        ra::services::ServiceLocator::ServiceOverride<ra::api::IServer> serviceOverride(new ConnectedServer("host.com"), true);
        auto& server = ra::services::ServiceLocator::GetMutable<ra::api::IServer>();

        SubmitLeaderboardEntry::Request request;
        request.LeaderboardId = 234;
        request.Score = -55667788;
        auto response = server.SubmitLeaderboardEntry(request);

        Assert::AreEqual(ApiResult::Success, response.Result);
        Assert::AreEqual(std::string(), response.ErrorMessage);
    }

    // ====================================================
    // FetchCodeNotes

    TEST_METHOD(TestFetchCodeNotesNormalizesLineEndings)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Username", "ApiToken");

        MockHttpRequester mockHttp([](const Http::Request&)
        {
            return Http::Response(Http::StatusCode::OK,
                "{\"Success\":true,\"CodeNotes\":["
                    "{\"User\":\"Username\",\"Address\":\"0x00f304\",\"Note\":\"Line1\\nLine2\\r\\nLine3\\n\"}"
                "]}");
        });

        MockLocalStorage mockLocalStorage;

        ra::services::ServiceLocator::ServiceOverride<ra::api::IServer> serviceOverride(new ConnectedServer("host.com"), true);
        auto& server = ra::services::ServiceLocator::GetMutable<ra::api::IServer>();

        FetchCodeNotes::Request request;
        request.GameId = 99;
        auto response = server.FetchCodeNotes(request);

        Assert::AreEqual(ApiResult::Success, response.Result);
        Assert::AreEqual(std::string(), response.ErrorMessage);

        // note text should be normalized to windows line endings
        Assert::AreEqual({1U}, response.Notes.size());
        Assert::AreEqual(std::string("Username"), response.Notes.at(0).Author);
        Assert::AreEqual({0x00f304}, response.Notes.at(0).Address);
        Assert::AreEqual(std::wstring(L"Line1\r\nLine2\r\nLine3\r\n"), response.Notes.at(0).Note);

        // cached file is not normalized (or unescaped)
        std::string sPatchData = "[{\"User\":\"Username\",\"Address\":\"0x00f304\",\"Note\":\"Line1\\nLine2\\r\\nLine3\\n\"}]";
        Assert::AreEqual(sPatchData, mockLocalStorage.GetStoredData(ra::services::StorageItemType::CodeNotes, L"99"));
    }
};

} // namespace tests
} // namespace api
} // namespace ra
