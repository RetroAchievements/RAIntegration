#ifndef RA_DATA_DATAMODEL_BASE_H
#define RA_DATA_DATAMODEL_BASE_H
#pragma once

#include "ModelBase.hh"

namespace ra {
namespace data {

class DataModelBase : public ModelBase
{
public:
    class NotifyTarget
    {
    public:
        NotifyTarget() noexcept = default;
        virtual ~NotifyTarget() noexcept = default;
        NotifyTarget(const NotifyTarget&) noexcept = delete;
        NotifyTarget& operator=(const NotifyTarget&) noexcept = delete;
        NotifyTarget(NotifyTarget&&) noexcept = default;
        NotifyTarget& operator=(NotifyTarget&&) noexcept = default;

        virtual void OnDataModelBoolValueChanged([[maybe_unused]] const BoolModelProperty::ChangeArgs& args) noexcept(false) {}
        virtual void OnDataModelStringValueChanged([[maybe_unused]] const StringModelProperty::ChangeArgs& args) noexcept(false) {}
        virtual void OnDataModelIntValueChanged([[maybe_unused]] const IntModelProperty::ChangeArgs& args) noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.insert(&pTarget); }

    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept
    {
#ifdef RA_UTEST
        GSL_SUPPRESS_F6 Expects(!m_bDestructed);
#endif
        GSL_SUPPRESS_F6 m_vNotifyTargets.erase(&pTarget);
    }

private:
    using NotifyTargetSet = std::set<NotifyTarget*>;

    /// <summary>
    /// A collection of pointers to other objects. These are not allocated object and do not need to be free'd. It's
    /// impossible to create a set of <c>NotifyTarget</c> references.
    /// </summary>
    NotifyTargetSet m_vNotifyTargets;

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
    GSL_SUPPRESS_F6 DataModelBase() = default;

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
        /// Gets the value of the property prior to any changes made in the current transaction
        /// </summary>
        const bool* GetPreviousValue(const BoolModelProperty& pProperty) const
        {
            const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(pProperty.GetKey());
            GSL_SUPPRESS_TYPE1 return (iter != m_mOriginalIntValues.end()) ? reinterpret_cast<const bool*>(&iter->second) : nullptr;
        }

        /// <summary>
        /// Gets the previous value for the property if it has been modified, or the default value if
        /// if has not been modified. Use <see cref="IsModified"/> to determined if if was modified.
        /// </summary>
        const std::wstring* GetPreviousValue(const StringModelProperty& pProperty) const
        {
            const StringModelProperty::ValueMap::const_iterator iter = m_mOriginalStringValues.find(pProperty.GetKey());
            return (iter != m_mOriginalStringValues.end()) ? &iter->second : nullptr;
        }

        /// <summary>
        /// Gets the previous value for the property if it has been modified, or the default value if
        /// if has not been modified. Use <see cref="IsModified"/> to determined if if was modified.
        /// </summary>
        const int* GetPreviousValue(const IntModelProperty& pProperty) const
        {
            const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(pProperty.GetKey());
            return (iter != m_mOriginalIntValues.end()) ? &iter->second : nullptr;
        }

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

        void Revert(DataModelBase& vmViewModel);
        void Commit(const DataModelBase& vmViewModel);

        std::unique_ptr<Transaction> m_pNext;

        // allow DataModelCollectionBase to call GetValue(Property) directly.
        friend class DataModelCollectionBase;

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

    std::unique_ptr<Transaction> m_pTransaction;

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    /// <summary>
    /// Disables change notifications.
    /// </summary>
    /// <remarks>
    /// Should not be called unless making multiple changes to the same property. 
    /// OnValueChanged will be called once after EndUpdate is called instead of for each intermediate change.
    /// </remarks>
    void BeginUpdate();

    /// <summary>
    /// Re-enables change notifications.
    /// </summary>
    void EndUpdate();

    /// <summary>
    /// Determines if change notifications are disabled.
    /// </summary>
    /// <remarks>
    /// Can be used in OnValueChanged overrides to determine if change notifications are disabled. If they are, OnValueChanged
    /// will be called again once the notifications are re-enabled, so processing should be delayed if this returns true.
    /// </remarks>
    virtual bool IsUpdating() const { return (m_pUpdateTransaction != nullptr); }

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

    struct UpdateTransaction
    {
        unsigned int m_nUpdateCount = 0;

        template<class T>
        struct DelayedChange
        {
            const ModelProperty<T>* pProperty;
            T tOldValue;
            T tNewValue;
        };
        std::vector<DelayedChange<int>> m_vDelayedIntChanges;
        std::vector<DelayedChange<std::wstring>> m_vDelayedStringChanges;
        std::vector<DelayedChange<bool>> m_vDelayedBoolChanges;
    };
    std::unique_ptr<UpdateTransaction> m_pUpdateTransaction;
};

} // namespace data
} // namespace ra

#endif RA_DATA_DATAMODEL_BASE_H
