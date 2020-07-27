#ifndef RA_UI_TRANSACTIONAL_VIEW_MODEL_BASE_H
#define RA_UI_TRANSACTIONAL_VIEW_MODEL_BASE_H
#pragma once

#include "ViewModelBase.hh"

namespace ra {
namespace ui {

class TransactionalViewModelBase : public ViewModelBase
{
public:
    /// <summary>
    /// Pushes a new transaction onto the stack.
    /// </summary>
    virtual void BeginTransaction();

    /// <summary>
    /// Completes the current transaction, keeping any changes.
    /// </summary>
    virtual void CommitTransaction();

    /// <summary>
    /// Completes the current transaction, resetting all tracked properties to the state 
    /// they were in when <see cref="BeginTransaction" /> was called.
    /// </summary>
    virtual void RevertTransaction();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the whether the viewmodel has been modified.
    /// </summary>
    static const BoolModelProperty IsModifiedProperty;

    /// <summary>
    /// Determines if any tracked property has been modified in the current transaction.
    /// </summary>
    /// <returns><c>true</c> if modified, <c>false</c> if not.</returns>
    bool IsModified() const
    {
        return GetValue(IsModifiedProperty);
    }

    /// <summary>
    /// Determines if the specified property has been modified in the current transaction.
    /// </summary>
    /// <param name="pProperty">The property to check.</param>
    /// <returns><c>true</c> if modified, <c>false</c> if not.</returns>
    bool IsModified(const BoolModelProperty& pProperty) const
    {
        return m_pTransaction != nullptr && m_pTransaction->IsModified(pProperty);
    }

    /// <summary>
    /// Determines if the specified property has been modified in the current transaction.
    /// </summary>
    /// <param name="pProperty">The property to check.</param>
    /// <returns><c>true</c> if modified, <c>false</c> if not.</returns>
    bool IsModified(const StringModelProperty& pProperty) const
    {
        return m_pTransaction != nullptr && m_pTransaction->IsModified(pProperty);
    }

    /// <summary>
    /// Determines if the specified property has been modified in the current transaction.
    /// </summary>
    /// <param name="pProperty">The property to check.</param>
    /// <returns><c>true</c> if modified, <c>false</c> if not.</returns>
    bool IsModified(const IntModelProperty& pProperty) const
    {
        return m_pTransaction != nullptr && m_pTransaction->IsModified(pProperty);
    }

protected:
    /// <summary>
    /// Sets the specified boolean property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="bValue">The value to set.</param>
    void SetTransactionalValue(const BoolModelProperty& pProperty, bool bValue);

    /// <summary>
    /// Sets the specified string property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="sValue">The value to set.</param>
    void SetTransactionalValue(const StringModelProperty& pProperty, const std::wstring& sValue);

    /// <summary>
    /// Sets the specified integer property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="nValue">The value to set.</param>
    void SetTransactionalValue(const IntModelProperty& pProperty, int nValue);

    class Transaction
    {
    public:
        /// <summary>
        /// Sets the specified boolean property to the specified value.
        /// </summary>
        /// <param name="pProperty">The property to set.</param>
        /// <param name="bValue">The value to set.</param>
        /// <param name="bCurrentValue">The value being replaced.</param>
        void SetValue(const BoolModelProperty& pProperty, bool bValue, bool bCurrentValue);

        /// <summary>
        /// Sets the specified string property to the specified value.
        /// </summary>
        /// <param name="pProperty">The property to set.</param>
        /// <param name="sValue">The value to set.</param>
        /// <param name="sCurrentValue">The value being replaced.</param>
        void SetValue(const StringModelProperty& pProperty, const std::wstring& sValue, const std::wstring& sCurrentValue);

        /// <summary>
        /// Sets the specified integer property to the specified value.
        /// </summary>
        /// <param name="pProperty">The property to set.</param>
        /// <param name="nValue">The value to set.</param>
        /// <param name="nCurrentValue">The value being replaced.</param>
        void SetValue(const IntModelProperty& pProperty, int nValue, int nCurrentValue);

        /// <summary>
        /// Determines if any property has been modified.
        /// </summary>
        bool IsModified() const
        {
            return (!m_mOriginalIntValues.empty() || !m_mOriginalStringValues.empty());
        }

        /// <summary>
        /// Determines if the specified property has been modified.
        /// </summary>
        /// <param name="pProperty">The property to check.</param>
        /// <returns><c>true</c> if modified, <c>false</c> if not.</returns>
        bool IsModified(const BoolModelProperty& pProperty) const
        {
            const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(pProperty.GetKey());
            return (iter != m_mOriginalIntValues.end());
        }

        /// <summary>
        /// Determines if the specified property has been modified.
        /// </summary>
        /// <param name="pProperty">The property to check.</param>
        /// <returns><c>true</c> if modified, <c>false</c> if not.</returns>
        bool IsModified(const StringModelProperty& pProperty) const
        {
            const StringModelProperty::ValueMap::const_iterator iter = m_mOriginalStringValues.find(pProperty.GetKey());
            return (iter != m_mOriginalStringValues.end());
        }

        /// <summary>
        /// Determines if the specified property has been modified.
        /// </summary>
        /// <param name="pProperty">The property to check.</param>
        /// <returns><c>true</c> if modified, <c>false</c> if not.</returns>
        bool IsModified(const IntModelProperty& pProperty) const
        {
            const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(pProperty.GetKey());
            return (iter != m_mOriginalIntValues.end());
        }

        void Revert(TransactionalViewModelBase& vmViewModel);

        std::shared_ptr<Transaction> m_pNext;

    private:
        StringModelProperty::ValueMap m_mOriginalStringValues;
        IntModelProperty::ValueMap m_mOriginalIntValues;

#ifdef _DEBUG
        /// <summary>
        /// Complete list of values as strings for viewing in the debugger
        /// </summary>
        std::map<std::string, std::wstring> m_mDebugOriginalValues;
#endif
    };

    std::shared_ptr<Transaction> m_pTransaction;

private:
    void DiscardTransaction();
};

} // namespace ui
} // namespace ra

#endif RA_UI_TRANSACTIONAL_VIEW_MODEL_BASE_H
