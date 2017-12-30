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

#ifndef SAFE_RELEASE
#define SAFE_RELEASE( p ) { if ( (p) != nullptr ) { (p)->Release(); (p) = nullptr; } }
#endif

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
	//hr = IWICBitmapSource_GetPixelFormat( pToRenderBitmapSource, &pixelFormat );
	hr = pToRenderBitmapSource->GetPixelFormat( &pixelFormat );

	if( SUCCEEDED( hr ) )
	{
		//hr = IWICBitmapSource_GetSize( pToRenderBitmapSource, &nWidth, &nHeight );
		hr = pToRenderBitmapSource->GetSize( &nWidth, &nHeight );
	}

	// Create a DIB section based on Bitmap Info
	// BITMAPINFO Struct must first be setup before a DIB can be created.
	// Note that the height is negative for top-down bitmaps
	if( SUCCEEDED( hr ) )
	{
		ZeroMemory( &bminfo, sizeof( bminfo ) );
		bminfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
		bminfo.bmiHeader.biWidth = nWidth;
		bminfo.bmiHeader.biHeight = -(LONG)nHeight;
		bminfo.bmiHeader.biPlanes = 1;
		bminfo.bmiHeader.biBitCount = 32;
		bminfo.bmiHeader.biCompression = BI_RGB;

		hWindow = GetActiveWindow();
		while( GetParent( hWindow ) != NULL )
			hWindow = GetParent( hWindow );

		// Get a DC for the full screen
		hdcScreen = GetDC( hWindow );
		hr = hdcScreen ? S_OK : E_FAIL;

		// Release the previously allocated bitmap 
		if( SUCCEEDED( hr ) )
		{
			if( hBitmapInOut )
			{
				DeleteObject( hBitmapInOut );
			}

			//	TBD: check this. As a handle this should just be as-is, right?
			hBitmapInOut = CreateDIBSection( hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0 );

			ReleaseDC( NULL, hdcScreen );

			hr = hBitmapInOut ? S_OK : E_FAIL;
		}
	}

	if( SUCCEEDED( hr ) )
	{
		// Size of a scan line represented in bytes: 4 bytes each pixel
		hr = UIntMult( nWidth, sizeof( ARGB ), &cbStride );
	}

	if( SUCCEEDED( hr ) )
	{
		// Size of the image, represented in bytes
		hr = UIntMult( cbStride, nHeight, &cbImage );
	}

	// Extract the image into the HBITMAP    
	if( SUCCEEDED( hr ) )
	{
		hr = pToRenderBitmapSource->CopyPixels(
			//hr = IWICBitmapSource_CopyPixels( pToRenderBitmapSource,
			NULL,
			cbStride,
			cbImage,
			(BYTE*)pvImageBits );
	}

	// Image Extraction failed, clear allocated memory
	if( FAILED( hr ) )
	{
		DeleteObject( hBitmapInOut );
		hBitmapInOut = NULL;
	}

	return hr;
}

BOOL InitializeUserImageFactory( HINSTANCE hInst )
{
	HRESULT hr = S_OK;;

	g_UserImageFactoryInst.m_pIWICFactory = NULL;
	g_UserImageFactoryInst.m_pOriginalBitmapSource = NULL;

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

	return( hr == S_OK );
}

HRESULT ConvertBitmapSource( RECT rcDest, IWICBitmapSource*& pToRenderBitmapSource )
{
	HRESULT hr = S_OK;
	IWICBitmapScaler* pScaler = nullptr;
	WICPixelFormatGUID pxformat;
	IWICFormatConverter* pConverter = nullptr;

	pToRenderBitmapSource = nullptr;

	// Get the client Rect
	//RECT rcClient = rcDest;
	//hr = GetClientRect(hWnd, &rcClient) ? S_OK: E_FAIL;

	if( SUCCEEDED( hr ) )
	{
		// Create a BitmapScaler
		hr = g_UserImageFactoryInst.m_pIWICFactory->CreateBitmapScaler( &pScaler );
		//hr = IWICImagingFactory_CreateBitmapScaler( g_UserImageFactoryInst.m_pIWICFactory, &pScaler );

		// Initialize the bitmap scaler from the original bitmap map bits
		if( SUCCEEDED( hr ) )
		{
			pScaler->Initialize( g_UserImageFactoryInst.m_pOriginalBitmapSource,
								 rcDest.right - rcDest.left,
								 rcDest.bottom - rcDest.top,
								 WICBitmapInterpolationModeFant );
		}

		//hr = IWICBitmapScaler_GetPixelFormat( pScaler, &pxformat );
		hr = pScaler->GetPixelFormat( &pxformat );

		// Format convert the bitmap into 32bppBGR, a convenient 
		// pixel format for GDI rendering 
		if( SUCCEEDED( hr ) )
		{
			//hr = IWICImagingFactory_CreateFormatConverter( g_UserImageFactoryInst.m_pIWICFactory, &pConverter );
			hr = g_UserImageFactoryInst.m_pIWICFactory->CreateFormatConverter( &pConverter );

			// Format convert to 32bppBGR
			if( SUCCEEDED( hr ) )
			{
				hr = pConverter->Initialize( static_cast<IWICBitmapSource*>( pScaler ),	// Input bitmap to convert
											 GUID_WICPixelFormat32bppBGR,				//	&GUID_WICPixelFormat32bppBGR,
											 WICBitmapDitherTypeNone,					// Specified dither patterm
											 NULL,										// Specify a particular palette 
											 0.f,										// Alpha threshold
											 WICBitmapPaletteTypeCustom );				// Palette translation type

				// Store the converted bitmap as ppToRenderBitmapSource 
				if( SUCCEEDED( hr ) )
					pConverter->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void**>( &pToRenderBitmapSource ) );
			}
			SAFE_RELEASE( pConverter );
		}

		SAFE_RELEASE( pScaler );
	}

	return hr;
}

HBITMAP LoadOrFetchBadge( const std::string& sBadgeURI, const RASize& sz )
{
	SetCurrentDirectory( NativeStr( g_sHomeDir ).c_str() );

	if( !_FileExists( RA_DIR_BADGE + sBadgeURI + ".png" ) )
	{
		PostArgs args;
		args[ 'b' ] = sBadgeURI;
		//	Ensure it's not in the queue to be processed or has been processed, waiting for handling:
		if( !RAWeb::HTTPRequestExists( RequestBadge, sBadgeURI ) && !RAWeb::HTTPResponseExists( RequestBadge, sBadgeURI ) )
			RAWeb::CreateThreadedHTTPRequest( RequestBadge, args, sBadgeURI );

		return NULL;
	}
	else
	{
		return LoadLocalPNG( RA_DIR_BADGE + sBadgeURI + ".png", sz );
	}
}

HBITMAP LoadOrFetchUserPic( const std::string& sUserName, const RASize& sz )
{
	SetCurrentDirectory(NativeStr( g_sHomeDir ).c_str() );

	if( !_FileExists( RA_DIR_USERPIC + sUserName + ".png" ) )
	{
		PostArgs args;
		args['u'] = sUserName;
		//	Ensure it's not in the queue to be processed or has been processed, waiting for handling:
		if( !RAWeb::HTTPRequestExists( RequestUserPic, sUserName ) && !RAWeb::HTTPResponseExists( RequestUserPic, sUserName ) )
			RAWeb::CreateThreadedHTTPRequest( RequestUserPic, args, sUserName );

		return NULL;
	}
	else
	{
		return LoadLocalPNG( RA_DIR_USERPIC + sUserName + ".png", sz );
	}
}

HBITMAP LoadLocalPNG( const std::string& sPath, const RASize& sz )
{
	SetCurrentDirectory(NativeStr( g_sHomeDir ).c_str() );

	ASSERT( _FileExists( sPath ) );
	if( !_FileExists( sPath ) )
	{
		RA_LOG( "File could not be found: %s\n", sPath.c_str() );
		return nullptr;
	}

	HBITMAP hRetVal = nullptr;
	// Step 2: Decode the source image to IWICBitmapSource
	IWICBitmapDecoder* pDecoder = nullptr;
	HRESULT hr = g_UserImageFactoryInst.m_pIWICFactory->CreateDecoderFromFilename( Widen( sPath ).c_str(),			// Image to be decoded
																				   nullptr,							// Do not prefer a particular vendor
																				   GENERIC_READ,					// Desired read access to the file
																				   WICDecodeMetadataCacheOnDemand,	// Cache metadata when needed
																				   &pDecoder );						// Pointer to the decoder
	
	// Retrieve the first frame of the image from the decoder
	IWICBitmapFrameDecode* pFrame = nullptr;
	if( SUCCEEDED( hr ) )
		hr = pDecoder->GetFrame( 0, &pFrame );

	// Retrieve IWICBitmapSource from the frame
	if( SUCCEEDED( hr ) )
	{
		SAFE_RELEASE( g_UserImageFactoryInst.m_pOriginalBitmapSource );	//##SD ???
		pFrame->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void**>( &g_UserImageFactoryInst.m_pOriginalBitmapSource ) );
	}

	// Step 3: Scale the original IWICBitmapSource to the client rect size
	// and convert the pixel format
	IWICBitmapSource* pToRenderBitmapSource = nullptr;
	if( SUCCEEDED( hr ) )
		hr = ConvertBitmapSource( { 0, 0, sz.Width(), sz.Height() }, pToRenderBitmapSource );

	// Step 4: Create a DIB from the converted IWICBitmapSource
	if( SUCCEEDED( hr ) )
		hr = UserImageFactory_CreateDIBSectionFromBitmapSource( pToRenderBitmapSource, hRetVal );

	SAFE_RELEASE( pToRenderBitmapSource );
	SAFE_RELEASE( pDecoder );
	SAFE_RELEASE( pFrame );
	SAFE_RELEASE( g_UserImageFactoryInst.m_pOriginalBitmapSource );
	
	return hRetVal;
}

//////////////////////////////////////////////////////////////////////////

void DrawImage( HDC hDC, HBITMAP hBitmap, int nX, int nY, int nW, int nH )
{
	ASSERT( hBitmap != nullptr );
	HDC hdcMem = CreateCompatibleDC( hDC );
	if( hdcMem )
	{
		//	Select new bitmap and backup old bitmap
		HBITMAP hbmOld = SelectBitmap( hdcMem, hBitmap );

		//	Fetch and blit bitmap
		BITMAP bm;
		if( GetObject( hBitmap, sizeof( bm ), &bm ) == sizeof( bm ) )
			BitBlt( hDC, nX, nY, nW, nH, hdcMem, 0, 0, SRCCOPY );	//	Blit!

		//	Restore old bitmap
		SelectBitmap( hdcMem, hbmOld );

		//	Discard MemDC
		DeleteDC( hdcMem );
	}
}

void DrawImageStretched( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest )
{
	ASSERT( hBitmap != nullptr );
	HDC hdcMem = CreateCompatibleDC( hDC );
	if( hdcMem )
	{
		//	Select new bitmap and backup old bitmap
		HBITMAP hbmOld = SelectBitmap( hdcMem, hBitmap );

		//	Fetch and blit bitmap
		BITMAP bm;
		if( GetObject( hBitmap, sizeof( bm ), &bm ) == sizeof( bm ) )
		{
			//	Blit!
			SetStretchBltMode( hDC, BLACKONWHITE );
			StretchBlt( hDC,
						rcDest.left, rcDest.top, ( rcDest.right - rcDest.left ), ( rcDest.bottom - rcDest.top ),
						hdcMem, 
						rcSource.left, rcSource.top, ( rcSource.right - rcSource.left ), ( rcSource.bottom - rcSource.top ),
						SRCCOPY );
		}

		//	Restore old bitmap
		SelectBitmap( hdcMem, hbmOld );

		//	Discard MemDC
		DeleteDC( hdcMem );
	}
}

void DrawImageTiled( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest )
{
	ASSERT( hBitmap != nullptr );
	HDC hdcMem = CreateCompatibleDC( hDC );
	if( hdcMem )
	{
		//	Select new bitmap and backup old bitmap
		HBITMAP hbmOld = SelectBitmap( hdcMem, hBitmap );

		//	Fetch and blit bitmap
		BITMAP bm;
		if( GetObject( hBitmap, sizeof( bm ), &bm ) == sizeof( bm ) )
		{
			//	Blit!
			//	Truncate the source rect if bigger than container
			if( rcSource.left < rcDest.left )
				rcSource.left += ( rcDest.left - rcSource.left );

			if( rcSource.top < rcDest.top )
				rcSource.top += ( rcDest.top - rcSource.top );

			int nSourceWidth = rcSource.right - rcSource.left;
			int nSourceHeight = rcSource.bottom - rcSource.top;

			int nTargetWidth = rcDest.right - rcDest.left;
			int nTargetHeight = rcDest.bottom - rcDest.top;

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

					if( ( nXOffs + nWidthToCopy > 0 ) && ( nYOffs + nHeightToCopy > 0 ) )
						BitBlt( hDC, nXOffs, nYOffs, nWidthToCopy, nHeightToCopy, hdcMem, 0, 0, SRCCOPY );
				}
			}
		}

		//	Restore old bitmap
		SelectBitmap( hdcMem, hbmOld );

		//	Discard MemDC
		DeleteDC( hdcMem );
	}
}