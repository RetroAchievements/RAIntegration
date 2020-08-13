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
    /// Indicates the specified property should be monitored in transactions.
    /// </summary>
    /// <param name="pProperty">The property to monitor.</param>
    void SetTransactional(const BoolModelProperty& pProperty) noexcept
    {
        GSL_SUPPRESS_F6 m_vTransactionalProperties.insert(pProperty.GetKey());
    }

    /// <summary>
    /// Indicates the specified property should be monitored in transactions.
    /// </summary>
    /// <param name="pProperty">The property to monitor.</param>
    void SetTransactional(const StringModelProperty& pProperty) noexcept
    {
        GSL_SUPPRESS_F6 m_vTransactionalProperties.insert(pProperty.GetKey());
    }

    /// <summary>
    /// Indicates the specified property should be monitored in transactions.
    /// </summary>
    /// <param name="pProperty">The property to monitor.</param>
    void SetTransactional(const IntModelProperty& pProperty) noexcept
    {
        GSL_SUPPRESS_F6 m_vTransactionalProperties.insert(pProperty.GetKey());
    }

    class Transaction
    {
    public:
        /// <summary>
        /// Updates the transaction with the new value for a property.
        /// </summary>
        /// <param name="args">Information about the change.</param>
        void ValueChanged(const BoolModelProperty::ChangeArgs& args);

        /// <summary>
        /// Updates the transaction with the new value for a property.
        /// </summary>
        /// <param name="args">Information about the change.</param>
        void ValueChanged(const StringModelProperty::ChangeArgs& args);

        /// <summary>
        /// Updates the transaction with the new value for a property.
        /// </summary>
        /// <param name="args">Information about the change.</param>
        void ValueChanged(const IntModelProperty::ChangeArgs& args);

        /// <summary>
        /// Determines if any property has been modified.
        /// </summary>
        bool IsModified() const noexcept
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

        std::unique_ptr<Transaction> m_pNext;

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

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    std::unique_ptr<Transaction> m_pTransaction;

private:
    void DiscardTransaction();

    bool IsTransactional(const BoolModelProperty& pProperty)
    {
        return (m_vTransactionalProperties.find(pProperty.GetKey()) != m_vTransactionalProperties.end());
    }

    bool IsTransactional(const StringModelProperty& pProperty)
    {
        return (m_vTransactionalProperties.find(pProperty.GetKey()) != m_vTransactionalProperties.end());
    }

    bool IsTransactional(const IntModelProperty& pProperty)
    {
        return (m_vTransactionalProperties.find(pProperty.GetKey()) != m_vTransactionalProperties.end());
    }

    std::set<int> m_vTransactionalProperties;
};

} // namespace ui
} // namespace ra

#endif RA_UI_TRANSACTIONAL_VIEW_MODEL_BASE_H
