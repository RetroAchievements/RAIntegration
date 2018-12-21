#ifndef RA_SERVICES_MOCK_AUDIO_SYSTEM_HH
#define RA_SERVICES_MOCK_AUDIO_SYSTEM_HH
#pragma once

#include "services\IAudioSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockAudioSystem : public IAudioSystem
{
public:
    MockAudioSystem() noexcept : m_Override(this)
    {
    }

    void PlayAudioFile(const std::wstring& sPath) const override
    {
        m_vAudioFilesPlayed.emplace_back(sPath);
    }

    bool WasAudioFilePlayed(const std::wstring& sPath) const noexcept
    {
        for (const auto& sFile : m_vAudioFilesPlayed)
        {
            if (sFile == sPath)
                return true;
        }

        return false;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IAudioSystem> m_Override;

    mutable std::vector<std::wstring> m_vAudioFilesPlayed;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_AUDIO_SYSTEM_HH
