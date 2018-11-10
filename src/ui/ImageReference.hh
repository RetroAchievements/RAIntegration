#ifndef RA_UI_IMAGEREPOSITORY_H
#define RA_UI_IMAGEREPOSITORY_H
#pragma once

#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {

enum class ImageType
{
    None,
    Badge,
    UserPic,
    Local,
};

class ImageReference;

class IImageRepository
{
public:
    virtual ~IImageRepository() noexcept = default;

    IImageRepository(const IImageRepository&) noexcept = delete;
    IImageRepository(IImageRepository&&) noexcept = delete;

    /// <summary>
    /// Ensures an image is available locally.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    virtual void FetchImage(ImageType nType, const std::string& sName) = 0;

    /// <summary>Adds a reference to an image.</summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    virtual void AddReference(ImageReference& pImage) noexcept = 0;

    /// <summary>Releases a reference to an image.</summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    virtual void ReleaseReference(ImageReference& pImage) noexcept = 0;

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

    explicit ImageReference(ImageType nType, const std::string& sName) noexcept
        : m_nType(nType), m_sName(sName)
    {
    }

    ImageReference(const ImageReference& source) = default;
    ImageReference& operator=(const ImageReference&) noexcept = delete;
    ImageReference(ImageReference&& source) noexcept = default;
    ImageReference& operator=(ImageReference&& source) noexcept = delete;
    
    /// <summary>
    /// Get the image type.
    /// </summary>
    ImageType Type() const { return m_nType; }
    
    /// <summary>
    /// Get the image name.
    /// </summary>
    const std::string& Name() const { return m_sName; }

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
    void Release()
    {
        if (m_nType != ImageType::None)
        {
            auto& pRepository = ra::services::ServiceLocator::GetMutable<IImageRepository>();
            pRepository.ReleaseReference(*this);
        }
    }
    
    /// <summary>
    /// Gets custom data associated to the reference - used to cache data by <see cref="ISurface::DrawImage" />.
    /// </summary>
    unsigned long GetData() const { return m_nData; }

    /// <summary>
    /// Sets custom data associated to the reference - used to cache data by <see cref="ISurface::DrawImage" />.
    /// </summary>
    void SetData(unsigned long nValue) { m_nData = nValue; }

private:
    ImageType m_nType = ImageType::None;
    std::string m_sName;
    unsigned long m_nData{};
};

} // namespace ui
} // namespace ra

#endif /* !RA_UI_IMAGEREPOSITORY_H */
