#ifndef RA_IMAGEFACTORY_H
#define RA_IMAGEFACTORY_H
#pragma once

//////////////////////////////////////////////////////////////////////////
//	Image Factory

extern void DrawImage(HDC hDC, HBITMAP hBitmap, int nX, int nY, int nW, int nH);
extern void DrawImageTiled(HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest);
extern void DrawImageStretched(HDC hDC, HBITMAP hBitmap, RECT& rcSource, RECT& rcDest);


#endif // !RA_IMAGEFACTORY_H
