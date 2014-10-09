
#define COBJMACROS
#define CINTERFACE
#include <wincodec.h>
#include <WTypes.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <stdio.h>

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_ImageFactory.h"
#include "RA_Achievement.h"
#include "RA_httpthread.h"

UserImageFactoryVars g_UserImageFactoryInst;


/******************************************************************
*  Creates a DIB Section from the converted IWICBitmapSource      *
******************************************************************/
HRESULT UserImageFactory_CreateDIBSectionFromBitmapSource( IWICBitmapSource *pToRenderBitmapSource, HBITMAP& hBitmapInOut )
{
	HRESULT hr = S_OK;

	UINT nWidth = 0;
	UINT nHeight = 0;

	void *pvImageBits = NULL;
	BITMAPINFO bminfo;
	HWND hWindow = NULL;
	HDC hdcScreen = NULL;

	WICPixelFormatGUID pixelFormat;
	
	UINT cbStride = 0;
	UINT cbImage = 0;


	// Check BitmapSource format
	hr = IWICBitmapSource_GetPixelFormat( pToRenderBitmapSource, &pixelFormat );
	//hr = pToRenderBitmapSource->GetPixelFormat(&pixelFormat);

	if (SUCCEEDED(hr))
	{
		//hr = (pixelFormat == GUID_WICPixelFormat32bppBGR) ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		hr = IWICBitmapSource_GetSize( pToRenderBitmapSource, &nWidth, &nHeight );
		//hr = pToRenderBitmapSource->GetSize( &nWidth, &nHeight );
	}

	// Create a DIB section based on Bitmap Info
	// BITMAPINFO Struct must first be setup before a DIB can be created.
	// Note that the height is negative for top-down bitmaps
	if (SUCCEEDED(hr))
	{
		ZeroMemory(&bminfo, sizeof(bminfo));
		bminfo.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
		bminfo.bmiHeader.biWidth        = nWidth;
		bminfo.bmiHeader.biHeight       = -(LONG)nHeight;
		bminfo.bmiHeader.biPlanes       = 1;
		bminfo.bmiHeader.biBitCount     = 32;
		bminfo.bmiHeader.biCompression  = BI_RGB;

		hWindow = GetActiveWindow();
		while( GetParent( hWindow ) != NULL )
			hWindow = GetParent( hWindow );

		// Get a DC for the full screen
		hdcScreen = GetDC( hWindow );
		hr = hdcScreen ? S_OK : E_FAIL;

		// Release the previously allocated bitmap 
		if (SUCCEEDED(hr))
		{
			if (hBitmapInOut)
			{
				DeleteObject(hBitmapInOut);
			}

			//	TBD: check this. As a handle this should just be as-is, right?
			hBitmapInOut = CreateDIBSection( hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0 );

			ReleaseDC(NULL, hdcScreen);

			hr = hBitmapInOut ? S_OK : E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		// Size of a scan line represented in bytes: 4 bytes each pixel
		hr = UIntMult(nWidth, sizeof(ARGB), &cbStride);
	}

	if (SUCCEEDED(hr))
	{
		// Size of the image, represented in bytes
		hr = UIntMult(cbStride, nHeight, &cbImage);
	}

	// Extract the image into the HBITMAP    
	if (SUCCEEDED(hr))
	{
		hr = IWICBitmapSource_CopyPixels( pToRenderBitmapSource,
			NULL,
			cbStride,
			cbImage, 
			(BYTE*)pvImageBits );
	}

	// Image Extraction failed, clear allocated memory
	if (FAILED(hr))
	{
		DeleteObject(hBitmapInOut);
		hBitmapInOut = NULL;
	}

	return hr;
}

BOOL InitializeUserImageFactory( HINSTANCE hInst )
{
	HRESULT hr = S_OK;;

	g_UserImageFactoryInst.m_pIWICFactory = NULL;
	g_UserImageFactoryInst.m_pOriginalBitmapSource = NULL;

	CoUninitialize();

	HeapSetInformation( NULL, HeapEnableTerminationOnCorruption, NULL, 0 );

	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );

	// Create WIC factory
	//#define IID_PPV_ARGS(ppType) ((LPVOID*)(ppType))
	hr = CoCreateInstance(

#if defined (__cplusplus)
		CLSID_WICImagingFactory,
#else
		&CLSID_WICImagingFactory,
#endif

		NULL,
		CLSCTX_INPROC_SERVER,

#if defined (__cplusplus)
		IID_IWICImagingFactory,
#else
		&IID_IWICImagingFactory,
#endif

		//IID_PPV_ARGS( &g_UserImageFactoryInst.m_pIWICFactory )
		(LPVOID*)( &g_UserImageFactoryInst.m_pIWICFactory )
		);

	return (hr==S_OK);
}

HRESULT ConvertBitmapSource( RECT rcDest, IWICBitmapSource** ppToRenderBitmapSource )
{
	HRESULT hr = S_OK;
	IWICBitmapScaler* pScaler = NULL;
	WICPixelFormatGUID pxformat;
	IWICFormatConverter* pConverter = NULL;

	*ppToRenderBitmapSource = NULL;

	// Get the client Rect
	//RECT rcClient = rcDest;
	//hr = GetClientRect(hWnd, &rcClient) ? S_OK: E_FAIL;

	if (SUCCEEDED(hr))
	{
		// Create a BitmapScaler

		hr = IWICImagingFactory_CreateBitmapScaler( g_UserImageFactoryInst.m_pIWICFactory, &pScaler );

		// Initialize the bitmap scaler from the original bitmap map bits
		if (SUCCEEDED(hr))
		{
			hr = IWICBitmapScaler_Initialize( pScaler, 
				g_UserImageFactoryInst.m_pOriginalBitmapSource, 
				rcDest.right - rcDest.left, 
				rcDest.bottom - rcDest.top, 
				WICBitmapInterpolationModeFant);
		}

		hr = IWICBitmapScaler_GetPixelFormat( pScaler, &pxformat );

		// Format convert the bitmap into 32bppBGR, a convenient 
		// pixel format for GDI rendering 
		if (SUCCEEDED(hr))
		{
			hr = IWICImagingFactory_CreateFormatConverter( g_UserImageFactoryInst.m_pIWICFactory, &pConverter );

			// Format convert to 32bppBGR
			if (SUCCEEDED(hr))
			{
				hr = IWICFormatConverter_Initialize( pConverter, 
					(IWICBitmapSource*)pScaler,      // Input bitmap to convert
					GUID_WICPixelFormat32bppBGR,	 //&GUID_WICPixelFormat32bppBGR,
					WICBitmapDitherTypeNone,         // Specified dither patterm
					NULL,                            // Specify a particular palette 
					0.f,                             // Alpha threshold
					WICBitmapPaletteTypeCustom       // Palette translation type
					);

				// Store the converted bitmap as ppToRenderBitmapSource 
				if (SUCCEEDED(hr))
				{

#if defined (__cplusplus)
					const IID& nIID = IID_IWICBitmapSource;
#else
					const IID* nIID = &IID_IWICBitmapSource;
#endif

					IWICFormatConverter_QueryInterface( 
						pConverter, 
						nIID,
						(LPVOID*)(ppToRenderBitmapSource)
						);
				}
			}

			IWICFormatConverter_Release( pConverter );
			pConverter = NULL;
			//SafeRelease(pConverter);
		}

		IWICBitmapScaler_Release( pScaler );
		pScaler = NULL;
		//SafeRelease(pScaler);
	}

	return hr;
}


HBITMAP LoadLocalPNG( const char* sPath, unsigned int nWidth, unsigned int nHeight )
{
	HRESULT hr = S_OK;
	WCHAR szFileName[MAX_PATH];
	char sReqBadgeName[1024];
	unsigned int i = 0;
	DWORD nBytesRead = 0;
	size_t requiredSize = 255;
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pFrame = NULL;
	IWICBitmapSource* pToRenderBitmapSource = NULL;
	RECT rc;
	HBITMAP hRetVal = NULL;

	SetCurrentDirectory( g_sHomeDir );

	FILE* fTemp = NULL;
	fopen_s( &fTemp, sPath, "r" );
	if( fTemp == NULL )
	{
		//	This is a badge I'm not aware of...
		//	Request this missing badge from the server
		
		const char* pCh = strchr( sPath, '\\' );
		while( pCh != NULL && strchr( pCh+1, '\\' ) != NULL )
			pCh = strchr( pCh+1, '\\' );
		
		sprintf_s( sReqBadgeName, 1024, "Badge/%s", pCh+1 );	//	NB: skip the ".\\Cache\\" part as input
		for( i = 0; i < strlen( sReqBadgeName ); ++i )		//	Replace backslashes with forwardslashes
			if( sReqBadgeName[i] == '\\' )
				sReqBadgeName[i] = '/';

		if( !HTTPRequestExists( sReqBadgeName ) )
			CreateHTTPRequestThread( sReqBadgeName, "", HTTPRequest_Get, 0, NULL );
		return hRetVal;
	}

	if( mbstowcs_s( &requiredSize, szFileName, requiredSize+1, sPath, requiredSize ) != (size_t)(-1) )
	// Step 1: Create the open dialog box and locate the image file
	//if (LocateImageFile(hWnd, szFileName, ARRAYSIZE(szFileName)))
	{
		// Step 2: Decode the source image to IWICBitmapSource

		// Create a decoder
		hr = IWICImagingFactory_CreateDecoderFromFilename( 
			g_UserImageFactoryInst.m_pIWICFactory, 
			szFileName,                      // Image to be decoded
			NULL,                            // Do not prefer a particular vendor
			GENERIC_READ,                    // Desired read access to the file
			WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
			&pDecoder                        // Pointer to the decoder
			);

		// Retrieve the first frame of the image from the decoder
		if (SUCCEEDED(hr))
		{
			hr = IWICBitmapDecoder_GetFrame( pDecoder, 0, &pFrame );
		}

// 		IWICColorContext* pContexts[5];
// 		ZeroMemory( pContexts, sizeof( IWICColorContext* ) * 5 );
// 		unsigned int nNumRead = 0;
// 		hr = pFrame->GetColorContexts( 5, pContexts, &nNumRead );
// 		DWORD nErr = GetLastError();

		// Retrieve IWICBitmapSource from the frame
		if (SUCCEEDED(hr))
		{
#if defined (__cplusplus)
			const IID& nIID = IID_IWICBitmapSource;
#else
			const IID* nIID = &IID_IWICBitmapSource;
#endif
			if( g_UserImageFactoryInst.m_pOriginalBitmapSource != NULL )
			{
				IWICBitmapSource_Release( g_UserImageFactoryInst.m_pOriginalBitmapSource );
				g_UserImageFactoryInst.m_pOriginalBitmapSource = NULL;
			}

			
			hr = IWICBitmapFrameDecode_QueryInterface( 
				pFrame,
				nIID, 
				(LPVOID*)(&g_UserImageFactoryInst.m_pOriginalBitmapSource)
				);
		
// 			WICPixelFormatGUID pxformat;
// 			hr = m_pOriginalBitmapSource->GetPixelFormat( &pxformat );
// 			pxformat;
		}

		// Step 3: Scale the original IWICBitmapSource to the client rect size
		// and convert the pixel format
		if (SUCCEEDED(hr))
		{
			SetRect( &rc, 0, 0, nWidth, nHeight );
			hr = ConvertBitmapSource( rc, &pToRenderBitmapSource );
		}

		// Step 4: Create a DIB from the converted IWICBitmapSource
		if (SUCCEEDED(hr))
		{
			hr = UserImageFactory_CreateDIBSectionFromBitmapSource( pToRenderBitmapSource, hRetVal );
		}

		if( pToRenderBitmapSource != NULL )
		{
			IWICBitmapSource_Release( pToRenderBitmapSource );
			pToRenderBitmapSource = NULL;
		}
		//SafeRelease(pToRenderBitmapSource);

		if( pDecoder != NULL )
		{
			IWICBitmapDecoder_Release( pDecoder );
			pDecoder = NULL;
		}
		//SafeRelease(pDecoder);

		if( pFrame != NULL )
		{
			IWICBitmapFrameDecode_Release( pFrame );
			pFrame = NULL;
		}
		//SafeRelease(pFrame);
	}

	if( g_UserImageFactoryInst.m_pOriginalBitmapSource != NULL )
	{
		IWICBitmapSource_Release( g_UserImageFactoryInst.m_pOriginalBitmapSource );
		g_UserImageFactoryInst.m_pOriginalBitmapSource = NULL;
	}
	//DeleteObject( m_pOriginalBitmapSource );
	//m_pOriginalBitmapSource = NULL;

	fclose( fTemp );
	return hRetVal;
}

//////////////////////////////////////////////////////////////////////////

void DrawImage( HDC hDC, HBITMAP hBitmap, int nX, int nY, int nW, int nH )
{
	//int nLockedResource = IDB_RA_IMG_LOCKED;
	HDC hdcMem = CreateCompatibleDC(hDC);

	assert( hBitmap != NULL );
	if( hdcMem )
	{
		//	Select new bitmap and backup old bitmap
		HBITMAP hbmOld = SelectBitmap( hdcMem, hBitmap );

		//	Fetch and blit bitmap
		BITMAP bm;
		if( GetObject( hBitmap, sizeof(bm), &bm) == sizeof(bm) )
		{
			//	Blit!
			BitBlt( hDC, nX, nY, nW, nH, hdcMem, 0, 0, SRCCOPY );
		}

		//	Restore old bitmap
		SelectBitmap( hdcMem, hbmOld );

		//	Discard MemDC
		DeleteDC(hdcMem);
	}
}

void DrawImageStretched( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest )
{
	//int nLockedResource = IDB_RA_IMG_LOCKED;
	HDC hdcMem = CreateCompatibleDC(hDC);

	assert( hBitmap != NULL );
	if( hdcMem )
	{
		//	Select new bitmap and backup old bitmap
		HBITMAP hbmOld = SelectBitmap( hdcMem, hBitmap );

		//	Fetch and blit bitmap
		BITMAP bm;
		if( GetObject( hBitmap, sizeof(bm), &bm) == sizeof(bm) )
		{
			//	Blit!
			SetStretchBltMode( hDC, BLACKONWHITE );
			StretchBlt( 
				hDC, rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top,
				hdcMem, rcSource.left, rcSource.top, rcSource.right-rcSource.left, rcSource.bottom-rcSource.top,
				SRCCOPY );
		}

		//	Restore old bitmap
		SelectBitmap( hdcMem, hbmOld );

		//	Discard MemDC
		DeleteDC(hdcMem);
	}
}

void DrawImageTiled( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest )
{
	//int nLockedResource = IDB_RA_IMG_LOCKED;
	HDC hdcMem = CreateCompatibleDC(hDC);

	assert( hBitmap != NULL );
	if( hdcMem )
	{
		//	Select new bitmap and backup old bitmap
		HBITMAP hbmOld = SelectBitmap( hdcMem, hBitmap );

		//	Fetch and blit bitmap
		BITMAP bm;
		if( GetObject( hBitmap, sizeof(bm), &bm) == sizeof(bm) )
		{
			//	Blit!

			//	Truncate the source rect if bigger than container
			if( rcSource.left < rcDest.left )
			{
				//OffsetRect( &rcSource, (rcDest.left-rcSource.left), 0 );
				rcSource.left += (rcDest.left-rcSource.left);
			}

			if( rcSource.top < rcDest.top )
			{
				//OffsetRect( &rcSource, 0, (rcDest.top-rcSource.top) );
				rcSource.top += (rcDest.top-rcSource.top);
			}

			int nSourceWidth = rcSource.right-rcSource.left;
			int nSourceHeight = rcSource.bottom-rcSource.top;

			int nTargetWidth = rcDest.right-rcDest.left;
			int nTargetHeight = rcDest.bottom-rcDest.top;

			int nWidthRemaining = nTargetWidth;
			for( int nXOffs = rcDest.left; nXOffs < rcDest.right; nXOffs += nSourceWidth )
			{
				for( int nYOffs = rcDest.top; nYOffs < rcDest.bottom; nYOffs += nSourceHeight )
				{
					int nDestLimitAtX = rcDest.right;
					int nDestLimitAtY = rcDest.bottom;

					int nWidthToCopy = nSourceWidth;
					int nHeightToCopy = nSourceHeight;

					//	If the full width of the source image goes outside the target area,
					//		Clip the rightmost/lower end of it by this much

					if( nXOffs+nSourceWidth > nDestLimitAtX )
						nWidthToCopy -= ( nXOffs+nSourceWidth - nDestLimitAtX );

					if( nYOffs+nSourceHeight >= nDestLimitAtY )
						nHeightToCopy -= ( nYOffs+nSourceHeight - nDestLimitAtY );

					if( (nXOffs + nWidthToCopy > 0) && (nYOffs + nHeightToCopy > 0) )
					{
						BitBlt( hDC, nXOffs, nYOffs, nWidthToCopy, nHeightToCopy, hdcMem, 0, 0, SRCCOPY );
					}
				}
			}
		}

		//	Restore old bitmap
		SelectBitmap( hdcMem, hbmOld );

		//	Discard MemDC
		DeleteDC(hdcMem);
	}
}