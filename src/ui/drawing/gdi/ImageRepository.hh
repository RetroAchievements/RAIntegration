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

    void FetchImage(ImageType nType, const std::string& sName) override;

    void AddReference(const ImageReference& pImage) override;
    GSL_SUPPRESS_F6 void ReleaseReference(ImageReference& pImage) noexcept override;

    bool HasReferencedImageChanged(ImageReference& pImage) const override;

private:
    static std::wstring GetFilename(ImageType nType, const std::string& sName);
    static HBITMAP LoadLocalPNG(const std::wstring& sFilename, size_t nWidth, size_t nHeight);

    HBITMAP GetImage(ImageType nType, const std::string& sName);
    HBITMAP GetDefaultImage(ImageType nType);

    HBitmapMap m_mBadges;
    HBitmapMap m_mUserPics;
    HBitmapMap m_mLocal;
    HBitmapMap m_mIcons;

    HBitmapMap* GetBitmapMap(ImageType nType) noexcept;

    std::mutex m_oMutex;
    std::set<std::wstring> m_vRequestedImages;
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif /* !RA_UI_DRAWING_GDI_IMAGEREPOSITORY_HH */
