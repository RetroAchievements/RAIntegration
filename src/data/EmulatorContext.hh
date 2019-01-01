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

    /// <summary>
    /// Sets the client version.
    /// </summary>
    void SetClientVersion(const std::string& sVersion) { m_sVersion = sVersion; }

    /// <summary>
    /// Gets the version of the emulator this context was initialized with.
    /// </summary>
    const std::string& GetClientVersion() const noexcept { return m_sVersion; }

    /// <summary>
    /// Validates the client version against the latest version known by the server
    /// </summary>
    /// <remarks>
    /// May show dialogs prompting the user to acknowledge their version is out of date.
    /// The first time this is called, it makes a synchronous server call.
    /// </remarks>
    /// <returns>
    /// <c>true</c> if client version is sufficient for the current configuration, 
    /// <c>false</c> if not.
    /// </returns>
    bool ValidateClientVersion();

    /// <summary>
    /// Gets the unique identifier of the emulator this context was initialized with.
    /// </summary>
    EmulatorID GetEmulatorId() const noexcept { return m_nEmulatorId; }

    /// <summary>
    /// Gets the client name for the emulator this context was initialized with.
    /// </summary>
    const std::string& GetClientName() const noexcept { return m_sClientName; }

    /// <summary>
    /// Disables hardcore mode and notifies other services of that fact.
    /// </summary>
    void DisableHardcoreMode();

    /// <summary>
    /// Enables hardcore mode and notifies other services of that fact.
    /// </summary>
    /// <returns><c>true</c> if hardcore mode was enabled, <c>false</c> if the user chose not to based on prompts they
    /// were given.</returns>
    bool EnableHardcoreMode();

    /// <summary>
    /// Builds a string for displaying in the title bar of the emulator.
    /// </summary>
    /// <param name="sMessage">A custom message provided by the emulator to also display in the title bar.</param>
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
