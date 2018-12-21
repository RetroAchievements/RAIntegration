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
    int QueueMessage(PopupMessageViewModel&& pMessage);

    /// <summary>
    /// Queues a popup message.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        return QueueMessage(std::move(vmMessage));
    }

    /// <summary>
    /// Queues a popup message with an image.
    /// </summary>
    int QueueMessage(const std::wstring& sTitle, const std::wstring& sDescription, ra::ui::ImageType nImageType, const std::string& sImageName)
    {
        PopupMessageViewModel vmMessage;
        vmMessage.SetTitle(sTitle);
        vmMessage.SetDescription(sDescription);
        vmMessage.SetImage(nImageType, sImageName);
        return QueueMessage(std::move(vmMessage));
    }
    
    /// <summary>
    /// Gets the message associated to the specified unique identifier.
    /// </summary>
    /// <param name="nId">The unique identifier of the popup.</param>
    /// <returns>Requested popup view model, <c>nullptr</c> if not found.</returns>
    PopupMessageViewModel* GetMessage(int nId)
    {
        for (auto& pMessage : m_vPopupMessages)
        {
            if (pMessage.GetPopupId() == nId)
                return &pMessage;
        }

        return nullptr;
    }

    /// <summary>
    /// Clears all popups.
    /// </summary>
    void ClearPopups();

private:
    void UpdateActiveMessage(double fElapsed);

    std::deque<PopupMessageViewModel> m_vPopupMessages;
    std::atomic<int> m_nPopupId{ 0 };
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_OVERLAY_MANAGER
