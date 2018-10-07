#ifndef RA_SERVICES_INITIALIZATION_HH
#define RA_SERVICES_INITIALIZATION_HH
#pragma once

#ifndef PCH_H
#include <string>  
#endif /* !PCH_H */

namespace ra {
namespace services {

class Initialization
{
public:
    static void RegisterServices(const std::string& sClientName);

    static void Shutdown();
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_INITIALIZATION_HH
