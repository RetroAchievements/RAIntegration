#ifndef RA_IMAGEFACTORY_H
#define RA_IMAGEFACTORY_H
#pragma once

//////////////////////////////////////////////////////////////////////////
//	Image Factory

_EXTERN_C
void DrawImage(_In_ HDC hDC, _In_ HBITMAP hBitmap, _In_ int nX, _In_ int nY, _In_ int nW, _In_ int nH);
void DrawImageTiled(_In_ HDC hDC, _In_ HBITMAP hBitmap, _Inout_ RECT& rcSource, _In_ const RECT& rcDest);
void DrawImageStretched(_In_ HDC hDC, _In_ HBITMAP hBitmap,
                        _In_ const RECT& rcSource, _In_ const RECT& rcDest);
_END_EXTERN_C

#endif // !RA_IMAGEFACTORY_H
