#pragma once

#include "RA_Defs.h"

#include <atomic>
#include <map>
#include <string>

namespace ra {
namespace services {

enum class ImageType
{
    None,
    Badge,
    UserPic,
    Local,
};

class ImageReference
{
public:
    ImageReference() {}
    ImageReference(ImageType nType, const std::string& sName)
    {
        ChangeReference(nType, sName);
    }

    ImageReference(const ImageReference& source)
        : m_nType(source.m_nType), m_sName(source.m_sName)
    {
        // don't copy m_hBitmap, it'll get initialized when GetHBitmap is called, which
        // will ensure the reference count is accurate.
    }

    ~ImageReference();
    
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
    void Release();

private:
    ImageType m_nType = ImageType::None;
    std::string m_sName;
    mutable HBITMAP m_hBitmap = nullptr;
};

class ImageRepository
{
public:        
    ~ImageRepository();

    /// <summary>
    /// Initializes the repository.
    /// </summary>
    bool Initialize();

    /// <summary>
    /// Ensures an image is available locally.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    void FetchImage(ImageType nType, const std::string& sName);
    
    /// <summary>
    /// Gets the HBITMAP for the requested image.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    /// <param name="bAddReference"><c>true</c> to add a reference to the image. Image will remain in memory until <see cref="ReleaseReference" /> is called.</param>
    /// <returns><see cref="HBITMAP" /> for the image, <c>nullptr</c> if not available.</returns>
    HBITMAP GetImage(ImageType nType, const std::string& sName, bool bAddReference);
    
    /// <summary>
    /// Releases a reference to an image.
    /// </summary>
    /// <param name="nType">Type of the image.</param>
    /// <param name="sName">Name of the image.</param>
    void ReleaseReference(ImageType nType, const std::string& sName);

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

    typedef std::map<std::string, HBitmapReference> HBitmapMap;
    HBitmapMap m_mBadges;
    HBitmapMap m_mUserPics;
    HBitmapMap m_mLocal;

    HBitmapMap* GetBitmapMap(ImageType nType);
};

extern ImageRepository g_ImageRepository;

} // namespace services
} // namespace ra

