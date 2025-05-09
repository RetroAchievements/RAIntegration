#ifndef RA_UI_DRAWING_GDI_IMAGEREPOSITORY_HH
#define RA_UI_DRAWING_GDI_IMAGEREPOSITORY_HH
#pragma once

#include "ui\ImageReference.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

class ImageRepository : public IImageRepository
{
    struct HBitmapReference
    {
        HBITMAP m_hBitmap{};
        std::atomic<unsigned int> m_nReferences;
    };

    using HBitmapMap = std::unordered_map<std::string, HBitmapReference>;
public:
    ImageRepository() noexcept(std::is_nothrow_default_constructible_v<HBitmapMap> &&
                               std::is_nothrow_default_constructible_v<std::set<std::wstring>>) = default;
    GSL_SUPPRESS_F6 ~ImageRepository() noexcept;
    ImageRepository(const ImageRepository&) = delete;
    ImageRepository& operator=(const ImageRepository&) = delete;
    ImageRepository(ImageRepository&&) = delete;
    ImageRepository& operator=(ImageRepository&&) = delete;

    /// <summary>
    /// Initializes the repository.
    /// </summary>
    bool Initialize();

    /// <summary>
    /// Gets the <see cref="HBITMAP" /> from an <see cref="ImageReference" />.
    /// </summary>
    static HBITMAP GetHBitmap(const ImageReference& pImage);

    bool IsImageAvailable(ImageType nType, const std::string& sName) const override;

    void FetchImage(ImageType nType, const std::string& sName, const std::string& sSourceUrl) override;
    std::string StoreImage(ImageType nType, const std::wstring& sPath) override;

    void AddReference(const ImageReference& pImage) override;
    GSL_SUPPRESS_F6 void ReleaseReference(ImageReference& pImage) noexcept override;

    bool HasReferencedImageChanged(ImageReference& pImage) const override;

    std::wstring GetFilename(ImageType nType, const std::string& sName) const override;

private:
    static HBITMAP LoadLocalPNG(const std::wstring& sFilename, unsigned int nWidth, unsigned int nHeight);

    HBITMAP GetImage(ImageType nType, const std::string& sName);
    HBITMAP GetDefaultImage(ImageType nType);

    HBitmapMap m_mBadges;
    HBitmapMap m_mUserPics;
    HBitmapMap m_mLocal;
    HBitmapMap m_mIcons;

    GSL_SUPPRESS_F6 HBitmapMap* GetBitmapMap(ImageType nType) noexcept;

    mutable std::mutex m_oMutex;
    std::set<std::wstring> m_vRequestedImages;
    bool m_bShutdownCOM = false;
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif /* !RA_UI_DRAWING_GDI_IMAGEREPOSITORY_HH */
