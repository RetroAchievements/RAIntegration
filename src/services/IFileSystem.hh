#ifndef RA_SERVICES_IFILESYSTEM
#define RA_SERVICES_IFILESYSTEM
#pragma once

#include "services/TextReader.hh"
#include "services/TextWriter.hh"

#undef DeleteFile
#undef MoveFile
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
    /// Gets the size of the file (in bytes).
    /// </summary>
    /// <remarks>Returns -1 if the file does not exist.</remarks>
    virtual int64_t GetFileSize(const std::wstring& sPath) const = 0;

    /// <summary>
    /// Deletes the specified file.
    /// </summary>
    /// <returns><c>true</c> if successful, <c>false</c> if not.</returns>
    virtual bool DeleteFile(const std::wstring& sPath) const = 0;

    /// <summary>
    /// Moves a file from one location to another.
    /// </summary>
    /// <remarks>Can be used to rename a file if the path to the file is the same.</remarks>
    /// <returns><c>true</c> if successful, <c>false</c> if not.</returns>
    virtual bool MoveFile(const std::wstring& sOldPath, const std::wstring& sNewPath) const = 0;

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
