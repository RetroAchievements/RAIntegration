#ifndef IMAGEREPOSITORY_H
#define IMAGEREPOSITORY_H
#pragma once

#include "RA_Defs.h"

namespace ra {
namespace services {

enum class ImageType
{
    None,
    Badge,
    UserPic,
    Local,
    Icon,
};

class ImageReference
{
public:
    ImageReference() noexcept = default;
    explicit ImageReference(ImageType nType, const std::string& sName) noexcept
    {
        ChangeReference(nType, sName);
    }

    ImageReference(const ImageReference& source) noexcept :
        m_nType{ source.m_nType }, m_sName{ source.m_sName }
    {
        // don't copy m_hBitmap, it'll get initialized when GetHBitmap is called, which
        // will ensure the reference count is accurate.
    }

    ImageReference& operator=(const ImageReference&) noexcept = delete;

    ImageReference(ImageReference&& source) noexcept :
        m_nType{source.m_nType},
        m_sName{std::move(source.m_sName)},
        m_hBitmap{ source.m_hBitmap }
    {
        source.m_nType = ImageType{};
        if (source.m_hBitmap != nullptr)
        {
            DeleteBitmap(source.m_hBitmap);
            source.m_hBitmap = nullptr;
        }
    }

    ImageReference& operator=(ImageReference&& source) noexcept
    {
        m_nType   = source.m_nType;
        m_sName   = std::move(source.m_sName);
        m_hBitmap = source.m_hBitmap;

        source.m_nType = ImageType{};
        if (source.m_hBitmap != nullptr)
        {
            DeleteBitmap(source.m_hBitmap);
            source.m_hBitmap = nullptr;
        }

        return *this;
    }

    ~ImageReference() noexcept;

    /// <summary>
    /// Updates the referenced image.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    void ChangeReference(ImageType nType, const std::string& sName);

    /// <summary>
    /// Gets the bitmap handle for the image.
    /// </summary>
    HBITMAP GetHBitmap() const;

    /// <summary>
    /// Releases the reference to the image. Will be re-acquired the next time <see cref="GetHBitmap" /> is called.
    /// </summary>
    void Release() noexcept;

private:
    ImageType m_nType = ImageType::None;
    std::string m_sName;
    mutable HBITMAP m_hBitmap = nullptr;
};

class ImageRepository
{
public:
    ImageRepository() noexcept(false) = default; // we still need the noexcept
    ~ImageRepository() noexcept;
    ImageRepository(const ImageRepository&) noexcept = delete;
    ImageRepository& operator=(const ImageRepository&) noexcept = delete;
    ImageRepository(ImageRepository&&) noexcept = delete;
    ImageRepository& operator=(ImageRepository&&) noexcept = delete;

    /// <summary>Initializes the repository.</summary>
    bool Initialize();

    /// <summary>Ensures an image is available locally.</summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    void FetchImage(ImageType nType, const std::string& sName);

    /// <summary>Gets the <c>HBITMAP</c> for the requested image.</summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    /// <param name="bAddReference">
    ///   <c>true</c> to add a reference to the image. Image will remain in memory until
    ///   <see cref="ReleaseReference" /> is called.
    /// </param>
    /// <returns><see cref="HBITMAP" /> for the image, <c>nullptr</c> if not available.</returns>
    HBITMAP GetImage(ImageType nType, const std::string& sName, bool bAddReference);

    /// <summary>Releases a reference to an image.</summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    void ReleaseReference(ImageType nType, const std::string& sName) noexcept;

private:
    static std::wstring GetFilename(ImageType nType, const std::string& sName);
    static HBITMAP LoadLocalPNG(const std::wstring& sFilename, size_t nWidth, size_t nHeight);

    friend class ImageReference;
    HBITMAP DefaultImage(ImageType nType);

    struct HBitmapReference
    {
        HBITMAP m_hBitmap;
        std::atomic<unsigned int> m_nReferences;
    };

    using HBitmapMap = std::unordered_map<std::string, HBitmapReference>;
    HBitmapMap m_mBadges;
    HBitmapMap m_mUserPics;
    HBitmapMap m_mIcons;
    HBitmapMap m_mLocal;

    HBitmapMap* GetBitmapMap(ImageType nType);

    std::mutex m_oMutex;
    std::set<std::wstring> m_vRequestedImages;
};

extern ImageRepository g_ImageRepository;

} // namespace services
} // namespace ra

#endif /* !IMAGEREPOSITORY_H */
