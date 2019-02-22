#ifndef RA_API_SUBMIT_NEW_TITLE_HH
#define RA_API_SUBMIT_NEW_TITLE_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class SubmitNewTitle
{
public:
    static constexpr const char* const Name() noexcept { return "SubmitNewTitle"; }

    struct Response : ApiResponseBase
    {
        unsigned int GameId{0U};
    };

    struct Request : ApiRequestBase
    {
        unsigned int ConsoleId{0U};
        std::string Hash;
        std::wstring GameName;

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

#endif // !RA_API_SUBMIT_NEW_TITLE_HH
