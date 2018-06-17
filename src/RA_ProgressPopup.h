#ifndef RA_PROGRESSPOPUP_H
#define RA_PROGRESSPOPUP_H

#include "RA_AchievementOverlay.h"

//	Graphic to display an progress towards an achievement


#define OVERLAY_MESSAGE_QUEUE_SIZE (5)

struct ControllerInput;
class ProgressPopup
{
public:
    ProgressPopup();

    void Update(ControllerInput& input, float fDelta, BOOL bFullScreen, BOOL bPaused);
    void Render(HDC hDC, RECT& rcDest);

    void AddMessage(const char* sTitle, const char* sMessage, int nMessageType = 0, HBITMAP hImage = nullptr);
    float GetYOffsetPct() const;

#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    BOOL IsActive() const { return m_sMessageTitleQueue[0][0] != '\0'; }
    const char* GetTitle() const { return m_sMessageTitleQueue[0]; }
    const char* GetDesc() const { return m_sMessageDescQueue[0]; }
    int GetMessageType() const { return m_nMessageType[0]; }
    HBITMAP GetImage() const { return m_hMessageImage[0]; }

    void SuppressNextDeltaUpdate() { m_bSuppressDeltaUpdate = true; }
#pragma warning(pop)

    void Clear();

private:
    void NextMessage();

private:
    float m_fTimer;
    char m_sMessageTitleQueue[OVERLAY_MESSAGE_QUEUE_SIZE][1024];
    char m_sMessageDescQueue[OVERLAY_MESSAGE_QUEUE_SIZE][1024];
    HBITMAP m_hMessageImage[OVERLAY_MESSAGE_QUEUE_SIZE];
    int m_nMessageType[OVERLAY_MESSAGE_QUEUE_SIZE];
    BOOL m_bSuppressDeltaUpdate;
};

//extern ProgressPopup g_ProgressPopup;

#endif // RA_PROGRESSPOPUP_H
