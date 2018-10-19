#include "ImageRepository.h"

#include "RA_Core.h"
#include "RA_httpthread.h"

#include "services\Http.hh"
#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {

inline constexpr const char* DefaultBadge{ "00000" };
inline constexpr const char* DefaultUserPic{ "_User" };

ImageRepository g_ImageRepository;

static CComPtr<IWICImagingFactory> g_pIWICFactory;
static bool g_bImageRepositoryValid = false;

ImageReference::~ImageReference() noexcept
{
    Release();
}

void ImageReference::ChangeReference(ImageType nType, const std::string& sName)
{
    Release();

    switch (nType)
    {
        case ImageType::None:
            // nothing to pre-fetch
            break;

        case ImageType::Local:
            // no need to pre-fetch local files
            break;

        default:
            g_ImageRepository.FetchImage(nType, sName);
            break;
    }

    m_nType = nType;
    m_sName = sName;
}

HBITMAP ImageReference::GetHBitmap() const
{
    // already have the reference, return it
    if (m_hBitmap != nullptr)
        return m_hBitmap;

    // nothing referenced yet
    if (m_nType == ImageType::None)
        return nullptr;

    // get the reference (and increment the reference count)
    m_hBitmap = g_ImageRepository.GetImage(m_nType, m_sName, true);
    if (m_hBitmap != nullptr)
        return m_hBitmap;

    // reference not yet available, return the default image
    return g_ImageRepository.DefaultImage(m_nType);
}

void ImageReference::Release() noexcept
{
    if (m_hBitmap != nullptr && g_bImageRepositoryValid)
        g_ImageRepository.ReleaseReference(m_nType, m_sName);
    m_hBitmap = nullptr;
}

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
    for (HBitmapMap::iterator iter = m_mBadges.begin(); iter != m_mBadges.end(); ++iter)
        DeleteObject(iter->second.m_hBitmap);
    m_mBadges.clear();

    for (HBitmapMap::iterator iter = m_mUserPics.begin(); iter != m_mUserPics.end(); ++iter)
        DeleteObject(iter->second.m_hBitmap);
    m_mUserPics.clear();

    // prevent errors disposing ImageReferences after the repository is disposed.
    g_bImageRepositoryValid = false;
}

std::wstring ImageRepository::GetFilename(ImageType nType, const std::string& sName)
{
    std::wstring sFilename = g_sHomeDir;

    switch (nType)
    {
        case ImageType::Badge:
            sFilename += RA_DIR_BADGE + ra::Widen(sName) + L".png";
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
    if (_FileExists(sFilename))
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
        default:
            ASSERT(!"Unsupported image type");
            return;
    }
    sUrl += sName;
    sUrl += ".png";

    RA_LOG_INFO("Downloading %s", sUrl.c_str());

    Http::Request request(sUrl);
    request.DownloadAsync(sFilename, [this,sFilename,sUrl](const Http::Response& response)
    {
        if (response.StatusCode() == 200)
        {
            //RA_LOG_INFO("Wrote %zu bytes to %s", ServiceLocator::Get<IFileSystem>().GetFileSize(sFilename), ra::Narrow(sFilename).c_str());
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

HBITMAP ImageRepository::DefaultImage(ImageType nType)
{
    switch (nType)
    {
        case ImageType::Badge:
            return GetImage(ImageType::Badge, DefaultBadge, false);

        case ImageType::UserPic:
            return GetImage(ImageType::UserPic, DefaultUserPic, false);

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
                GUID_WICPixelFormat32bppBGR,				// &GUID_WICPixelFormat32bppBGR,
                WICBitmapDitherTypeNone,					// Specified dither patterm
                nullptr,									// Specify a particular palette 
                0.f,										// Alpha threshold
                WICBitmapPaletteTypeCustom);				// Palette translation type

                                                            // Store the converted bitmap as ppToRenderBitmapSource 
            if (SUCCEEDED(hr))
                pConverter->QueryInterface(IID_IWICBitmapSource, reinterpret_cast<void**>(&pToRenderBitmapSource));
        }

        pConverter.Release();
    }

    pScaler.Release();
    return hr;
}

static HRESULT CreateDIBFromBitmapSource(_In_ IWICBitmapSource *pToRenderBitmapSource, _Inout_ HBITMAP& hBitmapInOut)
{
    // Get BitmapSource format and size
    WICPixelFormatGUID pixelFormat;
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
    BITMAPINFOHEADER info_header{
        sizeof(BITMAPINFOHEADER),      // biSize
        static_cast<LONG>(nWidth),
        -static_cast<LONG>(nHeight),
        WORD{ 1 },                     // biPlanes
        WORD{ 32 },                    // biBitCount
        static_cast<DWORD>(BI_RGB)
    };
    BITMAPINFO bminfo{ info_header };
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
    HRESULT hr = g_pIWICFactory->CreateDecoderFromFilename(ra::Widen(sFilename).c_str(),			// Image to be decoded
        nullptr,						// Do not prefer a particular vendor
        GENERIC_READ,                   // Desired read access to the file
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
        hr = ConvertBitmapSource({ 0, 0, static_cast<LONG>(nWidth), static_cast<LONG>(nHeight) }, pOriginalBitmapSource, *&pToRenderBitmapSource);

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

ImageRepository::HBitmapMap* ImageRepository::GetBitmapMap(ImageType nType)
{
    switch (nType)
    {
        case ImageType::Badge:
            return &m_mBadges;

        case ImageType::UserPic:
            return &m_mUserPics;

        case ImageType::Local:
            return &m_mLocal;

        default:
            ASSERT(!"Unsupported image type");
            return nullptr;
    }
}

HBITMAP ImageRepository::GetImage(ImageType nType, const std::string& sName, bool bAddReference)
{
    HBitmapMap* mMap = GetBitmapMap(nType);
    if (mMap == nullptr)
        return nullptr;
    
    HBitmapMap::iterator iter = mMap->find(sName);
    if (iter != mMap->end())
    {
        if (bAddReference)
            iter->second.m_nReferences++;

        return iter->second.m_hBitmap;
    }

    if (sName.empty())
        return nullptr;

    std::wstring sFilename = GetFilename(nType, sName);
    if (!_FileExists(sFilename))
    {
        FetchImage(nType, sName);
        return nullptr;
    }

    size_t nSize = (nType == ImageType::Local) ? 0 : 64;
    HBITMAP hBitmap = LoadLocalPNG(sFilename, nSize, nSize);
    auto& item = (*mMap)[sName];
    item.m_hBitmap = hBitmap;
    item.m_nReferences = bAddReference ? 1 : 0;
    return hBitmap;
}

void ImageRepository::ReleaseReference(ImageType nType, const std::string& sName) noexcept
{
    HBitmapMap* mMap = GetBitmapMap(nType);
    if (mMap != nullptr)
    {
        HBitmapMap::iterator iter = mMap->find(sName);
        if (iter != mMap->end())
        {
            HBITMAP hBitmap = iter->second.m_hBitmap;
            if (--iter->second.m_nReferences == 0)
            {
                mMap->erase(iter);
                DeleteObject(hBitmap);
            }
        }
    }
}

} // namespace services
} // namespace ra
