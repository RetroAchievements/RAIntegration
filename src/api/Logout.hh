#ifndef RA_API_LOGOUT_HH
#define RA_API_LOGOUT_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class Logout
{
public:
    static const char* const Name() { return "Logout"; }

    struct Response : ApiResponseBase
    {
        Response() noexcept = default;
        ~Response() noexcept = default;
        Response(const Response&) = default;
        Response& operator=(const Response&) = default;
        Response(Response&&) noexcept = default;
        Response& operator=(Response&&) noexcept = default;
    };

    struct Request : ApiRequestBase
    {
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

#endif // !RA_API_LOGOUT_HH
