#include "ServiceLocator.hh"

#include "services/ILogger.hh"
#include "services/IMessageDispatcher.hh"

#include "util/GSL.hh"
#include "util/Strings.hh"

namespace ra {
namespace services {

static std::function<void(const std::string&)> s_fNoServiceHandler;

void ServiceLocator::SetNoServiceHandler(std::function<void(const std::string&)> fHandler)
{
    s_fNoServiceHandler = fHandler;
}

GSL_SUPPRESS_F6
void ServiceLocator::ReportNoServiceProvided(const char* sFunctionName) noexcept
{
    // Extract the class name from the templated Service<TClass> attribute.
    std::string sServiceName = sFunctionName;

    const size_t index = sServiceName.find('<');
    const size_t index2 = sServiceName.rfind('>');
    if (index >= 0 && index2 > index)
    {
        sServiceName.erase(index2);
        sServiceName.erase(0, index + 1);

        if (ra::util::String::StartsWith(sServiceName, "class "))
            sServiceName.erase(0, 6);
    }

    std::string sMessage = "No service provided for " + sServiceName;

    if (s_fNoServiceHandler)
        s_fNoServiceHandler(sMessage);

    try
    {
        if (Service<ra::services::ILogger>::Exists())
        {
            const auto& pLogger = Service<ra::services::ILogger>::Get();
            if (pLogger.IsEnabled(ra::services::LogLevel::Error))
                pLogger.LogMessage(ra::services::LogLevel::Error, "ERROR: " + sMessage);
        }

        if (Service<ra::services::IMessageDispatcher>::Exists())
        {
            Service<ra::services::IMessageDispatcher>::Get()
                .ReportErrorMessage(L"Unexpected error", ra::util::String::Widen(sMessage));
        }
    } catch(...) {
        // ignore exceptions thrown trying to report the missing service. we're about to terminate anyway.
    }

    gsl::details::terminate();
}

} // namespace services
} // namespace ra
