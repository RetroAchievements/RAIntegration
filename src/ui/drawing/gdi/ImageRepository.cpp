#include "ImageRepository.hh"

#include "RA_Core.h"
#include "RA_httpthread.h"

#include "services\Http.hh"
#include "services\IFileSystem.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

inline constexpr const char* DefaultBadge{ "00000" };
inline constexpr const char* DefaultUserPic{ "_User" };

static CComPtr<IWICImagingFactory> g_pIWICFactory;

bool ImageRepository::Initialize()
{
    // pre-fetch the default images
    FetchImage(ImageType::Badge, DefaultBadge);
    FetchImage(ImageType::UserPic, DefaultUserPic);

    // Create WIC factory - this lives for the life of the application.
    // NOTE: RPC_E_CHANGED_MODE error indicates something has already initialized COM
    // with different settings, assume they're acceptable settings and proceed anyway
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, SIZE_T{});
    auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE)
    {
        hr = CoCreateInstance(

#if defined (__cplusplus)
            CLSID_WICImagingFactory,
#else
            &CLSID_WICImagingFactory,
#endif
            nullptr,
            CLSCTX_INPROC_SERVER,

#if defined (__cplusplus)
            IID_IWICImagingFactory,
#else
            &IID_IWICImagingFactory,
#endif

            reinterpret_cast<LPVOID*>(&g_pIWICFactory)
        );
    }

    return SUCCEEDED(hr);
}

ImageRepository::~ImageRepository() noexcept
{
    // clean up anything that's still referenced
    for (auto& badge : m_mBadges)
        DeleteBitmap(badge.second.m_hBitmap);
    m_mBadges.clear();

    for (auto& userPic : m_mUserPics)
        DeleteBitmap(userPic.second.m_hBitmap);
    m_mUserPics.clear();

    g_pIWICFactory.Release();
}

std::wstring ImageRepository::GetFilename(ImageType nType, const std::string& sName)
{
    std::wstring sFilename = g_sHomeDir;

    switch (nType)
    {
        case ImageType::Badge:
            sFilename += RA_DIR_BADGE + ra::Widen(sName) + L".png";
            break;
        case ImageType::Icon:
            sFilename += RA_DIR_BADGE + std::wstring(L"i") + ra::Widen(sName) + L".png";
            break;
        case ImageType::UserPic:
            sFilename += RA_DIR_USERPIC + ra::Widen(sName) + L".png";
            break;
        case ImageType::Local:
            sFilename += ra::Widen(sName);
            break;
        default:
            ASSERT(!"Unsupported image type");
            break;
    }

    return sFilename;
}

void ImageRepository::FetchImage(ImageType nType, const std::string& sName)
{
    if (sName.empty())
        return;

    std::wstring sFilename = GetFilename(nType, sName);
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    if (pFileSystem.GetFileSize(sFilename) > 0)
        return;

    // check to see if it's already queued
    {
        std::lock_guard<std::mutex> lock(m_oMutex);
        if (m_vRequestedImages.find(sFilename) != m_vRequestedImages.end())
            return;

        m_vRequestedImages.emplace(sFilename);
    }

    // fetch it
    std::string sUrl;
    switch (nType)
    {
        case ImageType::Badge:
            sUrl = "http://i.retroachievements.org/Badge/";
            break;
        case ImageType::UserPic:
            sUrl = _RA_HostName();
            sUrl += "/UserPic/";
            break;
        case ImageType::Icon:
            sUrl = "http://i.retroachievements.org/Images/";
            break;
        default:
            ASSERT(!"Unsupported image type");
            return;
    }
    sUrl += sName;
    sUrl += ".png";

    RA_LOG_INFO("Downloading %s", sUrl.c_str());

    ra::services::Http::Request request(sUrl);
    request.DownloadAsync(sFilename, [this,sFilename,sUrl](const ra::services::Http::Response& response)
    {
        if (response.StatusCode() == ra::services::Http::StatusCode::OK)
        {
            auto nFileSize = ra::services::ServiceLocator::Get<ra::services::IFileSystem>().GetFileSize(sFilename);
            RA_LOG_INFO("Wrote %l bytes to %s", nFileSize, ra::Narrow(sFilename).c_str());
        }
        else
        {
            RA_LOG_WARN("Error %u fetching %s", response.StatusCode(), sUrl.c_str());
        }

        {
            std::lock_guard<std::mutex> lock(m_oMutex);
            m_vRequestedImages.erase(sFilename);
        }
    });
}

HBITMAP ImageRepository::GetDefaultImage(ImageType nType)
{
    switch (nType)
    {
        case ImageType::Badge:
        case ImageType::Icon:
            return GetImage(ImageType::Badge, DefaultBadge);

        case ImageType::UserPic:
            return GetImage(ImageType::UserPic, DefaultUserPic);

        default:
            ASSERT(!"Unsupported image type");
            return nullptr;
    }
}

static HRESULT ConvertBitmapSource(_In_ RECT rcDest, _In_ IWICBitmapSource* pOriginalBitmapSource, _Inout_ IWICBitmapSource*& pToRenderBitmapSource)
{
    pToRenderBitmapSource = nullptr;

    // Create a BitmapScaler
    CComPtr<IWICBitmapScaler> pScaler;
    auto hr = g_pIWICFactory->CreateBitmapScaler(&pScaler);

    // Initialize the bitmap scaler from the original bitmap map bits
    if (SUCCEEDED(hr))
    {
        pScaler->Initialize(pOriginalBitmapSource,
            rcDest.right - rcDest.left,
            rcDest.bottom - rcDest.top,
            WICBitmapInterpolationModeFant);
    }

    // Convert the bitmap into 32bppBGR, a convenient pixel format for GDI rendering 
    if (SUCCEEDED(hr))
    {
        CComPtr<IWICFormatConverter> pConverter;
        hr = g_pIWICFactory->CreateFormatConverter(&pConverter);

        // Format convert to 32bppBGR
        if (SUCCEEDED(hr))
        {
            hr = pConverter->Initialize(static_cast<IWICBitmapSource*>(pScaler), // Input bitmap to convert
                GUID_WICPixelFormat32bppBGR,                // &GUID_WICPixelFormat32bppBGR,
                WICBitmapDitherTypeNone,                    // Specified dither pattern
                nullptr,                                    // Specify a particular palette 
                0.f,                                        // Alpha threshold
                WICBitmapPaletteTypeCustom);                // Palette translation type

            // Store the converted bitmap as ppToRenderBitmapSource 
            if (SUCCEEDED(hr))
                GSL_SUPPRESS_TYPE1
                pConverter->QueryInterface(IID_IWICBitmapSource, reinterpret_cast<void**>(&pToRenderBitmapSource));
        }

        pConverter.Release();
    }

    pScaler.Release();
    return hr;
}

static HRESULT CreateDIBFromBitmapSource(_In_ IWICBitmapSource* pToRenderBitmapSource,
                                         _Inout_ HBITMAP& hBitmapInOut)
{
    Expects(pToRenderBitmapSource != nullptr);
    // Get BitmapSource format and size
    WICPixelFormatGUID pixelFormat{};
    UINT nWidth = 0U;
    UINT nHeight = 0U;

    auto hr = pToRenderBitmapSource->GetPixelFormat(&pixelFormat);
    if (SUCCEEDED(hr))
        hr = pToRenderBitmapSource->GetSize(&nWidth, &nHeight);

    if (FAILED(hr))
        return hr;

    // Create a DIB section based on Bitmap Info
    // BITMAPINFO Struct must first be setup before a DIB can be created.
    // Note that the height is negative for top-down bitmaps
    BITMAPINFOHEADER info_header{sizeof(BITMAPINFOHEADER), // biSize
                                 to_signed(nWidth),
                                 -to_signed(nHeight),
                                 WORD{1},  // biPlanes
                                 WORD{32}, // biBitCount
                                 DWORD{BI_RGB}};
    BITMAPINFO bminfo{info_header};
    void *pvImageBits = nullptr;

    auto hWindow = GetActiveWindow();
    while (GetParent(hWindow) != nullptr)
        hWindow = GetParent(hWindow);

    // Get a DC for the full screen
    auto hdcScreen = GetDC(hWindow);
    if (hdcScreen)
    {
        // Release the previously allocated bitmap 
        if (hBitmapInOut)
            DeleteBitmap(hBitmapInOut);

        //	TBD: check this. As a handle this should just be as-is, right?
        hBitmapInOut = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, nullptr, DWORD{});

        ReleaseDC(nullptr, hdcScreen);

        hr = hBitmapInOut ? S_OK : E_FAIL;
    }

    // Size of a scan line represented in bytes: 4 bytes each pixel
    UINT cbStride = 0U;
    if (SUCCEEDED(hr))
        hr = UIntMult(nWidth, sizeof(ra::ARGB), &cbStride);

    // Size of the image, represented in bytes
    UINT cbImage = 0U;
    if (SUCCEEDED(hr))
        hr = UIntMult(cbStride, nHeight, &cbImage);

    // Extract the image into the HBITMAP    
    if (SUCCEEDED(hr) && pvImageBits)
    {
        hr = pToRenderBitmapSource->CopyPixels(
            //hr = IWICBitmapSource_CopyPixels( pToRenderBitmapSource,
            nullptr,
            cbStride,
            cbImage,
            static_cast<BYTE*>(pvImageBits));
    }

    // Image Extraction failed, clear allocated memory
    if (FAILED(hr) && hBitmapInOut)
    {
        DeleteBitmap(hBitmapInOut);
        hBitmapInOut = nullptr;
    }

    return hr;
}

HBITMAP ImageRepository::LoadLocalPNG(const std::wstring& sFilename, size_t nWidth, size_t nHeight)
{
    if (g_pIWICFactory == nullptr)
        return nullptr;

    // Decode the source image to IWICBitmapSource
    CComPtr<IWICBitmapDecoder> pDecoder;
    HRESULT hr = g_pIWICFactory->CreateDecoderFromFilename(ra::Widen(sFilename).c_str(), // Image to be decoded
                                                           nullptr,      // Do not prefer a particular vendor
                                                           GENERIC_READ, // Desired read access to the file
                                                           WICDecodeMetadataCacheOnDemand, // Cache metadata when needed
                                                           &pDecoder);                     // Pointer to the decoder

    // Retrieve the first frame of the image from the decoder
    CComPtr<IWICBitmapFrameDecode> pFrame;
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrame(0, &pFrame);

    // Retrieve IWICBitmapSource from the frame
    CComPtr<IWICBitmapSource> pOriginalBitmapSource;
    if (SUCCEEDED(hr))
    {
        GSL_SUPPRESS_TYPE1
        hr = pFrame->QueryInterface(IID_IWICBitmapSource, reinterpret_cast<void**>(&pOriginalBitmapSource));
        if (SUCCEEDED(hr))
        {
            if (nWidth == 0 || nHeight == 0)
                hr = pOriginalBitmapSource->GetSize(&nWidth, &nHeight);
        }
    }

    // Scale the original IWICBitmapSource to the client rect size and convert the pixel format
    CComPtr<IWICBitmapSource> pToRenderBitmapSource;
    if (SUCCEEDED(hr))
        hr = ConvertBitmapSource({0, 0, to_signed(nWidth), to_signed(nHeight)}, pOriginalBitmapSource,
                                 *&pToRenderBitmapSource);

    // Create a DIB from the converted IWICBitmapSource
    HBITMAP hBitmap = nullptr;
    if (SUCCEEDED(hr))
        hr = CreateDIBFromBitmapSource(pToRenderBitmapSource, hBitmap);

    pToRenderBitmapSource.Release();
    pOriginalBitmapSource.Release();
    pFrame.Release();
    pDecoder.Release();

    return hBitmap;
}

ImageRepository::HBitmapMap* ImageRepository::GetBitmapMap(ImageType nType) noexcept
{
    switch (nType)
    {
        case ImageType::Badge:
            return &m_mBadges;

        case ImageType::UserPic:
            return &m_mUserPics;

        case ImageType::Local:
            return &m_mLocal;

        case ImageType::Icon:
            return &m_mIcons;

        default:
            ASSERT(!"Unsupported image type");
            return nullptr;
    }
}

HBITMAP ImageRepository::GetImage(ImageType nType, const std::string& sName)
{
    if (sName.empty())
        return nullptr;

    HBitmapMap* mMap = GetBitmapMap(nType);
    if (mMap == nullptr)
        return nullptr;

    HBitmapMap::iterator iter = mMap->find(sName);
    if (iter != mMap->end())
        return iter->second.m_hBitmap;

    std::wstring sFilename = GetFilename(nType, sName);

    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    if (pFileSystem.GetFileSize(sFilename) <= 0)
    {
        FetchImage(nType, sName);
        return nullptr;
    }

    const size_t nSize = (nType == ImageType::Local) ? 0 : 64;
    HBITMAP hBitmap = LoadLocalPNG(sFilename, nSize, nSize);
    if (hBitmap != nullptr)
    {
        // bracket operator appears to be the only way to add an item to the map since
        // std::atomic deleted its move and copy constructors.
        auto& item = (*mMap)[sName];
        item.m_hBitmap = hBitmap;
    }

    return hBitmap;
}

HBITMAP ImageRepository::GetHBitmap(const ImageReference& pImage)
{
    HBITMAP hBitmap{};
    GSL_SUPPRESS_TYPE1 hBitmap = reinterpret_cast<HBITMAP>(pImage.m_nData);

    if (hBitmap == nullptr)
    {
        auto pImageRepository = dynamic_cast<ImageRepository*>(&ra::services::ServiceLocator::GetMutable<IImageRepository>());
        if (pImageRepository != nullptr)
        {
            hBitmap = pImageRepository->GetImage(pImage.Type(), pImage.Name());
            if (hBitmap == nullptr)
                return pImageRepository->GetDefaultImage(pImage.Type());

            GSL_SUPPRESS_TYPE1 pImage.m_nData = reinterpret_cast<unsigned long>(hBitmap);

            // ImageReference will release the reference
            pImageRepository->AddReference(pImage);
        }
    }

    return hBitmap;
}

void ImageRepository::AddReference(const ImageReference& pImage)
{
    if (pImage.Name().empty())
        return;

    HBitmapMap* mMap = GetBitmapMap(pImage.Type());
    if (mMap == nullptr)
        return;

    HBitmapMap::iterator iter = mMap->find(pImage.Name());
    ASSERT(iter != mMap->end()); // AddReference should only be called if an HBITMAP exists, which will be in the map
    if (iter != mMap->end())
        ++iter->second.m_nReferences;
}

void ImageRepository::ReleaseReference(ImageReference& pImage) noexcept
{
    // if data isn't set, we don't have a reference to release.
    if (pImage.m_nData == 0)
        return;

    HBitmapMap* mMap = GetBitmapMap(pImage.Type());
    if (mMap != nullptr)
    {
        HBitmapMap::iterator iter = mMap->find(pImage.Name());

        if(iter != mMap->end())
        {
            HBITMAP hBitmap = iter->second.m_hBitmap;

            if(--iter->second.m_nReferences == 0)
            {
                mMap->erase(iter);
                DeleteObject(hBitmap);
            }
        }
    }

    pImage.m_nData = {};
}

bool ImageRepository::HasReferencedImageChanged(ImageReference& pImage) const
{
    if (pImage.Type() == ra::ui::ImageType::None)
        return false;

    const auto hBitmapBefore = pImage.m_nData;
    GetHBitmap(pImage); // TBD: Is the return value supposed to be discarded?
    return (pImage.m_nData != hBitmapBefore);
}

} // namespace gdi
} // namespace drawing
} // namespace services
} // namespace ra
