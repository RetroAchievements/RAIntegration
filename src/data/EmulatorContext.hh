#ifndef RA_DATA_EMULATORCONTEXT_HH
#define RA_DATA_EMULATORCONTEXT_HH
#pragma once

#include "RA_Interface.h"

#include <string>

namespace ra {
namespace data {

class EmulatorContext
{
public:
    EmulatorContext() noexcept = default;
    virtual ~EmulatorContext() noexcept = default;
    EmulatorContext(const EmulatorContext&) noexcept = delete;
    EmulatorContext& operator=(const EmulatorContext&) noexcept = delete;
    EmulatorContext(EmulatorContext&&) noexcept = delete;
    EmulatorContext& operator=(EmulatorContext&&) noexcept = delete;

    /// <summary>
    /// Initializes the emulator context.
    /// </summary>
    void Initialize(EmulatorID nEmulatorId);

    void SetClientVersion(const std::string& sVersion) { m_sVersion = sVersion; }

    bool ValidateClientVersion();

    EmulatorID GetEmulatorId() const noexcept { return m_nEmulatorId; }

    const std::string& GetClientName() const noexcept { return m_sClientName; }

    const std::string& GetClientVersion() const noexcept { return m_sVersion; }

    void DisableHardcoreMode();

    bool EnableHardcoreMode();

    std::string GetAppTitle(const std::string& sMessage) const;

protected:
    EmulatorID m_nEmulatorId = EmulatorID::UnknownEmulator;
    std::string m_sVersion;
    std::string m_sLatestVersion;
    std::string m_sLatestVersionError;
    std::string m_sClientName;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_EMULATORCONTEXT_HH
