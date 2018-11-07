#include "RA_ImageFactory.h"

#include "RA_Defs.h"

//////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
void DrawImage(HDC hDC, HBITMAP hBitmap, int nX, int nY, int nW, int nH)
{
    ASSERT(hBitmap != nullptr);
    HDC hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem)
    {
        //	Select new bitmap and backup old bitmap
        HBITMAP hbmOld = SelectBitmap(hdcMem, hBitmap);

        //	Fetch and blit bitmap
        BITMAP bm;
        if (GetObject(hBitmap, sizeof(bm), &bm) == sizeof(bm))
            BitBlt(hDC, nX, nY, nW, nH, hdcMem, 0, 0, SRCCOPY);	//	Blit!

        //	Restore old bitmap
        SelectBitmap(hdcMem, hbmOld);

        //	Discard MemDC
        DeleteDC(hdcMem);
    }
}

_Use_decl_annotations_
void DrawImageStretched(HDC hDC, HBITMAP hBitmap, const RECT& rcSource, const RECT& rcDest)
{
    ASSERT(hBitmap != nullptr);
    HDC hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem)
    {
        //	Select new bitmap and backup old bitmap
        HBITMAP hbmOld = SelectBitmap(hdcMem, hBitmap);

        //	Fetch and blit bitmap
        BITMAP bm;
        if (GetObject(hBitmap, sizeof(bm), &bm) == sizeof(bm))
        {
            //	Blit!
            SetStretchBltMode(hDC, BLACKONWHITE);
            StretchBlt(hDC,
                rcDest.left, rcDest.top, (rcDest.right - rcDest.left), (rcDest.bottom - rcDest.top),
                hdcMem,
                rcSource.left, rcSource.top, (rcSource.right - rcSource.left), (rcSource.bottom - rcSource.top),
                SRCCOPY);
        }

        //	Restore old bitmap
        SelectBitmap(hdcMem, hbmOld);

        //	Discard MemDC
        DeleteDC(hdcMem);
    }
}

_Use_decl_annotations_
void DrawImageTiled(HDC hDC, HBITMAP hBitmap, RECT& rcSource, const RECT& rcDest)
{
    ASSERT(hBitmap != nullptr);
    HDC hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem)
    {
        //	Select new bitmap and backup old bitmap
        HBITMAP hbmOld = SelectBitmap(hdcMem, hBitmap);

        //	Fetch and blit bitmap
        BITMAP bm;
        if (GetObject(hBitmap, sizeof(bm), &bm) == sizeof(bm))
        {
            //	Blit!
            //	Truncate the source rect if bigger than container
            if (rcSource.left < rcDest.left)
                rcSource.left += (rcDest.left - rcSource.left);

            if (rcSource.top < rcDest.top)
                rcSource.top += (rcDest.top - rcSource.top);

            const int nSourceWidth = rcSource.right - rcSource.left;
            const int nSourceHeight = rcSource.bottom - rcSource.top;

            for (int nXOffs = rcDest.left; nXOffs < rcDest.right; nXOffs += nSourceWidth)
            {
                for (int nYOffs = rcDest.top; nYOffs < rcDest.bottom; nYOffs += nSourceHeight)
                {
                    const int nDestLimitAtX = rcDest.right;
                    const int nDestLimitAtY = rcDest.bottom;

                    int nWidthToCopy = nSourceWidth;
                    int nHeightToCopy = nSourceHeight;

                    //	If the full width of the source image goes outside the target area,
                    //		Clip the rightmost/lower end of it by this much

                    if (nXOffs + nSourceWidth > nDestLimitAtX)
                        nWidthToCopy -= (nXOffs + nSourceWidth - nDestLimitAtX);

                    if (nYOffs + nSourceHeight >= nDestLimitAtY)
                        nHeightToCopy -= (nYOffs + nSourceHeight - nDestLimitAtY);

                    if ((nXOffs + nWidthToCopy > 0) && (nYOffs + nHeightToCopy > 0))
                        BitBlt(hDC, nXOffs, nYOffs, nWidthToCopy, nHeightToCopy, hdcMem, 0, 0, SRCCOPY);
                }
            }
        }

        //	Restore old bitmap
        SelectBitmap(hdcMem, hbmOld);

        //	Discard MemDC
        DeleteDC(hdcMem);
    }
}

