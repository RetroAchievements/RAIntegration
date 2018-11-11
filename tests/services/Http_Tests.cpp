#include "CppUnitTest.h"

#include "services\Http.hh"
#include "services\IHttpRequester.hh"

#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockHttpRequester.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::mocks::MockFileSystem;
using ra::services::mocks::MockHttpRequester;
using ra::services::mocks::MockThreadPool;

namespace ra {
namespace services {
namespace tests {

TEST_CLASS(Http_Tests)
{
public:
    TEST_METHOD(TestUrlEncode)
    {
        Assert::AreEqual(std::string("1234567890"), Http::UrlEncode("1234567890"));
        Assert::AreEqual(std::string("s3%3FAJhs%3Au%2BS%3DD%3C%25-%5D%40%7BO"), Http::UrlEncode("s3?AJhs:u+S=D<%-]@{O"));
        Assert::AreEqual(std::string("This+is+a+test."), Http::UrlEncode("This is a test."));
    }

    TEST_METHOD(TestUrlEncodeAppend)
    {
        std::string sText("m=");
        Http::UrlEncodeAppend(sText, "This is a test.");
        Assert::AreEqual(std::string("m=This+is+a+test."), sText);
    }

    TEST_METHOD(TestRequestInitialization)
    {
        Http::Request request("foo.com");
        Assert::AreEqual(std::string("foo.com"), request.GetUrl());
        Assert::AreEqual(std::string(""), request.GetQueryString());
        Assert::AreEqual(std::string(""), request.GetPostData());
        Assert::AreEqual(std::string("application/x-www-form-urlencoded"), request.GetContentType());
    }

    TEST_METHOD(TestRequestInitializationQueryString)
    {
        Http::Request request("foo.com?a=b&c=10");
        Assert::AreEqual(std::string("foo.com"), request.GetUrl());
        Assert::AreEqual(std::string("a=b&c=10"), request.GetQueryString());
        Assert::AreEqual(std::string(""), request.GetPostData());
        Assert::AreEqual(std::string("application/x-www-form-urlencoded"), request.GetContentType());
    }

    TEST_METHOD(TestRequestAddQueryParm)
    {
        Http::Request request("foo.com");
        Assert::AreEqual(std::string(""), request.GetQueryString());
        request.AddQueryParm("a", "b");
        Assert::AreEqual(std::string("a=b"), request.GetQueryString());
        request.AddQueryParm("c", "10");
        Assert::AreEqual(std::string("a=b&c=10"), request.GetQueryString());
        request.AddQueryParm("d", "1.25%");
        Assert::AreEqual(std::string("a=b&c=10&d=1.25%25"), request.GetQueryString());
    }

    TEST_METHOD(TestCall)
    {
        MockHttpRequester mockHttp([](const Http::Request& request)
        {
            Assert::AreEqual(std::string("foo.com"), request.GetUrl());
            return Http::Response(Http::StatusCode::OK, "Hello");
        });
        Http::Request request("foo.com");
        auto response = request.Call();

        Assert::AreEqual(Http::StatusCode::OK, response.StatusCode());
        Assert::AreEqual(std::string("Hello"), response.Content());
    }

    TEST_METHOD(TestCallNotFound)
    {
        MockHttpRequester mockHttp([](const Http::Request& request)
        {
            Assert::AreEqual(std::string("foo.com"), request.GetUrl());
            return Http::Response(Http::StatusCode::NotFound, "");
        });
        Http::Request request("foo.com");
        auto response = request.Call();

        Assert::AreEqual(Http::StatusCode::NotFound, response.StatusCode());
        Assert::AreEqual(std::string(""), response.Content());
    }

    TEST_METHOD(TestCallAsync)
    {
        MockHttpRequester mockHttp([](const Http::Request& request)
        {
            Assert::AreEqual(std::string("foo.com"), request.GetUrl());
            return Http::Response(Http::StatusCode::OK, "Hello");
        });
        Http::Request request("foo.com");

        MockThreadPool mockThreadPool;
        bool bCallbackCalled = false;
        request.CallAsync([&bCallbackCalled](const Http::Response& response)
        {
            Assert::AreEqual(Http::StatusCode::OK, response.StatusCode());
            Assert::AreEqual(std::string("Hello"), response.Content());
            bCallbackCalled = true;
        });

        Assert::IsFalse(bCallbackCalled);
        mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(bCallbackCalled);
    }

    TEST_METHOD(TestDownload)
    {
        MockFileSystem mockFileSystem;
        MockHttpRequester mockHttp([](const Http::Request& request)
        {
            Assert::AreEqual(std::string("foo.com"), request.GetUrl());
            return Http::Response(Http::StatusCode::OK, "Hello");
        });
        Http::Request request("foo.com");
        auto response = request.Download(L".\\test.txt");

        Assert::AreEqual(Http::StatusCode::OK, response.StatusCode());
        Assert::AreEqual(std::string(), response.Content());
        Assert::AreEqual(std::string("Hello"), mockFileSystem.GetFileContents(L".\\test.txt"));
    }

    TEST_METHOD(TestDownloadAsync)
    {
        MockFileSystem mockFileSystem;
        MockHttpRequester mockHttp([](const Http::Request& request)
        {
            Assert::AreEqual(std::string("foo.com"), request.GetUrl());
            return Http::Response(Http::StatusCode::OK, "Hello");
        });
        Http::Request request("foo.com");

        MockThreadPool mockThreadPool;
        bool bCallbackCalled = false;
        request.DownloadAsync(L".\\test.txt", [&bCallbackCalled,&mockFileSystem](const Http::Response& response)
        {
            Assert::AreEqual(Http::StatusCode::OK, response.StatusCode());
            Assert::AreEqual(std::string(), response.Content());
            Assert::AreEqual(std::string("Hello"), mockFileSystem.GetFileContents(L".\\test.txt"));
            bCallbackCalled = true;
        });

        Assert::IsFalse(bCallbackCalled);
        mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(bCallbackCalled);
    }
};

} // namespace tests
} // namespace services
} // namespace ra
