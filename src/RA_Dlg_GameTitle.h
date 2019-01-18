#ifndef RA_DLG_GAMETITLE_H
#define RA_DLG_GAMETITLE_H
#pragma once

#include "ra_fwd.h"

class Dlg_GameTitle
{
public:
    std::string CleanRomName(const std::string& sTryName);
    static void DoModalDialog(HINSTANCE hInst, HWND hParent, std::string& sMD5InOut, std::string& sEstimatedGameTitleInOut, unsigned int& nGameIDOut);

    INT_PTR GameTitleProc(HWND hDlg, UINT uMsg, WPARAM wParam, _UNUSED LPARAM);

private:
    unsigned int m_nReturnedGameID{};
    std::string m_sMD5;
    std::string m_sEstimatedGameTitle;

    std::map<std::string, unsigned int> m_aGameTitles;
};

extern Dlg_GameTitle g_GameTitleDialog;


#endif // !RA_DLG_GAMETITLE_H
