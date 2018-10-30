#include "Exports.hh"

#include "RA_BuildVer.h"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

API const char* CCONV _RA_IntegrationVersion()
{
    return RA_INTEGRATION_VERSION;
}

API const char* CCONV _RA_HostName()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.GetHostName().c_str();
}

API int CCONV _RA_HardcoreModeIsActive()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    return pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore);
}
