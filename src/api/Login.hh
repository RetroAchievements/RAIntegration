#ifndef RA_API_LOGIN_HH
#define RA_API_LOGIN_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class Login
{
public:
    static const char* const Name() { return "Login"; }

    struct Response : ApiResponseBase
    {
        std::string Username;
        std::string ApiToken;
        unsigned int Score{ 0 };
        unsigned int NumUnreadMessages{ 0 };

        Response() noexcept = default;
        ~Response() noexcept = default;
        Response(const Response&) = default;
        Response& operator=(const Response&) = default;
        Response(Response&&) noexcept = default;
        Response& operator=(Response&&) noexcept = default;
    };

    struct Request : ApiRequestBase
    {
        std::string Username;
        std::string Password;
        std::string ApiToken;

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

#endif // !RA_API_LOGIN_HH
