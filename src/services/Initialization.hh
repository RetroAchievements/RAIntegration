#ifndef RA_SERVICES_INITIALIZATION_HH
#define RA_SERVICES_INITIALIZATION_HH
#pragma once

#include "RA_Interface.h"

namespace ra {
namespace services {

class Initialization
{
public:
    static void RegisterCoreServices();

    static void RegisterServices(EmulatorID nEmulatorId);

    static void Shutdown();
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_INITIALIZATION_HH
