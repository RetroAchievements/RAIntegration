#ifndef RA_SERVICES_WINDOWS_AUDIOSYSTEM_HH
#define RA_SERVICES_WINDOWS_AUDIOSYSTEM_HH
#pragma once

#include "services\IAudioSystem.hh"
#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace impl {

class WindowsAudioSystem : public ra::services::IAudioSystem
{
public:
    void PlayAudioFile(const std::wstring& sPath) const override
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        std::wstring sFullPath = pFileSystem.BaseDirectory() + sPath;

        PlaySoundW(sFullPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_WINDOWS_AUDIOSYSTEM_HH
