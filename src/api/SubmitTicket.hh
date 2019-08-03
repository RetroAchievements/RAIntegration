#ifndef RA_API_SUBMIT_TICKET_HH
#define RA_API_SUBMIT_TICKET_HH
#pragma once

#include "ApiCall.hh"

#include "data\Types.hh"

namespace ra {
namespace api {

class SubmitTicket
{
public:
    static constexpr const char* const Name() noexcept { return "SubmitTicket"; }

    struct Response : ApiResponseBase
    {
        unsigned int TicketsCreated{};
    };

    enum class ProblemType
    {
        WrongTime = 1,
        DidNotTrigger = 2,
    };

    struct Request : ApiRequestBase
    {
        std::string GameHash;
        std::set<ra::AchievementID> AchievementIds;
        ProblemType Problem = ProblemType::WrongTime;
        std::string Comment;

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

#endif // !RA_API_SUBMIT_TICKET_HH
