#ifndef RA_UI_LOGINVIEWMODEL_H
#define RA_UI_LOGINVIEWMODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class LoginViewModel : public WindowViewModelBase
{
public:
    LoginViewModel();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the username.
    /// </summary>
    static const StringModelProperty UsernameProperty;

    /// <summary>
    /// Gets the username.
    /// </summary>
    const std::wstring& GetUsername() const { return GetValue(UsernameProperty); }

    /// <summary>
    /// Sets the username.
    /// </summary>
    void SetUsername(const std::wstring& sValue) { SetValue(UsernameProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the password.
    /// </summary>
    static const StringModelProperty PasswordProperty;

    /// <summary>
    /// Gets the password.
    /// </summary>
    const std::wstring& GetPassword() const { return GetValue(PasswordProperty); }

    /// <summary>
    /// Sets the password.
    /// </summary>
    void SetPassword(const std::wstring& sValue) { SetValue(PasswordProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the password.
    /// </summary>
    static const BoolModelProperty IsPasswordRememberedProperty;

    /// <summary>
    /// Gets whether the password should be remembered.
    /// </summary>
    bool IsPasswordRemembered() const { return GetValue(IsPasswordRememberedProperty); }

    /// <summary>
    /// Sets whether the password should be remembered.
    /// </summary>
    void SetPasswordRemembered(bool bValue) { SetValue(IsPasswordRememberedProperty, bValue); }

    bool DoLogin() const;

protected:
    LoginViewModel(const std::wstring&& sUsername); // alternate costructor for unit tests
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_LOGINVIEWMODEL_H
