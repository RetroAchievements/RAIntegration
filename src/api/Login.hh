#ifndef RA_API_LOGIN_HH
#define RA_API_LOGIN_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class Login
{
public:
    static constexpr const char* const Name() noexcept { return "Login"; }

    struct Response : ApiResponseBase
    {
        std::string Username;
        std::string ApiToken;
        unsigned int Score{ 0U };
        unsigned int NumUnreadMessages{ 0U };
    };

    struct Request : ApiRequestBase
    {
        std::string Username;
        std::string Password;
        std::string ApiToken;

        using Callback = std::function<void(const Response& response)>;

        Response Call() const noexcept;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }

        void CallAsyncWithRetry(Callback&& callback) const
        {
            ApiRequestBase::CallAsyncWithRetry<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_LOGIN_HH
