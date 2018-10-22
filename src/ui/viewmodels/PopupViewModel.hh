#ifndef RA_UI_POPUP_VIEW_MODEL_H
#define RA_UI_POPUP_VIEW_MODEL_H
#pragma once

#include "ui/ViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class PopupViewModel : public ViewModelBase
{
public:
    enum class Type
    {
        Info,
        Error,
        Login,
        Unlock,
        LeaderboardAvailable,
        LeaderboardCancel,
        Message,
    };

    explicit PopupViewModel(Type nType, const std::wstring& sTitle, const std::wstring& sDescription) noexcept 
        : ViewModelBase()
    {
        SetType(nType);
        SetTitle(sTitle);
        SetDescription(sDescription);
    }

    explicit PopupViewModel(Type nType, const std::wstring& sTitle, const std::wstring& sDescription, const std::wstring& sDetail) noexcept
        : PopupViewModel(nType, sTitle, sDescription)
    {
        SetDetail(sDetail);
    }

    ~PopupViewModel() noexcept = default;
    PopupViewModel(const PopupViewModel&) noexcept = delete;
    PopupViewModel operator=(const PopupViewModel&) noexcept = delete;
    PopupViewModel(PopupViewModel&&) noexcept = default;
    PopupViewModel operator=(PopupViewModel&&) noexcept = delete;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the title message.
    /// </summary>
    static const StringModelProperty TitleProperty;

    /// <summary>
    /// Gets the title message to display.
    /// </summary>
    const std::wstring& GetTitle() const { return GetValue(TitleProperty); }

    /// <summary>
    /// Sets the title message to display.
    /// </summary>
    void SetTitle(const std::wstring& sValue) { SetValue(TitleProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the description.
    /// </summary>
    static const StringModelProperty DescriptionProperty;

    /// <summary>
    /// Gets the description to display.
    /// </summary>
    const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

    /// <summary>
    /// Sets the description to display.
    /// </summary>
    void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the detail.
    /// </summary>
    static const StringModelProperty DetailProperty;

    /// <summary>
    /// Gets the detail message to display.
    /// </summary>
    const std::wstring& GetDetail() const { return GetValue(DetailProperty); }

    /// <summary>
    /// Sets the detail message to display.
    /// </summary>
    void SetDetail(const std::wstring& sValue) { SetValue(DetailProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the detail.
    /// </summary>
    static const StringModelProperty ImageNameProperty;

    /// <summary>
    /// Gets the name of the image to display.
    /// </summary>
    const std::wstring& GetImageName() const { return GetValue(ImageNameProperty); }

    /// <summary>
    /// Sets the name of the image to display.
    /// </summary>
    void SetImageName(const std::wstring& sValue) { SetValue(ImageNameProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the popup type.
    /// </summary>
    static const IntModelProperty TypeProperty;

    /// <summary>
    /// Gets the popup type.
    /// </summary>
    const Type GetType() const { return static_cast<Type>(GetValue(TypeProperty)); }

    /// <summary>
    /// Sets the popup type.
    /// </summary>
    void SetType(Type nValue) { SetValue(TypeProperty, static_cast<int>(nValue)); }
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_POPUP_VIEW_MODEL_H
