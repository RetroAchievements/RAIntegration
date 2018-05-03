#pragma once

#include <wincodec.h>
#include <WTypes.h>

#include "RA_Defs.h"
#include <atlbase.h>
//////////////////////////////////////////////////////////////////////////
//	Image Factory

struct IWICBitmapSource;
struct IWICImagingFactory;

struct UserImageFactoryVars
{
<<<<<<< HEAD
public:
	UserImageFactoryVars() :m_pOriginalBitmapSource( nullptr ), m_pIWICFactory( nullptr ) {}

public:
	IWICBitmapSource* m_pOriginalBitmapSource;
	IWICImagingFactory* m_pIWICFactory; 
=======
	CComPtr<IWICBitmapSource> m_pOriginalBitmapSource;
	CComPtr<IWICImagingFactory> m_pIWICFactory;
>>>>>>> com_smart_pointers
};

_Success_(return != 0)
BOOL InitializeUserImageFactory([[maybe_unused]] _In_ HINSTANCE hInst );

_Success_(return != nullptr)
HBITMAP LoadOrFetchBadge( _In_ const std::string& sBadgeURI, _In_ const RASize& nSZ = RA_BADGE_PX );

_Success_(return != nullptr)
HBITMAP LoadOrFetchUserPic(_In_ const std::string& sUser, _In_ const RASize& nSZ = RA_USERPIC_PX );

_Success_(return != nullptr)
HBITMAP LoadLocalPNG(_In_ const std::string& sPath, _In_ const RASize& nSZ );

extern UserImageFactoryVars g_UserImageFactoryInst;

extern void DrawImage( HDC hDC, HBITMAP hBitmap, int nX, int nY, int nW, int nH );
extern void DrawImageTiled( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest );
extern void DrawImageStretched( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest );
