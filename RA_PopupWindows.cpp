#include "RA_PopupWindows.h"

PopupWindows g_PopupWindows;


//	Stubs for non-class based, indirect calling of these functions.
API int _RA_UpdatePopups( ControllerInput* input, float fDTime, bool Full_Screen, bool Paused )
{
	g_PopupWindows.Update( input, fDTime, Full_Screen, Paused );
	return 0;
}

API int _RA_RenderPopups( HDC hDC, RECT* rcSize )
{
	g_PopupWindows.Render( hDC, rcSize );
	return 0;
}