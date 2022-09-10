#ifndef RA_SERVICES_INITIALIZATION_HH
#define RA_SERVICES_INITIALIZATION_HH
#pragma once

#include "RAInterface\RA_Emulators.h"

namespace ra {
namespace services {

class Initialization
{
public:
    static void RegisterCoreServices();

    static void RegisterServices(EmulatorID nEmulatorId, const char* sClientName);

    static void Shutdown();

    static bool IsInitialized() noexcept { return s_bIsInitialized; }

private:
    static void InitializeNotifyTargets();

    static bool s_bIsInitialized;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_INITIALIZATION_HH
