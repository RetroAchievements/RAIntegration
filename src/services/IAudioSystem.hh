#ifndef RA_SERVICES_IAUDIOSYSTEM_HH
#define RA_SERVICES_IAUDIOSYSTEM_HH
#pragma once

#include <string>

namespace ra {
namespace services {

class IAudioSystem
{
public:
    virtual ~IAudioSystem() noexcept = default;
    IAudioSystem(const IAudioSystem&) noexcept = delete;
    IAudioSystem& operator=(const IAudioSystem&) noexcept = delete;
    IAudioSystem(IAudioSystem&&) noexcept = delete;
    IAudioSystem& operator=(IAudioSystem&&) noexcept = delete;

    /// <summary>
    /// Plays the specified audio file.
    /// </summary>
    virtual void PlayAudioFile(const std::wstring& sPath) const = 0;

protected:
    IAudioSystem() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IAUDIOSYSTEM_HH
