#ifndef RA_IMAGEFACTORY_H
#define RA_IMAGEFACTORY_H
#pragma once

//////////////////////////////////////////////////////////////////////////
//	Image Factory

void DrawImage(_In_ HDC hDC, _In_ HBITMAP hBitmap, _In_ int nX, _In_ int nY, _In_ int nW, _In_ int nH) noexcept;
void DrawImageTiled(_In_ HDC hDC, _In_ HBITMAP hBitmap, _Inout_ RECT& rcSource, _In_ const RECT& rcDest) noexcept;
void DrawImageStretched(_In_ HDC hDC, _In_ HBITMAP hBitmap,
                        _In_ const RECT& rcSource, _In_ const RECT& rcDest) noexcept;

#endif // !RA_IMAGEFACTORY_H
