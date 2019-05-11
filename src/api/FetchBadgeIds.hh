#ifndef RA_API_FETCH_BADGE_IDS_HH
#define RA_API_FETCH_BADGE_IDS_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class FetchBadgeIds
{
public:
    static constexpr const char* const Name() noexcept { return "FetchBadgeIds"; }

    struct Response : ApiResponseBase
    {
        unsigned int FirstID;
        unsigned int NextID;
    };

    struct Request : ApiRequestBase
    {
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

#endif // !RA_API_FETCH_BADGE_IDS_HH
