#ifndef RA_SERVICES_MOCK_FILESYSTEM_HH
#define RA_SERVICES_MOCK_FILESYSTEM_HH
#pragma once

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"
#include "services\impl\StringTextReader.hh"
#include "services\impl\StringTextWriter.hh"

#include <map>

namespace ra {
namespace services {
namespace mocks {

class MockFileSystem : public IFileSystem
{
public:
    MockFileSystem() noexcept 
        : m_Override(this)
    {
    }

    const std::wstring& BaseDirectory() const override { return m_sBaseDirectory; }
    void SetBaseDirectory(const std::wstring& sBaseDirectory)
    {
        m_sBaseDirectory = sBaseDirectory;
    }

    bool DirectoryExists(const std::wstring& sDirectory) const override
    {
        assert(!"not implemented");
        return false;
    }

    bool CreateDirectory(const std::wstring& sDirectory) const override
    {
        assert(!"not implemented");
        return false;
    }

    void MockFile(const std::wstring& sPath, const std::string& sContents)
    {
        m_mFileContents[sPath] = sContents;
    }

    std::string& GetFileContents(const std::wstring& sPath)
    {
        auto pIter = m_mFileContents.find(sPath);
        if (pIter != m_mFileContents.end())
            return pIter->second;

        static std::string sEmpty;
        return sEmpty;
    }

    std::unique_ptr<TextReader> OpenTextFile(const std::wstring& sPath) const override
    {
        auto pIter = m_mFileContents.find(sPath);
        if (pIter == m_mFileContents.end())
            return std::unique_ptr<TextReader>();

        auto pReader = std::make_unique<ra::services::impl::StringTextReader>(pIter->second);
        return std::unique_ptr<TextReader>(pReader.release());
    }

    std::unique_ptr<TextWriter> CreateTextFile(const std::wstring& sPath) const override
    {
        std::string& sContents = m_mFileContents[sPath];
        auto pWriter = std::make_unique<ra::services::impl::StringTextWriter>(sContents);
        return std::unique_ptr<TextWriter>(pWriter.release());
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IFileSystem> m_Override;
    std::wstring m_sBaseDirectory = L".\\";
    mutable std::map<std::wstring, std::string> m_mFileContents;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_FILESYSTEM_HH
