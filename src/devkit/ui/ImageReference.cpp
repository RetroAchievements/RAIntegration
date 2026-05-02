#include "ImageReference.hh"
#include "IImageRepository.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace ui {

ImageReference::~ImageReference() noexcept
{
    if (ra::services::ServiceLocator::Exists<IImageRepository>())
        Release();
}

GSL_SUPPRESS_F6
void ImageReference::Release() noexcept
{
    if (m_nType != ImageType::None)
    {
        auto& pRepository = ra::services::ServiceLocator::GetMutable<IImageRepository>();
        pRepository.ReleaseReference(*this);
    }
}

} // namespace ui
} // namespace ra
