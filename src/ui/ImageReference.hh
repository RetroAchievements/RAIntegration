#ifndef RA_UI_IMAGEREPOSITORY_H
#define RA_UI_IMAGEREPOSITORY_H
#pragma once

#include "ra_fwd.h"

#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace drawing::gdi {

class ImageRepository;

} // namespace drawing::gdi

enum class ImageType
{
    None,
    Badge,
    UserPic,
    Local,
    Icon,
};

class ImageReference;

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

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.insert(&pTarget); }
    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.erase(&pTarget); }

protected:
    GSL_SUPPRESS_F6 IImageRepository() = default;

    void OnImageChanged(ImageType nType, const std::string& sName)
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnImageChanged(nType, sName);
        }
    }

private:
    using NotifyTargetSet = std::set<NotifyTarget*>;
    NotifyTargetSet m_vNotifyTargets;
};

class ImageReference
{
public:
    ImageReference() noexcept = default;
    ~ImageReference() noexcept
    {
        if (ra::services::ServiceLocator::Exists<IImageRepository>())
            Release();
    }

    explicit ImageReference(ImageType nType, const std::string& sName) : m_nType(nType), m_sName(sName) {}

    ImageReference(const ImageReference& source) = default;
    ImageReference& operator=(const ImageReference&) noexcept = delete;
    ImageReference(ImageReference&& source) noexcept = default;
    ImageReference& operator=(ImageReference&& source) noexcept = delete;

    /// <summary>
    /// Get the image type.
    /// </summary>
    inline constexpr ImageType Type() const noexcept { return m_nType; }

    /// <summary>
    /// Get the image name.
    /// </summary>
    const std::string& Name() const noexcept { return m_sName; }

    /// <summary>
    /// Updates the referenced image.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    void ChangeReference(ImageType nType, const std::string& sName)
    {
        if (nType != m_nType || sName != m_sName)
        {
            Release();

            m_nType = nType;
            m_sName = sName;
        }
    }

    /// <summary>
    /// Releases this reference image.
    /// </summary>
    GSL_SUPPRESS_F6 void Release() noexcept
    {
        if (m_nType != ImageType::None)
        {
            // Suppress not working inline, but should
            GSL_SUPPRESS_F6 auto& pRepository = ra::services::ServiceLocator::GetMutable<IImageRepository>();
            pRepository.ReleaseReference(*this);
        }
    }

private:
    ImageType m_nType{};
    std::string m_sName;
    mutable unsigned long long m_nData{};
    friend class drawing::gdi::ImageRepository;
};

} // namespace ui
} // namespace ra

#endif /* !RA_UI_IMAGEREPOSITORY_H */
