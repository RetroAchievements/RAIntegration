#ifndef _IMAGEFACTORY_H_
#define _IMAGEFACTORY_H_

#include <wincodec.h>
#include <WTypes.h>


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
HBITMAP LoadLocalPNG( const char* sPath, unsigned int nWidth, unsigned int nHeight );

extern UserImageFactoryVars g_UserImageFactoryInst;

extern void DrawImage( HDC hDC, HBITMAP hBitmap, int nX, int nY, int nW, int nH );
extern void DrawImageTiled( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest );
extern void DrawImageStretched( HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest );

#endif //_IMAGEFACTORY_H_