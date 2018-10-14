#ifndef RA_SERVICES_ICLIPBOARD_HH
#define RA_SERVICES_ICLIPBOARD_HH
#pragma once

#include <string>

namespace ra {
namespace services {

class IClipboard
{
public:
    virtual ~IClipboard() noexcept = default;
    
    /// <summary>
    /// Puts the provided text onto the clipboard.
    /// </summary>
    virtual void SetText(const std::wstring& sValue) const = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ICLIPBOARD_HH
