#include "RA_Implementation.h"
#include "../../RA_Integration/RA_Interface.h"

//	Include any emulator-side headers, externs or functions here
//#include "../Common.h"

// returns -1 if not found
//int GetMenuItemIndex(HMENU hMenu, const char* ItemName)
//{
//	int index = 0;
//	char buf[256];
//
//	while(index < GetMenuItemCount(hMenu))
//	{
//		if(GetMenuString(hMenu, index, buf, sizeof(buf)-1, MF_BYPOSITION))
//		{
//			if(!strcmp(ItemName, buf))
//				return index;
//		}
//		index++;
//	}
//	return -1;
//}


//	Return whether a game has been loaded. Should return FALSE if
//	 no ROM is loaded, or a ROM has been unloaded.
bool GameIsActive()
{
	return true;
}

//	Perform whatever action is required to unpause emulation.
void CauseUnpause()
{
	//FCEUI_SetEmulationPaused( false );
}

//	Perform whatever function in the case of needing to rebuild the menu.
void RebuildMenu()
{
	//// get main menu handle
	//HMENU hMainMenu = GetMenu(hAppWnd);
	//if(!hMainMenu) return;

	//// get file menu index
	//int index = GetMenuItemIndex(hMainMenu, "&RetroAchievements");
	//if(index >= 0)
	//	DeleteMenu( hMainMenu, index, MF_BYPOSITION );

	////	##RA embed RA
	//AppendMenu( hMainMenu, MF_POPUP|MF_STRING, (UINT_PTR)RA_CreatePopupMenu(), TEXT("&RetroAchievements") );

	//DrawMenuBar( hAppWnd );
}

//	sNameOut points to a 64 character buffer.
//	sNameOut should have copied into it the estimated game title 
//	 for the ROM, if one can be inferred from the ROM.
void GetEstimatedGameTitle( char* sNameOut )
{
	//if( emu && emu->get_NES_ROM() )
	//	strcpy_s( sNameOut, 49, emu->get_NES_ROM()->GetRomName() );
}

void ResetEmulation()
{
	//FCEUI_ResetNES();
}

void LoadROM( char* sFullPath )
{
	//Pre_Load_Rom( HWnd, sFullPath );
	
}

//	Installs these shared functions into the DLL
void RA_InitShared()
{
	RA_InstallSharedFunctions( &GameIsActive, &CauseUnpause, &RebuildMenu, &GetEstimatedGameTitle, &ResetEmulation, &LoadROM );
}

