#ifndef RA_UI_IMAGEREFERENCE_H
#define RA_UI_IMAGEREFERENCE_H
#pragma once

#include <string>

namespace ra {
namespace ui {

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
    ~ImageReference() noexcept;

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
    void Release() noexcept;

    /// <summary>
    /// Gets custom data associated with a loaded image (as defined by the IImageRepository implementation).
    /// </summary>
    unsigned long long GetData() const noexcept { return m_nData; }

    /// <summary>
    /// Sets custom data associated with a loaded image (as defined by the IImageRepository implementation).
    /// </summary>
    /// <remarks>Allows setting the data on a const ImageReference so the IImageRepository can lazy-load the data.</remarks>
    void SetData(unsigned long long nData) const noexcept { m_nData = nData; }

private:
    ImageType m_nType{};
    std::string m_sName;

    mutable unsigned long long m_nData{};
};

} // namespace ui
} // namespace ra

#endif /* !RA_UI_IMAGEREFERENCE_H */
