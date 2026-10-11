#include "gsl.hh"

#include "services/ILogger.hh"
#include "services/IMessageDispatcher.hh"
#include "services/ServiceLocator.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

static inline constexpr const char* __gsl_filename(const char* const str)
{
    if (str == nullptr)
        return str;

    if (str[0] == 's' && str[1] == 'r' && str[2] == 'c' && str[3] == '\\')
        return str;

    const char* scan = str;
    if (scan == nullptr)
        return str;

    while (*scan != '\\')
    {
        if (!*scan)
            return str;
        scan++;
    }

    return __gsl_filename(scan + 1);
}

#ifdef NDEBUG

GSL_SUPPRESS_F6 /* exists checks prevent exception */
void __gsl_contract_handler(const char* const file, unsigned int line) noexcept
{
    static char buffer[128];
    snprintf(buffer, sizeof(buffer), "Assertion failure at %s: %u", __gsl_filename(file), line);

    if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
    {
        RA_LOG_ERR(buffer);
    }

    if (ra::services::ServiceLocator::Exists<ra::services::IMessageDispatcher>())
    {
        ra::services::ServiceLocator::Get<ra::services::IMessageDispatcher>()
            .ReportErrorMessage(L"Unexpected error", ra::util::String::Widen(buffer));
    }

    gsl::details::terminate();
}

#else

GSL_SUPPRESS_F6 /* exists checks prevent exception */
void __gsl_contract_handler(const char* const file, unsigned int line, const char* const error) noexcept
{
    const auto sError = ra::util::String::Printf("Assertion failure at %s: %u: %s", __gsl_filename(file), line, error);

    if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
    {
        RA_LOG_ERR("%s", sError.c_str());
    }

    if (ra::services::ServiceLocator::Exists<ra::services::IMessageDispatcher>())
    {
        ra::services::ServiceLocator::Get<ra::services::IMessageDispatcher>()
            .ReportErrorMessage(L"Unexpected error", ra::util::String::Widen(sError));
    }

    gsl::details::terminate();
}

#endif
