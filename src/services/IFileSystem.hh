#ifndef RA_SERVICES_IFILESYSTEM
#define RA_SERVICES_IFILESYSTEM
#pragma once

#include "services/TextReader.hh"
#include "services/TextWriter.hh"

#ifndef PCH_H
#include <string>
#include <memory>

// nuke WinAPI #defines - anything including this file should be using these functions
#undef CreateDirectory   
#endif /* !PCH_H */

namespace ra {
namespace services {

class IFileSystem
{
public:
    virtual ~IFileSystem() noexcept = default;

    /// <summary>
    /// Gets the base directory of the running executable.
    /// </summary>
    virtual const std::wstring& BaseDirectory() const = 0;

    /// <summary>
    /// Determines if the specified directory exists.
    /// </summary>
    virtual bool DirectoryExists(const std::wstring& sDirectory) const = 0;

    /// <summary>
    /// Creates the specified directory.
    /// </summary>
    /// <returns><c>true</c> if successful, <c>false</c> if not.</returns>
    virtual bool CreateDirectory(const std::wstring& sDirectory) const = 0;

    /// <summary>
    /// Opens the specified file.
    /// </summary>
    /// <returns>Pointer to a <see cref="TextReader"/> for reading the contents of the file. nullptr if the file was not found.</returns>
    virtual std::unique_ptr<TextReader> OpenTextFile(const std::wstring& sPath) const = 0;

    /// <summary>
    /// Creates the specified file.
    /// </summary>
    /// <returns>Pointer to a <see cref="TextWriter"/> for writing the contents of the file. nullptr if the file could not be created.</returns>
    virtual std::unique_ptr<TextWriter> CreateTextFile(const std::wstring& sPath) const = 0;

    /// <summary>
    /// Opens the specified file for appending.
    /// </summary>
    /// <returns>Pointer to a <see cref="TextWriter"/> for writing the contents of the file. nullptr if the file could not be opened.</returns>
    /// <remarks>Will create the file if it doesn't already exist.</remarks>
    virtual std::unique_ptr<TextWriter> AppendTextFile(const std::wstring& sPath) const = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IFILESYSTEM
