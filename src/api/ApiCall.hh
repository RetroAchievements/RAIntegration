#ifndef RA_API_CALL_HH
#define RA_API_CALL_HH
#pragma once

#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include <string>

namespace ra {
namespace api {

enum class ApiResult
{    
    None = 0,       // unspecified
    Success,        // call was successful
    Error,          // an error occurred
    Failed,         // call was unsuccessful, but didn't error
    Unsupported,    // call is not supported and was not made
};

struct ApiRequestBase
{
protected:
    ApiRequestBase() noexcept = default;
    ~ApiRequestBase() noexcept = default;
    ApiRequestBase(const ApiRequestBase&) = default;
    ApiRequestBase& operator=(const ApiRequestBase&) = default;
    ApiRequestBase(ApiRequestBase&&) noexcept = default;
    ApiRequestBase& operator=(ApiRequestBase&&) noexcept = default;

    template<class TRequest, class TCallback>
    static void CallAsync(const TRequest& request, TCallback&& callback)
    {
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([request, callback = std::move(callback)]
            {
                callback(request.Call());
            });
    }
};

struct ApiResponseBase
{
protected:
    ApiResponseBase() noexcept = default;
    ~ApiResponseBase() noexcept = default;
    ApiResponseBase(const ApiResponseBase&) = default;
    ApiResponseBase& operator=(const ApiResponseBase&) = default;
    ApiResponseBase(ApiResponseBase&&) noexcept = default;
    ApiResponseBase& operator=(ApiResponseBase&&) noexcept = default;

public:
    /// <summary>
    /// The overall result of the API call.
    /// </summary>
    ApiResult Result;

    /// <summary>
    /// An error message associated with a failed call.
    /// </summary>
    std::string ErrorMessage;

    /// <summary>
    /// Gets whether the API call was considered successful.
    /// </summary>
    /// <returns><c>true</c> if the call was successful, <c>false</c> if not.</returns>
    bool Succeeded() const noexcept
    {
        return (Result == ApiResult::Success);
    }

    /// <summary>
    /// Gets whether the API call was considered unsuccessful.
    /// </summary>
    /// <returns><c>true</c> if the call was unsuccessful, <c>false</c> if not.</returns>
    bool Failed() const noexcept
    {
        return (Result == ApiResult::Error || Result == ApiResult::Unsupported || Result == ApiResult::Failed);
    }
};

// ---- template for generating a subclass file (replace XXXX with API name) ----
// NOTE: Implementation of Call method will be in ApiCall.cpp to avoid references IServer and all of its dependencies

/*
#ifndef RA_API_XXXX_HH
#define RA_API_XXXX_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class XXXX
{
public:
    static const char* const Name() { return "XXXX"; }

    struct Response : ApiResponseBase
    {
        // [RESPONSE PARAMETERS]

        Response() noexcept = default;
        ~Response() noexcept = default;
        Response(const Response&) = default;
        Response& operator=(const Response&) = default;
        Response(Response&&) noexcept = default;
        Response& operator=(Response&&) noexcept = default;
    };

    struct Request : ApiRequestBase
    {
        // [INPUT PARAMETERS]

        Request() noexcept = default;
        ~Request() noexcept = default;
        Request(const Request&) = default;
        Request& operator=(const Request&) = default;
        Request(Request&&) noexcept = default;
        Request& operator=(Request&&) noexcept = default;
        using Callback = std::function<void(const Response& response)>;

        Response Call() const noexcept;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_XXXX_HH
*/
// ---- template for generating a subclass file ----

} // namespace api
} // namespace ra

#endif // !RA_API_CALL_HH
