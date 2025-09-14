#ifndef RA_API_UPDATE_RICHPRESENCE_HH
#define RA_API_UPDATE_RICHPRESENCE_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class UpdateRichPresence
{
public:
    static constexpr const char* const Name() noexcept { return "UpdateRichPresence"; }

    struct Response : ApiResponseBase
    {
    };

    struct Request : ApiRequestBase
    {
        unsigned int GameId{ 0U };
        std::string Script;

        using Callback = std::function<void(const Response& response)>;

        Response Call() const;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_UPDATE_RICHPRESENCE_HH
