#ifndef RA_UI_WIN32_IMAGEBINDING_H
#define RA_UI_WIN32_IMAGEBINDING_H
#pragma once

#include "ControlBinding.hh"

#include "ui\ImageReference.hh"
#include "ui\drawing\gdi\ImageRepository.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class ImageBinding : public ControlBinding, protected IImageRepository::NotifyTarget
{
public:
    explicit ImageBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);

        TryUpdateImage();
    }

    void BindImage(const StringModelProperty& pSourceProperty, ImageType nImageType)
    {
        m_pImageBoundProperty = &pSourceProperty;

        UpdateImageReference(nImageType);
    }

protected:
    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override
    {
        if (m_pImageBoundProperty && *m_pImageBoundProperty == args.Property)
            UpdateImageReference(m_pImageReference.Type());
    }

    void OnImageChanged(ImageType nType, const std::string& sName) 
    {
        if (nType == m_pImageReference.Type() && sName == m_pImageReference.Name())
        {
            auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
            pImageRepository.RemoveNotifyTarget(*this);

            UpdateImage();
        }
    }

private:
    void UpdateImageReference(ImageType nImageType)
    {
        auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
        pImageRepository.RemoveNotifyTarget(*this);

        m_pImageReference.ChangeReference(nImageType, ra::Narrow(GetValue(*m_pImageBoundProperty)));

        TryUpdateImage();
    }

    void TryUpdateImage()
    {
        if (m_hWnd)
        {
            auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
            if (pImageRepository.IsImageAvailable(m_pImageReference.Type(), m_pImageReference.Name()))
            {
                UpdateImage();
            }
            else
            {
                pImageRepository.AddNotifyTarget(*this);
                pImageRepository.FetchImage(m_pImageReference.Type(), m_pImageReference.Name());
            }
        }
    }

    void UpdateImage()
    {
        //GSL_SUPPRESS_CON4 // inline suppression for this currently not working
        const auto hBitmap = ra::ui::drawing::gdi::ImageRepository::GetHBitmap(m_pImageReference);

        SendMessage(m_hWnd, STM_SETIMAGE, ra::to_unsigned(IMAGE_BITMAP), reinterpret_cast<LPARAM>(hBitmap));
        ControlBinding::ForceRepaint(m_hWnd);
    }

    const StringModelProperty* m_pImageBoundProperty = nullptr;
    ra::ui::ImageReference m_pImageReference;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_IMAGEBINDING_H
