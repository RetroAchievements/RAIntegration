#ifndef RA_API_START_SESSION_HH
#define RA_API_START_SESSION_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class StartSession
{
public:
    static constexpr const char* const Name() noexcept { return "StartSession"; }

    struct Response : ApiResponseBase
    {
    };

    struct Request : ApiRequestBase
    {
        unsigned int GameId{ 0U };

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

#endif // !RA_API_START_SESSION_HH
