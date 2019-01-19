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
    /// Ensures an image is available locally.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    virtual void FetchImage(ImageType nType, const std::string& sName) = 0;

    /// <summary>Adds a reference to an image.</summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    virtual void AddReference(const ImageReference& pImage) = 0;

    /// <summary>Releases a reference to an image.</summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    virtual void ReleaseReference(ImageReference& pImage) noexcept = 0;

    /// <summary>
    /// Determines whether the referenced image has changed.
    /// </summary>
    /// <remarks>
    /// Updates the internal state of the <see cref="ImageReference" /> if <c>true</c>.
    /// </remarks>
    virtual bool HasReferencedImageChanged(ImageReference& pImage) const = 0;

protected:
    IImageRepository() noexcept = default;
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
    mutable unsigned long m_nData{};
    friend class drawing::gdi::ImageRepository;
};

} // namespace ui
} // namespace ra

#endif /* !RA_UI_IMAGEREPOSITORY_H */
