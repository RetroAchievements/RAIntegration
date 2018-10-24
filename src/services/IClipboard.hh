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
    IClipboard(const IClipboard&) noexcept = delete;
    IClipboard& operator=(const IClipboard&) noexcept = delete;
    IClipboard(IClipboard&&) noexcept = delete;
    IClipboard& operator=(IClipboard&&) noexcept = delete;

    /// <summary>
    /// Puts the provided text onto the clipboard.
    /// </summary>
    virtual void SetText(const std::wstring& sValue) const = 0;

protected:
    IClipboard() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ICLIPBOARD_HH
