#ifndef RA_API_UPLOAD_BADGE_HH
#define RA_API_UPLOAD_BADGE_HH
#pragma once

#include "ApiCall.hh"

#include "ra_fwd.h"

namespace ra {
namespace api {

class UploadBadge
{
public:
    static constexpr const char* const Name() noexcept { return "UploadBadge"; }

    struct Response : ApiResponseBase
    {
        std::string BadgeId;
    };

    struct Request : ApiRequestBase
    {
        std::wstring ImageFilePath;

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

#endif // !RA_API_UPLOAD_BADGE_HH
