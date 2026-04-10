#include "context\impl\RcClient.hh"

#include "services\mocks\MockHttpRequester.hh"
#include "services\mocks\MockLogger.hh"
#include "services\mocks\MockLocalStorage.hh"
#include "services\mocks\MockThreadPool.hh"

#include "testutil\CppUnitTest.hh"

#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace context {
namespace impl {
namespace tests {

TEST_CLASS(RcClient_Tests)
{
public:
    class RcClientHarness : public RcClient
    {
    public:
        RcClientHarness() noexcept {}

        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::mocks::MockLogger mockLogger;
        ra::services::mocks::MockHttpRequester mockRequester;
        ra::services::mocks::MockThreadPool mockThreadPool;
    };

    TEST_METHOD(TestDispatchRequest)
    {
        RcClientHarness client;

        rc_api_request_t pRequest;
        memset(&pRequest, 0, sizeof(pRequest));
        pRequest.url = "https://retroachievements.org/dorequest.php";
        pRequest.post_data = "r=patch&u=User&t=APITOKEN&g=1234";
        pRequest.content_type = "application/x-www-form-urlencoded";

        client.mockRequester.SetHandler([&pRequest](const ra::services::Http::Request& pHttpRequest)
            {
                Assert::AreEqual(std::string(pRequest.url), pHttpRequest.GetUrl());
                Assert::AreEqual(std::string(pRequest.post_data), pHttpRequest.GetPostData());
                Assert::AreEqual(std::string(pRequest.content_type), pHttpRequest.GetContentType());

                return ra::services::Http::Response(ra::services::Http::StatusCode::OK, "{\"Success\":true}");
            });

        bool bCallbackCalled = false;
        auto fCallback = [](const rc_api_server_response_t& server_response, void* callback_data)
            {
                Assert::AreEqual("{\"Success\":true}", server_response.body);
                Assert::AreEqual({ 16 }, server_response.body_length);
                Assert::AreEqual(200, server_response.http_status_code);

                if (callback_data != nullptr)
                    *(static_cast<bool*>(callback_data)) = true;
            };

        client.DispatchRequest(pRequest, fCallback, &bCallbackCalled);

        Assert::IsFalse(bCallbackCalled);

        client.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(bCallbackCalled);

        client.mockLogger.AssertContains(">> patch request: r=patch&u=User&t=[redacted]&g=1234");
        client.mockLogger.AssertContains("<< patch response (200): {\"Success\":true}");
    }

    TEST_METHOD(TestDispatchRequestCodeNotesWritesToCache)
    {
        RcClientHarness client;

        rc_api_request_t pRequest;
        memset(&pRequest, 0, sizeof(pRequest));
        pRequest.url = "https://retroachievements.org/dorequest.php";
        pRequest.post_data = "r=codenotes2&g=1234";
        pRequest.content_type = "application/x-www-form-urlencoded";

        const auto* pResponse =
            "{\"Success\":true,\"CodeNotes\":["
                "{\"User\":\"Username\",\"Address\":\"0x00f304\",\"Note\":\"Line1\\nLine2\\r\\nLine3\\n\"}"
            "]}";

        client.mockRequester.SetHandler([&pRequest, pResponse](const ra::services::Http::Request& pHttpRequest)
            {
                Assert::AreEqual(std::string(pRequest.url), pHttpRequest.GetUrl());
                Assert::AreEqual(std::string(pRequest.post_data), pHttpRequest.GetPostData());
                Assert::AreEqual(std::string(pRequest.content_type), pHttpRequest.GetContentType());

                return ra::services::Http::Response(ra::services::Http::StatusCode::OK, pResponse);
            });

        bool bCallbackCalled = false;
        auto fCallback = [pResponse](const rc_api_server_response_t& server_response, void* callback_data)
            {
                Assert::AreEqual(pResponse, server_response.body);
                Assert::AreEqual(strlen(pResponse), server_response.body_length);
                Assert::AreEqual(200, server_response.http_status_code);

                if (callback_data != nullptr)
                    *(static_cast<bool*>(callback_data)) = true;
            };

        client.DispatchRequest(pRequest, fCallback, &bCallbackCalled);

        Assert::IsFalse(bCallbackCalled);

        client.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(bCallbackCalled);

        // cached file contains only the notes array, but is not normalized (or unescaped)
        std::string sPatchData = "[{\"User\":\"Username\",\"Address\":\"0x00f304\",\"Note\":\"Line1\\nLine2\\r\\nLine3\\n\"}]";
        Assert::AreEqual(sPatchData, client.mockLocalStorage.GetStoredData(ra::services::StorageItemType::CodeNotes, L"1234"));
    }

    TEST_METHOD(TestLoggingRedactToken)
    {
        RcClientHarness client;

        rc_api_request_t pRequest;
        memset(&pRequest, 0, sizeof(pRequest));
        pRequest.url = "https://retroachievements.org/dorequest.php";
        pRequest.post_data = "r=login2&u=User&p=PASSWORD";
        pRequest.content_type = "application/x-www-form-urlencoded";

        client.mockRequester.SetHandler([&pRequest](const ra::services::Http::Request& pHttpRequest)
            {
                Assert::AreEqual(std::string(pRequest.url), pHttpRequest.GetUrl());
                Assert::AreEqual(std::string(pRequest.post_data), pHttpRequest.GetPostData());
                Assert::AreEqual(std::string(pRequest.content_type), pHttpRequest.GetContentType());

                return ra::services::Http::Response(ra::services::Http::StatusCode::OK, "{\"Success\":true,\"User\":\"User\",\"Token\":\"APITOKEN\",\"Score\":1234}");
            });

        bool bCallbackCalled = false;
        auto fCallback = [](const rc_api_server_response_t& server_response, void* callback_data)
            {
                Assert::AreEqual("{\"Success\":true,\"User\":\"User\",\"Token\":\"APITOKEN\",\"Score\":1234}", server_response.body);
                Assert::AreEqual({ 62 }, server_response.body_length);
                Assert::AreEqual(200, server_response.http_status_code);

                if (callback_data != nullptr)
                    *(static_cast<bool*>(callback_data)) = true;
            };

        client.DispatchRequest(pRequest, fCallback, &bCallbackCalled);

        Assert::IsFalse(bCallbackCalled);

        client.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(bCallbackCalled);

        client.mockLogger.AssertContains(">> login2 request: r=login2&u=User&p=[redacted]");
        client.mockLogger.AssertContains("<< login2 response (200): {\"Success\":true,\"User\":\"User\",\"Token\":\"[redacted]\",\"Score\":1234}");
    }
};

} // namespace tests
} // namespace impl
} // namespace context
} // namespace ra
