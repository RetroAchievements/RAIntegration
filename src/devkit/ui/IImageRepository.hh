#ifndef RA_UI_IMAGEREPOSITORY_H
#define RA_UI_IMAGEREPOSITORY_H
#pragma once

#include "ImageReference.hh"

#include "data/NotifyTargetSet.hh"

namespace ra {
namespace ui {

class IImageRepository
{
public:
    virtual ~IImageRepository() noexcept = default;

    IImageRepository(const IImageRepository&) noexcept = delete;
    IImageRepository& operator=(const IImageRepository&) noexcept = delete;
    IImageRepository(IImageRepository&&) noexcept = delete;
    IImageRepository& operator=(IImageRepository&&) noexcept = delete;

    /// <summary>
    /// Determines if an image is available locally.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    virtual bool IsImageAvailable(ImageType nType, const std::string& sName) const = 0;

    /// <summary>
    /// Ensures an image is available locally.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    /// <param name="sSourceUrl">The url of the image to download (empty string to generate a URL).</param>
    virtual void FetchImage(ImageType nType, const std::string& sName, const std::string& sSourceUrl) = 0;

    /// <summary>
    /// Store an image locally.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Path to image.</param>
    /// <returns>Generated name for referencing the image.</param>
    virtual std::string StoreImage(ImageType nType, const std::wstring& sPath) = 0;

    /// <summary>
    /// Gets the full path to the file for the specified image.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Path to image.</param>
    /// <returns>Path to the file, or empty string if a file is not available.</returns>
    virtual std::wstring GetFilename(ImageType nType, const std::string& sName) const = 0;

    /// <summary>Adds a reference to an image.</summary>
    virtual void AddReference(const ImageReference& pImage) = 0;

    /// <summary>Releases a reference to an image.</summary>
    virtual void ReleaseReference(ImageReference& pImage) noexcept(false) = 0;

    /// <summary>
    /// Determines whether the referenced image has changed.
    /// </summary>
    /// <remarks>
    /// Updates the internal state of the <see cref="ImageReference" /> if <c>true</c>.
    /// </remarks>
    virtual bool HasReferencedImageChanged(ImageReference& pImage) const = 0;

    class NotifyTarget
    {
    public:
        NotifyTarget() noexcept = default;
        virtual ~NotifyTarget() noexcept = default;
        NotifyTarget(const NotifyTarget&) noexcept = delete;
        NotifyTarget& operator=(const NotifyTarget&) noexcept = delete;
        NotifyTarget(NotifyTarget&&) noexcept = default;
        NotifyTarget& operator=(NotifyTarget&&) noexcept = default;

        virtual void OnImageChanged([[maybe_unused]] ImageType nType, [[maybe_unused]] const std::string& sName) noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { m_vNotifyTargets.Add(pTarget); }
    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept { m_vNotifyTargets.Remove(pTarget); }

protected:
    GSL_SUPPRESS_F6 IImageRepository() = default;

    void OnImageChanged(ImageType nType, const std::string& sName)
    {
        if (m_vNotifyTargets.LockIfNotEmpty())
        {
            for (auto& target : m_vNotifyTargets.Targets())
                target.OnImageChanged(nType, sName);

            m_vNotifyTargets.Unlock();
        }
    }

private:
    ra::data::NotifyTargetSet<NotifyTarget> m_vNotifyTargets;
};

} // namespace ui
} // namespace ra

#endif /* !RA_UI_IMAGEREPOSITORY_H */
