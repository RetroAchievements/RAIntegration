#pragma once

#include <wincodec.h>
#include <WTypes.h>

#include "RA_Defs.h"

//////////////////////////////////////////////////////////////////////////
//	Image Factory

struct IWICBitmapSource;
struct IWICImagingFactory;

class UserImageFactoryVars
{
public:
	UserImageFactoryVars() :m_pOriginalBitmapSource( NULL ), m_pIWICFactory( NULL ) {}

public:
	IWICBitmapSource* m_pOriginalBitmapSource;
	IWICImagingFactory* m_pIWICFactory; 
};


BOOL InitializeUserImageFactory( HINSTANCE hInst );
HBITMAP LoadOrFetchBadge( const std::string& sBadgeURI, const RASize& nSZ = RA_BADGE_PX );
HBITMAP LoadOrFetchUserPic( const std::string& sUser, const RASize& nSZ = RA_USERPIC_PX );
HBITMAP LoadLocalPNG( const std::string& sPath, const RASize& nSZ );

extern UserImageFactoryVars g_UserImageFactoryInst;

extern void DrawImage( HDC hDC, HBITMAP hBitmap, int nX, int nY, int nW, int nH );
extern void DrawImageTiled( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest );
extern void DrawImageStretched( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest );
