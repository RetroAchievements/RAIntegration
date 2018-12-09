#ifndef RA_UI_OVERLAY_MANAGER
#define RA_UI_OVERLAY_MANAGER

#include "PopupMessageViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlayManager {
public:    
    /// <summary>
    /// Updates the overlay.
    /// </summary>
    /// <param name="fElapsed">The amount of seconds that have passed.</param>
    void Update(double fElapsed);

    /// <summary>
    /// Renders the overlay.
    /// </summary>
    void Render(ra::ui::drawing::ISurface& pSurface) const;
    
    /// <summary>
    /// Queues a popup message.
    /// </summary>
    void QueueMessage(PopupMessageViewModel&& pMessage);

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    void QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        QueueMessage(std::move(vmMessage));
    }

    /// <summary>
    /// Queues a popup message with an image.
    /// </summary>
    void QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        vmMessage.SetImage(nImageType, sImageName);
        QueueMessage(std::move(vmMessage));
    }

    /// <summary>
    /// Clears all popups.
    /// </summary>
    void ClearPopups();

private:
    void UpdateActiveMessage(double fElapsed);

    std::queue<PopupMessageViewModel> m_vPopupMessages;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_MANAGER
